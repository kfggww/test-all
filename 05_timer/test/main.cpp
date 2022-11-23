#include "timer_posix.h"
#include "timer_soft.h"
#include <assert.h>
#include <cstdio>

void callback1(void *data) {

    int *pa = (int *)data;
    printf("[%s] args: %d\n", __func__, *pa);
}

int main(int argc, char **argv) {
    // {
    int a = 10;
    int b = 20;
    int c = 30;
    // TimerNormalResolution timer1;
    // TimerHighResolution timer2;
    TimerLowResolution timer3;

    // timer1.AddCallback({callback1, &a, 2000});
    // timer2.AddCallback({callback1, &b, 3000});
    timer3.AddCallback({callback1, &c, 1000});
    // }

    sleep(4);

    return 0;
}