#include "posix-ipc.h"

static struct {
    /* shared memory related data, */
    int conn_buf_fd;
    struct connection *conn_buf;
    sem_t *conn_buf_ready;
    sem_t *conn_new_ready;

    /* pipe descriptors */
    int pipefd[2];
} ipc_server;

int handle_req_add(struct msgbuf *req_msg, struct connection *conn) {
    if (req_msg == NULL || conn == NULL)
        return -1;

    int err = 0;

    int c = req_msg->data.request_add.a + req_msg->data.request_add.b;
    struct msgbuf rsp_msg = {.type = RSP_ADD, .data.response_add.c = c};

    err = mq_send(conn->mqrsp_fd, (const char *)&rsp_msg, sizeof(struct msgbuf),
                  0);
    log_info("handle_req_add: a=%d, b=%d, c=%d\n", req_msg->data.request_add.a,
             req_msg->data.request_add.b, c);
    return err;
}

int handle_req_disconnect(struct msgbuf *req_msg, struct connection *conn) {
    // check if it is really a disconnect request
    if (req_msg == NULL || conn == NULL)
        return 0;

    if (req_msg->type != REQ_DISCONNECT ||
        req_msg->data.request_disconnect.disconect == 0)
        return 0;

    log_info("handle_req_disconnect\n");
    return -1;
}

int handle_req_kill_server(struct msgbuf *req_msg, struct connection *conn) {
    // check if it is really a stop server request
    if (req_msg == NULL || conn == NULL)
        return 0;

    if (req_msg->type != REQ_STOP_SERVER ||
        req_msg->data.request_kill_server.kill_server == 0)
        return 0;

    int kill_server = req_msg->data.request_kill_server.kill_server;
    write(ipc_server.pipefd[1], &kill_server, sizeof(int));
    sem_post(ipc_server.conn_new_ready);

    log_info("handle_req_kill_server\n");
    return -1;
}

int handle_request(struct msgbuf *req_msg, struct connection *conn) {
    if (req_msg == NULL)
        return -1;

    int err = 0;

    switch (req_msg->type) {
    case REQ_ADD:
        err = handle_req_add(req_msg, conn);
        break;
    case REQ_DISCONNECT:
        err = handle_req_disconnect(req_msg, conn);
        break;
    case REQ_STOP_SERVER:
        err = handle_req_kill_server(req_msg, conn);
        break;
    default:
        break;
    }

    return err;
}

void handle_connection(struct connection *conn) {
    if (fork() != 0)
        return;

    close(ipc_server.pipefd[0]);

    conn->mqreq_fd = mq_open(conn->mqreq, O_RDONLY);
    conn->mqrsp_fd = mq_open(conn->mqrsp, O_WRONLY);

    struct msgbuf req_msg;
    struct msgbuf rsp_msg;

    int connected = 1;
    while (connected) {
        ssize_t sz =
            mq_receive(conn->mqreq_fd, (char *)&req_msg, sizeof(req_msg), NULL);
        if (sz <= 0) {
            log_warning("request is NOT valid\n");
            break;
        }

        int err = handle_request(&req_msg, conn);
        if (err)
            connected = 0;
    }

    mq_close(conn->mqreq_fd);
    mq_close(conn->mqrsp_fd);
    close(ipc_server.pipefd[1]);

    log_info("handle_connection process [%d] exit\n", getpid());
    exit(0);
}

int server_init() {
    memset(&ipc_server, 0, sizeof(ipc_server));

    // shared memory init
    ipc_server.conn_buf_fd =
        shm_open(CONNECTION_SHM, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (ipc_server.conn_buf_fd < 0) {
        log_warning("server failed shm_open\n");
        return -1;
    }

    if (ftruncate(ipc_server.conn_buf_fd, CONNECTION_SHM_SIZE) < 0) {
        log_warning("server failed ftruncate\n");
        return -1;
    }

    ipc_server.conn_buf = (struct connection *)mmap(
        NULL, CONNECTION_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
        ipc_server.conn_buf_fd, 0);
    if (ipc_server.conn_buf == MAP_FAILED) {
        log_warning("server failed mmap\n");
        return -1;
    }

    memset(ipc_server.conn_buf, 0, CONNECTION_SHM_SIZE);

    ipc_server.conn_buf_ready =
        sem_open(CONNECTION_BUF_SEM, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (ipc_server.conn_buf_ready == SEM_FAILED) {
        log_warning("server failed sem_open conn_buf_ready\n");
        return -1;
    }

    ipc_server.conn_new_ready =
        sem_open(CONNECTION_NEW_SEM, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (ipc_server.conn_new_ready == SEM_FAILED) {
        log_warning("server failed sem_open conn_new_ready\n");
        return -1;
    }

    // pipe init
    int pipefd[2];
    if (pipe2(ipc_server.pipefd, O_NONBLOCK)) {
        log_warning("server failed pipe2\n");
        return -1;
    }

    log_info("server init done\n");
    return 0;
}

void server_start() {
    int err = sem_post(ipc_server.conn_buf_ready);
    if (err) {
        log_warning("server_start failed\n");
        return;
    }

    log_info("server running...\n");

    struct connection conn;
    int stop = 0;
    while (!stop) {
        // handle new connection
        sem_wait(ipc_server.conn_new_ready);
        if (read(ipc_server.pipefd[0], &stop, sizeof(int)) <= 0)
            stop = 0;

        if (ipc_server.conn_buf->valid) {
            log_info("new connection established\n");
            memcpy(&conn, ipc_server.conn_buf, sizeof(conn));
            handle_connection(&conn);
            memset(ipc_server.conn_buf, 0, sizeof(struct connection));
            sem_post(ipc_server.conn_buf_ready);
        }
    }
}

void server_shutdown() {
    close(ipc_server.conn_buf_fd);
    munmap(ipc_server.conn_buf, CONNECTION_SHM_SIZE);
    shm_unlink(CONNECTION_SHM);

    sem_close(ipc_server.conn_buf_ready);
    sem_unlink(CONNECTION_BUF_SEM);

    sem_close(ipc_server.conn_new_ready);
    sem_unlink(CONNECTION_NEW_SEM);

    close(ipc_server.pipefd[0]);
    close(ipc_server.pipefd[1]);

    log_info("server_shutdown\n");
}

int main(int argc, char **argv) {

    int err = server_init();
    if (err) {
        log_warning("server_init failed\n");
        return -1;
    }
    server_start();
    server_shutdown();

    return 0;
}