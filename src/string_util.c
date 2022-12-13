#include "string_util.h"


#define OK "\nOK\n"
#define ERROR "\nERROR\n"
#define MSG_START "+CMGL:"


json_object *jobj;
json_object *j;

int parse_json(char *json, char *phone_number, char *message)
{
    jobj = json_tokener_parse(json);
    printf("jobj\n");
    j = json_object_object_get(jobj, "phone");
    printf("j\n");
    if( !j ){
        printf("no phon");
        return 1;
    }
    strncpy(phone_number, json_object_get_string(j), PHONE_SIZE-1);
    j = json_object_object_get(jobj, "message");
    if( !j ){
        printf("no message");
        return 1;
    }
    strncpy(message, json_object_get_string(j), MESSAGE_SIZE-1);
    return 0;
}

void put_json_objects()
{
    json_object_put(jobj);
    json_object_put(j);
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

int message_to_pdu(char *buffer, char *phone_number, char *message)
{
	if( !phone_number || !message || strlen(phone_number) < 2 || 
		strlen(message) < 1){
		return 1;
	}
	//buffer = "001100";
	//strcat(buffer, "001100");
	int p_len = strlen(phone_number);
	printf("%X\n", p_len);
	int p_m_len = p_len;
	char *address_type = "81"; 
	int i = 0;
	int data_size = 0;
	if( phone_number[0] == '+' ){
		p_m_len--;
		address_type = "91";
		i++;
	}
	sprintf(buffer, "001100%X%X%s", p_m_len/16, p_m_len%16, address_type);
	if(p_len < 29 && p_m_len % 2 != 0){
		phone_number[p_len++] = 'F';
		phone_number[p_len] = '\0';
		p_m_len++;
		printf("%s\n", phone_number);
	}
	for(; i < p_len; i+=2){
		sprintf(buffer, "%s%c%c", buffer, phone_number[i+1], phone_number[i]);
	}
	strcat(buffer, "00080B00");
	for(int j = 0; j < strlen(message); j++){
		__uint8_t byte = message[j];
		//printf("%d\n", byte);
		__uint16_t temp = 0;
		if(byte > 239){
			j += 3;
			continue;
		}
		else if(byte > 223){
			temp =  temp | (message[j] & 0xF);
			temp = (temp << 6) | (message[j+1] & 0x3F);
			temp = (temp << 6) | (message[j+2] & 0x3F);
			//printf("%X\n", temp);
			j += 2;
			// __uint16_t h = (temp & 0x1F00) >> 2;
			// __uint16_t l = temp & 0x3F;
		}
		else if(byte > 127){
			temp = temp | (message[j] & 0x1F);
			temp = (temp << 6) | (message[j+1] & 0x3F);
			//printf("%X\n", temp);
			j++;
		}
		else{
			temp = message[j];
		}
		char t[5] = "0000";
		int k = 3;
		while(temp > 0 && k > -1 ){
			int tmp = temp %16;
			if(tmp < 10){
				t[k] = tmp + 48;
			}
			else{
				t[k] = tmp + 55;
			}
			temp = temp/16;
			k--;
		}
		data_size += 2;
		strcat(buffer, t);
	}
	if(data_size < 1){
		return 1;
	}
	int a = data_size % 16;
	if(a < 10){
		a += 48;
	}
	else{
		a += 55;
	}
	buffer[p_m_len + 17] = a;
	a = (data_size/16) % 16;
	if(a < 10){
		a += 48;
	}
	else{
		a += 55;
	}
	buffer[p_m_len + 16] = a;
	// for(int j = 0; j < strlen(message); j++){
	// 	temp = message[j];
	// 	printf("j%d: %c, %d, %d, %d\n", j, message[j], temp, temp/(16*16*16), 16*16*16);
	// 	sprintf(buffer, "%s%x%x%x%x", buffer, temp/(16*16*16), (temp%(16*16*16))/(16*16), (temp%(16*16))/16, temp%16);
	// }
	return 0;
}

int parse_messages(char *buffer, char json[], int *start_found, char *end_string)
{
    if( !buffer || !json){
        return 0;
    }
    if( strlen(buffer) < 2 || strlen(json) >= MAX_MESSAGES_SIZE - 1 ){
        return 0;
    }
    return 0;
}

// int parse_messages(char *buffer, char json[], int *start_found, char *end_string)
// {
//     if( !buffer || !json){
//         return 0;
//     }
//     printf("tekstas\n");
//    // printf("%s\n", json);
//     if( strlen(buffer) < 2 || strlen(json) >= MAX_MESSAGES_SIZE - 1 ){
//         return 0;
//     }
//     int read_next_line = 1;
//     int read_next_buffer = 1;
//     char *msg_ptr;
//     char *tmp_ptr;
//     char *tmp_ptr2;
//     msg_ptr = strstr(buffer, MSG_START);
//     if( !msg_ptr ){
//         tmp_ptr = strstr(buffer, ERROR);
//         if( tmp_ptr ){
//             return 0;
//         }
//         else{
//             return 1;
//         }
//     }
//     tmp_ptr = strstr(msg_ptr, OK);
//     if( tmp_ptr ){
//         read_next_buffer = 0;
//         tmp_ptr[0]='\0';
//     }
//     // char *end_ptr = strstr(buffer, OK);
//     // if( end_ptr ){
//     //     read_next_buffer = 0;
//     //     end_ptr[0] = '\0';
//     // }
//     // else{
//     //     end_ptr = strstr(buffer, ERROR);
//     //     if( end_ptr ){
//     //         read_next_buffer = 0;
//     //         end_ptr[0] = '\0';
//     //     }
//     // }
//     // char json_variable[1200];
//     // strcat(json_variable, "{\"status\":\"");
//     char status[50];
//     char phone_number[50];
//     char date[50];
//     char message[1100];
//     int rc = 0;
//     while( msg_ptr ){
//         tmp_ptr = msg_ptr;
//         rc = get_string_between_quotes(&tmp_ptr, &tmp_ptr2, status, 49);
//         if( rc ){
//             break;
//         }
//         printf("%s\n", status);
//         rc = get_string_between_quotes(&tmp_ptr, &tmp_ptr2, phone_number, 49);
//         if( rc ){
//             break;
//         }
//         rc = get_string_between_quotes(&tmp_ptr, &tmp_ptr2, date, 49);
//         if( rc ){
//             break;
//         }
//         tmp_ptr = strstr(tmp_ptr, "\n");
//         if( !tmp_ptr ){
//             break;
//         }
//         tmp_ptr2 = tmp_ptr+1;
//         tmp_ptr = strstr(tmp_ptr2, "\n");
//         if( !tmp_ptr ){
//             break;
//         }
//         tmp_ptr[0] = '\0';
//         tmp_ptr++;
//         rc = add_json_variable(json, status, phone_number, date, message);
//         if( rc ){
//             return 0;
//         }
//         // strncat(json, buffer, MAX_MESSAGES_SIZE - 1 - strlen(json));
//         msg_ptr = strstr(tmp_ptr, MSG_START);
//         break;
//     }
//     if( msg_ptr ){
//         strncpy(end_string, msg_ptr, END_STRING_SIZE -1);
//     }
//     else if( strlen(tmp_ptr) > 0 ){
//         strncpy(end_string, tmp_ptr, END_STRING_SIZE-1);
//     }
//     return read_next_buffer;
// }