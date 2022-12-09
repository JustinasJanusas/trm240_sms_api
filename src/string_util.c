#include "string_util.h"


#define OK "\nOK\n"
#define ERROR "\nERROR\n"
#define MSG_START "+CMGL:"

int parse_json(char *json, char *phone_number, char *message)
{
    int rc = 1;
    json_object *jobj;
    printf("huh: %s\n", json);
    jobj = json_tokener_parse(json);
    printf("jobj\n");
    json_object *j;
    j = json_object_object_get(jobj, "phone");
    // printf("j\n");
    // if( !j ){
    //     printf("no phon");
    //     rc = 1;
    //     goto end;
    // }
    // strncpy(phone_number, json_object_get_string(j), sizeof(phone_number)-1);
    // j = json_object_object_get(jobj, "message");
    // if( !j ){
    //     printf("no message");
    //     rc = 1;
    //     goto end;
    // }
    // strncpy(message, json_object_get_string(j), sizeof(phone_number)-1);
    end:
        json_object_put(j);
        json_object_put(jobj);
        return rc;
}

static int get_string_between_quotes(char **tmp_ptr, char **tmp_ptr2, 
                                    char value[], int size)
{
    *tmp_ptr = strstr(*tmp_ptr, "\"");
    if( !*tmp_ptr ){
        return 1;
    }
    printf("size: %lu\n", sizeof('\"'));
    (*tmp_ptr) += 1;
    printf("%s\n", *tmp_ptr);
    *tmp_ptr2 = *tmp_ptr+2;
    *tmp_ptr = strstr(*tmp_ptr2, "\"");
    if( !*tmp_ptr ){
        return 1;
    }
    printf("%s\n", *tmp_ptr);
    *tmp_ptr[0] = '\0';
    *tmp_ptr++;
    printf("%s\n", *tmp_ptr);
    strncat(value, *tmp_ptr2, size);
    return 0;
}

static int add_json_variable(char *json, char *status, char *phone_number, char *date, 
                            char *message){
    char json_variable[1350];
    sprintf(json_variable, "{\"status\":\"%s\",\"phone_number\":\"%s\",\"date\""
            ":\"%s\",\"message\":\"%s\"}", status, phone_number, date, message);
    if(MAX_MESSAGES_SIZE - strlen(json) - strlen(json_variable) - 3 < 1){
        return 1;
    }
    if(strlen(json) > 5)
        strcat(json, ",");
    strcat(json, json_variable);
    return 0;
}

int parse_messages(char *buffer, char json[], int *start_found, char *end_string)
{
    if( !buffer || !json){
        return 0;
    }
    printf("tekstas\n");
   // printf("%s\n", json);
    if( strlen(buffer) < 2 || strlen(json) >= MAX_MESSAGES_SIZE - 1 ){
        return 0;
    }
    int read_next_line = 1;
    int read_next_buffer = 1;
    char *msg_ptr;
    char *tmp_ptr;
    char *tmp_ptr2;
    msg_ptr = strstr(buffer, MSG_START);
    if( !msg_ptr ){
        tmp_ptr = strstr(buffer, ERROR);
        if( tmp_ptr ){
            return 0;
        }
        else{
            return 1;
        }
    }
    tmp_ptr = strstr(msg_ptr, OK);
    if( tmp_ptr ){
        read_next_buffer = 0;
        tmp_ptr[0]='\0';
    }
    // char *end_ptr = strstr(buffer, OK);
    // if( end_ptr ){
    //     read_next_buffer = 0;
    //     end_ptr[0] = '\0';
    // }
    // else{
    //     end_ptr = strstr(buffer, ERROR);
    //     if( end_ptr ){
    //         read_next_buffer = 0;
    //         end_ptr[0] = '\0';
    //     }
    // }
    // char json_variable[1200];
    // strcat(json_variable, "{\"status\":\"");
    char status[50];
    char phone_number[50];
    char date[50];
    char message[1100];
    int rc = 0;
    while( msg_ptr ){
        tmp_ptr = msg_ptr;
        rc = get_string_between_quotes(&tmp_ptr, &tmp_ptr2, status, 49);
        if( rc ){
            break;
        }
        printf("%s\n", status);
        rc = get_string_between_quotes(&tmp_ptr, &tmp_ptr2, phone_number, 49);
        if( rc ){
            break;
        }
        rc = get_string_between_quotes(&tmp_ptr, &tmp_ptr2, date, 49);
        if( rc ){
            break;
        }
        tmp_ptr = strstr(tmp_ptr, "\n");
        if( !tmp_ptr ){
            break;
        }
        tmp_ptr2 = tmp_ptr+1;
        tmp_ptr = strstr(tmp_ptr2, "\n");
        if( !tmp_ptr ){
            break;
        }
        tmp_ptr[0] = '\0';
        tmp_ptr++;
        rc = add_json_variable(json, status, phone_number, date, message);
        if( rc ){
            return 0;
        }
        // strncat(json, buffer, MAX_MESSAGES_SIZE - 1 - strlen(json));
        msg_ptr = strstr(tmp_ptr, MSG_START);
        break;
    }
    if( msg_ptr ){
        strncpy(end_string, msg_ptr, END_STRING_SIZE -1);
    }
    else if( strlen(tmp_ptr) > 0 ){
        strncpy(end_string, tmp_ptr, END_STRING_SIZE-1);
    }
    return read_next_buffer;
}