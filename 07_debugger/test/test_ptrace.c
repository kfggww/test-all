#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

static int log_id = 0;

#define log(fmt, args...)                                                      \
    do {                                                                       \
        printf("[LOG-%d]: ", log_id++);                                        \
        printf(fmt, ##args);                                                   \
    } while (0)

#define exit_with_err(err, fmt, args...)                                       \
    do {                                                                       \
        log(fmt, ##args);                                                      \
        exit(err);                                                             \
    } while (0)

int main(int argc, char **argv) {

    if (argc < 2)
        exit_with_err(-1, "Usage: \n\tdebugger executable_name\n");

    char *tracee = argv[1];
    long long err = 0;
    log("start debugging [%s]\n", tracee);

    pid_t tracee_pid = fork();
    if (tracee_pid < 0) {
        exit_with_err(-2, "failed to fork child process\n");
    } else if (tracee_pid == 0) {
        err = ptrace(PTRACE_TRACEME, 0, 0, PTRACE_O_TRACEEXEC);
        if (err)
            exit_with_err(-3, "failed to create child with ptrace\n");
        log("creating child process with ptrace...\n");
        execv(argv[1], &argv[1]);
    }

    long long addr = 0x401b90;
    long long data = 1024;
    int wstatus = 0;
    siginfo_t info;
    int n = 10;

    while (n--) {
        pid_t wpid = waitpid(tracee_pid, &wstatus, 0);
        if (wpid != tracee_pid)
            exit_with_err(-4, "failed to waitpid on [%d]\n", tracee_pid);

        if (WIFSTOPPED(wstatus)) {
            log("tracee [%d] is caught\n", tracee_pid);
            err = ptrace(PTRACE_PEEKUSER, tracee_pid, 16 * 8, &data);
            log("RIP: %llx\n", err);
            err = ptrace(PTRACE_PEEKTEXT, tracee_pid, addr, NULL);
            log("data: %llx\n", err);
            log("signal no: %d\n", WSTOPSIG(wstatus));
            err = ptrace(PTRACE_GETSIGINFO, tracee_pid, 0, &info);
            log("getsiginfo return code: %llx\n", err);
            log("siginfo_t: %d, %d, %d\n", info.si_signo, info.si_code,
                info.si_errno);

            err = ptrace(PTRACE_SINGLESTEP, tracee_pid, NULL, NULL);
            if (err)
                exit_with_err(-5, "PTRACE_CONT failed\n");
        } else
            exit_with_err(-6, "tracee [%d] with wrong status, %d\n", tracee_pid,
                          wstatus);
    }

    log("stop debugging [%s]\n", tracee);
    return 0;
}