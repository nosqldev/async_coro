/* © Copyright 2012 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | test suites for acore.c                                              |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |
 * +----------------------------------------------------------------------+
 * | Created: 2012-02-02 11:50                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CUnit/Basic.h>

#include "acoro.h"

/* {{{ copied from acoro.c for internal tests */

#include <string.h>
#include <ucontext.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include "bsdqueue.h"

#ifndef MANAGER_CNT
#define MANAGER_CNT (1)
#endif /* ! MANAGER_CNT */

#ifndef BACKGROUND_WORKER_CNT
#define BACKGROUND_WORKER_CNT (2)
#endif /* ! BACKGROUND_WORKER_CNT */

struct coroutine_env_s
{
    pthread_t manager_tid[MANAGER_CNT];
    pthread_t background_worker_tid[BACKGROUND_WORKER_CNT];

    sem_t manager_sem[MANAGER_CNT];
    int pipe_channel[BACKGROUND_WORKER_CNT * 2];

    struct
    {
        volatile uint64_t cid;
        volatile uint64_t ran; /* how many coroutine had been ran */
    } info;
    ucontext_t manager_context;

    list_head_ptr(task_queue) timer_queue;
    list_head_ptr(task_queue) todo_queue;
    list_head_ptr(task_queue) doing_queue;
    list_head_ptr(task_queue) done_queue;
};

struct coroutine_env_s coroutine_env;

/* }}} */

int lowest_fd;

void
test_init_coroutine_env(void)
{
    int ret;
    lowest_fd = dup(0);
    close(lowest_fd);

    ret = init_coroutine_env();

    CU_ASSERT(ret == 0);
    CU_ASSERT(coroutine_env.timer_queue != NULL);
    CU_ASSERT(coroutine_env.todo_queue  != NULL);
    CU_ASSERT(coroutine_env.doing_queue != NULL);
    CU_ASSERT(coroutine_env.done_queue  != NULL);

    for (int i=0; i<BACKGROUND_WORKER_CNT*2; i++)
    {
        CU_ASSERT(coroutine_env.pipe_channel[i] != 0);
    }
}

int
check_init(void)
{
    /* {{{ init CU suite check_init */
    CU_pSuite pSuite = NULL;
    pSuite = CU_add_suite("check_init", NULL, NULL);
    if (pSuite == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */

    /* {{{ CU_add_test: test_init_coroutine_env */
    if (CU_add_test(pSuite, "test_init_coroutine_env", test_init_coroutine_env) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */

    return 0;
}

void
test_disk_read(void)
{

}

static void *
null_coroutine(void *arg)
{
    CU_ASSERT((intptr_t)arg == 0xbeef);

    return NULL;
}

void
test_null_coroutine(void)
{
    coroutine_t cid;
    int ret;

    CU_ASSERT(coroutine_env.info.cid == 0);
    CU_ASSERT(coroutine_env.info.ran == 0);

    ret = coroutine_create(&cid, NULL, null_coroutine, (void*)(intptr_t)0xbeef);

    CU_ASSERT(coroutine_env.info.cid == 1);
    CU_ASSERT(cid == 1);

    CU_ASSERT(ret == 0);

    usleep(1000*10);

    CU_ASSERT(coroutine_env.info.ran == 1);
}

int
check_coroutine(void)
{
    /* {{{ init CU suite check_coroutine */
    CU_pSuite pSuite = NULL;
    pSuite = CU_add_suite("check_coroutine", NULL, NULL);
    if (pSuite == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */

    /* {{{ CU_add_test: test_null_coroutine */
    if (CU_add_test(pSuite, "test_null_coroutine", test_null_coroutine) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */

    /* {{{ CU_add_test: test_disk_read */
    if (CU_add_test(pSuite, "test_disk_read", test_disk_read) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */

    return 0;
}

void
test_destroy_coroutine_env(void)
{
    int ret;
    ret = destroy_coroutine_env();
    CU_ASSERT(ret == 0);

    int curr_lowest_fd = dup(0);
    CU_ASSERT(lowest_fd == curr_lowest_fd);
    close(curr_lowest_fd);
}

int
check_destroy_coroutine_env(void)
{
    /* {{{ init CU suite check_destroy_coroutine_env */
    CU_pSuite pSuite = NULL;
    pSuite = CU_add_suite("check_destroy_coroutine_env", NULL, NULL);
    if (pSuite == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */

    /* {{{ CU_add_test: test_destroy_coroutine_env */
    if (CU_add_test(pSuite, "test_destroy_coroutine_env", test_destroy_coroutine_env) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */

    return 0;
}

int
main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    check_init();

    check_coroutine();

    check_destroy_coroutine_env();

    /* {{{ CU run & cleanup */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    /* }}} */

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
