#include <pool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

void handle_client(int client);

void *_worker(void *args)
{
    task_t task;
    threadpool_t *pool = args;
    while (1)
    {
        pthread_mutex_lock(pool->mutex);
        read(pool->queue_pipe[0], &task, sizeof(task_t));
        pthread_mutex_unlock(pool->mutex);
        if (task.worker == NULL)
        {
            break;
        }
        else
        {
            task.worker(task.args, task.pipe);
        }
    }
    return NULL;
}

void pool_initialize(threadpool_t *pool, size_t thread_count)
{
    pool->mutex = malloc(sizeof(pthread_mutex_t));
    pipe(pool->queue_pipe);
    pthread_mutex_init(pool->mutex, NULL);
    pool->threads = calloc(sizeof(threadpool_t), thread_count);
    for (size_t i = 0; i < thread_count; i++)
    {
        pthread_t thread;
        pthread_create(&thread, NULL, _worker, pool);
        ((pthread_t *)pool->threads)[i] = thread;
    }
    pool->thread_count = thread_count;
}

void pool_destroy(threadpool_t *pool)
{
    for (size_t i = 0; i < pool->thread_count; i++)
    {
        int fd[2];
        pool_execute(pool, NULL, NULL, fd);
    }
    for (size_t i = 0; i < pool->thread_count; i++)
    {
        void *ret;
        pthread_join(((pthread_t *)pool->threads)[i], &ret);
    }
    free(pool->threads);
    pthread_mutex_destroy(pool->mutex);
    free(pool->mutex);
    close(pool->queue_pipe[0]);
    close(pool->queue_pipe[1]);
}

void pool_execute(threadpool_t *pool, void *worker, void *args, int *pipe_fds)
{
    task_t task;
    task.worker = worker;
    task.args = args;

    pipe(task.pipe);
    pipe(pipe_fds);
    //swap
    int tmp = task.pipe[1];
    task.pipe[1] = pipe_fds[1];
    pipe_fds[1] = tmp;

    write(pool->queue_pipe[1], &task, sizeof(task_t));
}