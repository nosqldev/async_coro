/* © Copyright 2014 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | coroutine semaphore                                                  |
 * +----------------------------------------------------------------------+
 * | Author: jingmi@gmail.com                                             |
 * +----------------------------------------------------------------------+
 * | Created: 2014-03-29 22:36                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "acoro.h"

#define NUM (1000000)

void *
slave(void *arg)
{
    crt_sem_t *sem_ptr = arg;

    for (int i=0; i<NUM; i++)
    {
        crt_sem_wait(&sem_ptr[0]);
        crt_sem_post(&sem_ptr[1]);
    }

    crt_exit(NULL);
}

void *
master(void *arg)
{
    int *pipefd = arg;
    crt_sem_t sem[2];

    crt_sem_init(&sem[0], 0, 0);
    crt_sem_init(&sem[1], 0, 0);

    crt_create(NULL, NULL, slave, &sem[0]);

    for (int i=0; i<NUM; i++)
    {
        crt_sem_post(&sem[0]);
        crt_sem_wait(&sem[1]);
    }

    write(*pipefd, "c", 1);

    crt_exit(NULL);
}

void
pingpong()
{
    int pipefd[2];
    char c;

    pipe(pipefd);
    crt_create(NULL, NULL, master, &pipefd[1]);

    read(pipefd[0], &c, 1);
}

int
main(void)
{
    init_coroutine_env(0);
    pingpong();

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
