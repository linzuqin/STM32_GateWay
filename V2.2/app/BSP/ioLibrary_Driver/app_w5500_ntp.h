#ifndef _APP_W5500_NTP_H_
#define _APP_W5500_NTP_H_
#include <stdint.h>

/* NTP状态机 */
typedef enum
{
    NTP_INIT = 0,
    NTP_RESOLVE,
    NTP_SYNCING,
    NTP_SYNC_OK,
    NTP_SYNC_FAIL,
} NTP_State_t;

/* 时间结构体 */
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} ntp_time_t;

/* NTP服务器域名，默认为阿里云NTP服务器 */
#define NTP_DEFAULT_SERVER  "ntp.aliyun.com"
#define NTP_PORT            123
#define NTP_SYNC_INTERVAL   3600    /* 默认每1小时同步一次 */

void app_w5500_ntp_proc(void);
void app_w5500_ntp_1s_callback(void);
NTP_State_t app_w5500_ntp_get_state(void);
int app_w5500_ntp_get_time(ntp_time_t *time);
int app_w5500_ntp_sync_now(void);
void app_w5500_ntp_set_server(const char *server);
void app_w5500_ntp_set_sync_interval(uint16_t interval_seconds);

#endif
