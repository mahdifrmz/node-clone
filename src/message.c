#include <message.h>
#include <stdlib.h>
#include <unistd.h>

void evloop_workmes(message_t mes, int *evl_pipe)
{
    write(evl_pipe[1], &mes, sizeof(message_t));
}

void evloop_workmes_terminate(int *evl_pipe)
{
    message_t mes;
    mes.mtype = mtype_terminate;
    message_terminate *_mes = malloc(sizeof(message_terminate));
    mes.ptr = _mes;
    evloop_workmes(mes, evl_pipe);
}

void evloop_workmes_fileread(char *content, int *evl_pipe)
{
    message_t mes;
    mes.mtype = mtype_fs_fileread;
    message_fs_fileread *_mes = malloc(sizeof(message_fs_fileread));
    _mes->content = content;
    mes.ptr = _mes;
    evloop_workmes(mes, evl_pipe);
}

void evloop_workmes_filewritten(int *evl_pipe)
{
    message_t mes;
    mes.mtype = mtype_fs_filewritten;
    message_fs_filewritten *_mes = malloc(sizeof(message_fs_filewritten));
    mes.ptr = _mes;
    evloop_workmes(mes, evl_pipe);
}