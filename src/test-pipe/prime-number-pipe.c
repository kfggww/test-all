#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include "cutest.h"

void generate_primes(int pipe1[2]) {
    close(pipe1[1]);

    int prime = 0;
    int err = read(pipe1[0], &prime, sizeof(int));
    if (err <= 0) {
        close(pipe1[0]);
        return;
    }
    printf("%d\n", prime);

    int pipe2[2];
    pipe(pipe2);

    pid_t pid = fork();
    if (pid == 0) {
        generate_primes(pipe2);
    } else {
        int num = 0;
        while ((err = read(pipe1[0], &num, sizeof(int))) > 0) {
            if (num % prime) {
                write(pipe2[1], &num, sizeof(int));
            }
        }
    }

    close(pipe1[0]);
    close(pipe2[0]);
    close(pipe2[1]);
    exit(0);
}

CUTEST_SUIT(prime_numbers_pipe)

CUTEST_CASE(prime_numbers_pipe, prime_number_max30) {
    int nmax = 30;

    int pipe1[2];
    pipe(pipe1);

    for (int i = 2; i <= nmax; ++i)
        write(pipe1[1], &i, sizeof(int));

    if (fork() == 0) {
        generate_primes(pipe1);
    }

    close(pipe1[0]);
    close(pipe1[1]);
}