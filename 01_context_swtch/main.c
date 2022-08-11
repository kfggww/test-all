#include "qthread.h"
#include <stdio.h>

void *thread1_start(void *args)
{
    int i = 1;
    while (i < 1000)
    {
        if ((i % 100) == 0)
        {
            printf("Thread #1: running\n");
            qthread_yield();
        }
        i++;
    }
    qthread_exit();
    return 0;
}

void *thread2_start(void *args)
{
    int i = 1;
    while (i < 1000)
    {
        if ((i % 100) == 0)
        {
            printf("Thread #2: running\n");
            qthread_yield();
        }
        i++;
    }
    qthread_exit();
    return 0;
}

int main(int argc, char **argv)
{
    qthread_create(thread1_start);
    qthread_create(thread2_start);
    printf("Start to run all threads...\n");
    qthread_run_all();
    printf("All threads are done!\n");

    return 0;
}