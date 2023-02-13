#include <stdio.h>
#include <stdlib.h>

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define MQ_REQUEST_NAME "/mq_request"
#define MQ_RESPONSE_NAME "/mq_response"

int main(int argc, char const *argv[]) {

    int request = 0;
    int response = 0;

    mqd_t mq_request = mq_open(MQ_REQUEST_NAME, O_WRONLY);
    mqd_t mq_response = mq_open(MQ_RESPONSE_NAME, O_RDONLY);

    if (mq_request == -1 || mq_response == -1) {
        perror("client failed mq_open");
        return -1;
    }

    for (;;) {
        scanf("%d", &request);
        int err = mq_send(mq_request, (char *)&request, sizeof(int), 0);
        if (err == -1) {
            perror("client failed mq_send");
            return -1;
        }

        if (request == -1)
            break;

        ssize_t len =
            mq_receive(mq_response, (char *)&response, sizeof(int), NULL);
        if (len != sizeof(int)) {
            perror("client failed mq_receive\n");
            return -1;
        }

        printf("client send %d receive %d\n", request, response);
    }

    printf("client exit\n");
    return 0;
}