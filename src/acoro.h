#ifndef _ACORO_H_
#define _ACORO_H_
/* © Copyright 2012 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | Header of coroutine library                                          |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |
 * +----------------------------------------------------------------------+
 * | Created: 2012-02-02 11:07                                            |
 * +----------------------------------------------------------------------+
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <ucontext.h>
#include <errno.h>

typedef void *(*begin_routine_t)(void*);
typedef uint64_t coroutine_t;
typedef struct
{
    size_t stacksize;
} coroutine_attr_t;

void coroutine_notify_background_worker(void);
void coroutine_get_context(ucontext_t **manager_context, ucontext_t **task_context);

void coroutine_set_finished_coroutine();
void coroutine_set_disk_open(const char *pathname, int flags, ...);
void coroutine_set_disk_read(int fd, void *buf, size_t count);
void coroutine_set_disk_write(int fd, void *buf, size_t count);
void coroutine_set_sock_read(int fd, void *buf, size_t count, int msec);
int  coroutine_get_retval();

int init_coroutine_env();
int destroy_coroutine_env();
int crt_create(coroutine_t *cid, const void * restrict attr, begin_routine_t br, void * restrict arg);

#define crt_errno (crt_get_err_code())
int crt_set_nonblock(int fd);
int crt_set_block(int fd);
int crt_get_err_code();

/* {{{ void     crt_exit(void *) */

#define crt_exit(value_ptr) do {        \
    coroutine_set_finished_coroutine(); \
    return NULL;                        \
} while (0)

/* }}} */
/* {{{ int      crt_disk_open(const char *, int, ...) */

/* Actually, open(2) is a slow call, might be block while disk addressing or io
 * queue is full, etc. */
#define crt_disk_open(pathname, flags, ...) ({                  \
    coroutine_set_disk_open(pathname, flags, ##__VA_ARGS__);    \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ ssize_t  crt_disk_read(int fd, void *buf, size_t count) */

#define crt_disk_read(fd, buf, count) ({                        \
    coroutine_set_disk_read(fd, buf, count);                    \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ ssize_t  crt_disk_write(int fd, const void *buf, size_t count) */

#define crt_disk_write(fd, buf, count) ({                       \
    coroutine_set_disk_write(fd, buf, count);                   \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ int      crt_disk_close(int fd) */

/* This is not a slow call, so we can call close(2) directly */
#define crt_disk_close(fd) ({                                   \
    int retval = close(fd);                                     \
    retval;                                                     \
})

/* }}} */
/* {{{ ssize_t  crt_tcp_read(int fd, void *buf, size_t count) */

#define crt_tcp_read(fd, buf, count) ({                         \
    coroutine_set_sock_read(fd, buf, count, 0);                 \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ ssize_t  crt_tcp_write(int fd, void *buf, size_t count) */

#define crt_tcp_write(fd, buf, count) ({                        \
    coroutine_set_sock_write(fd, buf, count);                   \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ ssize_t  crt_tcp_read_to(int fd, void *buf, size_t count, int msec) */

#define crt_tcp_read_to(fd, buf, count, msec) ({                \
    coroutine_set_sock_read(fd, buf, count, msec);              \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */

#endif /* ! _ACORO_H_ */
/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
