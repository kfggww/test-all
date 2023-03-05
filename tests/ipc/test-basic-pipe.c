#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include "cutest.h"

CUTEST_SUIT(basic_pipe)

/**
 * 0. talking to myself
 * 1. parent write, child read
 * 2. parent read, child write
 * 3. parent write, children read
 * 4. parent read, children write
 * 5. first child write, second child read
 * 6. blocking read.
 */

/*0*/
CUTEST_CASE(basic_pipe, talking_to_myself) {
    int pipefd[2];
    pipe(pipefd);

    const char *msg = "I'm talking to myself";
    write(pipefd[1], msg, strlen(msg));

    char buf[32];
    read(pipefd[0], buf, 32);
    printf("talking_to_myself: %s\n", buf);

    close(pipefd[0]);
    close(pipefd[1]);
}

/*1*/
CUTEST_CASE(basic_pipe, parent2child) {
    int pipefd[2];
    pipe(pipefd);

    const char *msg = "parent write, child read";
    write(pipefd[1], msg, strlen(msg));

    if (fork() == 0) {
        close(pipefd[1]);
        char buf[64];
        memset(buf, 0, 64);
        read(pipefd[0], buf, 64);
        printf("parent2child: %s\n", buf);
        exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);
}

/*2*/
CUTEST_CASE(basic_pipe, child2parent) {
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        close(pipefd[0]);
        const char *msg = "parent read, child write";
        write(pipefd[1], msg, strlen(msg));
        close(pipefd[1]);
        exit(0);
    }

    close(pipefd[1]);

    char buf[64];
    memset(buf, 0, 64);
    read(pipefd[0], buf, 64);
    printf("child2parent: %s\n", buf);
    close(pipefd[0]);
}

void fork_child_read(int id, int pipefd[2], const char *msg_pregix) {
    if (fork() == 0) {
        close(pipefd[1]);
        int n;
        read(pipefd[0], &n, sizeof(int));
        printf("%s: child %d get data %d\n", msg_pregix, id, n);
        close(pipefd[0]);
        exit(0);
    }
}

/*3*/
CUTEST_CASE(basic_pipe, parent2children) {
    int pipefd[2];
    pipe(pipefd);

    for (int i = 1; i <= 10; i++)
        write(pipefd[1], &i, sizeof(int));

    const char *msg_prefix = "parent2children:";
    fork_child_read(1, pipefd, msg_prefix);
    fork_child_read(2, pipefd, msg_prefix);
    fork_child_read(3, pipefd, msg_prefix);

    close(pipefd[0]);
    close(pipefd[1]);
}

void fork_child_write(int pipefd[2], int data) {
    if (fork() == 0) {
        close(pipefd[0]);
        write(pipefd[1], &data, sizeof(int));
        close(pipefd[1]);
        exit(0);
    }
}

/*4*/
CUTEST_CASE(basic_pipe, children2parent) {
    int pipefd[2];
    pipe(pipefd);

    int data[] = {512, 1024};

    fork_child_write(pipefd, data[0]);
    fork_child_write(pipefd, data[1]);

    close(pipefd[1]);
    int n;
    while (read(pipefd[0], &n, sizeof(int)) == sizeof(int)) {
        printf("children2parent: get data %d\n", n);
    }
    close(pipefd[0]);
}

/*5*/
CUTEST_CASE(basic_pipe, two_children) {
    int pipefd[2];
    pipe(pipefd);

    const char *msg = "pipe between two children";
    if (fork() == 0) {
        close(pipefd[0]);
        write(pipefd[1], msg, strlen(msg));
        close(pipefd[1]);
        exit(0);
    }

    if (fork() == 0) {
        close(pipefd[1]);
        char buf[64];
        memset(buf, 0, 64);
        read(pipefd[0], buf, 64);
        printf("two_children: %s\n", buf);
        close(pipefd[0]);
        exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);
}

/*6*/
CUTEST_CASE(basic_pipe, blocking_read) {
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        /* NOTE: remove the comment below if you don't want child process
         * blocking while reading data from pipe. Otherwise you will see that
         * there is still a "basic-pipe" process after you finish this test, and
         * you have to kill it manually.*/
        // close(pipefd[1]);

        int num;
        read(pipefd[0], &num, sizeof(int));

        /* NOTE: since the write end of pipe is a valid file descriptor in
         * current process, the print below should never execute.*/
        printf("should NEVER goes here\n");
        exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    printf("blocking_read: parent process exit\n");
}