#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <hashmap.h>
#include <server.h>
#include <loop-internal.h>

int evloop_task_cmp(const void *t1, const void *t2, void *_)
{
    int p1 = ((evloop_task *)t1)->pipe[0];
    int p2 = ((evloop_task *)t2)->pipe[0];
    if (p1 > p2)
    {
        return 1;
    }
    else if (p1 < p2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

uint64_t evloop_task_hash(const void *task, uint64_t seed0, uint64_t seed1)
{
    int p = ((evloop_task *)task)->pipe[0];
    return hashmap_sip(&p, sizeof(int), seed0, seed1);
}

bool evloop_task_iter_readpipe(const void *item, void *udata)
{
    int **array_ptr = udata;
    **array_ptr = ((evloop_task *)item)->pipe[0];
    (*array_ptr)++;
    return true;
}

bool evloop_task_iter_writepipe(const void *item, void *udata)
{
    int **array_ptr = udata;
    **array_ptr = ((evloop_task *)item)->pipe[1];
    (*array_ptr)++;
    return true;
}

void evloop_task_hmap_init(evloop_t *loop)
{
    loop->map = hashmap_new(sizeof(evloop_task), 0, 0, 0, evloop_task_hash, evloop_task_cmp, NULL, NULL);
}
void evloop_task_hmap_destory(evloop_t *loop)
{
    hashmap_free(loop->map);
}
void evloop_task_hmap_add(evloop_t *loop, evloop_task *task)
{
    hashmap_set(loop->map, task);
}
evloop_task *evloop_task_hmap_get(evloop_t *loop, int pipe_fd)
{
    evloop_task task;
    task.pipe[0] = pipe_fd;
    return hashmap_get(loop->map, &task);
}
void evloop_task_hmap_delete(evloop_t *loop, int pipe_fd)
{
    evloop_task task;
    task.pipe[0] = pipe_fd;
    hashmap_delete(loop->map, &task);
}

int *evloop_task_hmap_list_readpipe(evloop_t *loop, size_t *len)
{
    *len = hashmap_count(loop->map);
    int *array = calloc(sizeof(int), *len);
    hashmap_scan(loop->map, evloop_task_iter_readpipe, &array);
    return array - *len;
}

size_t evloop_task_hmap_count(evloop_t *loop)
{
    return hashmap_count(loop->map);
}

int *evloop_task_hmap_list_writepipe(evloop_t *loop, size_t *len)
{
    *len = hashmap_count(loop->map);
    int *array = calloc(sizeof(int), *len);
    hashmap_scan(loop->map, evloop_task_iter_writepipe, &array);
    return array - *len;
}

evloop_task *evloop_task_create(evloop_t *loop, void *cb)
{
    evloop_task *task = malloc(sizeof(evloop_task));
    task->cb = cb;
    task->id = loop->next_task_id++;
    return task;
}

int poll_dual(int fd1, int fd2)
{
    struct pollfd fds[2];
    fds[0].fd = fd1;
    fds[0].events = POLLIN;
    fds[1].fd = fd2;
    fds[1].events = POLLIN;
    poll(fds, 2, -1);
    return fds[1].revents == POLLIN;
}

int poll_timeout(int fd, int milisecs)
{
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    poll(fds, 1, milisecs);
    return fds[1].revents == POLLIN;
}

size_t poll_array(int *fds, size_t len)
{
    struct pollfd pfds[len];
    for (size_t i = 0; i < len; i++)
    {
        pfds[i].fd = fds[i];
        pfds[i].events = POLLIN;
    }
    poll(pfds, len, -1);
    for (size_t i = 0; i < len; i++)
    {
        if (pfds[i].revents == POLLIN)
        {
            return i;
        }
    }
    return 0;
}

char *full_read(int fd, size_t *size)
{
    size_t local_size;
    if (size == NULL)
    {
        size = &local_size;
    }

    int to_read = 1;
    char *buffer = calloc(sizeof(char), to_read + 1);
    while (1)
    {
        ssize_t is_read = read(fd, buffer + *size, to_read);
        if (is_read == -1)
        {
            *size = 0;
            return NULL;
        }
        *size += is_read;
        if (is_read < to_read)
        {
            break;
        }
        else
        {
            to_read *= 2;
            buffer = realloc(buffer, *size + to_read + 1);
        }
    }
    buffer[*size] = '\0';
    return realloc(buffer, *size + 1);
}

void evloop_initialize(evloop_t *loop, size_t thread_count)
{
    loop->mode = evloop_mode_normal;
    loop->end = 0;
    loop->next_task_id = 0;
    evloop_task_hmap_init(loop);
    pool_initialize(&loop->pool, thread_count);
}

void evloop_destroy(evloop_t *loop)
{
    pool_destroy(&loop->pool);
    evloop_task_hmap_destory(loop);
}

void evloop_set_free(evloop_t *loop)
{
    loop->mode = evloop_mode_free;
}

uint64_t evloop_task_id(evloop_t *loop)
{
    return loop->current_task_id;
}

message_t evloop_get_message(int pollfd)
{
    message_t mes;
    read(pollfd, &mes, sizeof(message_t));
    return mes;
}

int evloop_poll(evloop_t *loop)
{
    size_t len;
    int *pollfds = evloop_task_hmap_list_readpipe(loop, &len);
    return pollfds[poll_array(pollfds, len)];
}

void evloop_terminate(evloop_t *loop)
{
    loop->end = 1;
}

void evloop_abort(evloop_t *loop)
{
    char term = '\n';
    size_t len;
    int *pollfds = evloop_task_hmap_list_writepipe(loop, &len);
    for (size_t i = 0; i < len; i++)
    {
        write(pollfds[i], &term, sizeof(char));
    }
}

void evloop_main_loop(evloop_t *loop)
{
    while (loop->end == 0)
    {
        if (loop->mode == evloop_mode_normal)
        {
            size_t count = evloop_task_hmap_count(loop);
            if (count == 0)
            {
                break;
            }
        }

        int pollfd = evloop_poll(loop);
        evloop_task *task = evloop_task_hmap_get(loop, pollfd);
        loop->current_task_id = task->id;
        message_t mes = evloop_get_message(pollfd);
        if (mes.mtype == mtype_terminate)
        {
            evloop_task_hmap_delete(loop, pollfd);
        }
        else if (mes.mtype == mtype_readline)
        {
            message_readline *_mes = mes.ptr;
            callback_readline cb = task->cb;
            cb(loop, _mes->string);
        }
        else if (mes.mtype == mtype_sock_accept_client)
        {
            message_sock_accept_client *_mes = mes.ptr;
            callback_sock_accept_client cb = task->cb;
            cb(loop, _mes->client);
        }
        else if (mes.mtype == mtype_sock_read_client)
        {
            message_sock_read_client *_mes = mes.ptr;
            callback_sock_read_client cb = task->cb;
            cb(loop, _mes->client, _mes->string);
        }
        else if (mes.mtype == mtype_sock_create_server)
        {
            message_sock_create_server *_mes = mes.ptr;
            callback_sock_create_server cb = task->cb;
            cb(loop, _mes->server);
        }
        else if (mes.mtype == mtype_fs_fileread)
        {
            message_fs_fileread *_mes = mes.ptr;
            callback_fs_fileread cb = task->cb;
            cb(loop, _mes->content);
        }
        else if (mes.mtype == mtype_fs_filewritten)
        {
            callback_fs_filewritten cb = task->cb;
            cb(loop);
        }
        else if (mes.mtype == mtype_timer_tick)
        {
            callback_timer_tick cb = task->cb;
            cb(loop);
        }
        else
        {
            // err
            // undefined message
        }
        free(mes.ptr);
    }

    evloop_abort(loop);
}