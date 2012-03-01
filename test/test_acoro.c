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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

/* include acoro.c directly instead of link, so that can share structures,
 * global variables, functions and so on.
 */
#include "acoro.c"

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

static void *
disk_read(void *arg)
{
    (void)arg;
    int fd = 0;
    char buf[32];

    Lock(doing_queue);
    CU_ASSERT(list_size(coroutine_env.doing_queue) == 0);
    UnLock(doing_queue);

    fd = crt_disk_open("./test_acoro.c", O_RDONLY);
    CU_ASSERT(fd > 0);
    ssize_t nread = crt_disk_read(fd, buf, sizeof buf);
    CU_ASSERT(crt_get_err_code() == 0);
    CU_ASSERT(nread == sizeof buf);
    CU_ASSERT(memcmp(buf, "/* © Copyright 2012 jingmi. All", sizeof buf) == 0);

    int ret = crt_disk_close(fd);
    CU_ASSERT(ret == 0);

    CU_ASSERT((uintptr_t)&buf[0] > (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr);
    CU_ASSERT((uintptr_t)&buf[0] < (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr + (uintptr_t)COROUTINE_STACK_SIZE);
    CU_ASSERT((uintptr_t)&buf[31] > (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr);
    CU_ASSERT((uintptr_t)&buf[31] < (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr + (uintptr_t)COROUTINE_STACK_SIZE);

    crt_exit(NULL);
}

void
test_disk_read(void)
{
    int ret;
    int lowestfd = dup(0);
    close(lowestfd);
    CU_ASSERT(coroutine_env.info.ran == 2);
    ret = crt_create(NULL, NULL, disk_read, NULL);
    CU_ASSERT(ret == 0);

    usleep(1000*20);
    CU_ASSERT(coroutine_env.info.ran == 3);
    int curr_lowestfd = dup(0);
    close(curr_lowestfd);
    CU_ASSERT(lowestfd == curr_lowestfd);
}

static void *
null_coroutine(void *arg)
{
    /* XXX Why crash when using printf() with arguments? */
    /*printf("hello, %s\n", "world");*/
    CU_ASSERT((intptr_t)arg == 0xbeef);

    crt_exit(NULL);
}

void
test_null_coroutine(void)
{
    coroutine_t cid;
    int ret;

    CU_ASSERT(coroutine_env.info.cid == 0);
    CU_ASSERT(coroutine_env.info.ran == 0);

    ret = crt_create(&cid, NULL, null_coroutine, (void*)(intptr_t)0xbeef);

    CU_ASSERT(coroutine_env.info.cid == 1);
    CU_ASSERT(cid == 1);

    CU_ASSERT(ret == 0);

    usleep(1000*10);
    CU_ASSERT(coroutine_env.info.ran == 1);
}

static int
mul(int a, int b)
{
    CU_ASSERT((uintptr_t)&a > (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr);
    CU_ASSERT((uintptr_t)&a < (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr + (uintptr_t)COROUTINE_STACK_SIZE);

    CU_ASSERT((uintptr_t)&b > (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr);
    CU_ASSERT((uintptr_t)&b < (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr + (uintptr_t)COROUTINE_STACK_SIZE);

    return a * b;
}

static void *
call_in_coroutine(void *arg)
{
    (void)arg;

    int c = mul(1, 2);
    CU_ASSERT(c == 2);
    CU_ASSERT((uintptr_t)&c > (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr);
    CU_ASSERT((uintptr_t)&c < (uintptr_t)coroutine_env.curr_task_ptr[ g_thread_id ]->stack_ptr + (uintptr_t)COROUTINE_STACK_SIZE);

    c = mul(100, 200);
    CU_ASSERT(c == 20000);

    crt_exit(NULL);
}

void
test_call_in_coroutine(void)
{
    CU_ASSERT(crt_create(NULL, NULL, call_in_coroutine, NULL) == 0);
    usleep(1000*10);
    CU_ASSERT(coroutine_env.info.ran == 2);
}

static void *
disk_write(void *arg)
{
    (void)arg;

    int fd;

    fd = crt_disk_open("/tmp/test_acoro.dat", O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, S_IREAD | S_IWRITE);
    CU_ASSERT(fd > 0);

    ssize_t nwrite = crt_disk_write(fd, "abc", 3);
    CU_ASSERT(nwrite == 3);
    CU_ASSERT(crt_get_err_code() == 0);

    CU_ASSERT(crt_disk_close(fd) == 0);

    fd = open("/tmp/test_acoro.dat", O_RDONLY);
    char buf[16];
    ssize_t nread = read(fd, buf, sizeof buf);
    CU_ASSERT(nread == 3);
    CU_ASSERT(memcmp(buf, "abc", 3) == 0);
    close(fd);
    struct stat sb;
    stat("/tmp/test_acoro.dat", &sb);
    CU_ASSERT(sb.st_size == 3);

    crt_exit(NULL);
}

void
test_disk_write(void)
{
    int lowestfd = dup(0);
    close(lowestfd);

    CU_ASSERT(crt_create(NULL, NULL, disk_write, NULL) == 0);
    usleep(1000 * 10);
    CU_ASSERT(coroutine_env.info.ran == 4);

    int curr_lowestfd = dup(0);
    close(curr_lowestfd);
    CU_ASSERT(lowestfd == curr_lowestfd);

    assert(unlink("/tmp/test_acoro.dat") == 0);
}

void *
disk_io_failed(void *arg)
{
    (void)arg;

    int fd = crt_disk_open("/UnableOpened.dat", O_CREAT, S_IREAD | S_IWRITE);
    CU_ASSERT(fd == -1);
    int err_code = crt_get_err_code();
    CU_ASSERT(err_code == EACCES);

    fd = crt_disk_open("/tmp/test_acoro.dat", O_RDONLY | O_CREAT | O_APPEND | O_TRUNC, S_IREAD);
    CU_ASSERT(fd > 0);
    char buf[16] = "abcdefg";
    ssize_t nwrite = crt_disk_write(fd, buf, sizeof buf);
    CU_ASSERT(errno == 0); /* means we can NOT use system errno in coroutine */
    CU_ASSERT(nwrite < 0);
    err_code = crt_get_err_code();
    CU_ASSERT(err_code == EBADF);

    CU_ASSERT(crt_disk_close(fd) == 0);

    crt_exit(NULL);
}

void
test_disk_io_failed(void)
{
    int lowestfd = dup(0);
    close(lowestfd);

    coroutine_t cid;
    CU_ASSERT(crt_create(&cid, NULL, disk_io_failed, NULL) == 0);
    usleep(1000 * 10);
    CU_ASSERT(coroutine_env.info.ran == 5);
    CU_ASSERT(cid == 5);
    assert(unlink("/tmp/test_acoro.dat") == 0);

    int curr_lowestfd = dup(0);
    close(curr_lowestfd);
    CU_ASSERT(lowestfd == curr_lowestfd);
}

void
test_utils(void)
{
    list_item_ptr(task_queue) task_ptr = NULL;
    list_item_ptr(task_queue) computed_addr = IO_WATCHER_REF_TASKPTR(&task_ptr->ec.sock_watcher);
    list_item_ptr(task_queue) computed_addr2 = TIMER_WATCHER_REF_TASKPTR(&task_ptr->ec.sock_timer);

    CU_ASSERT(computed_addr == task_ptr);
    CU_ASSERT(computed_addr2 == task_ptr);
}

void *
dummy_slow_server(void *arg)
{
    int listenfd;
    struct sockaddr_in server_addr;
    int flag = 1;
    (void)arg;

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(listenfd > 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag);
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    assert(bind(listenfd, (struct sockaddr *) &server_addr, sizeof server_addr) == 0);
    assert(listen(listenfd, 5) == 0);

    int sockfd = accept(listenfd, NULL, NULL);
    assert(sockfd > 0);

    usleep(1000 * 10);

    close(sockfd);
    close(listenfd);

    return NULL;
}

void *
dummy_write_server(void *arg)
{
    int listenfd;
    struct sockaddr_in server_addr;
    int flag = 1;
    (void)arg;

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(listenfd > 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag);
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    assert(bind(listenfd, (struct sockaddr *) &server_addr, sizeof server_addr) == 0);
    assert(listen(listenfd, 5) == 0);

    int sockfd = accept(listenfd, NULL, NULL);
    assert(sockfd > 0);
    CU_ASSERT(write(sockfd, "abc", 3) == 3);
    CU_ASSERT(write(sockfd, "abc", 3) == 3);
    close(sockfd);
    close(listenfd);

    return NULL;
}

static void *
sock_read(void *arg)
{
    (void)arg;
    struct sockaddr_in server_addr;
    char buf[32];
    ssize_t nread;

    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(sockfd > 0);
    CU_ASSERT(connect(sockfd, (struct sockaddr *)&server_addr, sizeof server_addr) == 0);
    crt_set_nonblock(sockfd);
    CU_ASSERT((nread=crt_tcp_read(sockfd, buf, sizeof buf)) == 6);
    if (nread != 6)
    {
        printf("\nnread = %zd, errcode = %d, err = %s\n", nread, crt_get_err_code(), strerror(crt_get_err_code()));
    }
    CU_ASSERT(crt_get_err_code() == 0);
    CU_ASSERT(memcmp(buf, "abcabc", 6) == 0);

    int ret = crt_tcp_read(sockfd, buf, sizeof buf);
    CU_ASSERT(ret == 0);
    CU_ASSERT(crt_get_err_code() == 0);

    close(sockfd);

    crt_exit(NULL);
}

void
test_sock_read(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, dummy_write_server, NULL);
    sched_yield();
    usleep(1000);

    crt_create(NULL, NULL, sock_read, NULL);
    usleep(1000);
    CU_ASSERT(coroutine_env.info.ran == 6);

    pthread_join(tid, NULL);
}

void *
sock_timeout(void *arg)
{
    (void)arg;
    struct sockaddr_in server_addr;
    char buf[32];
    struct timeval start, end, used;

    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(sockfd > 0);
    CU_ASSERT(connect(sockfd, (struct sockaddr *)&server_addr, sizeof server_addr) == 0);
    crt_set_nonblock(sockfd);

    gettimeofday(&start, NULL);
    CU_ASSERT(crt_tcp_read_to(sockfd, buf, sizeof buf, 1) == -1);
    CU_ASSERT(crt_get_err_code() == EWOULDBLOCK);
    gettimeofday(&end, NULL);
    (&used)->tv_sec = (&end)->tv_sec - (&start)->tv_sec;
    (&used)->tv_usec = (&end)->tv_usec - (&start)->tv_usec;
    if ((&used)->tv_usec < 0){
        (&used)->tv_sec --;
        (&used)->tv_usec += 1000000;
    }
    CU_ASSERT(used.tv_sec == 0);
    CU_ASSERT(used.tv_usec <= 1000 * 20); /* more time to make sure: 20 ms */

    close(sockfd);

    crt_exit(NULL);
}

void
test_sock_timeout(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, dummy_slow_server, NULL);
    sched_yield();
    usleep(1000);

    crt_create(NULL, NULL, sock_timeout, NULL);
    usleep(20 * 1000);
    CU_ASSERT(coroutine_env.info.ran == 7);

    pthread_join(tid, NULL);
}

void *
dummy_read_server(void *arg)
{
    int listenfd;
    struct sockaddr_in server_addr;
    int flag = 1;
    (void)arg;
    char buf1[32] = {0};
    char buf2[32] = {0};

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(listenfd > 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag);
    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    assert(bind(listenfd, (struct sockaddr *) &server_addr, sizeof server_addr) == 0);
    assert(listen(listenfd, 5) == 0);

    int sockfd = accept(listenfd, NULL, NULL);
    assert(sockfd > 0);
    CU_ASSERT(read(sockfd, buf1, 3) == 3);
    CU_ASSERT(read(sockfd, buf2, 3) == 3);
    CU_ASSERT(memcmp(buf1, "abc", 3) == 0);
    CU_ASSERT(memcmp(buf2, "xyz", 3) == 0);
    close(sockfd);
    close(listenfd);

    return NULL;
}

static void *
sock_write(void *arg)
{
    (void)arg;
    struct sockaddr_in server_addr;
    ssize_t nwrite;

    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(sockfd > 0);
    CU_ASSERT(connect(sockfd, (struct sockaddr *)&server_addr, sizeof server_addr) == 0);
    crt_set_nonblock(sockfd);
    CU_ASSERT((nwrite=crt_tcp_write(sockfd, "abc", 3)) == 3);
    if (nwrite != 3) printf("written: %zd\n", nwrite);
    CU_ASSERT((nwrite=crt_tcp_write(sockfd, "xyz", 3)) == 3);
    if (nwrite != 3) printf("written: %zd\n", nwrite);
    CU_ASSERT(crt_get_err_code() == 0);
    int ret = crt_tcp_write(sockfd, "123", 3);
    CU_ASSERT(ret == 3);
    CU_ASSERT(crt_get_err_code() == 0);

    close(sockfd);

    crt_exit(NULL);
}

void
test_sock_write(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, dummy_read_server, NULL);
    sched_yield();
    usleep(1000);

    crt_create(NULL, NULL, sock_write, NULL);
    usleep(1000);
    CU_ASSERT(coroutine_env.info.ran == 8);

    pthread_join(tid, NULL);
}

static void *
tcp_blocked_connect(void *arg)
{
    (void)arg;
    int fd;

    fd = crt_tcp_blocked_connect(inet_addr("127.0.0.1"), htons(2000));
    CU_ASSERT(fd > 0);
    crt_sock_close(fd);

    crt_exit(NULL);
}

void
test_tcp_blocked_connect(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, dummy_slow_server, NULL);
    sched_yield();
    usleep(1000);

    crt_create(NULL, NULL, tcp_blocked_connect, NULL);

    usleep(20 * 1000);
    CU_ASSERT(coroutine_env.info.ran == 9);

    pthread_join(tid, NULL);
}

static void *
tcp_timeout_connect(void *arg)
{
    (void)arg;
    int fd;

    fd = crt_tcp_timeout_connect(inet_addr("127.0.0.1"), htons(2000), 100);
    CU_ASSERT(fd > 0);

    char buf[1024];
    CU_ASSERT(crt_tcp_read(fd, buf, 6) == 6);
    CU_ASSERT(strcmp(buf, "abcabc") == 0);

    crt_sock_close(fd);

    crt_exit(NULL);
}

void
test_tcp_timeout_connect(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, dummy_write_server, NULL);
    sched_yield();
    usleep(1000);

    crt_create(NULL, NULL, tcp_timeout_connect, NULL);

    usleep(20 * 1000);
    CU_ASSERT(coroutine_env.info.ran == 10);
    pthread_join(tid, NULL);
}

void
test_set_stacksize(void)
{
    coroutine_attr_t attr;

    crt_attr_setstacksize(&attr, 1024);
    CU_ASSERT(attr.stacksize == 1024);
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
    /* {{{ CU_add_test: test_call_in_coroutine */
    if (CU_add_test(pSuite, "test_call_in_coroutine", test_call_in_coroutine) == NULL)
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
    /* {{{ CU_add_test: test_disk_write */
    if (CU_add_test(pSuite, "test_disk_write", test_disk_write) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_disk_io_failed */
    if (CU_add_test(pSuite, "test_disk_io_failed", test_disk_io_failed) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_utils */
    if (CU_add_test(pSuite, "test_utils", test_utils) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_sock_read */
    if (CU_add_test(pSuite, "test_sock_read", test_sock_read) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_sock_timeout */
    if (CU_add_test(pSuite, "test_sock_timeout", test_sock_timeout) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_sock_write */
    if (CU_add_test(pSuite, "test_sock_write", test_sock_write) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_tcp_blocked_connect */
    if (CU_add_test(pSuite, "test_tcp_blocked_connect", test_tcp_blocked_connect) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_tcp_timeout_connect */
    if (CU_add_test(pSuite, "test_tcp_timeout_connect", test_tcp_timeout_connect) == NULL)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }
    /* }}} */
    /* {{{ CU_add_test: test_set_stacksize */
    if (CU_add_test(pSuite, "test_set_stacksize", test_set_stacksize) == NULL)
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
    int ret = 0;
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
