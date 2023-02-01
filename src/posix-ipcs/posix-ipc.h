#ifndef POSIX_IPC_H
#define POSIX_IPC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE
#include <fcntl.h> /* For O_* constants */
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <mqueue.h>
#include <semaphore.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_MAX
#endif

enum {
    LOG_INFO,
    LOG_DEBUG,
    LOG_WARNING,
    LOG_LEVEL_MAX,
};

#define __log_print(level, fmt, ...)                                           \
    do {                                                                       \
        if (level > LOG_LEVEL)                                                 \
            break;                                                             \
        switch (level) {                                                       \
        case LOG_INFO:                                                         \
            printf("[info]: ");                                                \
            break;                                                             \
        case LOG_DEBUG:                                                        \
            printf("[debug]: ");                                               \
            break;                                                             \
        case LOG_WARNING:                                                      \
            printf("[warning]: ");                                             \
            break;                                                             \
        }                                                                      \
        printf(fmt, ##__VA_ARGS__);                                            \
    } while (0)

#define log_info(fmt, ...) __log_print(LOG_INFO, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) __log_print(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) __log_print(LOG_WARNING, fmt, ##__VA_ARGS__)

#define CONNECTION_SHM_NAME "/connection-shm"
#define CONNECTION_SEM_NAME "/connection-sem"
#define NEW_CONNECTION_SEM_NAME "/new-connection-sem"
#define CONNECTION_SHM_SIZE 4096
#define MQ_NAME_SIZE 64

struct msgbuf {
    int type;
    union {
        struct {
            int a;
            int b;
        } request_add;

        struct {
            int c;
        } response_add;

        struct {
            int disconect;
        } request_disconnect;

        struct {
            int stop_server;
        } request_stop_server;
    } data;
};

enum msgtype {
    REQ_ADD,
    RSP_ADD,
    REQ_DISCONNECT,
    REQ_STOP_SERVER,
    MSG_TYPE_MAX,
};

struct connection {
    short valid;
    char mqreq[MQ_NAME_SIZE];
    char mqrsp[MQ_NAME_SIZE];
    int mqreq_fd;
    int mqrsp_fd;
};

#endif