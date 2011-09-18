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
        static void Initialize(Handle<Object> target);
        static Handle<Value> New(request_rec *r);

    protected:
        static Handle<Value> Write(const Arguments &args);

    };
};

#endif
