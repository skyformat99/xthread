//
// Created by Josin on 2018/6/12.
// All rights reserved Licensing with LGPLs
// All users must keep the header note announcement reserved
//

#include <stdio.h>

#include "xh.h"

void *job(void *arg)
{
    printf("job working done:%d=>%ld\n",(int)arg, (long)pthread_self());
    return NULL;
}


int main(int argc, char *argv[])
{
    _pool *pools = pool_init(4, NULL, NULL);

    int task_num = 10, task_index = 0;
    for (; task_index < task_num; task_index++)
    {
        pool_add_detail_task(pools, job, (void *)(long)task_index, task_index);
    }
    pool_set_main_thread_run(pools);
    sleep(5);
    pool_set_threads_stop_or_pause(pools, THREAD_PAUSE);
    sleep(3);
    pool_set_threads_stop_or_pause(pools, THREAD_STOP);
    pthread_exit(NULL);
    return 0;
}