#include "app_w5500_http.h"
#include "app_w5500.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include "dns.h"
#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_ENABLE 1
#define DEBUG_LOG "[ HTTP-Client ]"
#include "debug_print.h"

#define HTTP_RECV_BUF_SIZE (OTA_BLOCK_LEN + 512)
#define HTTP_POST_BUF_SIZE (1024)

#define TCP_CONN_TIMEOUT_MS 3000
#define HTTP_GLOBAL_TIMEOUT_MS 5000

static uint8_t dns_buf[256];

struct http_url_info_t
{
    char http_domain[128];
    uint16_t http_port;
    uint8_t http_dest_ip[4];
    char http_path[256];
    char http_query[256];   // GET查询参数
    char http_headers[384]; // 额外请求头
    HTTP_Method_t http_method;
    char http_content_type[64];
}http_url_info;

struct http_msg_t
{
    uint8_t http_post_data[HTTP_POST_BUF_SIZE];
    uint16_t http_post_data_len;
    uint8_t http_recv_buf[HTTP_RECV_BUF_SIZE];
    uint16_t http_recv_len;
    http_response_t http_response;

}http_msg;

static uint8_t http_request_pending = 0;
static HTTP_State_t HTTP_State = HTTP_IDLE;

static uint32_t state_tick = 0;  // 计时时间戳，外部SysTick毫秒自增
static uint8_t dns_resolved = 0; // DNS是否已解析成功

/**
 * 
 * @brief 重置http的所有参数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:49:52
 * @copyright Copyright (c) 2026
 */
static void http_reset_all_param(void)
{
    memset(http_url_info.http_domain, 0, sizeof(http_url_info.http_domain));
    memset(http_url_info.http_path, 0, sizeof(http_url_info.http_path));
    memset(http_url_info.http_query, 0, sizeof(http_url_info.http_query));
    memset(http_url_info.http_headers, 0, sizeof(http_url_info.http_headers));
    memset(http_url_info.http_content_type, 0, sizeof(http_url_info.http_content_type));
    memset(http_url_info.http_dest_ip, 0, sizeof(http_url_info.http_dest_ip));
    http_url_info.http_port = 80;
    http_url_info.http_method = HTTP_GET;

    dns_resolved = 0;

    memset(http_msg.http_recv_buf, 0, sizeof(http_msg.http_recv_buf));
    http_msg.http_recv_len = 0;

    memset(http_msg.http_post_data, 0, sizeof(http_msg.http_post_data));
    http_msg.http_post_data_len = 0;

    memset(&http_msg.http_response, 0, sizeof(http_msg.http_response));

    //    dns_resolve_done = 0;
    state_tick = 0;
}

/**
 * 
 * @brief 重置http的socket
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:49:31
 * @copyright Copyright (c) 2026
 */
static void http_close_socket_reset(void)
{
    close(HTTP_SOCKET);
    http_request_pending = 0;
    HTTP_State = HTTP_IDLE;
    http_reset_all_param();
}

/**
 * @brief 发起HTTP GET请求
 * @param domain 域名；若填写IP字符串则自动走固定IP模式
 * @param port 端口
 * @param path 请求路径
 * @return 0成功 -1失败
 */
int app_w5500_http_get(const char *domain, uint16_t port, const char *path,const char *query, const char *headers)
{
    if (http_request_pending)
    {
        DEBUG_PRINT("已有请求运行，强制复位\r\n");
        http_close_socket_reset();
    }
    http_reset_all_param();

    // 判断是否为IP地址，不走DNS
    if (strchr(domain, '.') != NULL && strlen(domain) < 16)
    {
        int ip[4] = {0};
        if (sscanf(domain, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
        {
            http_url_info.http_dest_ip[0] = (uint8_t)ip[0];
            http_url_info.http_dest_ip[1] = (uint8_t)ip[1];
            http_url_info.http_dest_ip[2] = (uint8_t)ip[2];
            http_url_info.http_dest_ip[3] = (uint8_t)ip[3];
            DEBUG_PRINT("使用固定IP，跳过DNS\r\n");
        }
    }

    strncpy(http_url_info.http_domain, domain, sizeof(http_url_info.http_domain) - 1);
    strncpy(http_url_info.http_path, path, sizeof(http_url_info.http_path) - 1);
    if (query != NULL)
        strncpy(http_url_info.http_query, query, sizeof(http_url_info.http_query) - 1);
    if (headers != NULL)
        strncpy(http_url_info.http_headers, headers, sizeof(http_url_info.http_headers) - 1);
    http_url_info.http_port = port;
    http_url_info.http_method = HTTP_GET;

    http_request_pending = 1;
    HTTP_State = HTTP_PARAMS_INIT;
    state_tick = HAL_GetTick();
    DEBUG_PRINT("HTTP-GET start:%s:%d%s\r\n", http_url_info.http_domain, http_url_info.http_port, http_url_info.http_path);
    return 0;
}

/**
 * 
 * @brief 发起HTTP POST请求
 * @param domain url
 * @param port 端口 http的端口一般固定80
 * @param path 地址
 * @param content_type 
 * @param data 
 * @param data_len 
 * @param headers 
 * @return int 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:48:52
 * @copyright Copyright (c) 2026
 */
int app_w5500_http_post(const char *domain, uint16_t port, const char *path, const char *query, const char *content_type, const uint8_t *data, uint16_t data_len, const char *headers)
{
    if (http_request_pending)
    {
        DEBUG_PRINT("已有请求运行，强制复位\r\n");
        http_close_socket_reset();
    }
    http_reset_all_param();

    if (strchr(domain, '.') != NULL && strlen(domain) < 16)
    {
        int ip[4] = {0};
        if (sscanf(domain, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
        {
            http_url_info.http_dest_ip[0] = (uint8_t)ip[0];
            http_url_info.http_dest_ip[1] = (uint8_t)ip[1];
            http_url_info.http_dest_ip[2] = (uint8_t)ip[2];
            http_url_info.http_dest_ip[3] = (uint8_t)ip[3];
        }
    }

    strncpy(http_url_info.http_domain, domain, sizeof(http_url_info.http_domain) - 1);
    strncpy(http_url_info.http_path, path, sizeof(http_url_info.http_path) - 1);
    if (query != NULL)
        strncpy(http_url_info.http_query, query, sizeof(http_url_info.http_query) - 1);
    strncpy(http_url_info.http_content_type, content_type, sizeof(http_url_info.http_content_type) - 1);
    if (headers != NULL)
    {
        strncpy(http_url_info.http_headers, headers, sizeof(http_url_info.http_headers) - 1);
    }
    http_url_info.http_port = port;
    http_url_info.http_method = HTTP_POST;
    memcpy(http_msg.http_post_data , (uint8_t *)data , data_len);
    http_msg.http_post_data_len = data_len;

    http_request_pending = 1;
    HTTP_State = HTTP_PARAMS_INIT;
    state_tick = HAL_GetTick();
    DEBUG_PRINT("HTTP-POST start:%s:%d%s len:%d\r\n", http_url_info.http_domain, http_url_info.http_port, http_url_info.http_path, data_len);
    return 0;
}

/**
 * 
 * @brief 获取应答数据供给外部文件调用
 * @param resp 
 * @return int 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:48:26
 * @copyright Copyright (c) 2026
 */
int app_w5500_http_get_response(http_response_t *resp)
{
    if (HTTP_State == HTTP_IDLE && resp != NULL)
    {
        *resp = http_msg.http_response;
        return http_msg.http_response.body_len;
    }
    return 0;
}

/**
 * 
 * @brief 获取http状态机的状态
 * @return HTTP_State_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:48:13
 * @copyright Copyright (c) 2026
 */
HTTP_State_t app_w5500_http_get_state(void)
{
    return HTTP_State;
}

/**
 * 
 * @brief dns解析 用来解析http域名
 * @return int 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:47:57
 * @copyright Copyright (c) 2026
 */
static int app_w5500_http_dns_resolve(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t dns_server_ip[4] = {114, 114, 114, 114};

    if ((now - state_tick) > 2000)
    {
        DEBUG_PRINT("DNS解析超时\r\n");
        return -1;
    }
    int ret = DNS_run(dns_server_ip, (uint8_t *)http_url_info.http_domain, http_url_info.http_dest_ip);
    if (ret == 1)
    {
        //        dns_resolve_done = 1;
        // DEBUG_PRINT("DNS解析成功 %s -> %d.%d.%d.%d\r\n",http_domain, http_dest_ip[0], http_dest_ip[1], http_dest_ip[2], http_dest_ip[3]);
        return 1;
    }
    return 0;
}

/**
 * 
 * @brief 建立tcp链接
 * @return int 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:47:35
 * @copyright Copyright (c) 2026
 */
static int app_w5500_http_tcp_connect(void)
{
    uint8_t sr = getSn_SR(HTTP_SOCKET);
    if (sr == SOCK_ESTABLISHED)
        return 1;

    uint32_t now = HAL_GetTick();
    if ((now - state_tick) > TCP_CONN_TIMEOUT_MS)
    {
        DEBUG_PRINT("TCP连接超时\r\n");
        return -1;
    }

    /* DNS解析后socket是UDP模式，需要关闭后重新打开为TCP */
    if (sr == SOCK_UDP || sr == SOCK_CLOSED)
    {
        close(HTTP_SOCKET);
        socket(HTTP_SOCKET, Sn_MR_TCP, http_url_info.http_port, 0);
        return 0;
    }

    if (sr == SOCK_INIT)
    {
        setSn_DIPR(HTTP_SOCKET, http_url_info.http_dest_ip);
        setSn_DPORT(HTTP_SOCKET, http_url_info.http_port);
        setSn_CR(HTTP_SOCKET, Sn_CR_CONNECT);
        while (getSn_CR(HTTP_SOCKET))
            ;
    }
    return 0;
}

/**
 * 
 * @brief http请求发送函数
 * @return int 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:44:50
 * @copyright Copyright (c) 2026
 */
static int app_w5500_http_send_request(void)
{
    char request[1024];
    int len = 0, total = 0;
    memset(request, 0, sizeof(request));

    if (http_url_info.http_method == HTTP_GET)
    {
        /* 请求行：GET /path?query HTTP/1.1 */
        if (strlen(http_url_info.http_query) > 0)
        {
            len = snprintf(request, sizeof(request),"GET %s?%s HTTP/1.1\r\n", http_url_info.http_path, http_url_info.http_query);
        }
        else
        {
            len = snprintf(request, sizeof(request),"GET %s HTTP/1.1\r\n", http_url_info.http_path);
        }
        total = len;

        /* 标准头部 */
        total += snprintf(request + total, sizeof(request) - total,
                          "Host: %s\r\n"
                          "Connection: close\r\n"
                          "User-Agent: W5500-MCU-Client\r\n",
                          http_url_info.http_domain);

        /* 自定义头部 */
        if (strlen(http_url_info.http_headers) > 0)
        {
            total += snprintf(request + total, sizeof(request) - total, "%s", http_url_info.http_headers);
        }
        /* 头部结束 */
        total += snprintf(request + total, sizeof(request) - total, "\r\n");
    }
    else if (http_url_info.http_method == HTTP_POST)
    {
        /* 请求行 */
        if (strlen(http_url_info.http_query) > 0)
        {
            len = snprintf(request, sizeof(request),"POST %s?%s HTTP/1.1\r\n", http_url_info.http_path, http_url_info.http_query);
        }
        else
        {
            len = snprintf(request, sizeof(request),"POST %s HTTP/1.1\r\n", http_url_info.http_path);
        }
        total = len;

        /* 标准头部 */
        total += snprintf(request + total, sizeof(request) - total,
                          "Host: %s\r\n"
                          "Connection: close\r\n"
                          "Content-Type: %s\r\n"
                          "Content-Length: %d\r\n"
                          "User-Agent: W5500-MCU-Client\r\n",
                          http_url_info.http_domain, http_url_info.http_content_type, http_msg.http_post_data_len);

        /* 自定义头部 */
        if (strlen(http_url_info.http_headers) > 0)
        {
            total += snprintf(request + total, sizeof(request) - total, "%s", http_url_info.http_headers);
        }

        /* 头部结束 */
        total += snprintf(request + total, sizeof(request) - total, "\r\n");

        /* POST body */
        if ((total + http_msg.http_post_data_len) < (int)sizeof(request))
        {
            memcpy(request + total, http_msg.http_post_data, http_msg.http_post_data_len);
            total += http_msg.http_post_data_len;
        }
        else
        {
            DEBUG_PRINT("POST数据超出缓冲区\r\n");
            return -1;
        }
    }

    if (getSn_SR(HTTP_SOCKET) == SOCK_ESTABLISHED)
    {
        //DEBUG_PRINT("send http request:%s\r\n" , request);
        int32_t ret = send(HTTP_SOCKET, (uint8_t *)request, total);
        if (ret > 0)
        {
            // DEBUG_PRINT("请求发送完成 len:%d\r\n", ret);
            return 0;
        }
    }
    DEBUG_PRINT("发送失败\r\n");
    return -1;
}

/**
 * 
 * @brief 将http返回的数据解析出来并保存到http_msg中 
 * @param body_start http应答的body其实位置
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:42:33
 * @copyright Copyright (c) 2026
 */
static void app_w5500_http_parse_response(char *body_start)
{
    http_msg.http_response.status_code = 0;
    http_msg.http_response.body = NULL;
    http_msg.http_response.body_len = 0;

    /* 从响应头中解析状态码 */
    char *status_line = strstr((char *)http_msg.http_recv_buf, "HTTP/");
    if(status_line != NULL)
    {
        sscanf(status_line, "HTTP/%*d.%*d %d", &http_msg.http_response.status_code);
    }

    http_msg.http_response.body = (uint8_t *)body_start;
    http_msg.http_response.body_len = http_msg.http_recv_len - (body_start - (char *)http_msg.http_recv_buf);
    // DEBUG_PRINT("响应:%s\r\n", http_recv_buf);
    // DEBUG_PRINT("解析完成 状态码:%d 响应体长度:%d\r\n", http_response.status_code, http_response.body_len);
}

/**
 * 
 * @brief 初始化http socket
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:43:22
 * @copyright Copyright (c) 2026
 */
void app_w5500_http_init(void)
{
    uint8_t sr = getSn_SR(HTTP_SOCKET);
    if (sr != SOCK_CLOSED)
        close(HTTP_SOCKET);

    int ret = socket(HTTP_SOCKET, Sn_MR_TCP, 0, 0x00);
    if (ret != HTTP_SOCKET)
    {
        DEBUG_PRINT("Socket创建失败\r\n");
        HTTP_State = HTTP_ERROR;
        return;
    }
    HTTP_State = HTTP_WAIT_CONNECT;
    state_tick = HAL_GetTick();
}

void app_w5500_http_abort(void)
{
    http_close_socket_reset();
}

/**
 * 
 * @brief 运行http处理函数 外部文件不直接调用收发函数 只通过读写标志位来后去
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-08-02 23:43:40
 * @copyright Copyright (c) 2026
 */
void app_w5500_http_proc(void)
{
    if (!http_request_pending)
        return;
    uint32_t now = HAL_GetTick();
    // 全局超时强制失败
    if ((now - state_tick) > HTTP_GLOBAL_TIMEOUT_MS)
    {
        DEBUG_PRINT("HTTP全局超时，强制结束\r\n");
        HTTP_State = HTTP_ERROR;
    }

    switch (HTTP_State)
    {
    case HTTP_PARAMS_INIT:
        DNS_init(HTTP_SOCKET, dns_buf);
        HTTP_State = HTTP_INIT;
        break;

    case HTTP_INIT:
        app_w5500_http_init();
        break;

    case HTTP_WAIT_CONNECT:
    {
        /* DNS解析阶段 */
        if (!dns_resolved)
        {
            int dns_ret = app_w5500_http_dns_resolve();
            if (dns_ret == 1)
            {
                dns_resolved = 1;
                state_tick = now;
            }
            else if (dns_ret == -1)
            {
                HTTP_State = HTTP_ERROR;
            }
            break;
        }

        /* TCP连接阶段 */
        int conn_ret = app_w5500_http_tcp_connect();
        if (conn_ret == 1)
        {
            // DEBUG_PRINT("TCP连接建立成功\r\n");
            HTTP_State = HTTP_SEND_REQUEST;
            state_tick = now;
        }
        else if (conn_ret == -1)
        {
            HTTP_State = HTTP_ERROR;
        }
        break;
    }

    case HTTP_SEND_REQUEST:
    {
        if (app_w5500_http_send_request() == 0)
        {
            HTTP_State = HTTP_RECV_RESPONSE;
            state_tick = now;
        }
        else
        {
            HTTP_State = HTTP_ERROR;
        }
        break;
    }

    case HTTP_RECV_RESPONSE:
    {
        uint8_t sr = getSn_SR(HTTP_SOCKET);
        uint16_t rsr = getSn_RX_RSR(HTTP_SOCKET);
        state_tick = now;

        if (rsr > 0 && http_msg.http_recv_len < HTTP_RECV_BUF_SIZE)
        {
            uint16_t read_len = (rsr > (HTTP_RECV_BUF_SIZE - http_msg.http_recv_len))
                                    ? (HTTP_RECV_BUF_SIZE - http_msg.http_recv_len)
                                    : rsr;
            int32_t ret = recv(HTTP_SOCKET, http_msg.http_recv_buf + http_msg.http_recv_len, read_len);
            if (ret > 0)
            {
                http_msg.http_recv_len += ret;
            }
        }

        /* 连接关闭，把剩余数据读完 */
        if (sr == SOCK_CLOSE_WAIT)
        {
            disconnect(HTTP_SOCKET);
        }
        if (sr == SOCK_CLOSED)
        {
            HTTP_State = HTTP_CLOSE;
            break;
        }

        if (http_msg.http_recv_len > 0)
        {
            char *header_end = strstr((char *)http_msg.http_recv_buf, "\r\n\r\n");
            if (header_end != NULL)
            {
                char *body_start = header_end + 4; /* "\r\n\r\n" = 4 字节 */
                uint16_t header_len = (uint16_t)((uint8_t *)body_start - http_msg.http_recv_buf);

                char *cl = strstr((char *)http_msg.http_recv_buf, "Content-Length:");
                if (cl != NULL)
                {
                    int content_len = 0;
                    sscanf(cl, "%*[^:]: %d", &content_len);
                    http_msg.http_response.content_length = content_len;

                    /* 等待完整 body 接收完毕 */
                    if (http_msg.http_recv_len >= header_len + content_len)
                    {
                        app_w5500_http_parse_response(body_start);
                        HTTP_State = HTTP_CLOSE;
                    }
                    /* 否则下次 proc 继续接收 */
                }
                else
                {
                    /* Transfer-Encoding: chunked 或无 Content-Length */
                    /* 以连接关闭作为结束标志 */
                    if (sr == SOCK_CLOSED)
                    {
                        app_w5500_http_parse_response(body_start);
                        HTTP_State = HTTP_CLOSE;
                    }
                }
            }
            else if (sr == SOCK_CLOSED && http_msg.http_recv_len > 0)
            {
                /* 无 header 结束标志，直接解析 */
                app_w5500_http_parse_response((char *)http_msg.http_recv_buf);
                HTTP_State = HTTP_CLOSE;
            }
        }
        break;
    }

    case HTTP_CLOSE:
    {
        close(HTTP_SOCKET);
        http_request_pending = 0;

        HTTP_State = HTTP_IDLE;
        break;
    }

    case HTTP_IDLE:

        break;

    case HTTP_ERROR:
        DEBUG_PRINT("HTTP请求异常终止\r\n");
        http_close_socket_reset();
        break;

    default:
        http_close_socket_reset();
        break;
    }
}
