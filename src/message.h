#ifndef MESSAGE_H
#define MESSAGE_H

#include <server.h>

struct evloop_t;
typedef struct evloop_t evloop_t;

enum mtype_t
{
    mtype_terminate,
    mtype_readline,
    mtype_sock_accept_client,
    mtype_sock_read_client,
    mtype_sock_create_server,
    mtype_timer_tick,
    mtype_fs_fileread,
    mtype_fs_filewritten,
};
typedef enum mtype_t mtype_t;

struct message_terminate
{
};
typedef struct message_terminate message_terminate;

struct message_readline
{
    char *string;
};
typedef struct message_readline message_readline;
typedef void (*callback_readline)(evloop_t *, char *);

struct message_sock_read_client
{
    int client;
    char *string;
};
typedef struct message_sock_read_client message_sock_read_client;
typedef void (*callback_sock_read_client)(evloop_t *, int, char *);

struct message_sock_accept_client
{
    int client;
};
typedef struct message_sock_accept_client message_sock_accept_client;
typedef void (*callback_sock_accept_client)(evloop_t *, int);

struct message_sock_create_server
{
    server_t server;
};
typedef struct message_sock_create_server message_sock_create_server;
typedef void (*callback_sock_create_server)(evloop_t *, server_t);

struct message_timer_tick
{
};
typedef struct message_timer_tick message_timer_tick;
typedef void (*callback_timer_tick)(evloop_t *);

struct message_fs_fileread
{
    char *content;
};
typedef struct message_fs_fileread message_fs_fileread;
typedef void (*callback_fs_fileread)(evloop_t *, char *);

struct message_fs_filewritten
{
};
typedef struct message_fs_filewritten message_fs_filewritten;
typedef void (*callback_fs_filewritten)(evloop_t *);

struct message_t
{
    mtype_t mtype;
    void *ptr;
};
typedef struct message_t message_t;

void evloop_workmes_terminate(int *evl_pipe);
void evloop_workmes_fileread(char *content, int *evl_pipe);
void evloop_workmes_filewritten(int *evl_pipe);
void evloop_workmes(message_t mes, int *evl_pipe);

#endif