#ifndef __ApacheRequest_h__
#define __ApacheRequest_h__

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

#include <apr_pools.h>
#include <apr_queue.h>

#include <httpd.h>
#include <apr_thread_proc.h>

#include "mod_node.h"

namespace mod_node {
    class ApacheRequest : public node::ObjectWrap {
    public:
        request_rec *r;
        static void Initialize(v8::Handle<v8::Object> target);
        static v8::Handle<v8::Value> New(request_rec *r);

    protected:
        static v8::Handle<v8::Value> Write(const v8::Arguments &args);
        ApacheRequest();
        ApacheRequest(request_rec *req);
        void write(char *str);


    };
};

#endif
