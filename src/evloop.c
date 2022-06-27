#include <loop.h>
#include <duktape.h>
#include <unistd.h>

extern evloop_t glb_evloop;
extern duk_context *glb_ctx;

char *stringify_index(size_t idx)
{
    char *string = calloc(sizeof(char), 64);
    int count = sprintf(string, "%lu", idx);
    string[count] = '\0';
    return string;
}

duk_ret_t examine(duk_context *ctx)
{
    printf("-->|");
    int len = duk_get_top(ctx);
    for (int i = 0; i < len; i++)
    {
        int typ = duk_get_type(ctx, i);
        if (typ == DUK_TYPE_BOOLEAN)
        {
            printf("bool|");
        }
        else if (typ == DUK_TYPE_NUMBER)
        {
            printf("number|");
        }
        else if (typ == DUK_TYPE_STRING)
        {
            printf("string|");
        }
        else if (typ == DUK_TYPE_OBJECT)
        {
            printf("object|");
        }
        else if (typ == DUK_TYPE_UNDEFINED)
        {
            printf("undefined|");
        }
        else
        {
            printf("???|");
        }
    }

    fflush(stdout);
    return 0;
}

duk_ret_t evl_do_reg(duk_context *ctx, size_t task_id, size_t cb_idx)
{
    duk_get_global_string(ctx, "Evloop");
    duk_pull(ctx, cb_idx);
    char *task_id_str = stringify_index(task_id);
    duk_put_prop_string(ctx, -2, task_id_str);
    free(task_id_str);
    duk_pop(ctx);
    return 0;
}

char *evl_on_evt_beg(evloop_t *loop)
{
    size_t task_id = evloop_task_id(loop);
    char *task_id_string = stringify_index(task_id);
    duk_get_global_string(glb_ctx, "Evloop");
    duk_get_prop_string(glb_ctx, -1, task_id_string);
    return task_id_string;
}

void evl_on_evt_end(evloop_t *loop, char *task_id_string, int del, int arglen)
{
    duk_call(glb_ctx, arglen);
    duk_pop(glb_ctx);
    if (del)
    {
        duk_del_prop_string(glb_ctx, -1, task_id_string);
    }
    free(task_id_string);
}

void evl_on_readline(evloop_t *loop, char *line)
{
    char *task_id_string = evl_on_evt_beg(loop);
    duk_push_string(glb_ctx, line);
    evl_on_evt_end(loop, task_id_string, 0, 1);
}

duk_ret_t evl_do_readline(duk_context *ctx)
{
    return evl_do_reg(ctx, evloop_do_readline(&glb_evloop, evl_on_readline), 0);
}

void evl_on_timer_timeout(evloop_t *loop)
{
    char *task_id_string = evl_on_evt_beg(loop);
    evl_on_evt_end(loop, task_id_string, 1, 0);
}

duk_ret_t evl_do_timer_timeout(duk_context *ctx)
{
    int secs = duk_get_int(ctx, 1);
    return evl_do_reg(ctx, evloop_do_timer_timeout(&glb_evloop, secs, evl_on_timer_timeout), 0);
}

void evl_on_timer_interval(evloop_t *loop)
{
    char *task_id_string = evl_on_evt_beg(loop);
    evl_on_evt_end(loop, task_id_string, 0, 0);
}

duk_ret_t evl_do_timer_interval(duk_context *ctx)
{
    int secs = duk_get_int(ctx, 1);
    return evl_do_reg(ctx, evloop_do_timer_interval(&glb_evloop, secs, evl_on_timer_interval), 0);
}

void evl_on_fs_readfile(evloop_t *loop, char *content)
{
    char *task_id_string = evl_on_evt_beg(loop);
    duk_push_string(glb_ctx, content);
    evl_on_evt_end(loop, task_id_string, 1, 1);
}

duk_ret_t evl_do_fs_readfile(duk_context *ctx)
{
    const char *address = duk_get_string(ctx, 0);
    return evl_do_reg(ctx, evloop_do_fs_readfile(&glb_evloop, address, evl_on_fs_readfile), 1);
}

void evl_on_fs_writefile(evloop_t *loop)
{
    char *task_id_string = evl_on_evt_beg(loop);
    evl_on_evt_end(loop, task_id_string, 1, 0);
}

duk_ret_t evl_do_fs_writefile(duk_context *ctx)
{
    const char *address = duk_get_string(ctx, 0);
    char *content = strdup(duk_get_string(ctx, 1));
    return evl_do_reg(ctx, evloop_do_fs_writefile(&glb_evloop, address, content, evl_on_fs_writefile), 2);
}

duk_ret_t evl_lib_fs(duk_context *ctx)
{
    duk_push_object(ctx);

    duk_push_c_function(ctx, evl_do_fs_readfile, 2);
    duk_put_prop_string(ctx, -2, "readFile");

    duk_push_c_function(ctx, evl_do_fs_writefile, 3);
    duk_put_prop_string(ctx, -2, "writeFile");

    return 0;
}

duk_ret_t evl_do_sock_accpet_client(duk_context *ctx);
duk_ret_t evl_do_sock_read_client(duk_context *ctx);
duk_ret_t sock_write_client(duk_context *ctx);

void push_server(duk_context *ctx, server_t server)
{
    duk_push_object(ctx);
    duk_push_int(ctx, server.fd);
    duk_put_prop_string(ctx, -2, "fd");
    duk_push_int(ctx, server.port);
    duk_put_prop_string(ctx, -2, "port");
    size_t buffer_size = sizeof(server.address);
    void *buffer_ptr = duk_push_buffer(ctx, buffer_size, 0);
    memcpy(buffer_ptr, &server.address, buffer_size);
    duk_put_prop_string(ctx, -2, "address");
    duk_push_c_function(ctx, evl_do_sock_accpet_client, 2);
    duk_put_prop_string(ctx, -2, "listen");
}

void push_client(duk_context *ctx, int client)
{
    duk_push_object(ctx);
    duk_push_int(ctx, client);
    duk_put_prop_string(ctx, -2, "fd");

    duk_push_c_function(ctx, evl_do_sock_read_client, 1);
    duk_put_prop_string(ctx, -2, "recieve");

    duk_push_c_function(ctx, sock_write_client, 1);
    duk_put_prop_string(ctx, -2, "send");
}

int pop_client(duk_context *ctx)
{
    duk_get_prop_string(ctx, -1, "fd");
    int fd = duk_get_int(ctx, -1);
    duk_pop(ctx);
    duk_pop(ctx);
    return fd;
}

server_t pop_server(duk_context *ctx)
{
    server_t server;
    duk_get_prop_string(ctx, -1, "fd");
    server.fd = duk_get_int(ctx, -1);
    duk_pop(ctx);
    duk_get_prop_string(ctx, -1, "port");
    server.port = duk_get_int(ctx, -1);
    duk_pop(ctx);
    duk_get_prop_string(ctx, -1, "address");
    size_t buffer_size = sizeof(server.address);
    void *buffer = duk_get_buffer(ctx, -1, &buffer_size);
    memcpy(&server.address, buffer, buffer_size);
    duk_pop(ctx);
    duk_pop(ctx);
    return server;
}

void evl_on_sock_create_server(evloop_t *loop, server_t server)
{
    char *task_id_string = evl_on_evt_beg(loop);
    push_server(glb_ctx, server);
    evl_on_evt_end(loop, task_id_string, 1, 1);
}

duk_ret_t evl_do_sock_create_server(duk_context *ctx)
{
    int port = duk_get_int(ctx, 0);
    return evl_do_reg(ctx, evloop_do_sock_create_server(&glb_evloop, port, evl_on_sock_create_server), 1);
}

void evl_on_sock_accpet_client(evloop_t *loop, int client)
{
    char *task_id_string = evl_on_evt_beg(loop);
    push_client(glb_ctx, client);
    evl_on_evt_end(loop, task_id_string, 0, 1);
}

duk_ret_t evl_do_sock_accpet_client(duk_context *ctx)
{
    server_t *server = malloc(sizeof(server_t));
    duk_push_this(ctx);
    *server = pop_server(ctx);
    return evl_do_reg(ctx, evloop_do_sock_accpet_client(&glb_evloop, server, evl_on_sock_accpet_client), 0);
}

void evl_on_sock_read_client(evloop_t *loop, int _, char *message)
{
    char *task_id_string = evl_on_evt_beg(loop);
    duk_push_string(glb_ctx, message);
    evl_on_evt_end(loop, task_id_string, 0, 1);
}

duk_ret_t evl_do_sock_read_client(duk_context *ctx)
{
    duk_push_this(ctx);
    int client = pop_client(ctx);
    return evl_do_reg(ctx, evloop_do_sock_read_client(&glb_evloop, client, evl_on_sock_read_client), 0);
}

duk_ret_t sock_write_client(duk_context *ctx)
{
    duk_push_this(ctx);
    int client = pop_client(ctx);
    const char *message = duk_get_string(ctx, 0);
    write(client, message, strlen(message));
    return 0;
}

duk_ret_t evl_lib_socket(duk_context *ctx)
{
    duk_push_object(ctx);

    duk_push_c_function(ctx, evl_do_sock_create_server, 2);
    duk_put_prop_string(ctx, -2, "createServer");

    return 0;
}

void evl_add_functions(duk_context *ctx)
{
    duk_push_c_function(ctx, evl_do_readline, 1);
    duk_put_global_string(ctx, "readline");

    duk_push_c_function(ctx, evl_do_timer_timeout, 2);
    duk_put_global_string(ctx, "setTimeout");

    duk_push_c_function(ctx, evl_do_timer_interval, 2);
    duk_put_global_string(ctx, "setInterval");
}