

## IPC methods

1. pipe
2. fifo
3. message queue
4. semaphore
5. shared memory

## System V IPC and POSIX IPC

POSIX IPC接口规范的定义借鉴了System V IPC的优点，摒弃了System V IPC的不足。因此POSIX IPC在使用上更加方便，但由于System V IPC出现较早，得到了很多系统的支持，因此在可移植性方面System V IPC更占优势，不过随着时间推移，这种优势也会越来越小。