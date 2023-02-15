#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <linux/input.h>

#include "logkeys.h"

static int get_kbd_event();

int main(int argc, char const *argv[]) {

    int kbd_event_id = get_kbd_event();
    char kbd_event_file[64];
    memset(kbd_event_file, 0, sizeof(kbd_event_file));
    sprintf(kbd_event_file, "%s/event%d", EVENT_PATH_PREFIX, kbd_event_id);

    int kbd_fd = open(kbd_event_file, O_RDONLY);
    if (kbd_fd < 0) {
        perror("open kbd_event_file failed");
        return -1;
    }

    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1 failed");
        return -1;
    }

    struct epoll_event ev1 = {.data.fd = kbd_fd, .events = EPOLLIN};
    int err = epoll_ctl(epfd, EPOLL_CTL_ADD, kbd_fd, &ev1);
    if (err == -1) {
        perror("epoll_ctl failed");
        return -1;
    }

    struct input_event input_event;
    struct epoll_event ev;
    ssize_t len = 0;

    for (;;) {
        err = epoll_wait(epfd, &ev, 1, -1);
        if (err == -1) {
            perror("epoll_wait failed");
            return -1;
        }

        if (ev.data.fd != kbd_fd) {
            perror("epoll_wait failed, not kbd_fd");
            return -1;
        }

        len = read(kbd_fd, &input_event, sizeof(input_event));
        if (len != sizeof(input_event)) {
            perror("read failed");
            return -1;
        }

        printf("keyboard event:\n\ttype: %d\n\tcode: %d\n\tvalue: %d\n",
               input_event.type, input_event.code, input_event.value);
    }

    close(kbd_fd);
    close(epfd);
    return 0;
}

static int get_kbd_event() { return 2; }