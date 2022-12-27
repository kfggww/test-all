#include "ipc_socket.h"

int main(int argc, char **argv) {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    if (inet_aton("127.0.0.1", &server_addr.sin_addr) != 1) {
        perror("Failed to convert ip to addr: ");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("Failed when connect: ");
        return -1;
    }

    char buf[32];
    memset(buf, 0, 32);
    read(sockfd, buf, 32);
    printf("Data recieved: %s\n", buf);
    close(sockfd);
    printf("client exit\n");

    return 0;
}