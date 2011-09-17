#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_log.h>
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_queue.h>

#include <mod_proxy.h>

#undef EV_MULTIPLICITY

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

#include "mod_node.h"

using namespace v8;

typedef struct {
    char *startup_script;
} node_module_config;

#define THROW_BAD_ARGS \
    ThrowException(Exception::TypeError(String::New("Bad argument")))

static const char * node_cmd_startup_script(cmd_parms *cmd, void *dcfg, const char *arg);
static void node_hook_child_init(apr_pool_t *p, server_rec *s);
static void *start_node(apr_thread_t *th, void *data);

static Persistent<String> emit_symbol;
static Persistent<String> log_symbol;
static Persistent<String> warn_symbol;
static Persistent<String> crit_symbol;
static Persistent<String> write_symbol;
static Persistent<String> connection_symbol;
static Persistent<String> proxies_symbol;
static Persistent<String> onrequest_sym;

static void *create_node_module_config(apr_pool_t *p, server_rec *s) {
    node_module_config *conf;
    conf = (node_module_config *) apr_pcalloc(p, sizeof(node_module_config));
    conf->startup_script = ap_server_root_relative(p, "runtime.js");
    return (void *)conf;
};

static const command_rec node_config_cmds[] = {
    AP_INIT_TAKE1("NodeStartupScript", reinterpret_cast<const char* (*)()>(node_cmd_startup_script), NULL, RSRC_CONF, "Path to the startup script"),
    { NULL }    
};

static void node_hook_child_init(apr_pool_t *p, server_rec *s) {
    ap_open_logs(p, p, p, s);
    mod_node::process = s->process;
    apr_thread_create(&mod_node::thread, NULL, start_node, (void *)s, s->process->pool);
};

namespace mod_node {

    typedef struct {
        int cont_request;
        apr_thread_cond_t *cv;
        apr_thread_mutex_t *mtx;
    } request_ext;

    static Persistent<Object> Process;
    static Persistent<Function> ProcessFunc;
    static Persistent<FunctionTemplate> req_function_template;

    class ApacheRequest : public node::ObjectWrap {
    public:
        static void Initialize(Handle<Object> target) {
            v8::Locker l;
            HandleScope scope;
            if(req_function_template.IsEmpty()) {
                req_function_template = Persistent<FunctionTemplate>(FunctionTemplate::New());
                req_function_template->SetClassName(String::NewSymbol("Request"));
                req_function_template->InstanceTemplate()->SetInternalFieldCount(1);
                NODE_SET_PROTOTYPE_METHOD(req_function_template, "write", Write);
            }

            target->Set(String::New("Request"), req_function_template->GetFunction());
        }

        request_rec *r;

        static Handle<Value> New(request_rec *r) {
            v8::Locker l;
            HandleScope scope;

            Handle<Object> req = req_function_template->GetFunction()->NewInstance();
            ApacheRequest *s = new ApacheRequest(r);
            s->Wrap(req);
            return req;
        }

        protected:

        static Handle<Value> Write(const Arguments &args) {
            v8::Locker l;
            ApacheRequest *req = ObjectWrap::Unwrap<ApacheRequest>(args.Holder());

            HandleScope scope;
            if (args.Length() == 0 || !args[0]->IsString()) {
                return ThrowException(Exception::TypeError(
                    String::New("First argument must be a string"))
                );
            }

            String::Utf8Value str(args[0]->ToString());
            req->write(*str);


            return Undefined();
        }

        void write(char *str) {
            ap_rputs(str, r);
        }

        ApacheRequest() {
            r = 0;
        }

        ApacheRequest(request_rec *req) {
            r = req;
        }
    };

    static Persistent<FunctionTemplate> proc_function_template;

    class ApacheProcess : public node::ObjectWrap {
        public:

        process_rec *process;
        apr_queue_t *queue;
        ev_async req_watcher;

        static void Initialize(Handle<Object> target) {
            HandleScope scope;
            v8::Locker l;
            proc_function_template = Persistent<FunctionTemplate>::New(FunctionTemplate::New());
            proc_function_template->InstanceTemplate()->SetInternalFieldCount(2); // 1 + ungodly HACK
            proc_function_template->SetClassName(String::New("ApacheProcess"));
            log_symbol = NODE_PSYMBOL("emit");
            log_symbol = NODE_PSYMBOL("log");
            warn_symbol = NODE_PSYMBOL("warn");
            crit_symbol = NODE_PSYMBOL("critical");
            write_symbol = NODE_PSYMBOL("write");
            proxies_symbol = NODE_PSYMBOL("proxies");
            connection_symbol = NODE_PSYMBOL("connection");
            onrequest_sym = NODE_PSYMBOL("onrequest");
            NODE_SET_PROTOTYPE_METHOD(proc_function_template, "log", Log);
            NODE_SET_PROTOTYPE_METHOD(proc_function_template, "warn", Warn);
            NODE_SET_PROTOTYPE_METHOD(proc_function_template, "critical", Crit);
            NODE_SET_PROTOTYPE_METHOD(proc_function_template, "write", Write); // HACK!
            ProcessFunc = Persistent<Function>::New(proc_function_template->GetFunction());
            Process = Persistent<Object>::New(ProcessFunc->NewInstance());
            ApacheProcess *p = new ApacheProcess(mod_node::process, Process);
            target->Set(String::New("Process"), ProcessFunc);
            target->Set(String::New("process"), Process);
        }

        static void register_hooks(apr_pool_t *p) {
            ap_hook_child_init (node_hook_child_init, NULL,NULL, APR_HOOK_MIDDLE);
            ap_hook_handler(ApacheProcess::handler, NULL, NULL, APR_HOOK_MIDDLE);
        };

        protected:
        ApacheProcess(process_rec *p, Handle<Object> Process) : ObjectWrap() {
            this->Wrap(Process);
            apr_queue_create(&queue, 100, p->pool); // @todo handle error
            ev_async_init(EV_DEFAULT_ &req_watcher, ApacheProcess::RequestCallback);
            req_watcher.data = &Process;
            ev_async_start(EV_DEFAULT_ &req_watcher);
            ev_ref(EV_DEFAULT);
        }

        ~ApacheProcess() {
            ev_unref(EV_DEFAULT);
        }

        static int handler(request_rec *r) {
            // Runs in the Apache thread
            apr_status_t rv;
            request_ext rex;
            rex.cont_request = 0;
            int rc = OK;
            v8::Locker l;
            HandleScope scope;
            ApacheProcess *p = ObjectWrap::Unwrap<ApacheProcess>(Process);
            assert(p);
            apr_thread_cond_create(&rex.cv, p->process->pool);
            apr_thread_mutex_create(&rex.mtx, APR_THREAD_MUTEX_DEFAULT,  p->process->pool);

            apr_thread_mutex_lock(rex.mtx);
            apr_table_setn(r->notes, "mod_node", (char *)&rex);
            apr_queue_push(p->queue, &r); // @todo Errors
            ev_async_send(EV_DEFAULT_ &(p->req_watcher));
            {
                v8::Unlocker unlock;
                while(!rex.cont_request) {
                    apr_thread_cond_wait(rex.cv, rex.mtx);
                }
            }
            apr_thread_mutex_unlock(rex.mtx);

            return rc;
        }


        static void RequestCallback(EV_P_ ev_async *w, int revents) {
            // Executes in Node's thread
            v8::Locker l;
            HandleScope scope;
            Handle<Object> proc = *static_cast<Handle<Object> *>(w->data);
            ApacheProcess *p = ObjectWrap::Unwrap<ApacheProcess>(proc);
            request_rec *r;
            apr_queue_pop(p->queue, (void**)r); // @todo Errors
            Handle<Value> req = ApacheRequest::New(r);
            Local<Value> callback_v = proc->Get(onrequest_sym);
            if(!callback_v->IsFunction()) return;
            Local<Function>::Cast(callback_v)->Call(proc, 1, &req);
        };

        static Handle<Value> Write(const Arguments& args) {
            if (args.Length() < 1) return Undefined();
            v8::Locker l;
            HandleScope scope;
            Handle<Value> arg = args[0];
            String::Utf8Value value(arg);
            Local<Object> self = args.Holder();
            Local<External> wrap = Local<External>::Cast(self->GetInternalField(1));

            request_rec *r = static_cast<request_rec*>(wrap->Value());
            request_ext *rex;
            apr_table_get(r->notes, "mod_node"); // @todo Errors

            ap_rputs(*value, r);

            rex->cont_request = 1;
            apr_thread_mutex_lock(rex->mtx);
            apr_thread_cond_signal(rex->cv);
            apr_thread_mutex_unlock(rex->mtx);

            return Undefined();
        }

        static Handle<Value> Crit(const Arguments &args) {
            return DoLog(APLOG_CRIT, args);
        }

        static Handle<Value> Warn(const Arguments &args) {
            return DoLog(APLOG_WARNING, args);
        }

        static Handle<Value> Log(const Arguments &args) {
            return DoLog(APLOG_INFO, args);
        }

        static Handle<Value> DoLog(int level, const Arguments &args) {
            v8::Locker l;
            ApacheProcess *p = ObjectWrap::Unwrap<ApacheProcess>(args.Holder());
            assert(p);
            if (args.Length() < 1 || !args[0]->IsString()) {
                return THROW_BAD_ARGS;
            }
            String::Utf8Value val(args[0]->ToString());
            ap_log_perror(APLOG_MARK, level, APR_SUCCESS, p->process->pool, "%s", *val);
            return Undefined();
        };

    };

    class ApacheServer : public node::ObjectWrap {
        public:
        static void Initialize(Handle<Object> target) {
            HandleScope scope;
            v8::Locker l;
            Local<FunctionTemplate> t = FunctionTemplate::New(New);
            // @todo Connect up the apache request to emitting a Request here
            target->Set(String::New("Server"), t->GetFunction());
        }

        protected:
        static Handle<Value> New(const Arguments &args) {
            HandleScope scope;

            if (args.Length() < 1 || !args[0]->IsFunction()) {
                return THROW_BAD_ARGS;
            }

            ApacheServer *s = new ApacheServer(args[0]);
            s->Wrap(args.Holder());

            return args.This();
        };

        ApacheServer(Handle<Value> v) {
            handle_->SetInternalField(1, v);
        }

    };

    extern void Initialize(v8::Handle<v8::Object> target) {
        ApacheProcess::Initialize(target);
        ApacheRequest::Initialize(target);
        ApacheServer::Initialize(target);
    };
};

extern "C" {
    module AP_MODULE_DECLARE_DATA node_module = {
        STANDARD20_MODULE_STUFF,
        NULL, /* dir config creater */
        NULL, /* dir merger --- default is to override */
        create_node_module_config, /* server config */
        NULL, /* merge server config */
        node_config_cmds, /* commands */
        mod_node::ApacheProcess::register_hooks
    };
};

static const char * node_cmd_startup_script(cmd_parms *cmd, void *dcfg, const char *arg) {
    node_module_config *conf = (node_module_config *)ap_get_module_config(cmd->server->module_config, &node_module);
    conf->startup_script = ap_server_root_relative(cmd->pool, arg);
    return NULL;
};

static void *start_node(apr_thread_t *th, void* data) {
    char **argv;
    server_rec *s = (server_rec *)data;
    node_module_config *conf = (node_module_config *)ap_get_module_config(s->module_config, &node_module);
    argv = (char **)malloc(sizeof(char *));
        argv[0] = (char *)malloc(32);
    strcpy(argv[0], "node");
    argv[1] = strdup(conf->startup_script);
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "mod_node::startup script: %s", argv[1]);
    // @todo error handling if the runtime script throws an exception
    // Probably atexit(3) and log an error and clean up, return a 500 error.
    node::Start(2, argv);
    return APR_SUCCESS;
}

