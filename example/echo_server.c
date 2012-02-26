/* © Copyright 2012 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | echo server                                                          |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |
 * +----------------------------------------------------------------------+
 * | Created: 2012-02-23 23:11                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "acoro.h"

#define PORT (2000)
#define MAX_BUF_LEN (1024 * 1024)

static volatile uint64_t conn_cnt = 0;

void *
echo_func(void *arg)
{
    char *buf;
    ssize_t nread;

    int fd = (uintptr_t)arg;
    buf = malloc(MAX_BUF_LEN);
    crt_set_nonblock(fd);

    uint64_t cnt = __sync_fetch_and_add(&conn_cnt, 1);
    int i;
    for (i=0; i<MAX_BUF_LEN-1; i++)
    {
        nread = crt_tcp_read_to(fd, &buf[i], 1, 1000 * 5);
        if (nread != 1)
            break;
        if (buf[i] == '\n')
            break;
    }
    buf[i+1] = '\0';
    /*printf("nread = %zd\n", nread);*/
    if (nread != 1)
    {
        crt_sock_close(fd);
        printf("err(%zd): %s - [%s]\n", nread, strerror(crt_errno), buf);
        free(buf);
        crt_exit(NULL);
    }
    buf[i + 1] = '\0';
    ssize_t nwrite = crt_tcp_write(fd, &buf[0], strlen(buf));
    buf[ strlen(buf) - 1 ] = '\0';
    printf("[%lu] read: %d, write: %zd, [%s]\n", cnt, i+1, nwrite, buf);

    free(buf);
    close(fd);

    crt_exit(NULL);
}

int
server(void)
{
    struct sockaddr_in server_addr;
    int listenfd;
    int flag;

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0)
    {
        perror("socket()");
        exit(-1);
    }
    flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof server_addr) < 0)
    {
        perror("bind()");
        exit(-1);
    }
    if (listen(listenfd, 128) < 0)
    {
        perror("listen()");
        exit(-1);
    }

    for (int i=0; i<1000 * 1000; i++)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int fd = accept(listenfd, (struct sockaddr *)&client_addr, &len);
        crt_create(NULL, NULL, echo_func, (void*)(uintptr_t)fd);
    }

    return 0;
}

int
main(void)
{
    init_coroutine_env();
    server();
    sleep(1);
    destroy_coroutine_env();

    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
