#include <stdio.h>
#include <stdlib.h>

#include <sys/user.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#define NBREAKPOINTS 16

struct breakpoint_ops;

/*breakpoint*/
typedef struct {
    void *addr;
    long original_data;
    int enabled;
    struct breakpoint_ops *ops;
} breakpoint_t;

/*breakpoint operations*/
typedef struct breakpoint_ops {
    void (*enable)(breakpoint_t *bp, pid_t tracee);
    void (*disable)(breakpoint_t *bp, pid_t tracee);
} breakpoint_ops_t;

void breakpoint_enable(breakpoint_t *bp, pid_t tracee) {
    void *addr = bp->addr;
    bp->original_data = ptrace(PTRACE_PEEKDATA, tracee, addr, NULL);
    if (bp->original_data == -1) {
        perror("debugger fail to enable breakpoint");
        exit(1);
    }

    long new_data = (bp->original_data & ~0xff) | 0xcc;
    if (ptrace(PTRACE_POKEDATA, tracee, addr, new_data) == -1) {
        perror("debugger fail to enable breakpoint");
        exit(1);
    }
    bp->enabled = 1;
}

void breakpoint_disable(breakpoint_t *bp, pid_t tracee) {
    if (ptrace(PTRACE_POKEDATA, tracee, bp->addr, bp->original_data) == -1) {
        perror("debugger fail to disable breakpoint");
        exit(1);
    }
    bp->enabled = 0;
}

static breakpoint_ops_t default_breakpoint_ops = {
    .enable = breakpoint_enable,
    .disable = breakpoint_disable,
};

struct debugger_ops;

/*debuggger*/
typedef struct {
    const char *tracee_prog;
    pid_t tracee_pid;
    struct debugger_ops *ops;
    int quit;
    int bp_index;
    breakpoint_t bps[NBREAKPOINTS];
} debugger_t;

/*debugger operations*/
typedef struct debugger_ops {
    void (*new_breakpoint)(debugger_t *dbg, void *addr);
    void (*step)(debugger_t *dbg);
    void (*continue_run)(debugger_t *dbg);
    void (*start_tracee)(debugger_t *dbg);
    void (*wait_signal)(debugger_t *dbg);
    void (*run)(debugger_t *dbg);
    void (*get_command)(debugger_t *dbg);
} debugger_ops_t;

void debugger_new_breakpoint(debugger_t *dbg, void *addr) {
    breakpoint_t *bp = &dbg->bps[dbg->bp_index++];
    bp->addr = addr;
    bp->ops->enable(bp, dbg->tracee_pid);
}

void debugger_continue_run(debugger_t *dbg) {
    if (ptrace(PTRACE_CONT, dbg->tracee_pid, NULL, NULL) == -1) {
        perror("debugger fail to continue");
        exit(1);
    }
}

void debugger_wait_signal(debugger_t *dbg) {
    int status = 0;
    if (waitpid(dbg->tracee_pid, &status, 0) == -1) {
        perror("debugger fail waitpid");
        exit(1);
    }

    if (WIFEXITED(status)) {
        printf("(simple debugger) tracee exit\n");
        dbg->quit = 1;
        return;
    }

    if (!WIFSTOPPED(status)) {
        printf("tracee is not in stopped state\n");
        exit(1);
    }

    /*check breakpoint hits*/
    if (WSTOPSIG(status) == SIGTRAP) {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, dbg->tracee_pid, NULL, &regs) == -1) {
            perror("debugger fail to get user regs");
            exit(1);
        }

        void *hit_addr = (void *)regs.rip;
        breakpoint_t *bp = NULL;
        for (int i = 0; i < dbg->bp_index; i++) {
            bp = &dbg->bps[i];
            if (bp->addr == hit_addr - 1) {
                printf("(simple debugger) hit breakpoint at [%p]\n", bp->addr);
                bp->ops->disable(bp, dbg->tracee_pid);
                // reset tracee pc
                regs.rip -= 1;
                if (ptrace(PTRACE_SETREGS, dbg->tracee_pid, NULL, &regs) ==
                    -1) {
                    perror("debugger fail to reset pc");
                    exit(1);
                }
                break;
            }
        }
    }
}

void debugger_start_tracee(debugger_t *dbg) {
    pid_t tracee = fork();
    if (tracee == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        printf("start debugging [%s]...\n", dbg->tracee_prog);
        execl(dbg->tracee_prog, dbg->tracee_prog, NULL);
        printf("tracee should NEVER goes here\n");
        exit(1);
    }
    dbg->tracee_pid = tracee;
}

void debugger_run(debugger_t *dbg) {
    dbg->ops->start_tracee(dbg);
    while (!dbg->quit) {
        dbg->ops->wait_signal(dbg);
        dbg->ops->get_command(dbg);
    }
}

void debugger_get_command(debugger_t *dbg) {
    char cmd[16];
    char val[16];

    int endinput = 0;
    while (!endinput) {
        printf("(simple debugger) ");
        scanf("%s", cmd);
        printf("(simple debugger) ");

        switch (cmd[0]) {
        case 'b':
            scanf("%s", val);
            void *addr = (void *)strtol(val, NULL, 16);
            dbg->ops->new_breakpoint(dbg, addr);
            break;
        case 'q':
            endinput = 1;
            dbg->quit = 1;
            break;
        case 'c':
        default:
            dbg->ops->continue_run(dbg);
            endinput = 1;
            break;
        }
    }
}

static debugger_ops_t default_debugger_ops = {
    .new_breakpoint = debugger_new_breakpoint,
    .continue_run = debugger_continue_run,
    .start_tracee = debugger_start_tracee,
    .wait_signal = debugger_wait_signal,
    .step = NULL,
    .run = debugger_run,
    .get_command = debugger_get_command,
};

void init_breakpoint(breakpoint_t *bp) {
    bp->addr = NULL;
    bp->original_data = 0;
    bp->enabled = 0;
    bp->ops = &default_breakpoint_ops;
}

debugger_t *create_debugger(const char *prog) {
    debugger_t *debugger = malloc(sizeof(*debugger));
    debugger->quit = 0;
    debugger->bp_index = 0;
    debugger->tracee_prog = prog;
    debugger->ops = &default_debugger_ops;
    for (int i = 0; i < NBREAKPOINTS; i++) {
        breakpoint_t *bp = &debugger->bps[i];
        init_breakpoint(bp);
    }
    return debugger;
}

void destroy_debugger(debugger_t *dbg) { free(dbg); }

int main(int argc, char const *argv[]) {
    debugger_t *debugger = create_debugger(argv[1]);
    debugger->ops->run(debugger);
    destroy_debugger(debugger);
    return 0;
}
