#include "posix-ipc.h"

int main(int argc, char **argv) {

    /**
     * 1. 创建用于和server通信的两个消息队列
     * 获取pid
     * 根据pid拼接两个消息队列的名字
     * 创建两个消息队列
     */

    pid_t pid = getpid();

    char mqc2s_name[MQ_NAME_SIZE];
    char mqs2c_name[MQ_NAME_SIZE];
    memset(mqc2s_name, 0, MQ_NAME_SIZE);
    memset(mqs2c_name, 0, MQ_NAME_SIZE);
    sprintf(mqc2s_name, "/posix-ipc-mqc2s_name.%d", pid);
    sprintf(mqs2c_name, "/posix-ipc-mqs2c_name.%d", pid);

    struct mq_attr attr = {.mq_maxmsg = 10,
                           .mq_msgsize = sizeof(struct msgbuf)};
    mqd_t mqc2s =
        mq_open(mqc2s_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    mqd_t mqs2c =
        mq_open(mqs2c_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

    /**
     * 2. 建立和server的连接
     * map共享内存到自己的地址空间
     * 获取公用的共享内存
     * 等待共享内存可用
     * 向共享内存写入数据，建立和server的连接，实际上是把两个消息队列的名字写到共享内存
     * 释放共享内存
     */
    int shmfd = shm_open(POSIX_IPC_SHM_NAME, O_RDWR, 0);
    struct connect_server_request *cs_req =
        (struct connect_server_request *)mmap(NULL, CS_REQ_SHM_SIZE,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED, shmfd, 0);

    sem_t *cs_req_mutex = sem_open(POSIX_IPC_SEM_NAME, O_RDWR);
    sem_wait(cs_req_mutex);
    cs_req->valid = 1;
    memcpy(cs_req->mqc2s_name, mqc2s_name, MQ_NAME_SIZE);
    memcpy(cs_req->mqs2c_name, mqs2c_name, MQ_NAME_SIZE);
    log_info("client [%d] request connection\n", pid);
    sem_post(cs_req_mutex);

    /**
     * 3. 处理用户命令
     * 获取命令
     * 获取参数
     * 写入请求消息队列
     * 读取响应消息队列
     */
    struct msgbuf msg;
    msg.type = REQ_DISCONNECT;
    msg.msg.request_disconnect.disconect = 1;
    mq_send(mqc2s, (char *)&msg, sizeof(msg), 0);

    /**
     * 4. 清理资源
     * 关闭消息队列
     * 删除消息队列
     * 关闭贡献内存
     * unmap共享内存
     */

    close(shmfd);
    munmap(cs_req, CS_REQ_SHM_SIZE);
    sem_close(cs_req_mutex);

    mq_close(mqc2s);
    mq_close(mqs2c);

    log_info("client [%d] exit\n", pid);
    return 0;
}