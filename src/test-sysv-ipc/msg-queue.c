#include <sys/msg.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cutest.h"

#define MSG_SIZE 64

struct msgbuf {
    long mtype;
    char buf[MSG_SIZE];
};

CUTEST_SUIT(msg_queue)

CUTEST_CASE(msg_queue, parent2child) {
    key_t key = ftok("/etc/passwd", 0x1234);
    int mqid = msgget(key, IPC_CREAT | S_IRUSR | S_IWUSR);
    CUT_EXPECT_NQ(mqid, -1);

    if (fork() == 0) {
        struct msgbuf msg = {.mtype = 1};
        memset(msg.buf, 0, MSG_SIZE);
        sprintf(msg.buf, "hello from system v msg queue");
        msgsnd(mqid, &msg, MSG_SIZE, 0);

        printf("parent2child: child exit\n");
        exit(0);
    } else {
        struct msgbuf msg;
        msgrcv(mqid, &msg, MSG_SIZE, 1, 0);
        printf("data received: %s\n", msg.buf);
    }

    msgctl(mqid, IPC_RMID, NULL);
    printf("message queue removed\n");
}