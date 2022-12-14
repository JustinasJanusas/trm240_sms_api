#include "string_util.h"


#define OK "\nOK"
#define ERROR "\nERROR\n"
#define MSG_START "+CMGL:"



json_object *jobj;
json_object *j;

int get_method(char *json)
{
	jobj = json_tokener_parse(json);
	j = json_object_object_get(jobj, "method");
	if( !j ){
		return METHOD_MISSING;
	}
	char *method_name = json_object_get_string(j);
	if( strcmp(method_name, "send") == 0){
		return METHOD_SEND;
	}
	else if( strcmp(method_name, "read") == 0){
		return METHOD_READ;
	}
	else if( strcmp(method_name, "custom") == 0){
		return METHOD_CUSTOM;
	}
	else{
		return METHOD_UNDEFINED;
	}
}

int parse_send_json(char *json, char *phone_number, char *message)
{
    jobj = json_tokener_parse(json);
    j = json_object_object_get(jobj, "phone");
    if( !j ){
        return 1;
    }
    strncpy(phone_number, json_object_get_string(j), PHONE_SIZE-1);
    j = json_object_object_get(jobj, "message");
    if( !j ){
        return 1;
    }
    strncpy(message, json_object_get_string(j), MESSAGE_SIZE-1);
    return 0;
}
int parse_custom_json(char *json, char *command)
{
	jobj = json_tokener_parse(json);
	j = json_object_object_get(jobj, "command");
	if( !j ){
		return 1;
	}
	strncpy(command, json_object_get_string(j), COMMAND_SIZE -1);
	return 0;
}

int parse_read_json(char *json, int *read_type)
{
    jobj = json_tokener_parse(json);
    j = json_object_object_get(jobj, "type");
    if( !j ){
        return 1;
    }
    char type[READ_TYPE_SIZE];
	strncpy(type, json_object_get_string(j), READ_TYPE_SIZE-1);
	if( strcmp(type, "all") == 0 ){
		*read_type = 4;
		return 0;
	}
	else if( strcmp(type, "read") == 0 ){
		*read_type = 1;
		return 0;
	}
	else if( strcmp(type, "unread") == 0 ){
		*read_type = 0;
		return 0;
	}
    return 1;
}

void put_json_objects()
{
    json_object_put(jobj);
    json_object_put(j);
}


static int add_json_variable(char *json, char *status, char *phone_number, char *date, 
                            char *message){
    char json_variable[1500];
    snprintf(json_variable, 1499, "{\"status\":\"%s\",\"phone_number\":\"%s\",\"date\""
            ":\"%s\",\"message\":\"%s\"}", status, phone_number, date, message);
    if(MAX_MESSAGES_SIZE - strlen(json) - strlen(json_variable) - 3 < 1){
        return 1;
    }
    if(strlen(json) > 15){
		strcat(json, ", ");
	}
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
	}
	for(; i < p_len; i+=2){
		sprintf(buffer, "%s%c%c", buffer, phone_number[i+1], phone_number[i]);
	}
	strcat(buffer, "00080B00");
	for(int j = 0; j < strlen(message); j++){
		__uint8_t byte = message[j];
		__uint16_t temp = 0;
		if(byte > 239){
			j += 3;
			continue;
		}
		else if(byte > 223){
			temp =  temp | (message[j] & 0xF);
			temp = (temp << 6) | (message[j+1] & 0x3F);
			temp = (temp << 6) | (message[j+2] & 0x3F);
			j += 2;
		}
		else if(byte > 127){
			temp = temp | (message[j] & 0x1F);
			temp = (temp << 6) | (message[j+1] & 0x3F);
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

		message[written_data++] = (char)0x7F & ((num << (i%7)) | rollover);

		rollover = num >> (7-(i%7));

		if( (i+1)%7 == 0 && written_data < data_len){
			//add num
			message[written_data++] = rollover;

			rollover = 0;

		}

	}
	message[written_data] = '\0';
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
}
static void data_to_char_16bit(char *message, char *pdu, int offset, 
							int data_len)
{
	__uint16_t num;
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
}


int pdu_to_json(char *json, char *pdu, char *status)
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
	
	if( strlen(phone) > 0 && strlen(date) > 0 && strlen(message) > 0 ){
		
	// 	//json formating
		char json_object[1500];
		add_json_variable(json, status, phone, date, message);
		return 0;
	}
	return 1;
}


int parse_messages(char *buffer, char json[], int *start_found, char *end_string)
{
    if( !buffer || !json){
        return 0;
    }

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

    int status;
	char status_str[20];
    char phone_number[50];
    char date[50];
    char message[1100];
    int rc = 0;
    while( msg_ptr ){
		tmp_ptr = strstr(msg_ptr, ",");
		if( !tmp_ptr || strlen(tmp_ptr) < 3 ){
			break;
		}
		status = tmp_ptr[1]-48;
		switch (status)
		{
			case STATUS_READ:
				strcpy(status_str, "Read");
				break;
			case STATUS_UNREAD:
				strcpy(status_str, "Unread");
				break;
			default:
				strcpy(status_str, "Unknown");
				break;
		}
        tmp_ptr = strstr(tmp_ptr, "\n");
		if( !tmp_ptr ){
			break;
		}
		tmp_ptr++;
        tmp_ptr2 = strstr(tmp_ptr, "\n");
		if( !tmp_ptr2 ){
			break;
		}
		msg_ptr = strstr(tmp_ptr2, MSG_START);
		tmp_ptr2[0] = '\0'; 
		if( pdu_to_json(json, tmp_ptr, status_str) ){
			return 0;
		}

    }
    if( msg_ptr ){
        strncpy(end_string, msg_ptr, END_STRING_SIZE -1);
    }
    else if( strlen(tmp_ptr) > 0 ){
        strncpy(end_string, tmp_ptr, END_STRING_SIZE-1);
    }
    return read_next_buffer;
}