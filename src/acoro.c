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
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sched.h>

#include "acoro.h"
#include "bsdqueue.h"
#include "ev.h"

/* }}} */
/* {{{ hooks & predefined subs */

#ifndef acoro_malloc
#define acoro_malloc malloc
#endif /* ! acoro_malloc */

#ifndef acoro_free
#define acoro_free free
#endif /* ! acoro_free */

#define Lock(name) list_lock(coroutine_env.name)
#define UnLock(name) list_unlock(coroutine_env.name)

#define Shift(name, ptr) do { Lock(name); list_shift(coroutine_env.name, ptr); UnLock(name); } while (0)
#define Push(name, ptr) do { Lock(name); list_push(coroutine_env.name, ptr); UnLock(name); } while (0)

/* }}} */
/* {{{ config */

/* TODO define MAX_* instead */

#ifndef MANAGER_CNT
/* ONLY 1 manager thread now */
#define MANAGER_CNT (1)
#endif /* ! MANAGER_CNT */

#ifndef BACKGROUND_WORKER_CNT
#define BACKGROUND_WORKER_CNT (2)
#endif /* ! BACKGROUND_WORKER_CNT */

#ifndef COROUTINE_STACK_SIZE
#define COROUTINE_STACK_SIZE (1024 * 4)
#endif /* ! COROUTINE_STACK_SIZE */

/* }}} */
/* {{{ enums & structures */

enum action_t
{
    act_new_coroutine,
    act_finished_coroutine,

    act_disk_open,
    act_disk_read,
    act_disk_write,

    act_sock_read,
    act_sock_write,

    act_disk_open_done,
    act_disk_read_done,
    act_disk_write_done,

    act_usleep,
};

struct init_arg_s
{
    begin_routine_t func;
    size_t stack_size;
    void *func_arg;
};

struct open_arg_s
{
    const char *pathname;
    int flags;
    mode_t mode;
};

struct io_arg_s
{
    int fd;
    void *buf;
    size_t count;
};

union args_u
{
    struct init_arg_s init_arg;
    struct open_arg_s open_arg;
    struct io_arg_s io_arg;
};

list_def(task_queue);
struct list_item_name(task_queue)
{
    enum action_t action;
    union args_u args;
    struct
    {
        ssize_t val;
        int err_code;
    } ret;

    ucontext_t task_context;
    void *stack_ptr;

    list_next_ptr(task_queue);
};

struct coroutine_env_s
{
    pthread_t manager_tid[MANAGER_CNT];
    pthread_t background_worker_tid[BACKGROUND_WORKER_CNT];

    sem_t manager_sem[MANAGER_CNT];
    int pipe_channel[BACKGROUND_WORKER_CNT * 2];

    struct
    {
        volatile uint64_t cid; /* coroutine id */
        volatile uint64_t ran; /* how many coroutines had been ran */
        volatile uint64_t bg_worker_id;
    } info;

    /* TODO manager_context should be an array */
    ucontext_t manager_context;
    list_item_ptr(task_queue) curr_task_ptr[MANAGER_CNT];
    struct
    {
        struct ev_loop *loop;
        ev_io watcher;
    } worker_ev[BACKGROUND_WORKER_CNT];

    list_head_ptr(task_queue) timer_queue;
    list_head_ptr(task_queue) todo_queue;
    list_head_ptr(task_queue) doing_queue;
    list_head_ptr(task_queue) done_queue;
};

/* }}} */

__thread volatile uint64_t g_thread_id;
struct coroutine_env_s coroutine_env;

/* {{{ manager thread */

static void *
new_manager(void *arg)
{
    list_item_ptr(task_queue) task_ptr;
    ucontext_t task_context;

    g_thread_id = (intptr_t)arg;

    for ( ; ; )
    {
        sem_wait(&coroutine_env.manager_sem[g_thread_id]);
        Shift(todo_queue, task_ptr);
        if (task_ptr == NULL)
            continue; /* should never reach here */

loop:
        switch (task_ptr->action)
        {
        case act_new_coroutine:
            getcontext(&coroutine_env.manager_context);
            getcontext(&task_context);
            task_ptr->stack_ptr = acoro_malloc(task_ptr->args.init_arg.stack_size);
            task_context.uc_stack.ss_sp = task_ptr->stack_ptr;
            task_context.uc_stack.ss_size = COROUTINE_STACK_SIZE;
            task_context.uc_link = &coroutine_env.manager_context;
            makecontext(&task_context,
                        (void(*)(void))task_ptr->args.init_arg.func,
                        1,
                        task_ptr->args.init_arg.func_arg);
            coroutine_env.curr_task_ptr[ g_thread_id ] = task_ptr;
            swapcontext(&coroutine_env.manager_context, &task_context);
            if (task_ptr->action == act_finished_coroutine)
                goto loop;
            break;

        case act_disk_open_done:
            swapcontext(&coroutine_env.manager_context, &task_ptr->task_context);
            if (task_ptr->action == act_finished_coroutine)
                goto loop;
            break;

        case act_disk_read_done:
            swapcontext(&coroutine_env.manager_context, &task_ptr->task_context);
            if (task_ptr->action == act_finished_coroutine)
                goto loop;
            break;

        case act_disk_write_done:
            swapcontext(&coroutine_env.manager_context, &task_ptr->task_context);
            if (task_ptr->action == act_finished_coroutine)
                goto loop;
            break;

        case act_finished_coroutine:
            acoro_free(coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr);
            acoro_free(coroutine_env.curr_task_ptr[ g_thread_id ]);
            __sync_add_and_fetch(&coroutine_env.info.ran, 1);
            break;

        default:
            /* should never reach here */
            abort();
            break;
        }
    }

    return NULL;
}

/* }}} */
/* {{{ background worker thread */

#define CurrLoop (coroutine_env.worker_ev[ g_thread_id ].loop)
#define CurrWatcher (coroutine_env.worker_ev[ g_thread_id ].watcher)
#define CurrReadPipe (coroutine_env.pipe_channel[g_thread_id * 2])

static int
do_disk_open(list_item_ptr(task_queue) task_ptr)
{
    struct open_arg_s *arg;

    arg = &task_ptr->args.open_arg;
    if (arg->flags | O_CREAT)
        task_ptr->ret.val = open(arg->pathname, arg->flags, arg->mode);
    else
        task_ptr->ret.val = open(arg->pathname, arg->flags);

    if (task_ptr->ret.val >= 0)
        task_ptr->ret.err_code = 0;
    else
        task_ptr->ret.err_code = errno;
    task_ptr->action = act_disk_open_done;

    return 0;
}

static int
do_disk_read(list_item_ptr(task_queue) task_ptr)
{
    struct io_arg_s *arg;

    arg = &task_ptr->args.io_arg;
    task_ptr->ret.val = read(arg->fd, arg->buf, arg->count);
    if (task_ptr->ret.val >= 0)
        task_ptr->ret.err_code = 0;
    else
        task_ptr->ret.err_code = errno;
    task_ptr->action = act_disk_read_done;

    return 0;
}

static int
do_disk_write(list_item_ptr(task_queue) task_ptr)
{
    struct io_arg_s *arg;

    arg = &task_ptr->args.io_arg;
    task_ptr->ret.val = write(arg->fd, arg->buf, arg->count);
    if (task_ptr->ret.val >= 0)
        task_ptr->ret.err_code = 0;
    else
        task_ptr->ret.err_code = errno;
    task_ptr->action = act_disk_write_done;

    return 0;
}

static void
do_task(list_item_ptr(task_queue) task_ptr)
{
    switch (task_ptr->action)
    {
    case act_disk_open:
        do_disk_open(task_ptr);
        break;

    case act_disk_read:
        do_disk_read(task_ptr);
        break;

    case act_disk_write:
        do_disk_write(task_ptr);
        break;

    default:
        abort();
        break;
    }
}

static void
new_event_handler(struct ev_loop *loop, ev_io *w, int event)
{
    ssize_t nread;
    unsigned char c;
    list_item_ptr(task_queue) task_ptr;
    uint64_t cid;

    (void)loop;
    (void)w;

    if (event == EV_READ)
    {
        nread = read(CurrReadPipe, &c, sizeof c);
        assert(nread == sizeof c);
        assert(c == 0xee);
        Shift(doing_queue, task_ptr);
        assert(task_ptr != NULL);

        do_task(task_ptr);
        Push(todo_queue, task_ptr);
        cid = __sync_fetch_and_add(&coroutine_env.info.cid, 0);
        sem_post(&coroutine_env.manager_sem[ cid % MANAGER_CNT ]);
    }
    else
    {
        abort();
    }
}

static void *
new_background_worker(void *arg)
{
    g_thread_id = (intptr_t)arg;

    CurrLoop = ev_loop_new(EVBACKEND_EPOLL);
    assert(CurrLoop != NULL);

    ev_io_init(&CurrWatcher, new_event_handler, CurrReadPipe, EV_READ);
    ev_io_start(CurrLoop, &coroutine_env.worker_ev[ g_thread_id ].watcher);

    ev_run(CurrLoop, 0);

    return NULL;
}

/* }}} */
/* {{{ public functions but internal used only */

#define GenSetActionFunc(action_name)       \
    void coroutine_set_##action_name() {    \
        coroutine_env.curr_task_ptr[ g_thread_id ]->action = act_##action_name; \
    }
GenSetActionFunc(finished_coroutine);

int
coroutine_get_retval()
{
    return coroutine_env.curr_task_ptr[ g_thread_id ]->ret.val;
}

void
coroutine_set_disk_open(const char *pathname, int flags, ...)
{
    va_list ap;
    list_item_ptr(task_queue) task_ptr;

    task_ptr = coroutine_env.curr_task_ptr[ g_thread_id ];
    task_ptr->action = act_disk_open;
    task_ptr->args.open_arg.pathname = pathname;
    task_ptr->args.open_arg.flags = flags;
    if (flags | O_CREAT)
    {
        va_start(ap, flags);
        task_ptr->args.open_arg.mode = va_arg(ap, mode_t);
        va_end(ap);
    }
    else
    {
        task_ptr->args.open_arg.mode = 0;
    }
}

void
coroutine_set_disk_read(int fd, void *buf, size_t count)
{
    list_item_ptr(task_queue) task_ptr;

    task_ptr = coroutine_env.curr_task_ptr[ g_thread_id ];
    task_ptr->action = act_disk_read;
    task_ptr->args.io_arg.fd    = fd;
    task_ptr->args.io_arg.buf   = buf;
    task_ptr->args.io_arg.count = count;
}

void
coroutine_set_disk_write(int fd, void *buf, size_t count)
{
    list_item_ptr(task_queue) task_ptr;

    task_ptr = coroutine_env.curr_task_ptr[ g_thread_id ];
    task_ptr->action = act_disk_write;
    task_ptr->args.io_arg.fd    = fd;
    task_ptr->args.io_arg.buf   = buf;
    task_ptr->args.io_arg.count = count;
}

void
coroutine_notify_background_worker(void)
{
    unsigned char c;
    ssize_t nwrite;
    uint64_t bg_worker_id;

    c = 0xee;
    Push(doing_queue, coroutine_env.curr_task_ptr[ g_thread_id ]);
    bg_worker_id = __sync_fetch_and_add(&coroutine_env.info.bg_worker_id, 1) % BACKGROUND_WORKER_CNT;
    nwrite = write(coroutine_env.pipe_channel[bg_worker_id * 2 + 1], &c, sizeof c);
    assert(nwrite == sizeof c);
}

void
coroutine_get_context(ucontext_t **manager_context, ucontext_t **task_context)
{
    *manager_context = &coroutine_env.manager_context;
    *task_context = &(coroutine_env.curr_task_ptr[ g_thread_id ]->task_context);
}

/* }}} */
/* {{{ public interfaces */

int
init_coroutine_env()
{
    int ret;
    int flag;

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
        sched_yield();
    }
    for (int i=0; i<BACKGROUND_WORKER_CNT; i++)
    {
        pipe(&coroutine_env.pipe_channel[i*2]);

        flag = fcntl(coroutine_env.pipe_channel[i*2], F_GETFL);
        assert(flag >= 0);
        ret = fcntl(coroutine_env.pipe_channel[i*2], F_SETFL, flag | O_NONBLOCK);
        assert(ret == 0);
        flag = fcntl(coroutine_env.pipe_channel[i*2+1], F_GETFL);
        assert(flag >= 0);
        ret = fcntl(coroutine_env.pipe_channel[i*2+1], F_SETFL, flag | O_NONBLOCK);
        assert(ret == 0);

        pthread_create(&coroutine_env.background_worker_tid[i], NULL, new_background_worker, (void*)(intptr_t)i);
        sched_yield();
    }

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

int
crt_get_err_code()
{
    return coroutine_env.curr_task_ptr[ g_thread_id ]->ret.err_code;
}

int
crt_create(coroutine_t *cid, const void * __restrict attr __attribute__((unused)),
                 begin_routine_t br, void * __restrict arg)
{
    /*TODO user can define stack size */
    list_item_new(task_queue, task_ptr);

    if (cid == NULL)
        (void)__sync_add_and_fetch(&coroutine_env.info.cid, 1);
    else
        *cid = __sync_add_and_fetch(&coroutine_env.info.cid, 1);

    task_ptr->action = act_new_coroutine;
    task_ptr->args.init_arg.func = br;
    task_ptr->args.init_arg.stack_size = COROUTINE_STACK_SIZE;
    task_ptr->args.init_arg.func_arg = arg;

    Push(todo_queue, task_ptr);
    sem_post(&coroutine_env.manager_sem[0]); /* TODO post to one manager by round robin */

    return 0;
}

int
crt_set_nonblock(int fd)
{
    int flag;
    int ret = 0;

    if ((flag = fcntl(fd, F_GETFL)) < 0)
    {
        return errno;
    }
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
    {
        return errno;
    }

    return ret;
}

int
crt_set_block(int fd)
{
    int flag;
    int ret = 0;

    if ((flag = fcntl(fd, F_GETFL)) < 0)
    {
        return errno;
    }
    flag = (flag & (~O_NONBLOCK));
    if (fcntl(fd, F_SETFL, flag) < 0)
    {
        return errno;
    }

    return ret;
}


/* }}} */

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
