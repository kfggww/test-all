#include "timer_posix.h"
#include <assert.h>
#include <cstdio>

void callback1(void *data) {

    int *pa = (int *)data;
    printf("[%s] args: %d\n", __func__, *pa);
}

void callback2(void *data) { callback1(data); }

int main(int argc, char **argv) {
    // {
    int a = 10;
    int b = 20;
    int c = 30;
    TimerNormalResolution timer1;
    TimerHighResolution timer2;

    timer1.AddCallback({callback1, &a, 2000});
    timer1.AddCallback({callback2, &b, 4000});
    timer2.AddCallback({callback1, &c, 3000});
    // }
    while (1)
        ;

    return 0;
}