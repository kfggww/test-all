#ifndef LOGKEYS_H
#define LOGKEYS_H

#define DEVICE_PATH "/proc/bus/input/devices"
#define EVENT_PATH_PREFIX "/dev/input"

int open_kbd_event_file();
void logkeys(int kbd_ev_fd, char *logfname);

#endif