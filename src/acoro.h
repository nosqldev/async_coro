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

int init_coroutine_env();
int destroy_coroutine_env();
int crt_create(coroutine_t *cid, const void * restrict attr, begin_routine_t br, void * restrict arg);
/* {{{ void crt_exit(void *) */
#define crt_exit(value_ptr) do {        \
    coroutine_set_finished_coroutine(); \
    return NULL;                        \
} while (0)
/* }}} */

#define crt_disk_open(pathname, flags, ...) ({                  \
    coroutine_set_disk_open(pathname, flags, ##__VA_ARGS__);    \
    coroutine_notify_background_worker();                       \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
})

#endif /* ! _ACORO_H_ */
/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
