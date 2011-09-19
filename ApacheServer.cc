#include "ApacheServer.h"

using namespace v8;
using namespace node;

namespace mod_node {
    void ApacheServer::Initialize(Handle<Object> target) {
        v8::Locker l;
        HandleScope scope;
        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        // @todo Connect up the apache request to emitting a Request here
        target->Set(String::New("Server"), t->GetFunction());
    }

    Handle<Value> ApacheServer::New(const Arguments &args) {
        HandleScope scope;

        if (args.Length() < 1 || !args[0]->IsFunction()) {
            return THROW_BAD_ARGS;
        }

        ApacheServer *s = new ApacheServer(args[0]);
        s->Wrap(args.Holder());

        return args.This();
    }

    ApacheServer::ApacheServer(Handle<Value> v) {
        handle_->SetInternalField(1, v);
    }

}
