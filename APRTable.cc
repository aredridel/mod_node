#include "APRTable.h"

using namespace v8;


namespace mod_node {
    static Persistent<FunctionTemplate> table_function_template;
    static Persistent<Function> table_function;

    /** Initialize the module when node loads us.
     *
     * Builds the constructor function and prototype.
     */
    void APRTable::Initialize(Handle<Object> target) {
        HandleScope scope;
        if(table_function_template.IsEmpty()) {
            table_function_template = Persistent<FunctionTemplate>(FunctionTemplate::New());

            table_function_template->SetClassName(String::NewSymbol("APRTable"));

            Handle <ObjectTemplate> itemplate = table_function_template->InstanceTemplate();
            itemplate->SetInternalFieldCount(1);

            NODE_SET_PROTOTYPE_METHOD(table_function_template, "isEmpty", IsEmpty);
            NODE_SET_PROTOTYPE_METHOD(table_function_template, "get", Get);
            NODE_SET_PROTOTYPE_METHOD(table_function_template, "set", Set);
            //NODE_SET_PROTOTYPE_METHOD(table_function_template, "add", Add);
            //
            table_function = Persistent<Function>::New(table_function_template->GetFunction());
        }

        target->Set(String::New("APRTable"), table_function);
    };

    /** Create the wrapper and wrap the table.
     */
    Handle<Object> APRTable::New(apr_table_t *tab) {
        Handle<Object> table = table_function->NewInstance();
        APRTable *s = new APRTable(tab);
        s->Wrap(table);
        return table;
    }

    /** Empty the table.
     */
    Handle<Value> APRTable::Clear(const Arguments &args) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(args.Holder());
        apr_table_clear(table->table);
        return Undefined();
    }

    /** Determine whether the table is empty.
     */
    Handle<Value> APRTable::IsEmpty(const Arguments &args) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(args.Holder());
        return apr_is_empty_table(table->table) ? True() : False();
    }

    /** Get an entry from the table.
     */
    Handle<Value> APRTable::Get(const Arguments &args) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(args.Holder());
        if (args.Length() < 1 || !args[0]->IsString())
             return THROW_BAD_ARGS;
        Local<String> arg = Local<String>::Cast(args[0]);

        String::Utf8Value str(arg);

        const char *ret = apr_table_get(table->table, *str);

        if(ret) {
            return String::New(ret);
        } else {
            return Null();
        }
    }

    /** Set an entry in the table.
     */
    Handle<Value> APRTable::Set(const Arguments &args) {
        APRTable *table = ObjectWrap::Unwrap<APRTable>(args.Holder());
        if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString())
             return THROW_BAD_ARGS;
        String::Utf8Value key(Local<String>::Cast(args[0]));
        String::Utf8Value val(Local<String>::Cast(args[1]));

        // @todo error handling
        apr_table_set(table->table, *key, *val);
        return args[1];
    }

    /** Minimal constructor
     */
    APRTable::APRTable(apr_table_t *table) : table(table) { }

};
