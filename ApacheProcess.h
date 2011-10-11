#ifndef __ApacheProcess_h__
#define __ApacheProcess_h__

#include "mod_node.h"

using namespace v8;

namespace mod_node {
    class ApacheProcess : public node::ObjectWrap {
        public:
        static void Initialize(Handle<Object> target);
        static void register_hooks(apr_pool_t *p);

        protected:
        static int handler(request_rec *r);
        static void RequestCallback(uv_async_t *handle, int status);
        static Handle<Value> Log(const Arguments &args);
        static Handle<Value> Write(const Arguments& args);
    };
};

#endif
