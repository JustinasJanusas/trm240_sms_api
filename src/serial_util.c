#include "serial_util.h"

#define PORTNAME "/dev/ttyUSB2"
#define SPEED B115200
#define CMGF "AT+CMGF=1\r"
#define UCS2 "AT+CSCS=\"UCS2\"\r"
#define GSM "AT+CSCS=\"GSM\"\r"
#define CSMP_UCS2 "AT+CSMP=17,167,0,8\r"
#define CSMP_GSM "AT+CSMP=17,167,0,0\r"
#define READ_ALL "AT+CMGL=4\r"
#define PDU "AT+CMGF=0\r"
#define SLEEP 1
#define BUFF_SIZE 10000


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
    tty.c_cflag &= ~CSIZE;
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
    tty.c_cc[VTIME] = 30;
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

int send_message_UCS2(int fd, char *phone_number, char *text)
{
    char msg[100];
    size_t bytes_sent = 0;
    bytes_sent += write(fd, CMGF, strlen(CMGF));
    sleep(SLEEP);
    bytes_sent += write(fd, UCS2, strlen(UCS2));
    sleep(SLEEP);
    bytes_sent += write(fd, CSMP_UCS2, strlen(CSMP_UCS2));
    sleep(SLEEP);
    snprintf(msg, 99, (char *)"AT+CMGS=\"%s\"\r", phone_number); 
    bytes_sent += write(fd, msg, strlen(msg));
    sleep(SLEEP);
    bytes_sent += write(fd, text, strlen(text));
    sleep(SLEEP);
    bytes_sent += write(fd, "\x1a", strlen("\x1a"));
    return bytes_sent;
}

int send_message_GSM(int fd, char *phone_number, char *text)
{
    char msg[30];
    size_t bytes_sent = 0;
    bytes_sent += write(fd, CMGF, strlen(CMGF));
    sleep(SLEEP);
    bytes_sent += write(fd, GSM, strlen(GSM));
    sleep(SLEEP);
    bytes_sent += write(fd, CSMP_GSM, strlen(CSMP_GSM));
    sleep(SLEEP);
    snprintf(msg, 30, (char *)"AT+CMGS=\"%s\"\r", phone_number); 
    bytes_sent += write(fd, msg, strlen(msg));
    sleep(SLEEP);
    bytes_sent += write(fd, text, strlen(text));
    sleep(SLEEP);
    bytes_sent += write(fd, "\x1a", strlen("\x1a"));
    return bytes_sent;
}

int send_message_PDU(int fd, char *text)
{
    char msg[30];
    size_t bytes_sent = 0;
    bytes_sent += write(fd, PDU, strlen(PDU));
    sleep(SLEEP);
    bytes_sent += write(fd, GSM, strlen(GSM));
    sleep(SLEEP);
    bytes_sent += write(fd, CSMP_GSM, strlen(CSMP_GSM));
    sleep(SLEEP);
    snprintf(msg, 30, (char *)"AT+CMGS=%d\r", (strlen(text)-2)/2); 
    printf("msg: %s\n", msg);
    bytes_sent += write(fd, msg, strlen(msg));
    sleep(SLEEP);
    bytes_sent += write(fd, text, strlen(text));
    sleep(SLEEP);
    bytes_sent += write(fd, "\x1a", strlen("\x1a"));
    return bytes_sent;
}

void read_all_messages(int fd, char *json)
{
    write(fd, PDU, strlen(PDU));
    sleep(SLEEP);
    write(fd, GSM, strlen(GSM));
    sleep(SLEEP);
    write(fd, READ_ALL, strlen(READ_ALL));
    char buff[BUFF_SIZE];
    int read_next = 1;
    int start_found = 0;
    char string_end[END_STRING_SIZE];
    sleep(SLEEP);
    while( read_next ){
        memset(buff, 0, sizeof(BUFF_SIZE));
        strcat(buff, string_end);
        read(fd, &(buff[strlen(string_end)]), BUFF_SIZE-strlen(string_end));
        read_next = parse_messages(buff, json, &start_found, string_end);
    }
}

// void read_all_messages(int fd, char json[])
// {
//     //write(fd, "AT\r", strlen("AT\r"));
//     printf("why?");
//     write(fd, PDU, strlen(PDU));
//     sleep(SLEEP);
//     write(fd, GSM, strlen(GSM));
//     sleep(SLEEP);
//     write(fd, READ_ALL, strlen(READ_ALL));
//     char buff[BUFF_SIZE];
//     int read_next = 1;
//     int start_found = 0;
//     char end_string[END_STRING_SIZE];
//     strcat(json, "[");
//     sleep(SLEEP);
//     while( read_next ){
//         memset(buff, 0, sizeof(BUFF_SIZE));
//         strcat(buff, end_string);
//         read(fd, &(buff[strlen(end_string)]), BUFF_SIZE-strlen(end_string));
//         read_next = parse_messages(buff, json, &start_found, end_string);
//         //read_next =0;
//     }
//     strncat(json, "]", MAX_MESSAGES_SIZE-1-strlen(json));
//     //printf("json: %s\n", json);
// }

// char *read_unread_messages(int fd)
// {

// }