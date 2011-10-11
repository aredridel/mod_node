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
        /** Write a string, strings, or buffer to the client
         *
         * @param args v8 Arguments a string, strings, or buffer to write 
         */
        static v8::Handle<v8::Value> Write(const v8::Arguments &args);

        /** Writes any final data to the client and releases the response
         *
         * @see ApacheRequest::Write
         */
        static v8::Handle<v8::Value> End(const v8::Arguments &args);

        ApacheRequest(request_ext *rex);
        void rputs(char *str);
        void end();


    };
};

#endif
