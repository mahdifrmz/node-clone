#ifndef LOOP_H
#define LOOP_H

#include <pool.h>
#include <server.h>
#include <message.h>

enum evloop_mode
{
    evloop_mode_free,
    evloop_mode_normal
};
typedef enum evloop_mode evloop_mode;

struct evloop_t
{
    threadpool_t pool;
    void *map;
    int end;
    evloop_mode mode;
    uint64_t next_task_id;
    uint64_t current_task_id;
};
typedef struct evloop_t evloop_t;

void evloop_initialize(evloop_t *loop, size_t thread_count);
void evloop_main_loop(evloop_t *loop);
int evloop_poll(evloop_t *loop);
void evloop_destroy(evloop_t *loop);
void evloop_terminate(evloop_t *loop);
uint64_t evloop_task_id(evloop_t *loop);

// api
uint64_t evloop_do_readline(evloop_t *loop, callback_readline cb);
uint64_t evloop_do_sock_accpet_client(evloop_t *loop, server_t *server, callback_sock_accept_client cb);
uint64_t evloop_do_sock_read_client(evloop_t *loop, int client, callback_sock_read_client cb);
uint64_t evloop_do_sock_write_client(evloop_t *loop, int client, char *message);
uint64_t evloop_do_sock_create_server(evloop_t *loop, int port, callback_sock_create_server cb);
uint64_t evloop_do_timer_timeout(evloop_t *loop, int milisecs, callback_timer_tick cb);
uint64_t evloop_do_timer_interval(evloop_t *loop, int milisecs, callback_timer_tick cb);
uint64_t evloop_do_fs_readfile(evloop_t *loop, const char *address, callback_fs_fileread cb);
uint64_t evloop_do_fs_writefile(evloop_t *loop, const char *address, char *content, callback_fs_filewritten cb);
#endif