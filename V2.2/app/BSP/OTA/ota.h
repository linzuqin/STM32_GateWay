#ifndef __OTA_H
#define __OTA_H

#include <stdint.h>
#include "app_w5500_http.h"
#include "app_w5500_mqtt.h"

#include "flashdb.h"

#define OTA_CHECK_DOMAIN    "iot-api.heclouds.com"
#define OTA_CHECK_PATH      "/fuse-ota/"MQTT_USERNAME"/"MQTT_CLIENTID"/check"

#define OTA_Authorization       "version=2022-05-01&res=products%2F2Its5wq8a3&et=1988355119&method=md5&sign=lVmNkIx%2ByZrhIW6EWZ1CPw%3D%3D"

// 固件下载单次分片大小
#define OTA_BLOCK_LEN       4096

#define OTA_FW_START_ADDR   0x00000000

enum ota_state_t
{
    OTA_INIT = 0,
    OTA_IDLE,
    OTA_CHECK,
    OTA_DOWNLOAD,
    OTA_FINISH,
    OTA_ERROR,
};
extern enum ota_state_t ota_state;
void ota_proc(void);
void ota_set_start(void);

#endif
