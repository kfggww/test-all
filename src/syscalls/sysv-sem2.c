#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/sem.h>
#include <fcntl.h>

int main(int argc, char **argv) {

    key_t key = ftok("/etc/shadow", 0x123456);
    int semid = semget(key, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
    pid_t pid = getpid();

    printf("process [%d] is waiting on semaphore [%d]\n", pid, semid);
    struct sembuf sop = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
    semop(semid, &sop, 1);
    printf("process [%d] finished waitting on semaphore [%d]\n", pid, semid);

    sleep(1);
    printf("process [%d] has done its work\n", pid);

    semctl(semid, 0, IPC_RMID);
    printf("process [%d] remove semaphore [%d]\n", pid, semid);
    printf("process [%d] exit\n", pid);
    return 0;
}