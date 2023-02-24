#include <stdio.h>
#include <string.h>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

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

int main(int argc, char const *argv[]) {
    event_loop_t *loop = create_eventloop();

    register_signal_event(loop, SIGPIPE, handle_sigpipe, NULL);
    register_signal_event(loop, SIGUSR1, handle_sigusr1, NULL);

    int fd = open("event.fifo", O_RDONLY | O_NONBLOCK);
    if (fd != -1) {
        register_file_event(loop, fd, EV_MASK_READABLE, handle_fifo, &fd);
    }

    run_eventloop(loop);
    remove_eventloop(loop);

    close(fd);
    return 0;
}
