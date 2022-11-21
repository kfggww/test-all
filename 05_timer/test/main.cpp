#include "timer_posix.h"
#include <cstdio>

void callback1(void *data) {

    int *pa = (int *)data;
    printf("[%s] args: %d\n", __func__, *pa);
}

void callback2(void *data) { callback1(data); }

int main(int argc, char **argv) {
    TimerHighResolution timer;
    int a = 10;
    // int b = 20;

    timer.AddCallback({callback1, &a, 2000});
    // timer.AddCallback({callback2, &b, 3000});

    while (1)
        ;

    return 0;
}