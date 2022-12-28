#include <assert.h>
#include <stdio.h>
#include <netdb.h>

int main(int argc, char **argv) {

    struct servent* servent = getservbyport(htonl(8889), NULL);
    assert(servent != NULL);

    return 0;
}