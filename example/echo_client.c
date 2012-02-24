/* © Copyright 2012 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | echo client                                                          |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |
 * +----------------------------------------------------------------------+
 * | Created: 2012-02-24 17:12                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "acoro.h"

static in_addr_t ip;
static in_port_t port;

void *
connector(void *arg)
{
    int id = (intptr_t)arg;
    char buf[128] = {0};

    int fd = crt_tcp_blocked_connect(ip, port);
    snprintf(buf, sizeof buf, "%d\n", rand());
    size_t len = strlen(buf);
    ssize_t nwrite = crt_tcp_write(fd, buf, len);
    assert(nwrite == (ssize_t)len);
    ssize_t nread = crt_tcp_read(fd, buf, len);
    assert(nread == (ssize_t)len);
    buf[nread] = '\0';
    printf("[%d] %s", id, buf);

    crt_sock_close(fd);

    crt_exit(NULL);
}

int
main(int argc, char **argv)
{
    init_coroutine_env();

    if (argc != 4)
    {
        printf("arg error\n");
        exit(-1);
    }

    ip = inet_addr(argv[1]);
    port = htons(atoi(argv[2]));
    int times = atoi(argv[3]);

    for (int i=0; i<times; i++)
    {
        crt_create(NULL, NULL, connector, (void*)(uintptr_t)i);
    }

    pause();

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
