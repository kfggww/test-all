#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include "ufsd.h"

void ufs_daemon() {
    /**
     * first fork
     */
    if (fork()) {
        _exit(0);
    }
    pid_t sid = setsid();
    if (sid == -1) {
        _exit(-1);
    }

    /**
     * second fork
     */
    if (fork()) {
        _exit(0);
    }
    if (chdir("/")) {
        _exit(-1);
    }
    for (int i = 0; i < 3; i++) {
        close(i);
    }

    int fd = open("/dev/null", O_RDWR);
    if (fd != 0) {
        exit(-1);
    }
    dup2(fd, 1);
    dup2(fd, 2);
}

int main(int argc, char const *argv[]) {

    ufs_daemon();

    int fd = open("/tmp/ufsd.log", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(fd, 0);

    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond;

    pthread_condattr_t condattr;
    pthread_condattr_init(&condattr);
    clockid_t clockid;
    pthread_condattr_getclock(&condattr, &clockid);
    pthread_cond_init(&cond, &condattr);

    char buf[32];
    int cnt = 0;

    while (1) {
        pthread_mutex_lock(&lock);

        struct timespec timeout = {0};
        clock_gettime(clockid, &timeout);
        timeout.tv_sec += 2;
        pthread_cond_timedwait(&cond, &lock, &timeout);

        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%d,%ld\n", cnt, timeout.tv_sec);
        write(fd, buf, strlen(buf));

        cnt += 1;
        pthread_mutex_unlock(&lock);
    }

    return 0;
}
