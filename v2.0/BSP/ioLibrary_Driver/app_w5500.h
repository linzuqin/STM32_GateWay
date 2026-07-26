#ifndef _APP_W5500_H_
#define _APP_W5500_H_
#include "app_w5500_tcp_client.h"
#include "app_w5500_tcp_server.h"
#include "app_w5500_udp.h"
#include "app_w5500_mqtt.h"


#define TCP_SOCKET	0
#define UDP_SOCKET  1
#define TCP_SERVER_SOCKET 2
#define MQTT_SOCKET 3
#define DNS_SOCKET      4 

typedef enum
{
    NETWORK_TCP_CLIENT = 0,
    NETWORK_TCP_SERVER,
    NETWORK_UDP,
}network_con_type;

typedef struct
{
    uint8_t digital_out_1;
    uint8_t digital_out_2;
    uint8_t digital_in_1;
    uint8_t digital_in_2;

    float analog_in_1;
    float analog_in_2;
    
    float analog_out_1;

    float temp;
    uint8_t beep_stae;
}network_report_info_t;

extern network_report_info_t network_report_info;

void tcp_1s_callback(void);
void network_proc(void);

#endif
