#define EV_MULTIPLICITY 0

#include <node.h>
#include <node_events.h>
#include <httpd.h>
#include <http_protocol.h>

#include "mod_node.h"

using namespace v8;

extern "C" void init(Handle<Object> target) {
	mod_node::Initialize(target);
};
