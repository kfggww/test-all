#include "posix-ipc.h"

void handle_connection(struct connect_server_request *cs_req) {
    if (fork() != 0)
        return;

    mqd_t mqc2s = mq_open(cs_req->mqc2s_name, O_RDONLY);
    mqd_t mqs2c = mq_open(cs_req->mqs2c_name, O_WRONLY);

    struct msgbuf req;
    struct msgbuf rsp;

    int connected = 1;
    while (connected) {
        ssize_t sz = mq_receive(mqc2s, (char *)&req, sizeof(req), NULL);
        switch (req.type) {
        case REQ_ADD:
            log_info("req_add\n");
            break;
        case REQ_DISCONNECT:
            log_info("req_disconnect\n");
            mq_close(mqc2s);
            mq_close(mqs2c);
            mq_unlink(cs_req->mqc2s_name);
            mq_unlink(cs_req->mqs2c_name);
            connected = 0;
            break;
        case REQ_STOP_SERVER:
            log_info("req_stop_server\n");
            connected = 0;
            break;
        default:
            log_warning("unknown message type\n");
            connected = 0;
            break;
        }
    }

    mq_close(mqc2s);
    mq_close(mqs2c);
    log_info("handle_connection exit\n");
    exit(0);
}

int main(int argc, char **argv) {

    /**
     * 1. 共享内存
     * 创建并打开共享内存，获得fd
     * map共享内存
     * 创建共享内存信号量
     * 初始化共享内存
     * 通知共享内存可用
     */

    sem_t *cs_req_mutex = sem_open(
        POSIX_IPC_SEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 0);

    int shmfd = shm_open(POSIX_IPC_SHM_NAME, O_CREAT | O_EXCL | O_RDWR,
                         S_IRUSR | S_IWUSR);
    ftruncate(shmfd, CS_REQ_SHM_SIZE);

    void *shm_buf = mmap(NULL, CS_REQ_SHM_SIZE, PROT_READ | PROT_WRITE,
                         MAP_SHARED, shmfd, 0);
    memset(shm_buf, 0, CS_REQ_SHM_SIZE);
    struct connect_server_request *cs_req =
        (struct connect_server_request *)shm_buf;

    log_info("server start\n");
    sem_post(cs_req_mutex);

    /**
     * 2. 检查共享内存
     * 等待共享内存可用
     * 读取共享内存，若有请求到来，得到消息队列，fork新进程去处理请求
     * 释放共享内存
     */

    struct connect_server_request cs_req_clone;
    for (;;) {
        sem_wait(cs_req_mutex);
        if (cs_req->valid) {
            log_info("new connection, mqc2s: %s, mqs2c: %s\n",
                     cs_req->mqc2s_name, cs_req->mqs2c_name);
            memcpy(&cs_req_clone, cs_req, sizeof(cs_req_clone));
            handle_connection(&cs_req_clone);
            memset(cs_req, 0, sizeof(*cs_req));
        }
        sem_post(cs_req_mutex);
    }

    /**
     * 3. 清理资源
     * 关闭共享内存
     * unmap共享内存
     * 删除共享内存
     */

    close(shmfd);
    munmap(shm_buf, CS_REQ_SHM_SIZE);
    shm_unlink(POSIX_IPC_SHM_NAME);

    sem_close(cs_req_mutex);
    sem_unlink(POSIX_IPC_SEM_NAME);

    log_info("server exit\n");
    return 0;
}