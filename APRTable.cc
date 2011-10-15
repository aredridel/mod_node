#include "APRTable.h"

using namespace v8;


namespace mod_node {
    static Persistent<FunctionTemplate> table_function_template;
    static Persistent<Function> table_function;

    void APRTable::Initialize(Handle<Object> target) {
        HandleScope scope;
        if(table_function_template.IsEmpty()) {
            table_function_template = Persistent<FunctionTemplate>(FunctionTemplate::New());

            table_function_template->SetClassName(String::NewSymbol("APRTable"));

            Handle <ObjectTemplate> itemplate = table_function_template->InstanceTemplate();
            itemplate->SetInternalFieldCount(1);
            itemplate->SetNamedPropertyHandler(APRTable::MapGet, APRTable::MapSet);

            NODE_SET_PROTOTYPE_METHOD(table_function_template, "isEmpty", IsEmpty);
            //NODE_SET_PROTOTYPE_METHOD(table_function_template, "add", Add);
            //
            table_function = Persistent<Function>::New(table_function_template->GetFunction());
        }

        target->Set(String::New("APRTable"), table_function);
    };

    Handle<Value> APRTable::MapGet(Local<String> property, const AccessorInfo& info) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(info.Holder());
        const char *ret = apr_table_get(table->table, *String::Utf8Value(property));
        if(ret) {
            return String::New(ret);
        } else {
            return Undefined();
        }
    }

    Handle<Value> APRTable::MapSet(Local<String> property, Local<Value> Value, const AccessorInfo& info) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(info.Holder());
        String::Utf8Value prop(property);
        String::Utf8Value val(Value);
        apr_table_set(table->table, *prop, *val);
        return Value;
    }

    Handle<Object> APRTable::New(apr_table_t *tab) {
        Handle<Object> table = table_function->NewInstance();
        APRTable *s = new APRTable(tab);
        s->Wrap(table);
        return table;
    }

    Handle<Value> APRTable::Clear(const Arguments &args) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(args.Holder());
        apr_table_clear(table->table);
        return Undefined();
    }

    Handle<Value> APRTable::IsEmpty(const Arguments &args) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(args.Holder());
        if (apr_is_empty_table(table->table)) {
            return True();
        } else {
            return False();
        }
    }

    APRTable::APRTable(apr_table_t *table) : table(table) {
    }

};
