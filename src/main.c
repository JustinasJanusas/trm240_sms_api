#include <signal.h>
#include <zmq.h>

#include "serial_util.h"

#define RESPOND_BUFFER_SIZE 2000
#define BAD_JSON_MESSAGE "Wrong json format"
#define BAD_DATA "Bad data"
#define MISSING_METHOD "Missing \"method\" variable"
#define UNDEFINED_METHOD "Unrecognized method. Known methods: \"send\", \"read\", \"custom\""
#define UNKNOWN_ERROR "Unknown error"
#define UNDEFINED_TYPE "Undefined or unrecognized read type. Types: \"all\", \"read\", \"unread\""
#define UNDEFINED_COMMAND "Undefined command"

volatile sig_atomic_t daemonize = 1;

void* responder;

static void term_proc(int sigterm) 
{
    daemonize = 0;
    write(responder, " ", sizeof(" "));
	
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
    put_json_objects();
    zmq_close(responder);
    zmq_ctx_destroy(*context);
    close(fd);
    closelog();
}

void send_message(char *json, int fd)
{
    char phone[PHONE_SIZE];
    char message[MESSAGE_SIZE];
    int rc = parse_send_json(json, phone, message);
    if( rc ){
        zmq_send(responder, BAD_JSON_MESSAGE, strlen(BAD_JSON_MESSAGE), 0);
        return;
    }
    rc = message_to_pdu(json, phone, message);
    if( rc ){
        zmq_send(responder, BAD_DATA, strlen(BAD_DATA), 0);
        return;
    }
    send_message_PDU(fd, json);
    zmq_send(responder, "OK", strlen("OK"), 0);
}

void read_messages(char *json, int fd)
{
    int type = 4;
    int rc = parse_read_json(json, &type);
    if( rc ){
        zmq_send(responder, UNDEFINED_TYPE, strlen(UNDEFINED_TYPE), 0);
        return;
    }
    char message_list[MAX_MESSAGES_SIZE] = "{\"messages\":[";
    read_message_list(fd, message_list, type);
    strcat(message_list, "]}");
    zmq_send(responder, (char *)message_list, strlen(message_list), 0);
}



void custom_method(char *json, int fd)
{
    char command[COMMAND_SIZE];
    int rc = parse_custom_json(json, command);
    if( rc ){
        zmq_send(responder, BAD_JSON_MESSAGE, strlen(BAD_JSON_MESSAGE), 0);
        return;
    }
    write(fd, command, strlen(command));
    write(fd, "\r", strlen("\r"));
    zmq_send(responder, "OK", strlen("OK"), 0);
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
    
    while( daemonize ){
        memset(buffer, 0, sizeof(buffer));
        zmq_recv(responder, buffer, RESPOND_BUFFER_SIZE, 0);
        if( !daemonize ){
            break;
        }
        int method = get_method(buffer);
        switch (method)
        {
            case METHOD_MISSING:
                zmq_send(responder, MISSING_METHOD, sizeof(MISSING_METHOD), 0);
                continue;
            case METHOD_UNDEFINED:
                zmq_send(responder, UNDEFINED_METHOD, sizeof(UNDEFINED_METHOD), 0);
                continue;
            case METHOD_SEND:
                send_message(buffer, fd);
                continue;        
            case METHOD_READ:
                read_messages(buffer, fd);
                continue;
            case METHOD_CUSTOM:
                custom_method(buffer, fd);
                continue;
            default:
                zmq_send(responder, UNKNOWN_ERROR, sizeof(UNKNOWN_ERROR), 0);
                continue;
        }
        

        
    }
   
    syslog(LOG_INFO, "trm240_sms_api is shutting down");
    cleanup(fd, &context);
    return 0;
}