#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

int setup_serial(int *fd);
int send_message(int fd, char *phone_number, char * text);