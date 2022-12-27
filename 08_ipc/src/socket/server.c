#include "ipc_socket.h"

int main(int argc, char **argv) {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(12345);
    if (inet_aton("127.0.0.1", &sockaddr.sin_addr) != 1) {
        perror("Failed to convert ip to addr: ");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) {
        perror("Failed when bind: ");
        return -1;
    }

    if (listen(sockfd, 10)) {
        perror("Failed when listen: ");
        return -1;
    }

    printf("server start...\n");

    int n = 10;
    while (n) {
        int fd = accept(sockfd, NULL, NULL);
        if (fd == -1) {
            perror("Failed when accept: ");
            continue;
        }
        printf("Get new connection\n");
        write(fd, "hello from server", 17);
        close(fd);
        n--;
    }

    close(sockfd);
    printf("server shutdown\n");
    return 0;
}