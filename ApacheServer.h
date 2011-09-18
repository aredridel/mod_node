#ifndef __ApacheServer_h__
#define __ApacheServer_h__

#include <v8.h>
#include <node.h>

#include "mod_node.h"

namespace mod_node {
    class ApacheServer : public node::ObjectWrap {
        public:
        static void Initialize(v8::Handle<v8::Object> target);

        protected:
        static v8::Handle<v8::Value> New(const v8::Arguments &args);
        ApacheServer(v8::Handle<v8::Value> v);
    };
}

#endif
