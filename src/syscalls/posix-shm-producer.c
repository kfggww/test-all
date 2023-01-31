#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <semaphore.h>

#define SHM_NAME "/posix_shm.demo"
#define SEM_NAME "/posix_sem.demo"

#define handle_error(pred, msg)                                                \
    do {                                                                       \
        if (pred) {                                                            \
            perror("posix-shm-producer failed" #msg ":");                      \
            exit(errno);                                                       \
        }                                                                      \
    } while (0)

int main(int argc, char **argv) {

    // 1. create posix shm
    int shmfd = shm_open(SHM_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    handle_error(shmfd < 0, shm_open);

    int *buf = (int *)mmap(NULL, 4096, PROT_WRITE, MAP_SHARED, shmfd, 0);
    handle_error(buf == MAP_FAILED, mmap);

    // 2. create posix sem
    sem_t *buf_ready = sem_open(SEM_NAME, O_CREAT | O_RDWR, 0);
    handle_error(buf_ready == SEM_FAILED, sem_open);

    // 3. write data to shm
    for (int i = 0; i <= 10; i++) {
        buf[i] = i < 10 ? i * i : -1;
    }

    // 4. notify consumer
    int err = sem_post(buf_ready);
    handle_error(err == -1, sem_post);

    // 5. clean up
    close(shmfd);
    munmap(buf, 4096);
    sem_close(buf_ready);

    return 0;
}