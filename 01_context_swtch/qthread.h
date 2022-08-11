#ifndef QTHREAD_H
#define QTHREAD_H
typedef unsigned long uint64;
typedef unsigned int uint32;

struct context
{
    uint64 ra;
    uint64 sp;
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

enum thread_state
{
    UNUSED,
    READY,
    RUNNING,
};

typedef void *(*qthread_entry_t)(void *);

typedef struct
{
    uint64 tid;
    enum thread_state state;
    qthread_entry_t qthread_entry;
    char stack[1024];
    struct context context;
} qthread_t;

int qthread_create(qthread_entry_t entry);
void qthread_yield();
void qthread_exit();
void qthread_run_all();

#endif