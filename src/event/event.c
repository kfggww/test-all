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
#ifdef EPOLL_EVENT
    loop->eventop = &epoll_event_op;
#endif
    loop->eventop->init_eventloop(loop);
    base_event_loop = loop;
    return loop;
}

void run_eventloop(event_loop_t *loop) {
    loop->running = 1;
    while (loop->running) {
        loop->eventop->dispath_events(loop);
    }
}

void remove_eventloop(event_loop_t *loop) {
    if (loop->eventop->fini_eventloop)
        loop->eventop->fini_eventloop(loop);
    free(loop);
}

void register_file_event(event_loop_t *loop, int fd, int mask,
                         event_handler_t *handler, void *arg) {
    if (fd >= EV_NFILE_EVENT)
        return;
    loop->eventop->add_file_event(loop, fd, mask, handler, arg);
}

void register_signal_event(event_loop_t *loop, int sig,
                           event_handler_t *handler, void *arg) {
    if (sig <= 0 || sig >= EV_NSIGAL_EVENT)
        return;
    loop->eventop->add_sig_event(loop, sig, handler, arg);
}

void unregister_file_event(event_loop_t *loop, int fd, int mask) {
    if (fd < 0 || fd >= EV_NFILE_EVENT)
        return;
    loop->eventop->del_file_event(loop, fd, mask);
}

void unregister_signal_event(event_loop_t *loop, int sig) {
    if (sig <= 0 || sig >= EV_NSIGAL_EVENT)
        return;
    loop->eventop->del_sig_event(loop, sig);
}