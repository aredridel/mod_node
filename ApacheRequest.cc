#include "ApacheRequest.h"
#include "APRTable.h"

using namespace v8;

namespace mod_node {
    static Persistent<FunctionTemplate> req_function_template;
    static Persistent<Function> req_function;

    void ApacheRequest::Initialize(Handle<Object> target) {
        HandleScope scope;
        if(req_function_template.IsEmpty()) {
            req_function_template = Persistent<FunctionTemplate>(FunctionTemplate::New());
            req_function_template->SetClassName(String::NewSymbol("ApacheRequest"));
            Handle <ObjectTemplate> itemplate = req_function_template->InstanceTemplate();
            itemplate->SetInternalFieldCount(1);
            itemplate->SetAccessor(String::New("headers_in"), GetHeadersIn);
            itemplate->SetAccessor(String::New("headers_out"), GetHeadersOut);
            NODE_SET_PROTOTYPE_METHOD(req_function_template, "write", Write);
            NODE_SET_PROTOTYPE_METHOD(req_function_template, "end", End);
        }

        req_function = Persistent<Function>::New(req_function_template->GetFunction());
        target->Set(String::New("ApacheRequest"), req_function);
    }

    Handle<Value> ApacheRequest::GetHeadersIn(Local<String> property, const AccessorInfo& info) {
        ApacheRequest *req = ObjectWrap::Unwrap<ApacheRequest>(info.Holder());
        return req->HeadersIn;
    }

    Handle<Value> ApacheRequest::GetHeadersOut(Local<String> property, const AccessorInfo& info) {
        ApacheRequest *req = ObjectWrap::Unwrap<ApacheRequest>(info.Holder());
        return req->HeadersOut;
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

        // @todo: handle buffers too; handle multiple strings with rvputs
        String::Utf8Value str(args[0]->ToString());

        req->rputs(*str);

        return Undefined();
    }

    Handle<Value> ApacheRequest::End(const Arguments &args) {
        if(args.Length() > 0) Write(args);

        ApacheRequest *req = ObjectWrap::Unwrap<ApacheRequest>(args.Holder());
        req->end();

        // Signal condition variable

        return Undefined();
    }

    ApacheRequest::ApacheRequest(request_ext *rex): rex(rex) {
        HeadersIn = Persistent<Value>::New(APRTable::New(rex->req->headers_in));
        HeadersOut = Persistent<Value>::New(APRTable::New(rex->req->headers_out));
    }

    ApacheRequest::~ApacheRequest() {
        HeadersIn.Dispose();
        HeadersOut.Dispose();
    }

    void ApacheRequest::rputs(char *str) {
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
