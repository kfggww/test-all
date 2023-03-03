#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cutest.h"
#include "event.h"

void handle_sigpipe(event_loop_t *loop, void *arg) {
    printf("handle SIGPIPE signal\n");
}

void handle_sigusr1(event_loop_t *loop, void *arg) {
    printf("handle SIGKUSR1 signal, bye\n");
    loop->running = 0;
}

void handle_fifo(event_loop_t *loop, void *arg) {
    int fifofd = *(int *)arg;
    char buf[32];
    memset(buf, 0, 32);
    read(fifofd, buf, 32);
    printf("handle fifo file event: %s", buf);
}

int create_tcpserver() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        return sock;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(2089);
    int err = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (err == -1)
        goto error;

    err = listen(sock, 128);
    if (err == -1)
        goto error;

    return sock;
error:
    close(sock);
    return -1;
}

void handle_client(event_loop_t *loop, void *arg) {
    int fd = *(int *)arg;
    char buf[32];
    memset(buf, 0, 32);

    ssize_t n = read(fd, buf, 32);
    if (n <= 0) {
        printf("unregister file event: %d\n", fd);
        unregister_file_event(loop, fd, EV_MASK_ALL);
        free(arg);
        close(fd);
    }
    printf("handle client: %s\n", buf);
}

void handle_connect(event_loop_t *loop, void *arg) {
    int sockfd = *(int *)arg;
    int fd = accept(sockfd, NULL, NULL);
    int *argptr = malloc(sizeof(int));
    *argptr = fd;
    register_file_event(loop, fd, EV_MASK_READABLE, handle_client, argptr);
}

CUTEST_SUIT(test_eventloop)

CUTEST_CASE(test_eventloop, eventloop) {
    event_loop_t *loop = create_eventloop();

    register_signal_event(loop, SIGPIPE, handle_sigpipe, NULL);
    register_signal_event(loop, SIGUSR1, handle_sigusr1, NULL);

    int err = mkfifo("/tmp/event.fifo", S_IRUSR | S_IWUSR);
    int fifofd = -1;
    if (err == 0) {
        fifofd = open("/tmp/event.fifo", O_RDONLY | O_NONBLOCK);
        if (fifofd != -1) {
            register_file_event(loop, fifofd, EV_MASK_READABLE, handle_fifo,
                                &fifofd);
        }
    }
    int sockfd = create_tcpserver();
    register_file_event(loop, sockfd, EV_MASK_READABLE, handle_connect,
                        &sockfd);

    run_eventloop(loop);
    remove_eventloop(loop);

    close(sockfd);
    close(fifofd);
    unlink("/tmp/event.fifo");
}
