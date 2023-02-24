#ifndef EVENT_H
#define EVENT_H

#include <sys/epoll.h>

#define EV_MASK_NONE 0
#define EV_MASK_READABLE 1
#define EV_MASK_WRITABLE 2
#define EV_MASK_ALL (EV_MASK_READABLE | EV_MASK_WRITABLE)

#define EV_TYPE_FILE 1
#define EV_TYPE_SIGNAL 2

#define EV_NFILE_EVENT 16
#define EV_NFIRED_EVENT 8

struct event_loop;
typedef void event_handler(struct event_loop *loop, void *arg);

typedef struct {
    int fd;
    event_handler *on_readable;
    event_handler *on_writable;
} file_event;

typedef struct {
    int sig;
    event_handler *on_signal;
} signal_event;

typedef struct event_loop {
    int epfd;
    int stop;
    struct epoll_event fired_events[EV_NFIRED_EVENT];
    file_event file_events[EV_NFILE_EVENT];
    signal_event sig_events[64];
    int sig_pipe[2];
} event_loop;

event_loop *createEventLoop();
void loopEvent(event_loop *loop);

void registerFileEvent(event_loop *loop, int fd, int mask,
                       event_handler *handler);
void registerSignalEvent(event_loop *loop, int sig, event_handler *handler);
void unregisterFileEvent(event_loop *loop, int fd, int mask);
void unregisterSignalEvent(event_loop *loop, int sig);

#endif