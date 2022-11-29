#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <signal.h>
#include <time.h>

void sig_handler(int signo, siginfo_t *info, void *data) {
    ucontext_t *ucp = (ucontext_t *)data;
    printf("ucontext: %p\n", ucp);
    printf("Signal #%d has been handled\n", signo);
}

int main(int argc, char **argv) {

    /*Create POSIX timer*/
    timer_t timerid;

    struct sigevent ev;
    ev.sigev_notify = SIGEV_SIGNAL;
    ev.sigev_signo = SIGRTMIN;
    ev.sigev_value.sival_ptr = &timerid;

    assert(!timer_create(CLOCK_MONOTONIC, &ev, &timerid));

    /*Set SIGRTMIN signal handler*/
    struct sigaction act = {.sa_sigaction = sig_handler,
                            .sa_flags = SA_SIGINFO};
    assert(!sigaction(SIGRTMIN, &act, NULL));

    /*Start POSIX timer*/
    struct itimerspec itv;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_nsec = 0;
    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_nsec = 0;
    assert(!timer_settime(timerid, 0, &itv, NULL));

    /*Wait for SIGRTMIN signl*/
    int err = sleep(10);
    printf("stack addr: %p\n", &err);
    printf("Time left: %ds\n", err);

    return 0;
}