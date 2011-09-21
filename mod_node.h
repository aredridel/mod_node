#ifndef __mod_node_h__
#define __mod_node_h__

#define xEV_MULTIPLICITY 1

#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_log.h>
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_queue.h>

#include <mod_proxy.h>

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

#define THROW_BAD_ARGS \
    v8::ThrowException(v8::Exception::TypeError(v8::String::New("Bad argument")))

namespace mod_node {
    extern apr_thread_t *thread;
    extern void Initialize(v8::Handle<v8::Object> target);
    extern void *start_node(apr_thread_t *th, void* data);
}


#endif
