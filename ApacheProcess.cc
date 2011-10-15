#include "ApacheProcess.h"

#include "ApacheRequest.h"

namespace mod_node {

    static apr_queue_t *queue;
    static uv_async_t req_watcher;

    static Persistent<Object> Process;
    static Persistent<Function> ProcessFunc;

    static Persistent<String> onrequest_sym;

    static process_rec *process;

    static void node_hook_child_init(apr_pool_t *p, server_rec *s);

    /** Initialize the module when we get loaded by node.
     *
     * This is the second stage initialization: First, we hook Apache and 
     * start node, then this gets called when node calls back when the 
     * startup script requires its binding.
     */
    void ApacheProcess::Initialize(Handle<Object> target) {
        using mod_node::process;
        using mod_node::queue;
        using mod_node::req_watcher;
        HandleScope scope;

        Local<FunctionTemplate> proc_function_template = FunctionTemplate::New();
        proc_function_template->InstanceTemplate()->SetInternalFieldCount(1);
        proc_function_template->SetClassName(String::New("ApacheProcess"));

        NODE_SET_PROTOTYPE_METHOD(proc_function_template, "log", Log);

        target->Set(String::New("APLOG_EMERG"), v8::Integer::NewFromUnsigned(APLOG_EMERG));
        target->Set(String::New("APLOG_ALERT"), v8::Integer::NewFromUnsigned(APLOG_ALERT));
        target->Set(String::New("APLOG_CRIT"), v8::Integer::NewFromUnsigned(APLOG_CRIT));
        target->Set(String::New("APLOG_ERR"), v8::Integer::NewFromUnsigned(APLOG_ERR));
        target->Set(String::New("APLOG_WARNING"), v8::Integer::NewFromUnsigned(APLOG_WARNING));
        target->Set(String::New("APLOG_INFO"), v8::Integer::NewFromUnsigned(APLOG_INFO));
        target->Set(String::New("APLOG_NOTICE"), v8::Integer::NewFromUnsigned(APLOG_NOTICE));
        target->Set(String::New("APLOG_DEBUG"), v8::Integer::NewFromUnsigned(APLOG_DEBUG));

        onrequest_sym = NODE_PSYMBOL("onrequest");

        ProcessFunc = Persistent<Function>::New(proc_function_template->GetFunction());
        Process = Persistent<Object>::New(ProcessFunc->NewInstance());

        // @todo handle error
        // @todo dynamically size queue (based on maxrequests?)
        apr_queue_create(&queue, 1000, process->pool); 
        
        /* This will leave the event loop with one reference, which we 
           want since we want to last as long as Apache */
        uv_async_init(uv_default_loop(), &req_watcher, ApacheProcess::RequestCallback);

        target->Set(String::New("ApacheProcess"), ProcessFunc);
        target->Set(String::New("process"), Process);
    }

    /** Hook into Apache so we can initialize ourself.
     *
     * child_init hook is to start Node.
     * handler hook is to pass requests to Node.
     */
    void ApacheProcess::register_hooks(apr_pool_t *p) {
        ap_hook_child_init (node_hook_child_init, NULL,NULL, APR_HOOK_MIDDLE);
        ap_hook_handler(ApacheProcess::handler, NULL, NULL, APR_HOOK_MIDDLE);
    };

    /** Handle a request from Apache, and pass to the node thread.
     *
     * Executes in Apache's thread.
     */
    int ApacheProcess::handler(request_rec *r) {
        // Runs in the Apache thread
        using mod_node::queue;
        using mod_node::req_watcher;

        request_ext rex;
        rex.req = r;
        rex.cont_request = 0;
        int rc = OK;
        apr_thread_cond_create(&rex.cv, process->pool);
        apr_thread_mutex_create(&rex.mtx, APR_THREAD_MUTEX_DEFAULT,  process->pool);

        apr_thread_mutex_lock(rex.mtx);
        apr_table_setn(r->notes, "mod_node", (char *)&rex);
        while (apr_queue_push(queue, &rex) == APR_EAGAIN); // @todo Errors
        {
            uv_async_send(&req_watcher);
            while(!rex.cont_request) {
                apr_thread_cond_wait(rex.cv, rex.mtx);
            }
        }
        apr_thread_mutex_unlock(rex.mtx);

        return rc;
    }

    /** Handle a request coming in from Apache.
     *
     * Executes in the Node thread.
     */ 
    void ApacheProcess::RequestCallback(uv_async_t* handle, int status) {
        using mod_node::queue;
        using mod_node::req_watcher;

        HandleScope scope;
        request_ext *rex;
        while (apr_queue_trypop(queue, (void **)&rex) == APR_SUCCESS) {
            // @todo Errors
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
        }

    };

    /** Log a message to the Apache error log
     */
    Handle<Value> ApacheProcess::Log(const Arguments &args) {
        if (args.Length() < 2 || !args[1]->IsString() || !args[0]->IsInt32()) {
            return THROW_BAD_ARGS;
        }
        String::Utf8Value val(args[1]->ToString());
        ap_log_perror(APLOG_MARK, args[0]->ToInt32()->Value(), APR_SUCCESS,
                process->pool, "%s", *val);
        return Undefined();
    };

    /** Start a thread for node.
     */
    static void node_hook_child_init(apr_pool_t *p, server_rec *s) {
        using mod_node::thread;
        using mod_node::process;
        ap_open_logs(p, p, p, s);
        mod_node::process = s->process;
        apr_thread_create(&mod_node::thread, NULL, start_node, (void *)s,
                s->process->pool);
    };

};
