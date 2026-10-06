#ifndef _MQTT_H_
#define _MQTT_H_
#include "cJSON.h"
#include "stdint.h"

typedef struct 
{
    char id[8];
    char ack_message[64];
    uint16_t ack_code;
    char version[8];
    uint8_t need_ack;
}mqtt_message_stuct;
extern mqtt_message_stuct mqtt_message;

char *MQTT_OneNet_BoardInfoRefresh(void);
char *MQTT_OneNet_MsgAck(char *id , char *msg , int code);
void MQTT_OneNet_MsgParse(char *payload);






#endif
