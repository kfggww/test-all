#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <signal.h>
#include "event.h"
#define EPOLL_NFIRED_EVENTS 8

extern event_loop_t *base_event_loop;

typedef struct {
    int epfd;
    struct epoll_event fired_events[EPOLL_NFIRED_EVENTS];
    int pipes[2];
} epoll_data_t;

static void epoll_sa_handler(int sig) {
    if (base_event_loop == NULL)
        return;

    epoll_data_t *epdata = (epoll_data_t *)base_event_loop->private_data;
    write(epdata->pipes[1], &sig, sizeof(int));
}

static void epoll_handle_signal_event(event_loop_t *loop, void *arg) {
    epoll_data_t *epdata = (epoll_data_t *)base_event_loop->private_data;
    int sig = -1;
    read(epdata->pipes[0], &sig, sizeof(int));
    if (sig == -1)
        return;
    if (loop->signal_events[sig].handle_signal)
        loop->signal_events[sig].handle_signal(loop, NULL);
}

void epoll_add_file_event(event_loop_t *loop, int fd, int mask,
                          event_handler_t *handler) {
    loop->file_events[fd].fd = fd;
    if (mask & EV_MASK_READABLE)
        loop->file_events[fd].handle_read = handler;
    if (mask & EV_MASK_WRITABLE)
        loop->file_events[fd].handle_write = handler;

    epoll_data_t *epoll_data = (epoll_data_t *)loop->private_data;
    int epoll_mask = 0;
    epoll_mask |= (mask & EV_MASK_READABLE) ? EPOLLIN : 0;
    epoll_mask |= (mask & EV_MASK_WRITABLE) ? EPOLLOUT : 0;
    struct epoll_event epoll_event = {.data.fd = fd, .events = epoll_mask};
    epoll_ctl(epoll_data->epfd, EPOLL_CTL_ADD, fd, &epoll_event);
}

void epoll_del_file_event(event_loop_t *loop, int fd, int mask) {
    if (mask == EV_MASK_NONE)
        return;

    epoll_data_t *epdata = (epoll_data_t *)loop->private_data;
    if (mask == EV_MASK_ALL) {
        epoll_ctl(epdata->epfd, EPOLL_CTL_DEL, fd, NULL);
    }

    mask = (EV_MASK_ALL) & (~mask);
    struct epoll_event epoll_event = {.data.fd = fd, .events = 0};
    epoll_event.events |= (mask & EV_MASK_READABLE) ? EPOLLIN : 0;
    epoll_event.events |= (mask & EV_MASK_WRITABLE) ? EPOLLOUT : 0;
    epoll_ctl(epdata->epfd, EPOLL_CTL_MOD, fd, &epoll_event);
}

void epoll_add_sig_event(event_loop_t *loop, int sig,
                         event_handler_t *handler) {
    loop->signal_events[sig].sig = sig;
    loop->signal_events[sig].handle_signal = handler;
}

void epoll_del_sig_event(event_loop_t *loop, int sig) {
    if (sig < 0 || sig >= EV_NSIGAL_EVENT)
        return;
    loop->signal_events[sig].handle_signal = NULL;
}

void epoll_dispath_events(event_loop_t *loop) {
    epoll_data_t *epoll_data = (epoll_data_t *)loop->private_data;

    int nfd = epoll_wait(epoll_data->epfd, epoll_data->fired_events,
                         EPOLL_NFIRED_EVENTS, -1);

    for (int i = 0; i < nfd; i++) {
        int evfd = epoll_data->fired_events[i].data.fd;
        int mask = epoll_data->fired_events[i].events;
        file_event_t *event = &loop->file_events[evfd];
        if ((mask & EPOLLIN) && event->handle_read) {
            event->handle_read(loop, NULL);
        }
        if ((mask & EPOLLOUT) && event->handle_write) {
            event->handle_write(loop, NULL);
        }
    }
}

void epoll_init_eventloop(event_loop_t *loop) {
    loop->eventop = &epoll_event_op;
    epoll_data_t *epoll_data = malloc(sizeof(epoll_data_t));
    memset(epoll_data, 0, sizeof(epoll_data_t));
    epoll_data->epfd = epoll_create1(0);
    pipe2(epoll_data->pipes, O_NONBLOCK | O_CLOEXEC);

    epoll_add_file_event(loop, epoll_data->pipes[0], EV_MASK_READABLE,
                         epoll_handle_signal_event);
}

void epoll_fini_eventloop(event_loop_t *loop) {}

eventop_t epoll_event_op = {
    .init_eventloop = epoll_init_eventloop,
    .fini_eventloop = epoll_fini_eventloop,
    .add_file_event = epoll_add_file_event,
    .del_file_event = epoll_del_file_event,
    .add_sig_event = epoll_add_sig_event,
    .del_sig_event = epoll_del_sig_event,
};