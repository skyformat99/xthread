# 多线程C语言库 #

**xthread** 只支持使用Pthread库的系统，如Unix or Linux

**Author**: **Josin** 774542602@qq.com

## 基于Pthreads库开发 ##

	开发此库的原因是想实现一个多任务的处理，各个线程并发执行，解决某些情况下单线程的效率问题
	
## 简化完整源码提供  ##
	
	两个文件：xh.c xh.h
	
## 使用方法 ##

1. 拷贝头文件、源文件到项目目录
	
	
2. 引入头文件 xh.h
	
	
3. 在需要的地方使用如下代码：


```c

/* 工作线程每个任务需要处理的逻辑 */
void *job(void *arg)
{
    printf("job working done:%d=>%ld\n",(int)arg, (long)pthread_self());
    return NULL;
}


/* 初始化4个线程，后两个参数分别表示：任务线程与主线程
 * 传入NULL，表示使用系统自带的线程，默认为NULL即可
 */
_pool *pools = pool_init(4, NULL, NULL);
	
/* 设置添加的任务多少 */
int task_num = 10, task_index = 0;
for (; task_index < task_num; task_index++)
{
	 /* 添加任务队列 */
    pool_add_detail_task(pools, job, (void *)(long)task_index, task_index);
}

/* 开启主线程，默认情况下主线程是暂停状态 */
pool_set_main_thread_run(pools);
/* 任务队列执行5秒 */
sleep(5);
/* 暂停任务队列的处理，系统会把当前的任务处理完成后，暂停 */
pool_set_threads_stop_or_pause(pools, THREAD_PAUSE);
/* 暂停任务队列3秒 */
sleep(3);
/* 暂停3秒后，退出所有的线程 */
pool_set_threads_stop_or_pause(pools, THREAD_STOP);
/* 关闭主线程 */
pthread_exit(NULL);
```

## APIs  ##

```c
/* 初始化线程池 */
struct __pool *pool_init(int num, callback_func, callback_func);
```

```c
/* 往线程池添加任务 */
void pool_add_task(_pool *pool, _task *task, int index_pos);
```

```c
/* 往线程池添加任务 */
void pool_add_detail_task(_pool *pool, void *func, void *arg, int index_pos);
```

```c
/* 设置线程池的线程状态： 支持THREAD_STOP & THREAD_PAUSE */
void pool_set_threads_stop_or_pause(_pool *pool, unsigned char status);
```

```c
/* 启动主线池 */
void pool_set_main_thread_run(_pool *pool);
```
	
	
