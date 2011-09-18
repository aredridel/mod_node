#ifndef __mod_node_h__
#define __mod_node_h__
#include <v8.h>
#include <node.h>
#include <apr_thread_proc.h>

#define THROW_BAD_ARGS \
    v8::ThrowException(v8::Exception::TypeError(v8::String::New("Bad argument")))

namespace mod_node {
    extern apr_thread_t *thread;
    extern void Initialize(v8::Handle<v8::Object> target);
    void *start_node(apr_thread_t *th, void* data);
}

#endif
