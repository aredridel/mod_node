#ifndef __APRTable_h__
#define __APRTable_h__

#include "mod_node.h"

namespace mod_node {

    class APRTable : public node::ObjectWrap {
    public:
        apr_table_t *table;

        static void Initialize(v8::Handle<v8::Object> target);
        static v8::Handle<v8::Object> New(apr_table_t *tab);
        static v8::Handle<v8::Value> Unset(const v8::Arguments &args);
        static v8::Handle<v8::Value> Overlay(const v8::Arguments &args);
        static v8::Handle<v8::Value> Clear(const v8::Arguments &args);
        static v8::Handle<v8::Value> Compress(const v8::Arguments &args);
        static v8::Handle<v8::Value> Add(const v8::Arguments &args);
        static v8::Handle<v8::Value> IsEmpty(const v8::Arguments &args);
        static v8::Handle<v8::Value> Get(const v8::Arguments &args);
        static v8::Handle<v8::Value> Set(const v8::Arguments &args);

        // Do copy operator using table clone function

        APRTable(apr_table_t *table);
    };
};

#endif
