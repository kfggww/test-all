#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cutest.h"

CUTEST_SUIT(basic_fifo)

void get_fifo_name(char *fifo_name, int sz) {
    const char *prefix = "/tmp/basic_fifo";
    memset(fifo_name, 0, sz);
    sprintf(fifo_name, "%s.%d", prefix, getpid());
}

int create_fifo(char *fifo_name) {
    struct stat stat_buf;
    int err = stat(fifo_name, &stat_buf);

    // create fifo if not exists.
    if (err < 0) {
        printf("create_fifo: %s\n", fifo_name);
        err = mkfifo(fifo_name, S_IRUSR | S_IWUSR);
    }

    return err;
}

CUTEST_CASE(basic_fifo, create_fifo) {
    // get fifo name.
    char fifo_name[64];
    get_fifo_name(fifo_name, sizeof(fifo_name));

    // create fifo, open it in read and wirte mode seperatelly.
    create_fifo(fifo_name);
    int fifo_rfd = open(fifo_name, O_RDONLY | O_NONBLOCK);
    int fifo_wfd = open(fifo_name, O_WRONLY | O_NONBLOCK);

    // write data to fifo.
    char buf[] = "hello from fifo";
    write(fifo_wfd, buf, sizeof(buf));

    // read data from fifo.
    char buf_rcv[64];
    memset(buf_rcv, 0, sizeof(buf_rcv));
    read(fifo_rfd, buf_rcv, sizeof(buf_rcv));

    // close fifo.
    printf("buf_rcv: %s\n", buf_rcv);
    close(fifo_rfd);
    close(fifo_wfd);

    // delete fifo.
    int err = unlink(fifo_name);
    CUT_EXPECT_EQ(err, 0);
    printf("delete fifo: %s\n", fifo_name);
}