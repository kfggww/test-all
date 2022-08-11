#include "qthread.h"
#include <stdio.h>

#define MAX_THREADS_NR 16

extern void swtch(void *, void *);

static qthread_t qthread_table[MAX_THREADS_NR];
static qthread_t *current;
static struct context qthread_context;

static uint64 get_next_tid()
{
    static uint64 tid = 0;
    return ++tid;
}

int qthread_create(qthread_entry_t entry)
{
    int index_found = -1;
    for (int i = 0; i < MAX_THREADS_NR; ++i)
    {
        if (qthread_table[i].state == UNUSED)
        {
            index_found = i;
            break;
        }
    }

    if (index_found < 0 || index_found >= MAX_THREADS_NR)
    {
        return -1;
    }

    qthread_table[index_found].tid = get_next_tid();
    qthread_table[index_found].state = READY;
    qthread_table[index_found].qthread_entry = entry;

    qthread_table[index_found].context.ra = (uint64)entry;
    qthread_table[index_found].context.sp = (uint64)qthread_table[index_found].stack + 1024;

    return 0;
}

void qthread_yield()
{
    current->state = READY;
    swtch(&current->context, &qthread_context);
}

void qthread_exit()
{
    current->state = UNUSED;
    current->tid = 0;
    swtch(&current->context, &qthread_context);
}

void qthread_run_all()
{
    int count = 0;
    while (1)
    {
        for (int i = 0; i < MAX_THREADS_NR; ++i)
        {
            if (qthread_table[i].state == READY)
            {
                current = &qthread_table[i];
                current->state = RUNNING;
                count++;
                swtch(&qthread_context, &current->context);
            }
        }

        if (count == 0)
            break;
        count = 0;
    }
}