/* © Copyright 2014 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | demo run task in background worker                                   |
 * +----------------------------------------------------------------------+
 * | Author: jingmi@gmail.com                                             |
 * +----------------------------------------------------------------------+
 * | Created: 2014-03-08 21:09                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "acoro.h"

int
fetch_bg_thread_id(void *arg, void *result)
{
    (void)arg;
    pthread_t *ptr = result;

    *ptr = pthread_self();

    return 0;
}

void *
bg_run_main_func(void *arg)
{
    int write_fd = *(int*)arg;
    pthread_t tid;

    crt_bg_run(fetch_bg_thread_id, NULL, &tid);

    printf("%zu %llu %llu\n", sizeof tid, (long long unsigned)pthread_self(),
          (long long unsigned)tid);

    write(write_fd, "e", 1);
    crt_exit(NULL);
}

int
main(void)
{
    int pipe_fd[2];
    char c;

    init_coroutine_env(0);

    pipe(pipe_fd);
    crt_create(NULL, NULL, bg_run_main_func, (void*)&pipe_fd[1]);
    read(pipe_fd[0], &c, 1);

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    assert(c == 'e');

    sleep(1);

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
