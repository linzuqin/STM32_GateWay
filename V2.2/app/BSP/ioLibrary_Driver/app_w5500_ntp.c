#include "app_w5500_ntp.h"
#include "app_w5500.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include "sntp.h"
#include "dns.h"
#include "socket.h"
#include <string.h>
#include <stdio.h>

#define DEBUG_ENABLE 1
#define DEBUG_LOG "[ NTP ]"
#include "debug_print.h"

/* DNS缓冲区 */
#define DNS_BUF_SIZE    256
static uint8_t ntp_dns_buf[DNS_BUF_SIZE];
static uint8_t dns_server_ip[4] = {114, 114, 114, 114};
static uint8_t ntp_dns_resolve_done = 0;

/* SNTP数据缓冲区 */
static uint8_t ntp_data_buf[128];

/* NTP服务器配置 */
static char ntp_server_domain[64] = NTP_DEFAULT_SERVER;
static uint8_t ntp_server_ip[4] = {0};

/* 时区配置 (中国 UTC+8, 对应索引39) */
static uint8_t ntp_timezone = 39;

/* 同步间隔(秒) */
static uint16_t ntp_sync_interval = NTP_SYNC_INTERVAL;

/* 时间戳计数器 */
static uint32_t ntp_tick_count = 0;
static uint32_t ntp_last_sync_tick = 0;

/* 当前时间 */
static ntp_time_t current_time = {0};

/* SNTP运行状态 */
static datetime ntp_datetime = {0};
static uint8_t ntp_sntp_init_done = 0;

/* 状态机 */
static NTP_State_t NTP_State = NTP_INIT;

/* 手动触发同步标记 */
static uint8_t ntp_sync_now_flag = 0;

/**
 * @brief 设置NTP服务器域名
 * @param server 服务器域名
 */
void app_w5500_ntp_set_server(const char *server)
{
    memset(ntp_server_domain, 0, sizeof(ntp_server_domain));
    strncpy(ntp_server_domain, server, sizeof(ntp_server_domain) - 1);
    ntp_dns_resolve_done = 0;
    NTP_State = NTP_RESOLVE;
    DEBUG_PRINT("NTP server set to: %s\r\n", ntp_server_domain);
}

/**
 * @brief 设置NTP同步间隔
 * @param interval_seconds 同步间隔(秒)
 */
void app_w5500_ntp_set_sync_interval(uint16_t interval_seconds)
{
    ntp_sync_interval = interval_seconds;
}

/**
 * @brief 手动触发一次NTP同步
 * @return 0:成功  -1:正在同步中
 */
int app_w5500_ntp_sync_now(void)
{
    if (NTP_State == NTP_SYNCING)
    {
        return -1;
    }
    ntp_sync_now_flag = 1;
    ntp_dns_resolve_done = 0;
    NTP_State = NTP_RESOLVE;
    return 0;
}

/**
 * @brief 获取当前已校准的时间
 * @param time 时间结构体指针
 * @return 0:未获取到时间  >0:成功获取
 */
int app_w5500_ntp_get_time(ntp_time_t *time)
{
    if (current_time.year == 0)
    {
        return 0;
    }
    
    if (time != NULL)
    {
        memcpy(time, &current_time, sizeof(ntp_time_t));
    }
    return 1;
}

/**
 * @brief 获取当前NTP状态
 * @return NTP_State_t
 */
NTP_State_t app_w5500_ntp_get_state(void)
{
    return NTP_State;
}

/**
 * @brief NTP 1秒定时器回调（需在硬件1秒定时器中断中调用）
 * @note 用于维持本地时钟走时
 */
void app_w5500_ntp_1s_callback(void)
{
    ntp_tick_count++;
    
    /* 更新当前时间 */
    if (current_time.year > 0)
    {
        current_time.second++;
        if (current_time.second >= 60)
        {
            current_time.second = 0;
            current_time.minute++;
            if (current_time.minute >= 60)
            {
                current_time.minute = 0;
                current_time.hour++;
                if (current_time.hour >= 24)
                {
                    current_time.hour = 0;
                    current_time.day++;
                    /* 简单的月份天数处理 */
                    uint8_t days_in_month = 31;
                    switch (current_time.month)
                    {
                    case 4: case 6: case 9: case 11:
                        days_in_month = 30;
                        break;
                    case 2:
                        /* 闰年判断 */
                        if ((current_time.year % 400 == 0) || 
                            (current_time.year % 100 != 0 && current_time.year % 4 == 0))
                        {
                            days_in_month = 29;
                        }
                        else
                        {
                            days_in_month = 28;
                        }
                        break;
                    }
                    if (current_time.day > days_in_month)
                    {
                        current_time.day = 1;
                        current_time.month++;
                        if (current_time.month > 12)
                        {
                            current_time.month = 1;
                            current_time.year++;
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief NTP状态机处理函数
 */
void app_w5500_ntp_proc(void)
{
    switch (NTP_State)
    {
    case NTP_INIT:
    {
        /* 首次初始化，解析NTP服务器域名 */
        ntp_dns_resolve_done = 0;
        NTP_State = NTP_RESOLVE;
        break;
    }
    
    case NTP_RESOLVE:
    {
        if (ntp_dns_resolve_done == 0)
        {
            DNS_init(DNS_SOCKET, ntp_dns_buf);
            if (DNS_run(dns_server_ip, (uint8_t *)ntp_server_domain, ntp_server_ip) == 1)
            {
                DEBUG_PRINT("DNS OK: %s -> %d.%d.%d.%d\r\n", ntp_server_domain,
                    ntp_server_ip[0], ntp_server_ip[1], ntp_server_ip[2], ntp_server_ip[3]);
                ntp_dns_resolve_done = 1;
                
                /* 初始化SNTP */
                SNTP_init(NTP_SOCKET, ntp_server_ip, ntp_timezone, ntp_data_buf);
                ntp_sntp_init_done = 1;
                NTP_State = NTP_SYNCING;
            }
            else
            {
                DEBUG_PRINT("DNS failed, retrying...\r\n");
            }
        }
        else
        {
            /* 重新初始化SNTP */
            SNTP_init(NTP_SOCKET, ntp_server_ip, ntp_timezone, ntp_data_buf);
            ntp_sntp_init_done = 1;
            NTP_State = NTP_SYNCING;
        }
        break;
    }
    
    case NTP_SYNCING:
    {
        if (ntp_sntp_init_done == 0)
        {
            NTP_State = NTP_RESOLVE;
            break;
        }
        
        if (SNTP_run(&ntp_datetime) == 1)
        {
            /* 同步成功，更新时间 */
            current_time.year = ntp_datetime.yy;
            current_time.month = ntp_datetime.mo;
            current_time.day = ntp_datetime.dd;
            current_time.hour = ntp_datetime.hh;
            current_time.minute = ntp_datetime.mm;
            current_time.second = ntp_datetime.ss;
            
            ntp_last_sync_tick = ntp_tick_count;
            ntp_sync_now_flag = 0;
            
            DEBUG_PRINT("NTP sync OK: %04d-%02d-%02d %02d:%02d:%02d\r\n",
                current_time.year, current_time.month, current_time.day,
                current_time.hour, current_time.minute, current_time.second);
            
            NTP_State = NTP_SYNC_OK;
        }
        /* 否则继续等待SNTP_run完成 */
        break;
    }
    
    case NTP_SYNC_OK:
    {
        /* 检查是否需要重新同步 */
        if (ntp_sync_now_flag)
        {
            ntp_dns_resolve_done = 0;
            ntp_sntp_init_done = 0;
            NTP_State = NTP_RESOLVE;
            break;
        }
        
        /* 定期同步 */
        if (ntp_sync_interval > 0)
        {
            if ((ntp_tick_count - ntp_last_sync_tick) >= ntp_sync_interval)
            {
                ntp_dns_resolve_done = 0;
                ntp_sntp_init_done = 0;
                NTP_State = NTP_RESOLVE;
                DEBUG_PRINT("NTP periodic sync triggered\r\n");
            }
        }
        break;
    }
    
    case NTP_SYNC_FAIL:
    {
        /* 同步失败，等待一段时间后重试 */
        if ((ntp_tick_count - ntp_last_sync_tick) >= 60)
        {
            ntp_dns_resolve_done = 0;
            ntp_sntp_init_done = 0;
            NTP_State = NTP_RESOLVE;
            DEBUG_PRINT("NTP retry after fail\r\n");
        }
        break;
    }
    
    default:
        break;
    }
}
