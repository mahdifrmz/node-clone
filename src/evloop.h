#include <loop.h>
#include <duktape.h>

void evl_add_functions(duk_context *ctx);
duk_ret_t evl_lib_fs(duk_context *ctx);
duk_ret_t evl_lib_socket(duk_context *ctx);