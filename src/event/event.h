#ifndef EVENT_H
#define EVENT_H

#include <sys/epoll.h>

#define EV_MASK_NONE 0
#define EV_MASK_READABLE 1
#define EV_MASK_WRITABLE 2
#define EV_MASK_ALL (EV_MASK_READABLE | EV_MASK_WRITABLE)

#define EV_NFILE_EVENT 16
#define EV_NSIGAL_EVENT 64

#define EPOLL_EVENT

struct event_loop;
typedef void event_handler_t(struct event_loop *loop, void *arg);

typedef struct {
    int fd;
    event_handler_t *handle_read;
    event_handler_t *handle_write;
    void *arg;
} file_event_t;

typedef struct {
    int sig;
    event_handler_t *handle_signal;
    void *arg;
} signal_event_t;

struct eventop;

typedef struct event_loop {
    int running;
    file_event_t file_events[EV_NFILE_EVENT];
    signal_event_t signal_events[EV_NSIGAL_EVENT];
    struct eventop *eventop;
    void *private_data;
} event_loop_t;

typedef struct eventop {
    void (*init_eventloop)(event_loop_t *loop);
    void (*fini_eventloop)(event_loop_t *loop);
    void (*add_file_event)(event_loop_t *loop, int fd, int mask,
                           event_handler_t *handler, void *arg);
    void (*del_file_event)(event_loop_t *loop, int fd, int mask);
    void (*add_sig_event)(event_loop_t *loop, int sig, event_handler_t *handler,
                          void *arg);
    void (*del_sig_event)(event_loop_t *loop, int sig);
    void (*dispath_events)(event_loop_t *loop);
} eventop_t;

/*event loop APIs*/
event_loop_t *create_eventloop();
void run_eventloop(event_loop_t *loop);
void remove_eventloop(event_loop_t *loop);

/*file event APIs*/
void register_file_event(event_loop_t *loop, int fd, int mask,
                         event_handler_t *handler, void *arg);
void ungister_file_event(event_loop_t *loop, int fd, int mask);

/*signal event APIs*/
void register_signal_event(event_loop_t *loop, int sig,
                           event_handler_t *handler, void *arg);
void unregister_signal_event(event_loop_t *loop, int sig);

#endif