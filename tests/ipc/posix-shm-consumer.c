#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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
            perror("posix-shm-producer failed " #msg);                         \
            exit(errno);                                                       \
        }                                                                      \
    } while (0)

int main(int argc, char **argv) {

    // 1. open shared memory
    int shmfd = shm_open(SHM_NAME, O_RDONLY, 0);
    handle_error(shmfd < 0, shm_open);

    int *buf = (int *)mmap(NULL, 4096, PROT_READ, MAP_SHARED, shmfd, 0);
    handle_error(buf == MAP_FAILED, mmap);

    // 2. open semaphore
    sem_t *buf_ready = sem_open(SEM_NAME, O_RDWR);
    handle_error(buf_ready == SEM_FAILED, sem_open);

    // 3. wait shared memory ready
    int err = sem_wait(buf_ready);
    handle_error(err == -1, sem_wait);

    // 4. read data from shared memory
    int i = 0;
    while (buf[i] != -1) {
        printf("posix-shm-consumer read: %d\n", buf[i]);
        i++;
    }

    // 5. clean up
    close(shmfd);
    munmap(buf, 4096);
    sem_close(buf_ready);

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    printf("posix-shm-consumer exit\n");
    return 0;
}