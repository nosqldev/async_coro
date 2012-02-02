/* © Copyright 2012 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | Implementation of coroutine                                          |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |
 * +----------------------------------------------------------------------+
 * | Created: 2012-02-02 11:10                                            |
 * +----------------------------------------------------------------------+
 */

/* {{{ include header files */

#include <string.h>
#include <ucontext.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#include "bsdqueue.h"
#include "acoro.h"

/* }}} */
/* {{{ hooks */

#ifndef acoro_malloc
#define acoro_malloc malloc
#endif /* ! acoro_malloc */

#ifndef acoro_free
#define acoro_free free
#endif /* ! acoro_free */

/* }}} */
/* {{{ config */

#ifndef MANAGER_CNT
/* ONLY 1 manager thread now */
#define MANAGER_CNT (1)
#endif /* ! MANAGER_CNT */

#ifndef BACKGROUND_WORKER_CNT
#define BACKGROUND_WORKER_CNT (2)
#endif /* ! BACKGROUND_WORKER_CNT */

/* }}} */
/* {{{ enums & structures */

enum action_t
{
    act_new_coroutine,

    act_disk_open,
    act_disk_read,
    act_disk_write,
    act_disk_close,

    act_sock_read,
    act_sock_write,

    act_disk_read_done,

    act_usleep,
};

struct io_arg_s
{
    int fd;
    void *buf;
    size_t count;

    int ret;
    int err_code;
};

list_def(task_queue);
struct list_item_name(task_queue)
{
    enum action_t action;

    struct io_arg_s io_arg;
    ucontext_t task_context;

    list_next_ptr(task_queue);
};

struct coroutine_env_s
{
    pthread_t manager_tid[MANAGER_CNT];
    pthread_t background_worker_tid[BACKGROUND_WORKER_CNT];

    sem_t manager_sem[MANAGER_CNT];
    int pipe_channel[BACKGROUND_WORKER_CNT * 2];

    ucontext_t manager_context;

    list_head_ptr(task_queue) timer_queue;
    list_head_ptr(task_queue) todo_queue;
    list_head_ptr(task_queue) doing_queue;
    list_head_ptr(task_queue) done_queue;
};

/* }}} */

struct coroutine_env_s coroutine_env;

static void *
new_manager(void *arg)
{
    int id;
    list_item_ptr(task_queue) task_ptr;

    id = (intptr_t)arg;

    for (; ;)
    {
        sem_wait(&coroutine_env.manager_sem[id]);
        list_pop(coroutine_env.todo_queue, task_ptr);
    }

    return NULL;
}

static void *
new_background_worker(void *arg)
{
    int id;

    id = (intptr_t)arg;
    sleep(1);

    return NULL;
}

int
init_coroutine_env()
{
    memset(&coroutine_env, 0, sizeof coroutine_env);

    list_new(task_queue, timer_queue);
    coroutine_env.timer_queue = timer_queue;
    list_new(task_queue, todo_queue);
    coroutine_env.todo_queue = todo_queue;
    list_new(task_queue, doing_queue);
    coroutine_env.doing_queue = doing_queue;
    list_new(task_queue, done_queue);
    coroutine_env.done_queue = done_queue;

    for (int i=0; i<MANAGER_CNT; i++)
    {
        sem_init(&coroutine_env.manager_sem[i], 0, 0);
        pthread_create(&coroutine_env.manager_tid[i], NULL, new_manager, (void*)(intptr_t)i);
    }
    for (int i=0; i<BACKGROUND_WORKER_CNT; i++)
    {
        pipe(&coroutine_env.pipe_channel[i*2]);
        pthread_create(&coroutine_env.background_worker_tid[i], NULL, new_background_worker, (void*)(intptr_t)i);
    }
    getcontext(&coroutine_env.manager_context);

    return 0;
}

int
destroy_coroutine_env()
{
    list_destroy(coroutine_env.timer_queue);
    list_destroy(coroutine_env.todo_queue);
    list_destroy(coroutine_env.doing_queue);
    list_destroy(coroutine_env.done_queue);
    for (int i=0; i<MANAGER_CNT; i++)
    {
        pthread_cancel(coroutine_env.manager_tid[i]);
        sem_destroy(&coroutine_env.manager_sem[i]);
    }
    for (int i=0; i<BACKGROUND_WORKER_CNT; i++)
        pthread_cancel(coroutine_env.background_worker_tid[i]);
    for (int i=0; i<BACKGROUND_WORKER_CNT*2; i++)
        close(coroutine_env.pipe_channel[i]);

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
