#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "event.h"

#ifdef EPOLL_EVENT
extern eventop_t epoll_event_op;
#endif

event_loop_t *base_event_loop = NULL;

event_loop_t *create_eventloop() {
    event_loop_t *loop = malloc(sizeof(*loop));
    memset(loop, 0, sizeof(*loop));
    loop->eventop = &epoll_event_op;
    loop->eventop->init_eventloop(loop);
    base_event_loop = loop;
    return loop;
}

void run_eventloop(event_loop_t *loop) {
    while (!loop->running) {
        loop->eventop->dispath_events(loop);
    }
}

void register_file_event(event_loop_t *loop, int fd, int mask,
                         event_handler_t *handler) {
    if (fd >= EV_NFILE_EVENT)
        return;
    loop->eventop->add_file_event(loop, fd, mask, handler);
}

void register_signal_event(event_loop_t *loop, int sig,
                           event_handler_t *handler) {
    if (sig <= 0 || sig >= EV_NSIGAL_EVENT)
        return;
    loop->eventop->add_sig_event(loop, sig, handler);
}

void ungister_file_event(event_loop_t *loop, int fd, int mask) {
    if (fd < 0 || fd >= EV_NFILE_EVENT)
        return;
    loop->eventop->del_file_event(loop, fd, mask);
}

void unregister_signal_event(event_loop_t *loop, int sig) {
    if (sig <= 0 || sig >= EV_NSIGAL_EVENT)
        return;
    loop->eventop->del_sig_event(loop, sig);
}