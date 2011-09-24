#ifndef __ApacheRequest_h__
#define __ApacheRequest_h__

#include "mod_node.h"

namespace mod_node {
    class ApacheRequest : public node::ObjectWrap {
    public:
        request_rec *r;
        static void Initialize(v8::Handle<v8::Object> target);
        static v8::Handle<v8::Object> New(request_rec *r);

    protected:
        static v8::Handle<v8::Value> Write(const v8::Arguments &args);
        static v8::Handle<v8::Value> End(const v8::Arguments &args);
        ApacheRequest(request_rec *req);
        void write(char *str);


    };
};

#endif
