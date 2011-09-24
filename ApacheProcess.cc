#include "ApacheProcess.h"

#include "ApacheRequest.h"

namespace mod_node {

    static apr_queue_t *queue;
    static ev_async req_watcher;

    static Persistent<String> log_symbol;
    static Persistent<String> warn_symbol;
    static Persistent<String> crit_symbol;
    static Persistent<String> write_symbol;
    static Persistent<String> onrequest_sym;

    static Persistent<Object> Process;
    static Persistent<Function> ProcessFunc;

    static process_rec *process;

    static void node_hook_child_init(apr_pool_t *p, server_rec *s);

    void ApacheProcess::Initialize(Handle<Object> target) {
        using mod_node::process;
        using mod_node::queue;
        using mod_node::req_watcher;
        HandleScope scope;

        Local<FunctionTemplate> proc_function_template = FunctionTemplate::New();
        proc_function_template->InstanceTemplate()->SetInternalFieldCount(1);
        proc_function_template->SetClassName(String::New("ApacheProcess"));

        log_symbol = NODE_PSYMBOL("log");
        NODE_SET_PROTOTYPE_METHOD(proc_function_template, "log", Log);

        warn_symbol = NODE_PSYMBOL("warn");
        NODE_SET_PROTOTYPE_METHOD(proc_function_template, "warn", Warn);

        crit_symbol = NODE_PSYMBOL("critical");
        NODE_SET_PROTOTYPE_METHOD(proc_function_template, "critical", Crit);

        onrequest_sym = NODE_PSYMBOL("onrequest");

        ProcessFunc = Persistent<Function>::New(proc_function_template->GetFunction());
        Process = Persistent<Object>::New(ProcessFunc->NewInstance());

        apr_queue_create(&queue, 100, process->pool); // @todo handle error
        ev_async_init(EV_DEFAULT_UC_ &req_watcher, ApacheProcess::RequestCallback);
        ev_async_start(EV_DEFAULT_UC_ &req_watcher);
        ev_ref(EV_DEFAULT_UC);

        target->Set(String::New("ApacheProcess"), ProcessFunc);
        target->Set(String::New("process"), Process);
    }

    void ApacheProcess::register_hooks(apr_pool_t *p) {
        ap_hook_child_init (node_hook_child_init, NULL,NULL, APR_HOOK_MIDDLE);
        ap_hook_handler(ApacheProcess::handler, NULL, NULL, APR_HOOK_MIDDLE);
    };

    int ApacheProcess::handler(request_rec *r) {
        // Runs in the Apache thread
        using mod_node::queue;
        using mod_node::req_watcher;

        apr_status_t rv;
        request_ext rex;
        rex.req = r;
        rex.cont_request = 0;
        int rc = OK;
        apr_thread_cond_create(&rex.cv, process->pool);
        apr_thread_mutex_create(&rex.mtx, APR_THREAD_MUTEX_DEFAULT,  process->pool);

        apr_thread_mutex_lock(rex.mtx);
        apr_table_setn(r->notes, "mod_node", (char *)&rex);
        apr_queue_push(queue, &rex); // @todo Errors
        {
            ev_async_send(EV_DEFAULT_ &req_watcher);
            while(!rex.cont_request) {
                apr_thread_cond_wait(rex.cv, rex.mtx);
            }
        }
        apr_thread_mutex_unlock(rex.mtx);

        return rc;
    }


    void ApacheProcess::RequestCallback(EV_P_ ev_async *w, int revents) {
        // Executes in Node's thread
        using mod_node::queue;
        using mod_node::req_watcher;

        HandleScope scope;
        request_ext *rex;
        apr_queue_pop(queue, (void **)&rex); // @todo Errors
        Handle<Value> req = ApacheRequest::New(rex);
        Local<Value> callback_v = Process->Get(onrequest_sym);
        if(!callback_v->IsFunction()) return;
        TryCatch trycatch;
        trycatch.SetVerbose(true);
        trycatch.SetCaptureMessage(true);
        const int argc = 1;
        Handle<Value> args[argc] = { req };
        Local<Function>::Cast(callback_v)->Call(Process, 1, args);
        if(trycatch.HasCaught()) {
            v8::String::Utf8Value str(trycatch.Message()->Get());
            ap_log_perror(APLOG_MARK, APLOG_ERR, APR_SUCCESS, process->pool, "%s", *str);
        }

    };

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
        if (args.Length() < 1 || !args[0]->IsString()) {
            return THROW_BAD_ARGS;
        }
        String::Utf8Value val(args[0]->ToString());
        ap_log_perror(APLOG_MARK, level, APR_SUCCESS, process->pool, "%s", *val);
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
