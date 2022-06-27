#include <stdio.h>
#include <unistd.h>
#include <loop-internal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

void evloop_worker_readline(void *_, int *evl_pipe)
{
    char *line = NULL;
    size_t alloc;
    while (1)
    {
        if (poll_dual(evl_pipe[0], STDIN_FILENO) == 0)
        {
            break;
        }
        else
        {
            getline(&line, &alloc, stdin);
            message_t mes;
            message_readline *_mes = malloc(sizeof(message_readline));
            _mes->string = line;
            mes.ptr = _mes;
            mes.mtype = mtype_readline;
            write(evl_pipe[1], &mes, sizeof(message_t));
        }
    }
}

void evloop_worker_sock_write_client(void *args, int *evl_pipe)
{
    arglist_sock_write_client arglist = *(arglist_sock_write_client *)args;
    write(arglist.client, arglist.message, strlen(arglist.message));
    evloop_workmes_terminate(evl_pipe);
    free(arglist.message);
    free(args);
}

void evloop_worker_sock_accept_client(void *arg, int *evl_pipe)
{
    arglist_sock_accept_client *arglist = arg;
    server_t *server = arglist->server;
    while (1)
    {
        if (poll_dual(evl_pipe[0], server->fd) == 0)
        {
            free(server);
            break;
        }
        else
        {
            message_t mes;
            message_sock_accept_client *_mes = malloc(sizeof(message_sock_accept_client));
            _mes->client = server_accept(server);
            mes.ptr = _mes;
            mes.mtype = mtype_sock_accept_client;
            write(evl_pipe[1], &mes, sizeof(message_t));
        }
    }
}

void evloop_worker_sock_read_client(void *arg, int *evl_pipe)
{
    arglist_sock_read_client *arglist = arg;
    int client = arglist->client;
    while (1)
    {
        if (poll_dual(evl_pipe[0], client) == 0)
        {
            // event loop
            break;
        }
        else
        {
            // client message
            message_t mes;
            char *string = server_recieve(client);
            if (strcmp(string, "") == 0)
            {
                // client closed
                evloop_workmes_terminate(evl_pipe);
                break;
            }
            else
            {
                // client message
                mes.mtype = mtype_sock_read_client;
                message_sock_read_client *_mes = malloc(sizeof(message_sock_read_client));
                _mes->client = client;
                _mes->string = string;
                mes.ptr = _mes;
                write(evl_pipe[1], &mes, sizeof(message_t));
            }
        }
    }
    close(client);
    free(arg);
}

void evloop_worker_timer_timeout(void *arg, int *evl_pipe)
{
    arglist_timer_timeout *arglist = arg;
    message_t mes;
    int pollrsl = poll_timeout(evl_pipe[0], arglist->milisecs);
    if (pollrsl == 1)
    {
        evloop_workmes_terminate(evl_pipe);
    }
    else
    {
        mes.mtype = mtype_timer_tick;
        message_timer_tick *_mes = malloc(sizeof(message_timer_tick));
        mes.ptr = _mes;
        write(evl_pipe[1], &mes, sizeof(message_t));
        evloop_workmes_terminate(evl_pipe);
    }
    free(arg);
}
void evloop_worker_timer_interval(void *arg, int *evl_pipe)
{
    arglist_timer_timeout *arglist = arg;
    message_t mes;
    while (1)
    {
        int pollrsl = poll_timeout(evl_pipe[0], arglist->milisecs);
        if (pollrsl == 1)
        {
            evloop_workmes_terminate(evl_pipe);
            break;
        }
        else
        {
            mes.mtype = mtype_timer_tick;
            message_timer_tick *_mes = malloc(sizeof(message_timer_tick));
            mes.ptr = _mes;
            write(evl_pipe[1], &mes, sizeof(message_t));
            evloop_workmes_terminate(evl_pipe);
        }
    }
    free(arg);
}

void evloop_worker_sock_create_server(void *arg, int *evl_pipe)
{
    arglist_sock_create_server *arglist = arg;
    message_sock_create_server *_mes = malloc(sizeof(message_sock_create_server));
    _mes->server = server_create(arglist->port);
    message_t mes;
    mes.mtype = mtype_sock_create_server;
    mes.ptr = _mes;
    server_listen(&_mes->server);
    write(evl_pipe[1], &mes, sizeof(message_t));
    free(arg);
}

void evloop_worker_fs_readfile(void *arg, int *evl_pipe)
{
    arglist_fs_readfile *arglist = arg;
    int fd = open(arglist->address, O_RDONLY);
    if (fd < 0)
    {
        evloop_workmes_fileread(NULL, evl_pipe);
    }
    else
    {
        char *content = full_read(fd, NULL);
        evloop_workmes_fileread(content, evl_pipe);
    }
    evloop_workmes_terminate(evl_pipe);
    free(arglist->address);
    free(arg);
}

void evloop_worker_fs_writefile(void *arg, int *evl_pipe)
{
    arglist_fs_writefile *arglist = arg;
    int fd = open(arglist->address, O_CREAT | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP);
    if (fd > -1)
    {
        write(fd, arglist->content, strlen(arglist->content));
    }
    evloop_workmes_filewritten(evl_pipe);
    evloop_workmes_terminate(evl_pipe);
    free(arglist->content);
    free(arglist->address);
    free(arg);
}

uint64_t evloop_do_readline(evloop_t *loop, callback_readline cb)
{
    arglist_readline *args = malloc(sizeof(arglist_readline));
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_readline, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}

uint64_t evloop_do_sock_accpet_client(evloop_t *loop, server_t *server, callback_sock_accept_client cb)
{
    arglist_sock_accept_client *args = malloc(sizeof(arglist_sock_accept_client));
    args->server = server;
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_sock_accept_client, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}

uint64_t evloop_do_sock_read_client(evloop_t *loop, int client, callback_sock_read_client cb)
{
    arglist_sock_read_client *args = malloc(sizeof(arglist_sock_read_client));
    args->client = client;
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_sock_read_client, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}

uint64_t evloop_do_sock_write_client(evloop_t *loop, int client, char *message)
{
    arglist_sock_write_client *args = malloc(sizeof(arglist_sock_write_client));
    args->client = client;
    args->message = message;
    evloop_task *task = evloop_task_create(loop, NULL);
    pool_execute(&loop->pool, evloop_worker_sock_write_client, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}

uint64_t evloop_do_sock_create_server(evloop_t *loop, int port, callback_sock_create_server cb)
{
    arglist_sock_create_server *args = malloc(sizeof(arglist_sock_write_client));
    args->port = port;
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_sock_create_server, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}

uint64_t evloop_do_timer_timeout(evloop_t *loop, int milisecs, callback_timer_tick cb)
{
    arglist_timer_timeout *args = malloc(sizeof(arglist_timer_timeout));
    args->milisecs = milisecs;
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_timer_timeout, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}
uint64_t evloop_do_timer_interval(evloop_t *loop, int milisecs, callback_timer_tick cb)
{
    arglist_timer_interval *args = malloc(sizeof(arglist_timer_interval));
    args->milisecs = milisecs;
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_timer_interval, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}
uint64_t evloop_do_fs_readfile(evloop_t *loop, const char *address, callback_fs_fileread cb)
{
    arglist_fs_readfile *args = malloc(sizeof(arglist_fs_readfile));
    args->address = strdup(address);
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_fs_readfile, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}
uint64_t evloop_do_fs_writefile(evloop_t *loop, const char *address, char *content, callback_fs_filewritten cb)
{
    arglist_fs_writefile *args = malloc(sizeof(arglist_fs_writefile));
    args->address = strdup(address);
    args->content = content;
    evloop_task *task = evloop_task_create(loop, cb);
    pool_execute(&loop->pool, evloop_worker_fs_writefile, args, task->pipe);
    evloop_task_hmap_add(loop, task);
    return task->id;
}