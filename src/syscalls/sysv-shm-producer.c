#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>

void semaphore_post(int semid) {
    struct sembuf sop = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
    semop(semid, &sop, 1);
}

void semaphore_wait(int semid) {
    struct sembuf sop = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
    semop(semid, &sop, 1);
}

/**
 * Write data to shared memory.
 */
int main(int argc, char **argv) {

    if (argc <= 1) {
        return -1;
    }

    int nmax = atoi(argv[1]);

    key_t sem_read_key = ftok("/etc", 0x1234);
    key_t sem_write_key = ftok("/etc", 0x3456);
    key_t shm_key = ftok("/etc", 0x5678);

    int sem_read_id = semget(sem_read_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    int sem_write_id = semget(sem_write_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    int shm_id = shmget(shm_key, 4096, IPC_CREAT | S_IRUSR | S_IWUSR);

    pid_t pid = getpid();

    int *buf = (int *)shmat(shm_id, NULL, 0);

    for (int i = 0; i <= nmax; i++) {
        buf[i] = ((i == nmax) ? -1 : i);
        printf("process [%d] write data: %d\n", pid, buf[i]);
        semaphore_post(sem_write_id);
        semaphore_wait(sem_read_id);
    }

    shmdt(buf);
    printf("process [%d] finish writting\n", pid);
    return 0;
}