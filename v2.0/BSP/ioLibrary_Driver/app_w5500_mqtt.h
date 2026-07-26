#ifndef _APP_W5500_MQTT_H_
#define _APP_W5500_MQTT_H_
#include <stdint.h>

/*MQTT状态机*/
typedef enum
{
    MQTT_PARAMS_INIT = 0,
    MQTT_DNS_RESOLVE,
    MQTT_DISCONNECT,
    MQTT_CONNECTED,
    MQTT_SUBSCRIBE,
    MQTT_IDLE,
    MQTT_PUBLISH,
}MQTT_State_t;

int app_w5500_mqtt_publish(const char *topic , char *data , uint16_t size);
void app_w5500_mqtt_proc(void);


#endif
