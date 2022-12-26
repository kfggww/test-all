#ifndef IPC_SYSV_H
#define IPC_SYSV_H

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>

#define TALKER_MSQ_ID 123456

typedef struct {
    long msg_type;
    char msg_data[128];
} msq_messag_t;

#endif