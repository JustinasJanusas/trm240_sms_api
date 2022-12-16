#include <string.h>
#include <stdio.h>
#include <json-c/json.h>

#define MAX_MESSAGES_SIZE 50000
#define END_STRING_SIZE 1200
#define PHONE_SIZE 60
#define MESSAGE_SIZE 1100
#define READ_TYPE_SIZE 20
#define STATUS_UNREAD 0
#define STATUS_READ 1
#define METHOD_SEND 0
#define METHOD_READ 1
#define METHOD_CUSTOM 2
#define METHOD_MISSING -1
#define METHOD_UNDEFINED -2

int parse_messages(char *buffer, char json[], int *start_found, char *end_string);
int parse_send_json(char *json, char *phone_number, char *message);
int message_to_pdu(char *buffer, char *phone_number, char *message);
void put_json_objects();
int pdu_to_json(char *json, char *pdu, char *status);
int get_method(char *json);
int parse_read_json(char *json, int *read_type);