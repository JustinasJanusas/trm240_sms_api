#include <signal.h>
#include <zmq.h>

#include "serial_util.h"

#define RESPOND_BUFFER_SIZE 2000
#define BAD_JSON_MESSAGE "Wrong json format"

volatile sig_atomic_t daemonize = 1;

void* responder;

static void term_proc(int sigterm) 
{
    write(responder, " ", sizeof(" "));
	daemonize = 0;
}

int init(int *fd, void **context)
{
    int rc = 0;
    openlog("trm240_sms_api", LOG_PID, LOG_USER);
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term_proc;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);
    rc = setup_serial(fd);
    if( rc ){
        syslog(LOG_ERR, "Failed to set up serial connection: %s", 
                strerror(errno));
        goto Error_serial;
    }
    *context = zmq_ctx_new();
    if( !*context ){
        syslog(LOG_ERR, "Failed to create context");
        goto Error_context;
    }
    responder = zmq_socket(*context, ZMQ_REP);
    if( !responder ){
        syslog(LOG_ERR, "Failed to opem zmq socket");
        goto Error_socket;
    }
    rc = zmq_bind(responder, "tcp://*:5555");
    if( rc ){
        syslog(LOG_ERR, "Failed to bind socket %d", rc);
        goto Error_bind;
    }
    return rc;
    Error_bind:
        zmq_close(responder);
    Error_socket:
        zmq_ctx_destroy(*context);
    Error_context:
        close(*fd);
    Error_serial:
        closelog();
        return rc;
}


void cleanup(int fd, void **context)
{
    zmq_close(responder);
    zmq_ctx_destroy(*context);
    close(fd);
    closelog();
}



int main()
{
    int fd;
    int rc = 0;
    void *context;
    rc = init(&fd, &context);
    if( rc ){
        return 1;
    }
    syslog(LOG_INFO, "trm240_sms_api has been started");
    char buffer[RESPOND_BUFFER_SIZE];
    char phone[60];
    char message[1100];
    int json_rc = 0;
    while( daemonize ){
        memset(buffer, 0, sizeof(buffer));
        zmq_recv(responder, buffer, RESPOND_BUFFER_SIZE, 0);
        if( !daemonize ){
            break;
        }
        printf("RECEIVED: %s\n", buffer);
        json_rc = parse_json(buffer, phone, message);
        if( json_rc ){
            zmq_send(responder, BAD_JSON_MESSAGE, sizeof(BAD_JSON_MESSAGE), 0);
            continue;
        }
        printf("p: %s\nm: %s\n", phone, message);
        // send_message_GSM(fd, phone, message);
        zmq_send(responder, "OK", strlen("OK"), 0);
    }
    printf("OOOOP\n");
    //send_message(fd, "+37064745286", "cia yra tekstas");
    //size_t bytes = send_message_GSM(fd, "+37064745286", "cia yra tekstas");
    //size_t bytes = send_message_UCS2(fd, "002B00330037003000360034003700340035003200380036", "00310105003200330043");
    
    
    // if(bytes < 1){
    //     syslog(LOG_ERR, "Failed to send a message");
    // }
    // char json[MAX_MESSAGES_SIZE];
    // read_all_messages(fd, json);
    // printf("%s\n", json);
    // if( json ){
    //     printf("%s\n", json);
    // }
    syslog(LOG_INFO, "trm240_sms_api is shutting down");
    cleanup(fd, &context);
    return 0;
}