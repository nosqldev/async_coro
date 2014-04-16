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
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "bsdqueue.h"

#ifdef __cplusplus

#ifndef restrict
#define restrict __restrict
#endif /* ! restrict */

extern "C" {
#endif /* ! __cplusplus */

/* {{{ structures */

struct coroutine_attr_s
{
    size_t stacksize;
};

/**
 * @brief To implement crt_sem_t & crt_channel_t
 */
struct crt_channel_s
{
    volatile int value; /* `volatile' is not necessary actually */
    list_item_ptr(task_queue) task_ptr;

    /* the following fields are effective in crt_channel_t */
    size_t buf_capacity;
    size_t buf_offset;
    void *buf;
};

/* }}} */
/* {{{ typedef */

typedef void *(*launch_routine_t)(void *);
typedef int (*bg_routine_t)(void *, void *);
typedef uint64_t coroutine_t;
typedef struct coroutine_attr_s coroutine_attr_t;
typedef struct crt_channel_s crt_sem_t;

/* }}} */

#define CRT_ERR_SOCKET (-1)
#define CRT_ERR_BIND (-2)
#define CRT_ERR_LISTEN (-3)
#define CRT_ERR_SET_NONBLOCK (-4)
#define CRT_ERR_SETSOCKOPT (-5)

#define CRT_SEM_NORMAL_PRIORITY (0) /* the same as calling crt_sem_pos() */
#define CRT_SEM_HIGH_PRIORITY (1) /* not implemented now, this will be effective after priority queue been implemented */
#define CRT_SEM_CRITICAL_PRIORITY (2) /* the coroutine been posted will be ran immediately */

void coroutine_notify_background_worker(void);
void coroutine_get_context(ucontext_t **manager_context, ucontext_t **task_context);

void coroutine_set_finished_coroutine();
void coroutine_set_disk_open(const char *pathname, int flags, ...);
void coroutine_set_disk_read(int fd, void *buf, size_t count);
void coroutine_set_disk_write(int fd, void *buf, size_t count);
void coroutine_set_sock_read(int fd, void *buf, size_t count, int msec);
void coroutine_set_sock_write(int fd, void *buf, size_t count, int msec);
void coroutine_set_sock_connect(in_addr_t ip, in_port_t port, int msec);
int  coroutine_get_retval();

int crt_sem_init(crt_sem_t *sem, int pshared __attribute__((unused)), unsigned int value);
int crt_sem_post(crt_sem_t *sem);
int crt_sem_priority_post(crt_sem_t *sem, int flag);
int crt_sem_wait(crt_sem_t *sem);
int crt_sem_destroy(crt_sem_t *sem);

int init_coroutine_env(size_t worker_count);
int destroy_coroutine_env();
int crt_create(coroutine_t *cid, const void * restrict attr, launch_routine_t br, void * restrict arg);
int crt_attr_setstacksize(coroutine_attr_t *attr, size_t stacksize);
int crt_sched_yield(void);

#define crt_errno (crt_get_err_code())
int crt_set_nonblock(int fd);
int crt_set_block(int fd);
int crt_get_err_code();
int crt_msleep(uint64_t msec);
int crt_bg_run(bg_routine_t bg_routine, void *arg, void *result);
int crt_bg_run2(bg_routine_t bg_routine, void *arg, void *result, uint64_t hashcode);
int crt_bg_order_run(bg_routine_t bg_routine, void *arg, void *result);

/* {{{ tcp IO */

int crt_tcp_prepare_sock(in_addr_t addr, uint16_t port);
int crt_tcp_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/* }}} */
/* {{{ disk IO */

int crt_disk_open(const char *pathname, int flags, ...);

/* }}} */

/* {{{ void     crt_exit(void *) */

#define crt_exit(value_ptr) do {        \
    coroutine_set_finished_coroutine(); \
    return NULL;                        \
} while (0)

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
    coroutine_set_sock_write(fd, buf, count, 0);                \
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
/* {{{ ssize_t  crt_tcp_write_to(int fd, void *buf, size_t count, int msec) */

#define crt_tcp_write_to(fd, buf, count, msec) ({               \
    coroutine_set_sock_write(fd, buf, count, msec);             \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ int      crt_tcp_blocked_connect(in_addr_t ip, in_port_t port) */
/* this will return nonblocking fd */

#define crt_tcp_blocked_connect(ip, port) ({                    \
    coroutine_set_sock_connect(ip, port, -1);                   \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ int      crt_tcp_timeout_connect(in_addr_t ip, in_port_t port, int msec) */
/* this will return nonblocking fd */

#define crt_tcp_timeout_connect(ip, port, msec) ({              \
    coroutine_set_sock_connect(ip, port, msec);                 \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})

/* }}} */
/* {{{ int      crt_sock_close(int fd) */
// TODO rename to crt_tcp_close()

/* This is not a slow call, so we can call close(2) directly */
#define crt_sock_close(fd) ({                                   \
    int retval = close(fd);                                     \
    retval;                                                     \
})

/* }}} */
/* {{{ int      crt_pthread_mutex_lock(pthread_mutex_t *mutex) */

#define crt_pthread_mutex_lock(mutex) ({                        \
    (void)mutex;                                                \
    0;                                                          \
})
/* }}} */
/* {{{ int      crt_pthread_mutex_unlock(pthread_mutex_t *mutex) */

#define crt_pthread_mutex_unlock(mutex) ({                        \
    (void)mutex;                                                \
    0;                                                          \
})
/* }}} */

#ifdef __cplusplus
}
#endif

#endif /* ! _ACORO_H_ */
/* vim: set expandtab tabstop=4 shiftwidth=4 ft=c foldmethod=marker: */
