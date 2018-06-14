//
// Created by Josin on 2018/6/12.
// All rights reserved Licensing with LGPLs
// All users must keep the header note announcement reserved
//


#include "xh.h"

/**
 * To create the pool with the number of the threads
 * @param num
 * @return
 */
struct __pool *pool_init(int num, void *(*main_thread_handler)(void *), void *(*work_thread_handler)(void *))
{
    unsigned int n_index;
    struct __pool *pool = (struct __pool *)malloc(sizeof(struct __pool));
    if (!pool)
    {
        xth_info("pool_init error: malloc failed. \n");
        return NULL;
    }
    bzero(pool, sizeof(_pool));

    pool->mutex_t = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pool->cond_t  = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    bzero(pool->mutex_t, sizeof(pthread_mutex_t));
    bzero(pool->cond_t,sizeof(pthread_cond_t));
    pthread_mutex_init(pool->mutex_t, NULL);
    pthread_cond_init(pool->cond_t, NULL);

    if (num < 0 || num > 256)
    {
        free(pool);
        xth_info("pool_init must know the number of the threads(0~256) to created. \n");
        return NULL;
    }
    pool->threads_num = num;
    bzero(pool->threads, sizeof(pool->threads));

    pool->main_thread_id = pthread_self();
    pool->main_thread_staus = THREAD_PAUSE;

    for ( n_index = 0; n_index < num; n_index++)
    {
        pool->threads[n_index].status = THREAD_WAIT;
        pthread_create( &(pool->threads[n_index].thread_id), NULL,
                        ( n_index == num - 1 ) ?
                        ( !main_thread_handler ? main_thread : main_thread_handler) :
                        ( !work_thread_handler ? work_thread : work_thread_handler), pool);
    }

    return pool;
}

/**
 * Add the task to the given pool with the [pool->threads_num] to locate the location.
 * [NOTE]:
 *  Each threads array only start with zero(0), so the pos must be [pool->threads_num - 1]
 * @param pool
 * @param task
 */
void pool_add_task(_pool *pool, _task *task, int index_pos)
{
    /* When the main thread status was stop or pause,
     * don't add task to the thread task chains */
    if ( pool->main_thread_staus == THREAD_STOP )
    {
        return ;
    }

    unsigned int _t_pos = (int)index_pos % (pool->threads_num - 1);
    xth_info("add job to thread_pos: %d \n", _t_pos);
    TASK_INSERT_TAIL(pool, task, _t_pos);

    pool->threads[_t_pos].status = THREAD_RUN;
    if ( (pool->main_thread_staus == THREAD_RUN)
         || (pool->main_thread_staus == THREAD_WAIT) )
    {
        pthread_cond_broadcast(pool->cond_t);
    }
}

/**
 * It was the detail function to use the before func one.
 * @param pool
 * @param func
 * @param arg
 */
void pool_add_detail_task(_pool *pool, void *func, void *arg, int index_pos)
{
    _task *task = (_task *)malloc(sizeof(_task));
    if (!task)
    {
        xth_info("create task error: malloc failed. \n");
        return ;
    }
    bzero(task, sizeof(_task));
    task->status = TASK_WAIT;
    task->arg    = arg;
    task->func   = func;
    task->next   = NULL;

    pool_add_task(pool, task, index_pos);
}

/**
 * Set the threads in the pools status, which can be THREAD_STOP | THREAD_RUN | THREAD_PAUSE | THREAD_WAIT
 * The main thread will omit the status: THREAD_RUN | THREAD_WAIT(default status in main thread)
 * @param status
 */
void pool_set_threads_stop_or_pause(_pool *pool, unsigned char status)
{
    if (status < THREAD_STOP || status > THREAD_PAUSE)
    {
        xth_info("Error status code, only can be THREAD_STOP or THREAD_PAUSE. \n");
        return ;
    }
    pool->main_thread_staus = status;
    unsigned index = 0;
    for ( ; index < pool->threads_num; index++ )
    {
        pool->threads[index].status = THREAD_STOP;
    }
    /* After setting the status, must send the signal */
    pthread_cond_broadcast(pool->cond_t);
}

/**
 * To start the main thread. if you don't do this, the thread will not be run
 * automatically
 * @param pool
 */
void pool_set_main_thread_run(_pool *pool)
{
    pool->main_thread_staus = THREAD_RUN;
    pthread_cond_broadcast(pool->cond_t);
}

/**
 * In this function, work_thread waiting for job.
 * if current job chain empty, it will block itself, until the chain was not empty. when chain deal over
 * it back to the sleep state.
 * @param arg
 * @return
 */
void *work_thread(void *arg)
{
    unsigned int p_index;
    struct __task *current_task;
    struct __threads *current_thread;
    _pool *pool = (_pool *)arg;
    pthread_mutex_t *__mutex_t = pool->mutex_t;
    pthread_cond_t  *__cond_t  = pool->cond_t;

    long pthread_id = (long)pthread_self();
    for (p_index = 0; p_index < pool->threads_num; p_index++)
    {
        if (pool->threads[p_index].thread_id == (pthread_t)pthread_id)
        {
            current_thread = &(pool->threads[p_index]);
            break;
        }
    }
    /* To loop the thread to do the job. it job chain empty, lock the thread to sleep. */
    while (1)
    {
        if (current_thread->status == THREAD_STOP)
        {
            pthread_exit(NULL);
        }

        pthread_mutex_lock(__mutex_t);
        while ( !current_thread->tasks
                || !current_task
                || (current_thread->status == THREAD_PAUSE) )
        {
            pthread_cond_wait(__cond_t, __mutex_t);
            current_thread = &(pool->threads[p_index]);
            current_task = current_thread->tasks;
        }
        pthread_mutex_unlock(__mutex_t);

        /* After the jobs joined. it will be waked up */
        TASK_FOREACH_VAL(current_thread->tasks, current_task)
        {
            if (current_task->status == TASK_DONE)
            {
                continue;
            }
            /* Deal with the task */
            xth_info("Deal the task: arg:%d<=>%ld \n", (int)current_task->arg, pthread_id);
            current_task->func(current_task->arg);
            xth_info("Deal the task finished: %ld\n", pthread_id);
            current_task->status = TASK_DONE;
        }
        xth_info("Work_thread is finishing, waiting for job coming: %ld: status: %d \n", pthread_id, current_thread->status );
    }
}

/**
 * The main thread to do the dispatching job.
 * @param arg
 * @return
 */
void *main_thread(void *arg)
{
    _pool *pool = (_pool *)arg;

    pthread_mutex_t *__mutex_t = pool->mutex_t;
    pthread_cond_t  *__cond_t  = pool->cond_t;

    while (1)
    {
        if (pool->main_thread_staus == THREAD_STOP)
        {
            pthread_exit(NULL);
        }
        if (pool->main_thread_staus == THREAD_PAUSE)
        {
            xth_info("main thread paused ..\n");
            pthread_mutex_lock(__mutex_t);
            pthread_cond_wait(__cond_t, __mutex_t);
            pthread_mutex_unlock(__mutex_t);
        }
        /* To cancel the main thread if exists. */
        // if (pool->main_thread_id)
        // {
        //     pthread_cancel(pool->main_thread_id);
        //     pool->main_thread_id = 0;
        // }
        pthread_cond_broadcast(pool->cond_t);
        //sleep(5);
    }
}
