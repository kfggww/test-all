#include <stdio.h>

#include <signal.h>
#include "event.h"

void sig_handler(event_loop *loop, void *arg) {
    printf("signal is caught\n");
    loop->stop = 1;
}

int main(int argc, char const *argv[]) {
    event_loop *loop = createEventLoop();
    registerSignalEvent(loop, SIGPIPE, sig_handler);
    loopEvent(loop);
    return 0;
}
