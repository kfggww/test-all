#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>

#include <stdio.h>
#include <string.h>

#define SEM_NAME_USED "/sem_temp"
#define SHM_NAME_USED "/shm_temp"
#define SHM_SIZE_USED 1024