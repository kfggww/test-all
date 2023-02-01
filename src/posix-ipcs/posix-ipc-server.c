#include "posix-ipc.h"

static struct {
    /* shared memory related data, */
    int conn_buf_fd;
    struct connection *conn_buf;
    sem_t *conn_buf_mutex;
    // sem_t *new_conn_sem;

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
    log_info("handler_req_add: a=%d, b=%d, c=%d\n", req_msg->data.request_add.a,
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

int handle_req_stop_server(struct msgbuf *req_msg, struct connection *conn) {
    // check if it is really a stop server request
    if (req_msg == NULL || conn == NULL)
        return 0;

    if (req_msg->type != REQ_STOP_SERVER ||
        req_msg->data.request_stop_server.stop_server == 0)
        return 0;

    int stop_server = req_msg->data.request_stop_server.stop_server;
    write(ipc_server.pipefd[1], &stop_server, sizeof(int));

    log_info("handle_req_stop_server\n");
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
        err = handle_req_stop_server(req_msg, conn);
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

    log_info("handle_connection exit\n");
    exit(0);
}

int server_init() {
    memset(&ipc_server, 0, sizeof(ipc_server));

    // shared memory init
    ipc_server.conn_buf_fd =
        shm_open(CONNECTION_SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
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

    ipc_server.conn_buf_mutex =
        sem_open(CONNECTION_SEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (ipc_server.conn_buf_mutex == SEM_FAILED) {
        log_warning("server failed sem_open\n");
        return -1;
    }

    // ipc_server.new_conn_sem = sem_open(NEW_CONNECTION_SEM_NAME,
    //                                    O_CREAT | O_RDWR, S_IRUSR | S_IWUSR,
    //                                    0);
    // if (ipc_server.new_conn_sem == SEM_FAILED) {
    //     log_warning("server failed sem_open new_conn_sem\n");
    //     return -1;
    // }

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
    int err = sem_post(ipc_server.conn_buf_mutex);
    if (err) {
        log_warning("server_start failed\n");
        return;
    }

    log_info("server running...\n");

    struct connection conn;
    int running = 1;
    while (running) {
        // handle new connection
        // sem_wait(ipc_server.new_conn_sem);
        sem_wait(ipc_server.conn_buf_mutex);
        if (ipc_server.conn_buf->valid) {
            log_info("new connection established\n");
            memcpy(&conn, ipc_server.conn_buf, sizeof(conn));
            handle_connection(&conn);
            memset(ipc_server.conn_buf, 0, sizeof(struct connection));
        }
        sem_post(ipc_server.conn_buf_mutex);

        // check pipe if it's necessary to stop server
        if (read(ipc_server.pipefd[0], &running, sizeof(int)) <= 0)
            running = 1;
        else
            running = 0;
    }
}

void server_shutdown() {
    close(ipc_server.conn_buf_fd);
    munmap(ipc_server.conn_buf, CONNECTION_SHM_SIZE);
    shm_unlink(CONNECTION_SHM_NAME);

    sem_close(ipc_server.conn_buf_mutex);
    sem_unlink(CONNECTION_SEM_NAME);

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