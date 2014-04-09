/* © Copyright 2014 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | test accept in echo model server                                     |
 * +----------------------------------------------------------------------+
 * | Author: jingmi@gmail.com                                             |
 * +----------------------------------------------------------------------+
 * | Created: 2014-03-27 11:40                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "acoro.h"

#define PORT (2000)

void *
connection_processor(void *arg)
{
    int fd = (int)(intptr_t)arg;
    char buffer[1024] = {0};
    ssize_t nread, nwrite;

    for (size_t i=0; i<sizeof buffer; i++)
    {
        nread = crt_tcp_read(fd, &buffer[i], 1);
        if (nread != 1)
        {
            printf("errno = %d, strerror(errno) = %s\n", errno, strerror(errno));
        }

        if (buffer[i] == '\n')
        {
            buffer[i+1] = '\0';
            break;
        }
    }
    printf("[%d] (nread = %zd) %s\n", fd, nread, buffer);

    nwrite = crt_tcp_write(fd, &buffer[0], strlen(buffer));
    printf("[%d] (nwrite = %zd) %s\n", fd, nwrite, buffer);

    crt_sock_close(fd);

    crt_exit(NULL);
}

void *
server(void *arg)
{
    int sockfd = crt_tcp_prepare_sock(inet_addr("127.0.0.1"), htons(PORT));

    (void)arg;
    assert(sockfd >= 0);

    for (; ;)
    {
        int fd = crt_tcp_accept(sockfd, NULL, NULL);
        crt_create(NULL, NULL, connection_processor, (void*)(intptr_t)fd);
    }

    crt_exit(NULL);
}

int
main(void)
{
    init_coroutine_env(0);


    crt_create(NULL, NULL, server, NULL);
    pause();

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
