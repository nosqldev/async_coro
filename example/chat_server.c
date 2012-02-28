/* © Copyright 2012 jingmi. All Rights Reserved.
 *
 * +----------------------------------------------------------------------+
 * | char server                                                          |
 * +----------------------------------------------------------------------+
 * | Author: nosqldev@gmail.com                                           |
 * +----------------------------------------------------------------------+
 * | Created: 2012-02-28 18:46                                            |
 * +----------------------------------------------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "acoro.h"

#define PORT (2000)

static char chat_client_fd[1024 * 1024] = {0};

void *
chat_server_func(void *arg)
{
    int fd = (uintptr_t)arg;
    char buf[1024];
    char c;
    int chat_client_id;
    int cnt;

    chat_client_fd[fd] = 1;
    printf("new connection, fd = %d\n", fd);
    crt_set_nonblock(fd);

go:
    cnt = 0;
    chat_client_id = 0;
    for ( ; ; )
    {
        ssize_t nread = crt_tcp_read(fd, &c, 1);
        if (nread == 1)
        {
            buf[cnt++] = c;
        }
        else if (nread <= 0)
        {
            printf("connect closed, fd = %d\n", fd);
            chat_client_fd[fd] = 0; /* actually, not thread-safe here */
            crt_sock_close(fd);
            crt_exit(NULL);
        }
        else
        {
            abort();
        }
        if ((c == '\n') || (cnt == sizeof buf - 1))
        {
            if (cnt == 1)
                goto go;
            buf[cnt-1] = '\0';
            printf("got `%s' from fd[%d]\n", buf, fd);
            buf[cnt-1] = '\n';
            buf[cnt] = '\0';
            sscanf(buf, "%d", &chat_client_id);
            break;
        }
    }

    printf("chat_client_id = %d\n", chat_client_id);
    if (chat_client_fd[chat_client_id] == 1)
    {
        crt_tcp_write(chat_client_id, buf, strlen(buf));
    }
    else
    {
        crt_tcp_write(fd, "not online\n", 11);
    }
    goto go;

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

    for ( ; ; )
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int fd = accept(listenfd, (struct sockaddr *)&client_addr, &len);
        crt_create(NULL, NULL, chat_server_func, (void*)(uintptr_t)fd);
    }

    return 0;
}

int
main(void)
{
    init_coroutine_env();
    server();

    return 0;
}

/*sheng mei*/
/* vim: set expandtab tabstop=4 shiftwidth=4 foldmethod=marker: */
