#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <mqueue.h>

int main(int argc, char **argv) {

    // 1. open posix message queue in read only mode
    mqd_t mqdes = mq_open("/mq.demo", O_RDONLY | O_NONBLOCK);
    if (mqdes < 0) {
        perror("mq-consumer failed mq_open:");
        return errno;
    }

    // 2. read data from posix message queue
    char buf[32];
    for (;;) {
        memset(buf, 0, sizeof(buf));
        ssize_t sz = mq_receive(mqdes, buf, sizeof(buf), NULL);
        if (sz <= 0) {
            if (sz < 0)
                break;
        }
        printf("read data: %s, size: %ld\n", buf, sz);
    }

    // 3. close posix message queue
    close(mqdes);
    mq_unlink("/mq.demo");
    printf("mq-consumer exit\n");
    return 0;
}
