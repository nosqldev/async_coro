/* © Copyright 2014 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | to test create a new corotine in current corotine                    |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |
 * +----------------------------------------------------------------------+
 * | Created: 2014-03-02 01:18                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "acoro.h"

void *
secondary_corotine(void *arg)
{
    printf("now in secondary corotine: %ld\n", (long)arg);

    return NULL;
}

void *
main_corotine(void *arg)
{
    (void)arg;

    puts("now in main corotine");

    for (long i=0; i<20; i++)
        crt_create(NULL, NULL, secondary_corotine, (void*)i);

    return NULL;
}

int
main(void)
{
    init_coroutine_env(); 

    crt_create(NULL, NULL, main_corotine, NULL);

    sleep(2);

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
