//
// Created by Josin on 2018/6/12.
//

#ifndef THREADS_XH_H
#define THREADS_XH_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

enum TASK_STATUS
{
    TASK_WAIT = 1,
    TASK_DONE
};

enum THREAD_STATUS
{
    THREAD_WAIT = 1,
    THREAD_RUN,
    THREAD_STOP,
    THREAD_PAUSE
};

typedef struct __task
{
    void *(*func)(void *);
    void *arg;
    unsigned char status;
    /* To find the next task from the chain. */
    struct __task *next;
} _task;

typedef struct __threads
{
    unsigned char status;
    pthread_t thread_id;
    /* It was a chain */
    struct __task *tasks;
    /* Not to modify the below name, only used in kernel. */
    struct __task *end_task;
} _threads;

typedef struct __pool
{
    unsigned char main_thread_staus;
    unsigned int threads_num;
    struct __threads threads[256];
    pthread_mutex_t *mutex_t;
    pthread_cond_t *cond_t;
    pthread_t main_thread_id;
} _pool;

#define xth_info(str, data...) printf(str, ##data)

#define TASK_INSERT_TAIL(pool, task, pos)               \
    do {                                                \
        if (!(pool)->threads[pos].tasks){               \
            (pool)->threads[pos].tasks = task;          \
            (pool)->threads[pos].end_task = task;       \
        } else {                                        \
            (pool)->threads[pos].end_task->next = task; \
            (pool)->threads[pos].end_task = task;       \
        }\
    } while (0);
#define TASK_FOREACH_VAL(task, var)                     \
    for ( (var) = (task); (var); (var) = (var)->next )

typedef void *(*callback_func)(void *);

struct __pool *pool_init(int num, callback_func, callback_func);

void *work_thread(void *arg);

void *main_thread(void *arg);

void pool_add_task(_pool *pool, _task *task, int index_pos);

void pool_add_detail_task(_pool *pool, void *func, void *arg, int index_pos);

void pool_set_threads_stop_or_pause(_pool *pool, unsigned char status);

void pool_set_main_thread_run(_pool *pool);

#endif //THREADS_XH_H
