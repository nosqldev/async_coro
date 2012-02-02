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

struct coroutine_env_s;
typedef void (*ucontext_func_t)(void);
typedef void *(*begin_routine_t)(void*);
typedef uint32_t coroutine_t;
typedef struct
{
    size_t stacksize;
} coroutine_attr_t;

int init_coroutine_env();
int destroy_coroutine_env();
int coroutine_create(coroutine_t *cid, const void * __restrict attr, begin_routine_t br, void * __restrict arg);

#endif /* ! _ACORO_H_ */
/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
