#include "posix-ipc.h"

static struct connection connection;

int build_connection() {

    // create and open 2 message queues
    memset(&connection, 0, sizeof(connection));

    pid_t pid = getpid();
    sprintf(connection.mqreq, "/client-mqreq.%d", pid);
    sprintf(connection.mqrsp, "/client-mqrsp.%d", pid);

    struct mq_attr attr = {.mq_maxmsg = 10,
                           .mq_msgsize = sizeof(struct msgbuf)};
    connection.mqreq_fd =
        mq_open(connection.mqreq, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    connection.mqrsp_fd =
        mq_open(connection.mqrsp, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

    if (connection.mqreq_fd == -1 || connection.mqrsp_fd == -1) {
        log_info("client %d failed mq_open\n", pid);
        return -1;
    }

    // open and map shared memory
    int fd = shm_open(CONNECTION_SHM, O_RDWR, 0);
    void *conn_buf = mmap(NULL, CONNECTION_SHM_SIZE, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0);

    // write connection to conn_buf, notify server new conncection is comming
    sem_t *conn_buf_ready = sem_open(CONNECTION_BUF_SEM, O_RDWR);
    sem_t *conn_new_ready = sem_open(CONNECTION_NEW_SEM, O_RDWR);

    if (conn_buf_ready == SEM_FAILED || conn_new_ready == SEM_FAILED) {
        log_warning("client %d failed sem_open\n");
        return -1;
    }

    sem_wait(conn_buf_ready);
    connection.valid = 1;
    memcpy(conn_buf, &connection, sizeof(connection));
    sem_post(conn_new_ready);

    sem_close(conn_buf_ready);
    sem_close(conn_new_ready);

    log_info("client %d build connection\n", pid);
    return 0;
}

void request_add() {
    int a = 0, b = 0;

    log_info("Enter a and b:\n");
    scanf("%d%d", &a, &b);

    struct msgbuf msgreq = {
        .type = REQ_ADD, .data.request_add.a = a, .data.request_add.b = b};
    int err =
        mq_send(connection.mqreq_fd, (const char *)&msgreq, sizeof(msgreq), 0);
    if (err) {
        log_warning("client failed mq_send in request_add\n");
        return;
    }

    struct msgbuf msgrsp;
    ssize_t sz =
        mq_receive(connection.mqrsp_fd, (char *)&msgrsp, sizeof(msgrsp), NULL);
    if (sz != sizeof(msgrsp)) {
        log_warning("client failed mq_receive in request_add, sz: %ld\n", sz);
        perror("client request_add mq_receive");
        return;
    }

    log_info("client request_add: %d + %d = %d\n", msgreq.data.request_add.a,
             msgreq.data.request_add.b, msgrsp.data.response_add.c);
}

void request_stop_server() {
    struct msgbuf msgreq = {.type = REQ_STOP_SERVER,
                            .data.request_stop_server.stop_server = 1};
    mq_send(connection.mqreq_fd, (char *)&msgreq, sizeof(msgreq), 0);
    log_info("client request_stop_server\n");
}

void request_disconnect() {
    struct msgbuf msgreq = {.type = REQ_DISCONNECT,
                            .data.request_disconnect.disconect = 1};
    mq_send(connection.mqreq_fd, (char *)&msgreq, sizeof(msgreq), 0);
    log_info("client request_disconnect\n");
}

void handle_command() {
    char cmd[16] = {0};
    while (cmd[0] != 'q') {
        log_info("Enter cmd: (a)dd, (d)isconnect, (k)ill server\n");
        scanf("%15s", cmd);
        switch (cmd[0]) {
        case 'a':
            request_add();
            break;
        case 'k':
            request_stop_server();
            cmd[0] = 'q';
            break;
        case 'd':
        default:
            request_disconnect();
            cmd[0] = 'q';
            break;
        }
    }

    log_info("client handle_command done\n");
}

void cleanup() {
    mq_close(connection.mqreq_fd);
    mq_close(connection.mqrsp_fd);

    mq_unlink(connection.mqreq);
    mq_unlink(connection.mqrsp);
}

int main(int argc, char **argv) {

    if (build_connection()) {
        log_info("client failed build_connection\n");
        return -1;
    }
    handle_command();
    cleanup();

    log_info("client %d exit\n", getpid());
    return 0;
}