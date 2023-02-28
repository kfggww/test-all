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
 * Read data from shared memory.
 */
int main(int argc, char **argv) {

    key_t sem_read_key = ftok("/etc", 0x1234);
    key_t sem_write_key = ftok("/etc", 0x3456);
    key_t shm_key = ftok("/etc", 0x5678);

    int sem_read_id = semget(sem_read_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    int sem_write_id = semget(sem_write_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    int shm_id = shmget(shm_key, 4096, IPC_CREAT | S_IRUSR | S_IWUSR);

    pid_t pid = getpid();

    int *buf = (int *)shmat(shm_id, NULL, SHM_RDONLY);

    int data = 0;
    int i = 0;

    while (data != -1) {
        semaphore_wait(sem_write_id);
        data = buf[i++];
        semaphore_post(sem_read_id);
        printf("process [%d] read data: %d\n", pid, data);
    }

    shmdt(buf);
    printf("process [%d] finish reading\n", pid);

    shmctl(shm_id, IPC_RMID, NULL);
    printf("process [%d] remove shm [%d]\n", pid, shm_id);
    return 0;
}