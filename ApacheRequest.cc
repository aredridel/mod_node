#include "ApacheRequest.h"

using namespace v8;

namespace mod_node {
    static Persistent<FunctionTemplate> req_function_template;

    void ApacheRequest::Initialize(Handle<Object> target) {
        //v8::Locker l;
        HandleScope scope;
        if(req_function_template.IsEmpty()) {
            req_function_template = Persistent<FunctionTemplate>(FunctionTemplate::New());
            req_function_template->SetClassName(String::NewSymbol("Request"));
            req_function_template->InstanceTemplate()->SetInternalFieldCount(1);
            NODE_SET_PROTOTYPE_METHOD(req_function_template, "write", Write);
        }

        target->Set(String::New("Request"), req_function_template->GetFunction());
    }

    Handle<Value> ApacheRequest::New(request_rec *r) {
        //v8::Locker l;
        HandleScope scope;

        Handle<Object> req = req_function_template->GetFunction()->NewInstance();
        ApacheRequest *s = new ApacheRequest(r);
        s->Wrap(req);
        return req;
    }

    Handle<Value> ApacheRequest::Write(const Arguments &args) {
        //v8::Locker l;
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

    ApacheRequest::ApacheRequest() {
        r = 0;
    }

    ApacheRequest::ApacheRequest(request_rec *req) {
        r = req;
    }

    void ApacheRequest::write(char *str) {
        ap_rputs(str, r);
    }

};
