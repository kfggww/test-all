#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "cunit.h"

TEST_CASE(IPC_pipe, simple_pipe) {
    int pfds[2];
    pipe2(pfds, O_NONBLOCK);

    char buf[128];
    int err = read(pfds[0], buf, 128);
    if (err < 0) {
        perror("error when read from pipe");
        printf("errno: %d\n", errno);
    } else if (err == 0) {
        printf("nothing in pipe\n");
    } else {
        printf("read data from pipe：%s\n", buf);
    }

err:
    close(pfds[0]);
    close(pfds[1]);
}

TEST_SUIT_BEGIN(IPC_pipe)
ADD_TEST(IPC_pipe, simple_pipe)
TEST_SUIT_END

int main(int argc, char **argv) { RUN_ALL(IPC_pipe); }