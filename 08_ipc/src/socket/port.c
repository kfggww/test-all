#include <assert.h>
#include <stdio.h>
#include <netdb.h>

int main(int argc, char **argv) {

    int n = 10;
    while (n--) {
        struct servent *servent = getservent();
        printf("name=%s, port=%d, proto=%s\n", servent->s_name,
               htons(servent->s_port), servent->s_proto);
    }

    struct servent *servent = getservbyport(htons(22), NULL);
    printf("name=%s, port=%d, proto=%s\n", servent->s_name,
           htons(servent->s_port), servent->s_proto);

    return 0;
}