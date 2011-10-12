#ifndef __ApacheTable_h__
#define __ApacheTable_h__

#include "mod_node.h"

namespace mod_node {

    class ApacheTable : public node::ObjectWrap {
    public:
        apr_table_t *table;

        static void Initialize(v8::Handle<v8::Object> target);
        static v8::Handle<v8::Object> New(apr_table_t *tab);
        static v8::Handle<v8::Value> Get(v8::Local<v8::String> property, const v8::AccessorInfo& info);
        static void Set(v8::Local<v8::String> property, v8::Local<v8::Value> Value, const v8::AccessorInfo& info);

        ApacheTable(apr_table_t *table);
    };
};

#endif
