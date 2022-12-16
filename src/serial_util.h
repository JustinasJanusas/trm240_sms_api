#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <termios.h>
#include <errno.h>

#include "string_util.h"

int setup_serial(int *fd);
int send_message_UCS2(int fd, char *phone_number, char *text);
int send_message_GSM(int fd, char *phone_number, char *text);
void read_message_list(int fd, char *json, int read_type);
int send_message_PDU(int fd, char *text);