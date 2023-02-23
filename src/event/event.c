#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>

#include "event.h"

event_loop *createEventLoop() {
    event_loop *loop = malloc(sizeof(*loop));
    memset(loop, 0, sizeof(*loop));

    loop->stop = 0;
    loop->epfd = epoll_create1(0);
    if (loop->epfd == -1) {
        free(loop);
        return NULL;
    }

    if (pipe2(loop->sig_pipe, O_CLOEXEC | O_NONBLOCK)) {
        free(loop);
        return NULL;
    }

    return loop;
}

void loopEvent(event_loop *loop) {
    while (!loop->stop) {
        int nfds = epoll_wait(loop->epfd, loop->fired_events, EV_LOOP_SIZE, -1);
        if (nfds < 0)
            continue;

        struct epoll_event *ev = NULL;
        for (int i = 0; i < nfds; i++) {
            ev = loop->fired_events[i];
            int fd = ev->data.fd;
            if (ev->events & EPOLLIN && loop->events[fd].on_readable) {
                loop->events[fd].on_readable(loop, NULL);
            }
            if (ev->events & EPOLLOUT && loop->events[fd].on_writable) {
                loop->events[fd].on_writable(loop, NULL);
            }
        }
    }
}

void registerFileEvent(event_loop *loop, int fd, int mask,
                       event_handler *handler) {
    if (fd >= EV_LOOP_SIZE)
        return;

    event *ev = &loop->events[fd];
    ev->fd = fd;
    if (mask & EV_MASK_READABLE) {
        ev->on_readable = handler;
    }
    if (mask & EV_MASK_WRITABLE) {
        ev->on_writable = handler;
    }

    struct epoll_event epoll_event;
    epoll_event.data.fd = fd;
    epoll_event.events = 0;
    epoll_event.events |= (mask & EV_MASK_READABLE) ? EPOLLIN : 0;
    epoll_event.events |= (mask & EV_MASK_WRITABLE) ? EPOLLOUT : 0;
    epoll_ctl(loop->epfd, EPOLL_CTL_ADD, fd, &epoll_event);
}

void registerSignalEvent(event_loop *loop, int sig, event_handler *handler) {
    if (sig <= 0 || sig > 63)
        return;
    loop->sig_events[sig].sig = sig;
    loop->sig_events[sig].on_signal = handler;
}

void unregisterFileEvent(event_loop *loop, int fd, int mask) {}

void unregisterSignalEvent(event_loop *loop, int sig) {}