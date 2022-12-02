#include "serial_util.h"

#define PORTNAME "/dev/ttyUSB2"
#define SPEED B115200
#define CMGF "AT+CMGF=1\r"
#define CSCS "AT+CSCS=\"UCS2\"\r"
#define CSMP "AT+CSMP=17,167,0,8\r"

static int configure_serial(int *fd){
    int rc = 0;
    struct termios tty;
    rc = tcgetattr(*fd, &tty);
    if( rc ){
        syslog(LOG_ERR, "Failed to get termios struct: %s", strerror(errno));
        return rc;
    }
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; 
    tty.c_lflag &= ~ECHOE; 
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;
    cfsetispeed(&tty, SPEED);
    cfsetospeed(&tty, SPEED);
    rc = tcsetattr(*fd, TCSANOW, &tty);
    if( rc ){
        syslog(LOG_ERR, "Failed to set serial settings: %s", strerror(rc));
        return rc;
    }
    return rc;
}

int setup_serial(int *fd)
{
    int rc = 0;
    struct termios tty;

    *fd = open(PORTNAME, O_RDWR | O_NOCTTY | O_SYNC);
    if( *fd < 0 ){
        syslog(LOG_ERR, "Failed to open serial: %s", strerror(errno));
        return 1;
    }
    rc = configure_serial(fd);
    if( rc ){
        syslog(LOG_ERR, "Failed to set serial flags: %s", strerror(errno));
        return rc;
    }
    return rc;
}

int send_message(int fd, char *phone_number, char *text)
{
    char msg[30];
    size_t bytes_sent = 0;
    bytes_sent += write(fd, CMGF, sizeof(CMGF));
    bytes_sent += write(fd, CSCS, sizeof(CSCS));
    bytes_sent += write(fd, CSMP, sizeof(CSMP));
    snprintf(msg, 30, (char *)"AT+CMGS=\"%s\"\r", phone_number); 
    bytes_sent += write(fd, msg, sizeof(msg)-1);
    bytes_sent += write(fd, text, sizeof(text)-1);
    bytes_sent += write(fd, "\x1a", sizeof("\x1a"));
    return bytes_sent;
}