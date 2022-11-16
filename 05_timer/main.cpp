#include "timer_posix.h"
#include <cstdio>

void callback(void *data) {

    int *pa = (int *)data;
    printf("[%s] args: %d\n", __func__, *pa);
}

int main(int argc, char **argv) {
    TimerPOSIX timer;
    int a = 10;
    int b = 20;

    timer.AddCallback(2000, callback, &a);
    timer.AddCallback(3000, callback, &b);

    while (1)
        ;

    return 0;
}