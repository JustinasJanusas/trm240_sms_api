#include <string.h>
#include <stdio.h>
#include <json-c/json.h>

#define MAX_MESSAGES_SIZE 50000
#define END_STRING_SIZE 1200
#define PHONE_SIZE 60
#define MESSAGE_SIZE 1100

int parse_messages(char *buffer, char json[], int *start_found, char *end_string);
int parse_json(char *json, char *phone_number, char *message);
int message_to_pdu(char *buffer, char *phone_number, char *message);
void put_json_objects();
int pdu_to_json(char *json, char *pdu, int first);