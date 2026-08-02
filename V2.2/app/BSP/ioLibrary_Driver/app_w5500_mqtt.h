#ifndef _APP_W5500_MQTT_H_
#define _APP_W5500_MQTT_H_
#include <stdint.h>

/*数组大小*/
#define BUFFER_SIZE 2048

/*心跳包间隔*/
#define KEEPALIVE 60

/*MQTT用户名*/
#define MQTT_USERNAME "2Its5wq8a3"

/*MQTT密码*/
#define MQTT_PASSWORD "version=2018-10-31&res=products%2F2Its5wq8a3%2Fdevices%2Flot_device&et=1988355119&method=md5&sign=zyATn1UFf7fvJEEjorA7Ww%3D%3D"

/*MQTT客户端*/
#define MQTT_CLIENTID "lot_device"

/*MQTT发布主题*/
#define MQTT_PUB_TOPIC_1 "$sys/"MQTT_USERNAME"/"MQTT_CLIENTID"/thing/property/post"

/*MQTT订阅主题*/
#define MQTT_SUB_TOPIC_1 "$sys/"MQTT_USERNAME"/"MQTT_CLIENTID"/thing/property/set"

/*MQTT命令应答主题*/
#define MQTT_ACK_TOPIC_1 "$sys/"MQTT_USERNAME"/"MQTT_CLIENTID"/thing/property/set"

/*产品级密钥*/
#define PRODUCT_KEY "czkTF5mpf5FrcQ+7hT99aeeJG5V7LQdF113JPQvpcQE="

/*MQTT鐘舵€佹満*/
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
