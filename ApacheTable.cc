#include "ApacheTable.h"

using namespace v8;


namespace mod_node {
    static Persistent<FunctionTemplate> table_function_template;
    static Persistent<Function> table_function;

    void ApacheTable::Initialize(Handle<Object> target) {
        HandleScope scope;
        if(table_function_template.IsEmpty()) {
            table_function_template = Persistent<FunctionTemplate>(FunctionTemplate::New());
            table_function_template->SetClassName(String::NewSymbol("ApacheTable"));
            Handle <ObjectTemplate> itemplate = table_function_template->InstanceTemplate();
            itemplate->SetInternalFieldCount(1);
            itemplate->SetNamedPropertyHandler(ApacheTable::MapGet, ApacheTable::MapSet);
            table_function = Persistent<Function>::New(table_function_template->GetFunction());
        }

        target->Set(String::New("ApacheTable"), table_function);
    };

    Handle<Value> ApacheTable::MapGet(Local<String> property, const AccessorInfo& info) {
        ApacheTable *table = ObjectWrap::Unwrap<ApacheTable>(info.Holder());
        const char *ret = apr_table_get(table->table, *String::Utf8Value(property));
        if(ret) {
            return String::New(ret);
        } else {
            return Undefined();
        }
    }

    Handle<Value> ApacheTable::MapSet(Local<String> property, Local<Value> Value, const AccessorInfo& info) {
        ApacheTable *table = ObjectWrap::Unwrap<ApacheTable>(info.Holder());
        String::Utf8Value prop(property);
        String::Utf8Value val(Value);
        apr_table_set(table->table, *prop, *val);
        return Value;
    }

    Handle<Object> ApacheTable::New(apr_table_t *tab) {
        Handle<Object> table = table_function->NewInstance();
        ApacheTable *s = new ApacheTable(tab);
        s->Wrap(table);
        return table;
    }

    ApacheTable::ApacheTable(apr_table_t *table) : table(table) {
    }

};
