#include <v8.h>
#include <node.h>

namespace mod_node {
	process_rec *process;
	apr_thread_t *thread;
	extern void Initialize(v8::Handle<v8::Object> target);
}
