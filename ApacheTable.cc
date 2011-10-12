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
        }

        table_function = Persistent<Function>::New(table_function_template->GetFunction());
        target->Set(String::New("ApacheTable"), table_function);
    };

    Handle<Object> ApacheTable::New(apr_table_t *tab) {
        Handle<Object> table = table_function->NewInstance();
        ApacheTable *s = new ApacheTable(tab);
        s->Wrap(table);
        return table;
    }

    ApacheTable::ApacheTable(apr_table_t *table) : table(table) {
    }

};
