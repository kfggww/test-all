#include "timer_posix.h"
#include <cstdio>

void callback1(void *data) {

    int *pa = (int *)data;
    printf("[%s] args: %d\n", __func__, *pa);
}

void callback2(void *data) { callback1(data); }

int main(int argc, char **argv) {
    TimerPOSIX timer;
    int a = 10;
    int b = 20;

    timer.AddCallback(2000, callback1, &a);
    timer.AddCallback(3000, callback2, &b);

    while (1)
        ;

    return 0;
}