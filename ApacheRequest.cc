#include "ApacheRequest.h"

using namespace v8;

namespace mod_node {
    static Persistent<FunctionTemplate> req_function_template;
    static Persistent<Function> req_function;

    void ApacheRequest::Initialize(Handle<Object> target) {
        HandleScope scope;
        if(req_function_template.IsEmpty()) {
            req_function_template = Persistent<FunctionTemplate>(FunctionTemplate::New());
            req_function_template->SetClassName(String::NewSymbol("ApacheRequest"));
            req_function_template->InstanceTemplate()->SetInternalFieldCount(1);
            NODE_SET_PROTOTYPE_METHOD(req_function_template, "write", Write);
            NODE_SET_PROTOTYPE_METHOD(req_function_template, "end", End);
        }

        req_function = Persistent<Function>::New(req_function_template->GetFunction());
        target->Set(String::New("ApacheRequest"), req_function);
    }

    Handle<Object> ApacheRequest::New(request_ext *rex) {
        Handle<Object> req = req_function->NewInstance();
        ApacheRequest *s = new ApacheRequest(rex);
        s->Wrap(req);
        return req;
    }

    Handle<Value> ApacheRequest::Write(const Arguments &args) {
        ApacheRequest *req = ObjectWrap::Unwrap<ApacheRequest>(args.Holder());

        if (args.Length() == 0 || !args[0]->IsString()) {
            return ThrowException(Exception::TypeError(
                String::New("First argument must be a string")));
        }

        String::Utf8Value str(args[0]->ToString());

        req->write(*str);

        return Undefined();
    }

    Handle<Value> ApacheRequest::End(const Arguments &args) {
        if(args.Length() > 0) Write(args);

        ApacheRequest *req = ObjectWrap::Unwrap<ApacheRequest>(args.Holder());
        req->end();

        // Signal condition variable

        return Undefined();
    }

    ApacheRequest::ApacheRequest(request_ext *rex): rex(rex)
    {
    }

    void ApacheRequest::write(char *str) {
        ap_rputs(str, rex->req);
    }

    void ApacheRequest::end() {
        apr_table_get(rex->req->notes, "mod_node"); // @todo Errors

        rex->cont_request = 1;
        apr_thread_mutex_lock(rex->mtx);
        apr_thread_cond_signal(rex->cv);
        apr_thread_mutex_unlock(rex->mtx);
    }

};
