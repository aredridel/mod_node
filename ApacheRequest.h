#ifndef __ApacheRequest_h__
#define __ApacheRequest_h__

#include "mod_node.h"

namespace mod_node {
    typedef struct {
        int cont_request;
        apr_thread_cond_t *cv;
        apr_thread_mutex_t *mtx;
        request_rec *req;
    } request_ext;

    class ApacheRequest : public node::ObjectWrap {
    public:
        request_ext *rex;
        static void Initialize(v8::Handle<v8::Object> target);
        static v8::Handle<v8::Object> New(request_ext *rex);

    protected:
        static v8::Handle<v8::Value> Write(const v8::Arguments &args);
        static v8::Handle<v8::Value> End(const v8::Arguments &args);
        ApacheRequest(request_ext *rex);
        void write(char *str);
        void end();


    };
};

#endif
