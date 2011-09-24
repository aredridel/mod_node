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
        }

        req_function = Persistent<Function>::New(req_function_template->GetFunction());
        target->Set(String::New("ApacheRequest"), req_function);
    }

    Handle<Object> ApacheRequest::New(request_rec *r) {
        Handle<Object> req = req_function->NewInstance();
        ApacheRequest *s = new ApacheRequest(r);
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

        // Signal condition variable

        return Undefined();
    }

    ApacheRequest::ApacheRequest(request_rec *req) {
        r = req;
    }

    void ApacheRequest::write(char *str) {
        ap_rputs(str, r);
    }

};
