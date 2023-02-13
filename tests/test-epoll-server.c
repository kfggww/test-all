#include <stdio.h>
#include <stdlib.h>

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define MQ_REQUEST_NAME "/mq_request"
#define MQ_RESPONSE_NAME "/mq_response"

struct EventLoop;

typedef void EventCallback(struct EventLoop *el, int fd, void *data);

typedef struct FileEvent {
    EventCallback *rdcb;
    EventCallback *wtcb;
    void *data;
} FileEvent;

typedef struct EventLoop {
    int stop;
    int size;
    FileEvent *events;
    int epfd;
    struct epoll_event *ee;
} EventLoop;

typedef struct {
    int valid;
    int data;
} Reply;

Reply reply;

EventLoop *CreateEventLoop(int size) {
    EventLoop *el = malloc(sizeof(*el));

    el->stop = 0;
    el->size = size;
    el->events = (FileEvent *)malloc(sizeof(FileEvent) * size);

    el->epfd = epoll_create1(0);
    el->ee =
        (struct epoll_event *)malloc(sizeof(struct epoll_event) * el->size);

    printf("server epfd: %d\n", el->epfd);
    return el;
}

int CreateFileEvent(EventLoop *el, int fd, int mask, EventCallback cb,
                    void *data) {
    if (fd >= el->size)
        return 1;

    el->events[fd].data = data;
    el->events[fd].rdcb = (mask & EPOLLIN) ? cb : NULL;
    el->events[fd].wtcb = (mask & EPOLLOUT) ? cb : NULL;

    struct epoll_event ev = {.data.fd = fd, .events = mask};
    return epoll_ctl(el->epfd, EPOLL_CTL_ADD, fd, &ev);
}

void HandleMqRequest(EventLoop *el, int fd, void *data) {
    int request = 0;
    ssize_t len = mq_receive(fd, (char *)&request, sizeof(int), 0);
    if (len != sizeof(int)) {
        perror("server mq_receive failed");
        el->stop = 1;
        return;
    }
    if (request == -1) {
        el->stop = 1;
    }

    Reply *rp = (Reply *)data;
    rp->valid = 1;
    rp->data = request + 1;
}

void HandleMqResponse(EventLoop *el, int fd, void *data) {

    Reply *rp = (Reply *)data;
    if (!rp->valid) {
        return;
    }

    int response = rp->data;
    if (mq_send(fd, (char *)&response, sizeof(int), 0)) {
        perror("server mq_send failed");
        el->stop = 1;
    }

    rp->valid = 0;
    rp->data = 0;
}

void ProcessEventLoop(EventLoop *el) {

    while (!el->stop) {
        int numbers = epoll_wait(el->epfd, el->ee, el->size, -1);
        if (numbers == -1) {
            perror("epoll_wait failed");
            return;
        }

        for (int i = 0; i < numbers; i++) {
            int fd = el->ee[i].data.fd;
            int mask = el->ee[i].events;

            if (mask & EPOLLIN) {
                el->events[fd].rdcb(el, fd, el->events[fd].data);
            }
            if (mask & EPOLLOUT) {
                el->events[fd].wtcb(el, fd, el->events[fd].data);
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    struct mq_attr attr = {.mq_msgsize = sizeof(int), .mq_maxmsg = 1};

    mqd_t mq_request =
        mq_open(MQ_REQUEST_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attr);
    mqd_t mq_response =
        mq_open(MQ_RESPONSE_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attr);
    if (mq_request == -1 || mq_response == -1) {
        perror("server mq_open failed");
        return -1;
    }

    reply.valid = 0;
    reply.data = 0;
    EventLoop *el = CreateEventLoop(10);
    CreateFileEvent(el, mq_request, EPOLLIN, HandleMqRequest, &reply);
    CreateFileEvent(el, mq_response, EPOLLOUT, HandleMqResponse, &reply);

    ProcessEventLoop(el);

    mq_unlink(MQ_REQUEST_NAME);
    mq_unlink(MQ_RESPONSE_NAME);
    printf("server exit\n");
    return 0;
}
