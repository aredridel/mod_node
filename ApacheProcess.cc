#include "ApacheProcess.h"

#include "ApacheRequest.h"

namespace mod_node {
    typedef struct {
        int cont_request;
        apr_thread_cond_t *cv;
        apr_thread_mutex_t *mtx;
    } request_ext;

    static Persistent<FunctionTemplate> proc_function_template;
    static Persistent<String> log_symbol;
    static Persistent<String> warn_symbol;
    static Persistent<String> crit_symbol;
    static Persistent<String> write_symbol;
    static Persistent<String> connection_symbol;
    static Persistent<String> proxies_symbol;
    static Persistent<String> onrequest_sym;

    static Persistent<Object> Process;
    static Persistent<Function> ProcessFunc;

    static process_rec *process;

    static void node_hook_child_init(apr_pool_t *p, server_rec *s);

    void ApacheProcess::Initialize(Handle<Object> target) {
        using mod_node::process;
        //v8::Locker l;
        HandleScope scope;
        proc_function_template = Persistent<FunctionTemplate>::New(FunctionTemplate::New());
        proc_function_template->InstanceTemplate()->SetInternalFieldCount(1);
        proc_function_template->SetClassName(String::New("ApacheProcess"));
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
        ApacheProcess *p = new ApacheProcess(process, Process);
        target->Set(String::New("ApacheProcess"), ProcessFunc);
        target->Set(String::New("process"), Process);
    }

    void ApacheProcess::register_hooks(apr_pool_t *p) {
        ap_hook_child_init (node_hook_child_init, NULL,NULL, APR_HOOK_MIDDLE);
        ap_hook_handler(ApacheProcess::handler, NULL, NULL, APR_HOOK_MIDDLE);
    };

    ApacheProcess::ApacheProcess(process_rec *p, Handle<Object> Process) : process(p) {
        this->Wrap(Process);
        apr_queue_create(&queue, 100, p->pool); // @todo handle error
        ev_async_init(EV_DEFAULT_ &req_watcher, ApacheProcess::RequestCallback);
        req_watcher.data = &Process;
        ev_async_start(EV_DEFAULT_ &req_watcher);
        ev_ref(EV_DEFAULT);
    }

    ApacheProcess::~ApacheProcess() {
        ev_unref(EV_DEFAULT);
    }

    int ApacheProcess::handler(request_rec *r) {
        // Runs in the Apache thread
        apr_status_t rv;
        request_ext rex;
        rex.cont_request = 0;
        int rc = OK;
        //v8::Locker l;
        HandleScope scope;
        ApacheProcess *p = ObjectWrap::Unwrap<ApacheProcess>(Process);
        assert(p);
        apr_thread_cond_create(&rex.cv, p->process->pool);
        apr_thread_mutex_create(&rex.mtx, APR_THREAD_MUTEX_DEFAULT,  p->process->pool);

        apr_thread_mutex_lock(rex.mtx);
        apr_table_setn(r->notes, "mod_node", (char *)&rex);
        apr_queue_push(p->queue, &r); // @todo Errors
        {
            //v8::Unlocker unlock;
            ev_async_send(EV_DEFAULT_ &(p->req_watcher));
            while(!rex.cont_request) {
                apr_thread_cond_wait(rex.cv, rex.mtx);
            }
        }
        apr_thread_mutex_unlock(rex.mtx);

        return rc;
    }


    void ApacheProcess::RequestCallback(EV_P_ ev_async *w, int revents) {
        // Executes in Node's thread
        //v8::Locker l;
        HandleScope scope;
        Handle<Object> proc = *static_cast<Handle<Object> *>(w->data);
        ApacheProcess *p = ObjectWrap::Unwrap<ApacheProcess>(proc);
        request_rec *r;
        apr_queue_pop(p->queue, (void**)r); // @todo Errors
        Handle<Value> req = ApacheRequest::New(r);
        Local<Value> callback_v = proc->Get(onrequest_sym);
        if(!callback_v->IsFunction()) return;
        TryCatch trycatch;
        trycatch.SetVerbose(true);
        trycatch.SetCaptureMessage(true);
        Local<Function>::Cast(callback_v)->Call(proc, 1, &req);
        if(trycatch.HasCaught()) {
            v8::String::Utf8Value str(trycatch.Message()->Get());
            ap_log_perror(APLOG_MARK, APLOG_ERR, APR_SUCCESS, p->process->pool, "%s", *str);
        }

    };

    Handle<Value> ApacheProcess::Write(const Arguments& args) {
        if (args.Length() < 1) return Undefined();
        //v8::Locker l;
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

    Handle<Value> ApacheProcess::Crit(const Arguments &args) {
        return DoLog(APLOG_CRIT, args);
    }

    Handle<Value> ApacheProcess::Warn(const Arguments &args) {
        return DoLog(APLOG_WARNING, args);
    }

    Handle<Value> ApacheProcess::Log(const Arguments &args) {
        return DoLog(APLOG_INFO, args);
    }

    Handle<Value> ApacheProcess::DoLog(int level, const Arguments &args) {
        //v8::Locker l;
        ApacheProcess *p = ObjectWrap::Unwrap<ApacheProcess>(args.Holder());
        assert(p);
        if (args.Length() < 1 || !args[0]->IsString()) {
            return THROW_BAD_ARGS;
        }
        String::Utf8Value val(args[0]->ToString());
        ap_log_perror(APLOG_MARK, level, APR_SUCCESS, p->process->pool, "%s", *val);
        return Undefined();
    };

    static void node_hook_child_init(apr_pool_t *p, server_rec *s) {
        using mod_node::thread;
        using mod_node::process;
        ap_open_logs(p, p, p, s);
        mod_node::process = s->process;
        apr_thread_create(&mod_node::thread, NULL, start_node, (void *)s, s->process->pool);
    };

};
