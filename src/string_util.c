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
static __uint16_t bytes_to_number(int tmp[], int count){
	int n = 0;
	int m = 1;
	for(int i = count-1; i > -1; i--){
		if(tmp[i] > 64){
			tmp[i] -= 55;
		}
		else{
			tmp[i] -= 48;
		}
		n += tmp[i]*m;
		m *= 16;
	}
	return n;
}
static void data_to_char_7bit(char *message, char *pdu, int offset, 
							int data_len)
{
	__uint8_t num;
	__uint8_t rollover = 0;
	int written_data = 0;
	int cycle_count = data_len - data_len / 8;
	for(int i = 0; i < cycle_count; i++){
		num = bytes_to_number((int[]){pdu[offset+2*i], pdu[offset+2*i+1]}, 2);
		printf("%x >", num);
		
		
		message[written_data++] = (char)0x7F & ((num << (i%7)) | rollover);
		printf("c: %c\n", message[written_data-1]);
		// num = 
		// 	(char)bytes_to_number((int[]){pdu[offset+i*2], pdu[offset+i*2+1]});
		rollover = num >> (7-(i%7));
		printf("%x, %x\n", message[written_data-1], rollover);
		// printf("p:%d\n", num);
		// printf("a:%d\n", 0x7F & ((pdu[offset+i] < i) | rollover));
		// printf("i: %d\n", message[written_data-1]);
		if( (i+1)%7 == 0 && written_data < data_len){
			//add num
			message[written_data++] = rollover;
			printf("roll: %x >c %c\n", rollover, rollover);
			rollover = 0;
			// printf("i: %d\n", message[written_data-1]);
		}

	}
	message[written_data] = '\0';
	printf("m: %s\n", message);
}

static void data_to_char_8bit(char *message, char *pdu, int offset, 
							int data_len)
{
	__uint8_t num;
	for(int i = 0; i < data_len; i++){
		num = bytes_to_number((int[]){pdu[offset+2*i], pdu[offset+2*i+1]}, 2);
		message[i] = num;
	}
	message[data_len] = '\0';
	printf("m: %s\n", message);
}
static void data_to_char_16bit(char *message, char *pdu, int offset, 
							int data_len)
{
	__uint16_t num;
	printf("oof\n");
	int written_data = 0;
	for(int i = 0; i < data_len/2; i++){
		num = bytes_to_number((int[]){pdu[offset+4*i], pdu[offset+4*i+1], 
							pdu[offset+4*i+2], pdu[offset+4*i+3]}, 4);

		if(num > 2047){
			message[written_data++] = 0xE0 | (num >> 12);
			message[written_data++] = 0x80 | ((num >> 6) & 0x3F);
			message[written_data++] = 0x80 | (num & 0x3F);
		}
		else if(num > 127){
			message[written_data++] = 0xC0 | (num >> 6);
			message[written_data++] = 0x80 | (num & 0x3F);
		}
		else{
			message[written_data++] = num;
		}
	}
	message[written_data] = '\0';
	printf("m: %s\n", message);
}


int pdu_to_json(char *json, char *pdu, int first)
{
	if( !json || !pdu || strlen(pdu) < 10){
		return 1;
	}
	int smsc_len = bytes_to_number((int[]){pdu[0], pdu[1]}, 2);
	int phone_len = bytes_to_number(
							(int[]){pdu[4+smsc_len*2], pdu[5+smsc_len*2]}, 2);
	int offset = smsc_len*2+8;

	//phone number
	char phone[phone_len+2];
	int plus = 0;
	if(pdu[offset-2] == '9' && pdu[offset-1] == '1'){
		phone[0] = '+';
		plus = 1;
	}
	for(int i = 0; i < phone_len; i++){
		//i % 2 == 0 ? phone[i+plus] = pdu[offset+1] : phone[i+plus] = pdu[offset-1];
		if( i % 2 == 0 ){
			phone[i+plus] = pdu[i+offset+1];
		}
		else{
			phone[i+plus] = pdu[i+offset-1];
		}
	}
	phone[phone_len+plus] = '\0';
	printf("phone: %s\n", phone);
	//data coding
	phone_len += phone_len % 2;
	int data_coding = bytes_to_number((int []){pdu[offset+phone_len+2], 
									pdu[offset+phone_len+3]}, 2);

	//date
	offset += phone_len + 4;
	char date[30] = "20";
	sprintf(date, "%s%c%c/%c%c/%c%c %c%c:%c%c:%c%c GMT%c%d", date, pdu[offset+1], 
			pdu[offset], pdu[offset+3], pdu[offset+2], pdu[offset+5], 
			pdu[offset+4], pdu[offset+7], pdu[offset+6], pdu[offset+9], 
			pdu[offset+8], pdu[offset+11], pdu[offset+10], 
			pdu[offset+13] > 55 ? '-' : '+', 
			bytes_to_number((int[]){pdu[offset+13], pdu[offset+12]}, 2)/4);
	printf("date: %s\n", date);
	//message
	offset += 16;
	int data_len = bytes_to_number((int[]){pdu[offset-2], pdu[offset-1]}, 2);
	char message[1100] = "";
	switch( data_coding / 4 ){
		case 0:
			data_to_char_7bit(message, pdu, offset, data_len);
			break;
		case 1:
			data_to_char_8bit(message, pdu, offset, data_len);
			break;
		case 2:
			data_to_char_16bit(message, pdu, offset, data_len);
			break;
		default:
			return 1;
	}
	if( strlen(phone) > 0 && strlen(date) && strlen(message) ){
		//json formating
		return 0;
	}
	return 1;
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