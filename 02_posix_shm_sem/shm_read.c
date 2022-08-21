#include "headers.h"

void *create_shm()
{
    int fd = shm_open(SHM_NAME_USED, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        return MAP_FAILED;
    }

    ftruncate(fd, SHM_SIZE_USED);

    void *addr = mmap(NULL, SHM_SIZE_USED, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return addr;
}

int main(int argc, char **argv)
{

    sem_t *sem = sem_open(SEM_NAME_USED, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED)
    {
        printf("Failed to create semaphore!\n");
        return -1;
    }

    void *addr = create_shm();

    sem_wait(sem);
    printf("read shm: %s\n", (char *)addr);
    memcpy(addr, "done\0", 5);
    sem_post(sem);

    return 0;
}