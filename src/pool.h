#ifndef POOL_H
#define POOL_H

#include <stddef.h>

typedef void *pthread_ptr;
typedef void *pthread_mutex_ptr;

struct threadpool_t
{
    pthread_ptr threads;
    size_t thread_count;
    int queue_pipe[2];
    pthread_mutex_ptr mutex;
};
typedef struct threadpool_t threadpool_t;

struct event_t
{
    void *ret;
    size_t task_id;
};
typedef struct event_t event_t;

typedef void (*worker_t)(void *, int *);

struct task_t
{
    worker_t worker;
    int pipe[2];
    void *args;
};
typedef struct task_t task_t;

void pool_initialize(threadpool_t *pool, size_t thread_count);
void pool_destroy(threadpool_t *pool);
void pool_execute(threadpool_t *pool, void *del, void *args, int *pipe_fds);
event_t pool_poll(threadpool_t *pool);

#endif