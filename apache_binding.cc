#include <node.h>
#include <httpd.h>
#include <http_protocol.h>

#include "mod_node.h"

using namespace v8;

extern "C" void init(Handle<Object> target) {
	mod_node::Initialize(target);
};
