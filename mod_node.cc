#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_log.h>
#include <apr_thread_proc.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_queue.h>

#include <mod_proxy.h>

#undef EV_MULTIPLICITY

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

#include "mod_node.h"
#include "ApacheProcess.h"
#include "ApacheRequest.h"

using namespace v8;

typedef struct {
    char *startup_script;
} node_module_config;

static const char * node_cmd_startup_script(cmd_parms *cmd, void *dcfg, const char *arg);


static void *create_node_module_config(apr_pool_t *p, server_rec *s) {
    node_module_config *conf;
    conf = (node_module_config *) apr_pcalloc(p, sizeof(node_module_config));
    conf->startup_script = ap_server_root_relative(p, "runtime.js");
    return (void *)conf;
};

static const command_rec node_config_cmds[] = {
    AP_INIT_TAKE1("NodeStartupScript", reinterpret_cast<const char* (*)()>(node_cmd_startup_script), NULL, RSRC_CONF, "Path to the startup script"),
    { NULL }    
};


namespace mod_node {

    class ApacheServer : public node::ObjectWrap {
        public:
        static void Initialize(Handle<Object> target) {
            HandleScope scope;
            v8::Locker l;
            Local<FunctionTemplate> t = FunctionTemplate::New(New);
            // @todo Connect up the apache request to emitting a Request here
            target->Set(String::New("Server"), t->GetFunction());
        }

        protected:
        static Handle<Value> New(const Arguments &args) {
            HandleScope scope;

            if (args.Length() < 1 || !args[0]->IsFunction()) {
                return THROW_BAD_ARGS;
            }

            ApacheServer *s = new ApacheServer(args[0]);
            s->Wrap(args.Holder());

            return args.This();
        };

        ApacheServer(Handle<Value> v) {
            handle_->SetInternalField(1, v);
        }

    };

    extern void Initialize(v8::Handle<v8::Object> target) {
        ApacheProcess::Initialize(target);
        ApacheRequest::Initialize(target);
        ApacheServer::Initialize(target);
    };
};

extern "C" {
    module AP_MODULE_DECLARE_DATA node_module = {
        STANDARD20_MODULE_STUFF,
        NULL, /* dir config creater */
        NULL, /* dir merger --- default is to override */
        create_node_module_config, /* server config */
        NULL, /* merge server config */
        node_config_cmds, /* commands */
        mod_node::ApacheProcess::register_hooks
    };
};

static const char * node_cmd_startup_script(cmd_parms *cmd, void *dcfg, const char *arg) {
    node_module_config *conf = (node_module_config *)ap_get_module_config(cmd->server->module_config, &node_module);
    conf->startup_script = ap_server_root_relative(cmd->pool, arg);
    return NULL;
};

namespace mod_node {
    void *start_node(apr_thread_t *th, void* data) {
        char **argv;
        server_rec *s = (server_rec *)data;
        node_module_config *conf = (node_module_config *)ap_get_module_config(s->module_config, &node_module);
        argv = (char **)malloc(sizeof(char *));
            argv[0] = (char *)malloc(32);
        strcpy(argv[0], "node");
        argv[1] = strdup(conf->startup_script);
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "mod_node::startup script: %s", argv[1]);
        // @todo error handling if the runtime script throws an exception
        // Probably atexit(3) and log an error and clean up, return a 500 error.
        node::Start(2, argv);
        return APR_SUCCESS;
    }

    apr_thread_t *thread;
}
