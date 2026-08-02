#include "app_w5500_mqtt.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include "MQTTClient.h"
#include "app_w5500.h"
#include "dns.h"
#include "socket.h"
#include <string.h>

#define DEBUG_ENABLE 1
#define DEBUG_LOG "[ MQTT-Client ]"
#include "debug_print.h"




/*MQTT服务器域名*/
char mqtt_server_domain[64] = "mqtts.heclouds.com";

/*MQTT服务器服务端IP w5500最终是要通过IP连接的 这里不需要手动设置 由DNS解析域名获取*/
static uint8_t mqtt_dest_ip[4] = {0};

/*MQTT服务器端口*/
uint16_t mqtt_port = 1883;

#define DNS_BUF_SIZE    256
static uint8_t dns_buf[DNS_BUF_SIZE];
static uint8_t dns_server_ip[4] = {114,114,114,114};
static uint8_t dns_resolve_done = 0;

unsigned char recvBuf[BUFFER_SIZE];
unsigned char sendBuf[BUFFER_SIZE];

struct opts_struct
{
    char *clientid;
    int nodelimiter;
    char *delimiter;
    enum QoS qos;
    char *username;
    char *password;
    int showtopics;
} opts = {.clientid = MQTT_CLIENTID, .username = MQTT_USERNAME, .password = MQTT_PASSWORD, .nodelimiter = 0, .delimiter = (char *)"\n", .qos = QOS0, .showtopics = 0};

struct mqtt_message_stuct
{
    char id[8];
    char ack_message[64];
    uint16_t ack_code;
    char version[8];
    uint8_t need_ack;
}mqtt_message;

static Network n;
static MQTTClient c;
static MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
static MQTT_State_t MQTT_State = MQTT_PARAMS_INIT;

uint8_t update_test = 0;

/**
 *
 * @param topic 发布主题
 * @param data 发布消息内容
 * @param size 发布消息长度
 * @return int 消息发布结果
 * @brief MQTT发布函数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 13:57:33
 * @copyright Copyright (c) 2026
 */
int app_w5500_mqtt_publish(const char *topic, char *data, uint16_t size)
{
    int ret = -1;
    MQTTMessage msg =
        {
            .dup = 0,
            .retained = 0,
            .qos = QOS0,
            .id = 0,
            .payload = (void *)data,
            .payloadlen = size};

    if (c.isconnected == 1)
    {
        ret = MQTTPublish(&c, topic, &msg);
        DEBUG_PRINT("result:%d\r\n", ret);
    }
    else
    {
        DEBUG_PRINT("state error\r\n");
    }
    return ret;
}

/**
 * 
 * @brief mqtt应答函数 根据mqtt_message中的参数向应答主题MQTT_ACK_TOPIC_1发送应答
 * @return int 发送结果
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-01 18:35:17
 * @copyright Copyright (c) 2026
 */
static int app_w5500_mqtt_ack(void)
{
    int ret = -1;
    char *topic = MQTT_ACK_TOPIC_1;
    if(mqtt_message.need_ack == 1)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root , "id" , mqtt_message.id);
        cJSON_AddStringToObject(root , "msg" , mqtt_message.ack_message);
        cJSON_AddNumberToObject(root , "code" , mqtt_message.ack_code);

        char *str = cJSON_PrintUnformatted(root);
        if(str != NULL)
        {
            ret = app_w5500_mqtt_publish(topic , str , strlen(topic));
            if(ret == SUCCESSS)
            {
                mqtt_message.need_ack = 0;
                memset(mqtt_message.id , 0 , sizeof(mqtt_message.id));
                memset(mqtt_message.ack_message , 0 , sizeof(mqtt_message.ack_message));
                mqtt_message.ack_code = 0;
                DEBUG_PRINT("mqtt publish success , topic:%s payload:%s\r\n" , topic , str);
            }
            else
            {
                DEBUG_PRINT("mqtt publish fail , ret:%d\r\n" , ret);
            }
        }
        else
        {
            DEBUG_PRINT("CJSON malloc fail\r\n");
        }

    }
    return ret;
}

/**
 * 
 * @brief mqtt解析函数 需要根据实际自定义
 * @param payload 消息内容
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-01 18:36:01
 * @copyright Copyright (c) 2026
 */
static void app_w5500_mqtt_parse(char *payload)
{
    cJSON *root = cJSON_Parse(payload);

    if(root != NULL)
    {
        cJSON *id_js = cJSON_GetObjectItem(root , "id");
        if(id_js!= NULL)
        {
            memcpy(mqtt_message.id , id_js->valuestring , strlen(id_js->valuestring));
        }

        cJSON *version_js = cJSON_GetObjectItem(root , "version");
        if(version_js!= NULL)
        {
            memcpy(mqtt_message.version , version_js->valuestring , strlen(version_js->valuestring));
        }

        cJSON *update_js = cJSON_GetObjectItem(root , "update");
        if(update_js != NULL)
        {
            update_test = update_js->valueint;
            DEBUG_PRINT("parse update , value:%d\r\n" , update_js->valueint);
        }

        cJSON_Delete(root);
        sprintf(mqtt_message.ack_message , "succ");
        mqtt_message.ack_code = 200;
        mqtt_message.need_ack = 1;

    }
}

/**
 *
 * @param md
 * @brief 收到数据的回调函数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 13:55:41
 * @copyright Copyright (c) 2026
 */
static void messageArrived(MessageData *md)
{
    MQTTMessage *message = md->message;
    MQTTString *topic = md->topicName;
    DEBUG_PRINT("receive mqtt message,topic:%s , payload:%s\r\n" , topic->cstring , (char *)message->payload);
    if(strcmp(topic->cstring , MQTT_SUB_TOPIC_1) == 0)
    {
        app_w5500_mqtt_parse((char *)message->payload);
    }
    
}

/**
 *
 * @brief 连接参数初始化
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 13:54:37
 * @copyright Copyright (c) 2026
 */
static void app_w5500_mqtt_params_init(void)
{
    NewNetwork(&n, MQTT_SOCKET);
    MQTTClientInit(&c, &n, 1000, sendBuf, sizeof(sendBuf), recvBuf, sizeof(recvBuf));

    data.willFlag = 0;                     // 不启用遗嘱
    data.clientID.cstring = opts.clientid; // 客户端ID
    data.username.cstring = opts.username; // 用户名
    data.password.cstring = opts.password; // 密码
    data.MQTTVersion = 4;                  // MQTT版本 4为3.1.1 默认值 
    data.keepAliveInterval = KEEPALIVE; // 心跳包间隔
    data.cleansession = 1;              // 不保留连接信息

    DNS_init(DNS_SOCKET, dns_buf);
    dns_resolve_done = 0;
}

/**
 *
 * @return int 连接结果
 * @brief 请求建立MQTT连接
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 13:54:50
 * @copyright Copyright (c) 2026
 */
static int app_w5500_mqtt_connect(void)
{
    int ret = -1;
    ret = MQTTConnect(&c, &data);
    DEBUG_PRINT("MQTTConnect ret=%d (0=accept, 1=bad_ver, 2=id_rejected, 3=svr_unavail, 4=bad_user/pwd, 5=not_auth)", ret);
    return ret;
}

/**
 *
 * @return int 订阅结果
 * @brief 请求订阅主题
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 13:55:09
 * @copyright Copyright (c) 2026
 */
static int app_w5500_mqtt_Sub(void)
{
    int ret = -1;
    opts.showtopics = 1;
    ret = MQTTSubscribe(&c, MQTT_SUB_TOPIC_1, opts.qos, messageArrived);

    return ret;
}

/**
 * 
 * @brief mqtt复位函数 将mqtt状态重置为未连接状态
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 14:27:01
 * @copyright Copyright (c) 2026
 */
static void app_w5500_mqtt_reset(void)
{
    MQTT_State = MQTT_DNS_RESOLVE;
    close(n.my_socket);                     // 关闭之前打开的 socket
    memset(sendBuf, 0, sizeof(sendBuf));
    memset(recvBuf, 0, sizeof(recvBuf));
    dns_resolve_done = 0;
}

/**
 *
 * @brief 发送心跳包保持连接
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 13:55:25
 * @copyright Copyright (c) 2026
 */
static void app_w5500_mqtt_keep(void)
{
    MQTTYield(&c, data.keepAliveInterval);
}

/**
 *
 * @brief MQTT状态机:MQTT_PARAMS_INIT(连接参数初始化) -> MQTT_DNS_RESOLVE（DNS解析） -> MQTT_DISCONNECT(未连接状态，尝试建立连接) -> MQTT_CONNECTED(连接已建立 开始订阅主题) -> MQTT_SUBSCRIBE(主题已订阅，预留状态) -> MQTT_IDLE(空闲状态 发送心跳包维持连接)
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 13:52:49
 * @copyright Copyright (c) 2026
 */
void app_w5500_mqtt_proc(void)
{
    switch (MQTT_State)
    {
    case MQTT_PARAMS_INIT:
    {
        app_w5500_mqtt_params_init();
        MQTT_State = MQTT_DNS_RESOLVE;
        break;
    }

    case MQTT_DNS_RESOLVE:
    {
        if (dns_resolve_done == 0)
        {
            if (DNS_run(dns_server_ip, (uint8_t *)mqtt_server_domain, mqtt_dest_ip) == 1)
            {
                DEBUG_PRINT("DNS OK: %s -> %d.%d.%d.%d", mqtt_server_domain, mqtt_dest_ip[0], mqtt_dest_ip[1], mqtt_dest_ip[2], mqtt_dest_ip[3]);
                dns_resolve_done = 1;
            }
            else
            {
                DEBUG_PRINT("DNS failed, retrying...");
            }
        }
        else
        {
            MQTT_State = MQTT_DISCONNECT;
        }
        break;
    }

    case MQTT_DISCONNECT:
    {
        if (ConnectNetwork(&n, mqtt_dest_ip, mqtt_port) == SOCK_OK)
        {
            if (app_w5500_mqtt_connect() == SUCCESSS)
            {
                DEBUG_PRINT("Connect Success dest ip:%d.%d.%d.%d, port:%d", mqtt_dest_ip[0], mqtt_dest_ip[1], mqtt_dest_ip[2], mqtt_dest_ip[3], mqtt_port);
                MQTT_State = MQTT_SUBSCRIBE;
            }
            else
            {
                DEBUG_PRINT("mqtt connect fail\r\n");
            }
        }
        else
        {
            DEBUG_PRINT("ip connect fail\r\n");
        }
        break;
    }

    case MQTT_CONNECTED:
    {
        if (app_w5500_mqtt_Sub() == SUCCESSS)
        {
            DEBUG_PRINT("Sub Success , topic:%s\r\n", MQTT_SUB_TOPIC_1);
            MQTT_State = MQTT_SUBSCRIBE;
        }
        break;
    }

    case MQTT_SUBSCRIBE:
    {
        MQTT_State = MQTT_IDLE;
        break;
    }

    case MQTT_IDLE:
    {
        app_w5500_mqtt_keep();
        if (c.isconnected == 0)
        {
					app_w5500_mqtt_reset();
					DEBUG_PRINT("DISCONNECT,reset MQTT state\r\n");
					app_w5500_mqtt_ack();
        }
        break;
    }

    case MQTT_PUBLISH:
    {

        break;
    }
    }
}

/**
 * 
 * @return MQTT_State_t 
 * @brief 获取当前MQTT状态 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-26 14:29:25
 * @copyright Copyright (c) 2026
 */
MQTT_State_t app_w5500_mqtt_GetState(void)
{
    return MQTT_State;
}
