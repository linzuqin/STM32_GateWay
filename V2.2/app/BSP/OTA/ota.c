#include "ota.h"
#include "app_w5500_http.h"
#include "debug_print.h"
#include <string.h>
#include <stdio.h>
#include "fal_cfg.h"
#include "main.h"
#include "app_flashdb.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ OTA-TASK ]"
#include "debug_print.h"

#define OTA_API_SUCCESS_CODE    0 //http 调用成功 body字段状态码
#define HTTP_SUCCESS_CODE    200 //HTTP调用成功返回状态码
#define HTTP_DOWNLOAD_BLOCK_SUCCESS_CODE    206//HTTP片段下载接口调用成功返回状态码

struct ota_info_t
{
    uint8_t ota_flag;               // 是否触发OTA检测任务
    char path[128];                 // 固件HTTP下载地址
    uint16_t download_page;         // 单次分片长度
    unsigned long download_size;              // 已下载总字节
    uint16_t download_count;        // 下载分片次数
    uint8_t is_need_update;         // 版本是否需要更新标记
    char *version;                  // 当前固件版本
    char dest_version[32];          // 云端目标新版本
    int tid;                        // OTA任务ID
    int fw_size;                    // 固件总大小
    char md5[64];                   // 固件MD5
}ota_info = {.version = VERSION};

static enum ota_state_t ota_state = OTA_INIT;
static uint8_t ota_download_buf[OTA_BLOCK_LEN]; //存储HTTP返回的数据

/**
 * 
 * @brief 自定义的重启函数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:12:03
 * @copyright Copyright (c) 2026
 */
static void ota_reboot(void)
{
    DEBUG_PRINT("OTA ready, system reboot...\r\n");
    NVIC_SystemReset();
}

/**
 * 
 * @brief 自定义的延时函数
 * @param ms 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:13:01
 * @copyright Copyright (c) 2026
 */
static void ota_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
 * 
 * @brief 上报升级状态到平台
 * @param step 状态码: 0-100 下载进度, 101-207 升级状态码
 *             201=升级成功, 107=下载失败未知异常, 206=升级失败未知异常
 * @return int 0=成功, -1=失败
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 10:03:20
 * @copyright Copyright (c) 2026
 */
static int ota_update_version(int step)
{
    char path[128];
    char query[128];
    char headers[128];
    char body[32];

    snprintf(path, sizeof(path), "/fuse-ota/%s/%s/%d/status", MQTT_USERNAME, MQTT_CLIENTID, ota_info.tid);
    snprintf(query, sizeof(query), "pro_id=%s&dev_name=%s&tid=%d", MQTT_USERNAME, MQTT_CLIENTID, ota_info.tid);
    snprintf(headers, sizeof(headers), "Authorization: %s\r\n", OTA_Authorization);
    snprintf(body, sizeof(body), "{\"step\":%d}", step);

    if (app_w5500_http_post(OTA_CHECK_DOMAIN, 80, path, query,
                            "application/json", (uint8_t *)body, strlen(body), headers) != 0)
    {
        DEBUG_PRINT("status report post fail\r\n");
        return -1;
    }

    /* 等待 HTTP 完成 */
    while (app_w5500_http_get_state() != HTTP_DONE && app_w5500_http_get_state() != HTTP_IDLE)
    {
        app_w5500_http_proc();
        ota_delay(10);
    }

    if (app_w5500_http_get_state() == HTTP_IDLE)
    {
        DEBUG_PRINT("status report timeout\r\n");
        return -1;
    }

    http_response_t resp;
    app_w5500_http_get_response(&resp);
    DEBUG_PRINT("status report step=%d, http_code=%d, resp=%s\r\n", step, resp.status_code, resp.body);

    return (resp.status_code == HTTP_SUCCESS_CODE) ? 0 : -1;
}

/**
 * 
 * @brief 解析 查询升级任务 指令的应答
 * @param resp API应答
 * @return int <0:应答异常 0:无升级任务 1:有升级任务
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:13:21
 * @copyright Copyright (c) 2026
 */
static int ota_check_parse(http_response_t *resp)
{
    if(resp == NULL || resp->status_code != HTTP_SUCCESS_CODE || resp->body == NULL)
    {
        DEBUG_PRINT("OTA check resp invalid, code:%d\r\n", resp ? resp->status_code : 0);
        return -1;
    }

    /* 检查 code 字段 */
    char *code_ptr = strstr((char *)resp->body, "\"code\":");
    if(code_ptr == NULL)
    {
        DEBUG_PRINT("No code field in response\r\n");
        return -1;
    }
    int code;
    sscanf(code_ptr, "\"code\":%d", &code);
    if(code != OTA_API_SUCCESS_CODE)
    {
        DEBUG_PRINT("API error code:%d\r\n", code);
        return -1;
    }

    /* 检查是否有升级任务: data 字段非 null */
    char *data_null = strstr((char *)resp->body, "\"data\":null");
    if(data_null != NULL)
    {
        DEBUG_PRINT("No upgrade task available\r\n");
        return 0;
    }

    /* 解析 target 版本号 */
    char *target_ptr = strstr((char *)resp->body, "\"target\":\"");
    if(target_ptr != NULL)
    {
        target_ptr += strlen("\"target\":\"");
        sscanf(target_ptr, "%[^\"]", ota_info.dest_version);
    }

    /* 解析 tid */
    char *tid_ptr = strstr((char *)resp->body, "\"tid\":");
    if(tid_ptr != NULL)
    {
        sscanf(tid_ptr, "\"tid\":%d", &ota_info.tid);
    }

    /* 解析 size */
    char *size_ptr = strstr((char *)resp->body, "\"size\":");
    if(size_ptr != NULL)
    {
        sscanf(size_ptr, "\"size\":%d", &ota_info.fw_size);
    }

    /* 解析 md5 */
    char *md5_ptr = strstr((char *)resp->body, "\"md5\":\"");
    if(md5_ptr != NULL)
    {
        md5_ptr += strlen("\"md5\":\"");
        sscanf(md5_ptr, "%[^\"]", ota_info.md5);
    }

    DEBUG_PRINT("Found OTA task: tid=%d, target=%s, size=%d, md5=%s\r\n",ota_info.tid, ota_info.dest_version, ota_info.fw_size, ota_info.md5);
    return 1;
}

/**
 * 
 * @brief 下载OTA固件
 * @param offset 已下载大小
 * @param buf 存储数组
 * @param len 
 * @return int 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:14:31
 * @copyright Copyright (c) 2026
 */
static int ota_download_block(unsigned long offset, uint8_t *buf)
{
    char path[128] = {0};
    char query[128] = {0};
    char headers[256] = {0};
    int ret = -1;

    snprintf(path, sizeof(path), "/fuse-ota/%s/%s/%d/download",MQTT_USERNAME, MQTT_CLIENTID, ota_info.tid);//拼接url和path

    snprintf(query, sizeof(query), "pro_id=%s&dev_name=%s&tid=%d",MQTT_USERNAME, MQTT_CLIENTID, ota_info.tid);//拼接query params
    snprintf(headers, sizeof(headers),"Authorization: %s\r\n""Range: %lu-%lu\r\n",OTA_Authorization, offset, offset + OTA_BLOCK_LEN - 1); //组装headers

    if(app_w5500_http_get(OTA_CHECK_DOMAIN, 80, path, query, headers) != 0) //发起HTTP GET请求
    {
        DEBUG_PRINT("http get fail\r\n");
        ret = -1;
    }

    while(app_w5500_http_get_state() != HTTP_DONE && app_w5500_http_get_state() != HTTP_IDLE)
    {
        app_w5500_http_proc();
        ota_delay(10);
    }

    if(app_w5500_http_get_state() == HTTP_IDLE)
    {
        DEBUG_PRINT("Download HTTP timeout or error\r\n");
        ret = -2;
    }

    http_response_t resp;
    int resp_len = app_w5500_http_get_response(&resp);
    if(resp.status_code != HTTP_SUCCESS_CODE && resp.status_code != HTTP_DOWNLOAD_BLOCK_SUCCESS_CODE)
    {
        DEBUG_PRINT("Download HTTP error code:%d\r\n", resp.status_code);
        ret = -2;
    }

    /* 用实际收到的 body_len，不能用 content_length（HTTP 头可能不准） */
    if(resp.body_len == 0)
    {
        ret = 0;
        DEBUG_PRINT("Firmware download finish\r\n");
    }
    else if(resp.body_len > 0 && resp.body != NULL)
    {
        /* 非最后一包时，校验实际收到的长度是否等于请求的长度 */
        if ((uint32_t)(offset + resp.body_len) < (uint32_t)ota_info.fw_size && resp.body_len != (int)OTA_BLOCK_LEN)
        {
            DEBUG_PRINT("Download length mismatch: expect=%d, actual=%d\r\n", OTA_BLOCK_LEN, resp.body_len);
            ret = -3;
        }
        else
        {
            memcpy(buf, resp.body, resp.body_len);
            DEBUG_PRINT("Download[%lu-%lu] len=%d\r\n", offset, offset + resp.body_len - 1, resp.body_len);
            ret = resp.body_len;
        }
    }
    return ret;
}

/**
 * 
 * @brief 供给外部调用 手动触发OTA升级流程
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-01 23:53:45
 * @copyright Copyright (c) 2026
 */
void ota_set_start(void)
{
    if(ota_state == OTA_IDLE)
    {
        ota_info.ota_flag = 1;
    }
}

/**
 * 
 * @brief ota处理函数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:50:31
 * @copyright Copyright (c) 2026
 */
void ota_proc(void)
{
    switch(ota_state)
    {
        case OTA_INIT:
        {
            // 上电读取Flash里保存的OTA配置
            app_flashdb_get("ota_info", &ota_info, sizeof(ota_info));
            DEBUG_PRINT("OTA init finish, current version:%s\r\n", ota_info.version);

            if(ota_info.is_need_update == 1)
            {
                if(ota_update_version(201) == 0)
                {
                    DEBUG_PRINT("ota upgrade success reported\r\n");
                }
                else
                {
                    DEBUG_PRINT("ota version update fail,retry\r\n");
                }
                ota_info.is_need_update = 0;
            }
            ota_state = OTA_IDLE;
            break;
        }

        case OTA_IDLE:
        {
            // 收到OTA任务标记，进入云端检测
            if(ota_info.ota_flag == 1)
            {
                ota_info.ota_flag = 0;
                memset(ota_info.path, 0, sizeof(ota_info.path));
                memset(ota_info.dest_version, 0, sizeof(ota_info.dest_version));
                memset(ota_info.md5, 0, sizeof(ota_info.md5));
                ota_info.download_size = 0;
                ota_info.download_count = 0;
                ota_info.tid = 0;
                ota_info.fw_size = 0;

                ota_state = OTA_CHECK;
                DEBUG_PRINT("get ota task , start check cloud version...\r\n");
            }
            break;
        }

        case OTA_CHECK:
        {
            char headers[256] = {0};
            char query[256] = {0};

            sprintf(headers, "Authorization: %s\r\n", OTA_Authorization);
            /* version 为空时给默认值 "1.0" */
            const char *ver = strlen(ota_info.version) > 0 ? ota_info.version : "1.0";
            sprintf(query, "type=%d&version=%s", 2, ver);

            int ret = app_w5500_http_get(OTA_CHECK_DOMAIN, 80, OTA_CHECK_PATH, query, headers); //发起http GET请求
            if(ret == 0)
            {
                http_response_t resp;

                /* 等待 HTTP 完成 */
                while (app_w5500_http_get_state() != HTTP_DONE && app_w5500_http_get_state() != HTTP_IDLE)
                {
                    app_w5500_http_proc();
                    ota_delay(10);
                }

                if (app_w5500_http_get_state() == HTTP_IDLE)
                {
                    DEBUG_PRINT("OTA check HTTP timeout\r\n");
                    ota_state = OTA_IDLE;
                    break;
                }

                app_w5500_http_get_response(&resp); //获取响应的数据
                // DEBUG_PRINT("recv resp:%s\r\n", resp.body);

                int parse_ret = ota_check_parse(&resp);
                if(parse_ret == 1)// 有新版本，进入下载
                {
                    ota_state = OTA_DOWNLOAD;
                    DEBUG_PRINT("Start firmware download\r\n");
                }
                else if(parse_ret == 0) //无升级任务
                {
                    ota_state = OTA_IDLE;
                    DEBUG_PRINT("No upgrade required\r\n");
                }
                else// 返回数据异常
                {
                    ota_state = OTA_ERROR;
                    DEBUG_PRINT("OTA check error\r\n");
                }

            }
            else
            {
                DEBUG_PRINT("HTTP check request send fail\r\n");
                ota_state = OTA_IDLE;
            }
            break;
        }

        case OTA_DOWNLOAD:
        {
            static uint8_t fail_count = 0;

            /* 首次下载前先擦除整个download分区 */
            if(ota_info.download_count == 0 && ota_info.download_size == 0)
            {
                const struct fal_partition *dl_part = fal_partition_find(DOWNLOAD_BLOCK_NAME);
                DEBUG_PRINT("Erase download partition, size=%d\r\n", dl_part->len);
                if(fal_partition_erase_all(dl_part) < 0)
                {
                    DEBUG_PRINT("Partition erase failed\r\n");
                    ota_state = OTA_ERROR;
                    break;
                }
                DEBUG_PRINT("Partition erase done\r\n");
            }

            DEBUG_PRINT("Download block %d offset=%d\r\n", ota_info.download_count, ota_info.download_size);
            int ret = ota_download_block(ota_info.download_size, ota_download_buf);
            if(ret < 0)
            {
                if(fail_count ++ > 5)
                {
                    ota_state = OTA_ERROR;
                }
                else
                {
                    ota_delay(1000);  /* 失败后等1秒再重试，避免触发限流 */
                }
                break;
            }
            else if(ret > 0)
            {
                fail_count = 0;
                if(fal_partition_write(fal_partition_find(DOWNLOAD_BLOCK_NAME) , OTA_FW_START_ADDR + ota_info.download_size, ota_download_buf, ret) >= 0)
                {
                    ota_info.download_size += ret;
                    ota_info.download_count ++;
                    DEBUG_PRINT("download block:%d , total:%d\r\n", ota_info.download_count, ota_info.download_size);
                }
                else
                {
                    DEBUG_PRINT("Flash write failed at offset=%d\r\n", ota_info.download_size);
                    ota_state = OTA_ERROR;
                    break;
                }
            }
            else if(ret == 0)
            {
                ota_state = OTA_FINISH;
            }

            break;
        }

        case OTA_FINISH:
        {
            /* 上报下载完成 (step=100，平台状态变为"正在升级") */
            if(ota_update_version(100) != 0)
            {
                DEBUG_PRINT("report download finish fail, retry\r\n");
                break;
            }
            ota_info.is_need_update = 1;
            /* 设置升级标志，通知 bootloader 从 W25Q download 分区搬运固件到片内 APP 区 */
            {
                uint8_t flag = 1;
                app_flashdb_set("upgrade_flag", &flag, sizeof(flag));
            }
            DEBUG_PRINT("OTA download all finish, reboot to upgrade\r\n");
            ota_reboot();
            break;
        }

        case OTA_ERROR:
        {
            DEBUG_PRINT("OTA task abort, back to idle\r\n");
            app_w5500_http_abort();
            ota_state = OTA_IDLE;
            break;
        }

        default:
            ota_state = OTA_IDLE;
            break;
    }
}
