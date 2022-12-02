#include <stdio.h>

#include "serial_util.h"


int init(int *fd)
{
    int rc = 0;
    openlog("trm240_sms_api", LOG_PID, LOG_USER);
    rc = setup_serial(fd);
    if( rc ){
        syslog(LOG_ERR, "Failed to set up serial connection: %s", 
                strerror(errno));
        return rc;
    }
    
    return rc;
}

void cleanup(int fd)
{
    close(fd);
    closelog();
}

int main()
{
    int fd;
    int rc = 0;
    rc = init(&fd);
    if( rc ){
        return 1;
    }
    syslog(LOG_INFO, "trm240_sms_api has been started");
    //send_message(fd, "+37064745286", "cia yra tekstas");
    // size_t bytes = send_message(fd, "+37064745286", "cia yra tekstas");
    // if(bytes < 1){
    //     syslog(LOG_ERR, "Failed to send a message");
    // }
    syslog(LOG_INFO, "trm240_sms_api is shutting down");
    cleanup(fd);
    return 0;
}