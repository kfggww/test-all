#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <mqueue.h>

int main(int argc, char **argv) {

    int n = argc >= 2 ? atoi(argv[1]) : 10;

    // 1. create posix message queue
    struct mq_attr mqattr;
    mqattr.mq_maxmsg = 10;
    mqattr.mq_msgsize = 30;

    mqd_t mqdes =
        mq_open("/mq.demo", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &mqattr);
    if (mqdes < 0) {
        perror("mq-producer failed mq_open:");
        return -1;
    }

    // 2. write data to message queue
    char msgbuf[16];
    for (int i = 0; i < n; i++) {
        memset(msgbuf, 0, sizeof(msgbuf));
        sprintf(msgbuf, "%d", i);
        int err = mq_send(mqdes, msgbuf, sizeof(msgbuf), 0);
        if (err != 0) {
            perror("mq-producer failed mq_send:");
            break;
        }
    }

    // 3. close mqdes
    close(mqdes);
    printf("mq-producer exit\n");
    return 0;
}