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

    // 创建信号量, 以及共享内存
    sem_t *sem = sem_open(SEM_NAME_USED, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED)
    {
        printf("Failed to create semaphore!\n");
        return -1;
    }

    void *addr = create_shm();

    // 向共享内存写入数据
    sem_wait(sem);
    if (addr != MAP_FAILED)
    {
        memcpy(addr, argv[1], strlen(argv[1]));
    }
    sem_post(sem);

    // 等待另一个进程读取完毕
    while (1)
    {
        sem_wait(sem);
        if (strcmp((char *)addr, "done") == 0)
        {
            sem_post(sem);
            break;
        }
        sem_post(sem);
    }

    // 销毁共享内存和信号量
    shm_unlink(SHM_NAME_USED);
    sem_unlink(SEM_NAME_USED);

    return 0;
}