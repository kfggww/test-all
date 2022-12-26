#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ipc_sysv.h"

char *trim(char *s, int n) {

    int i = n - 1;
    while (i >= 0 && s[i] == '\0')
        i--;
    while (i >= 0 && isspace(s[i]))
        s[i--] = '\0';

    i = 0;
    while (i < n && isspace(s[i]))
        i++;

    return s + i;
}

int main(int argc, char **argv) {

    int msqid = msgget(TALKER_MSQ_ID, IPC_CREAT | 0600);
    if (msqid == -1) {
        printf("Failed to get message queue id\n");
        return -1;
    }
    printf("Successfully get msqid: %d\n", msqid);

    int send_first = 0;
    int running = 1;
    msq_messag_t msg_snd;
    msq_messag_t msg_rcv;

    send_first = atoi(argv[1]);
    msg_snd.msg_type = atol(argv[2]);
    msg_rcv.msg_type = atol(argv[3]);

    memset(msg_snd.msg_data, 0, 128);
    memset(msg_rcv.msg_data, 0, 128);

    if (send_first) {
        printf(">>>: ");
        fgets(msg_snd.msg_data, 128, stdin);
        msgsnd(msqid, &msg_snd, 128, 0);
    }

    while (running) {
        msgrcv(msqid, &msg_rcv, 128, msg_rcv.msg_type, 0);
        printf("<<<: %s\n", trim(msg_rcv.msg_data, 128));

        if (strncmp(msg_rcv.msg_data, "end", 3) == 0) {
            printf("End\n");
            sprintf(msg_snd.msg_data, "end");
            running = 0;
        } else {
            printf(">>>: ");
            fgets(msg_snd.msg_data, 128, stdin);
        }
        msgsnd(msqid, &msg_snd, 128, 0);
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        printf("Failed to remove message queue with id: %d\n", msqid);
        return -1;
    }

    printf("Successfully remove message queue with id: %d\n", msqid);
    return 0;
}