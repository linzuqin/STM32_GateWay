#ifndef __APP_W5500_HTTP_H
#define __APP_W5500_HTTP_H

#include <stdint.h>

typedef enum
{
    HTTP_IDLE= 0,
    HTTP_PARAMS_INIT,
    HTTP_INIT,
    HTTP_WAIT_CONNECT,
    HTTP_SEND_REQUEST,
    HTTP_RECV_RESPONSE,
    HTTP_ERROR,
    HTTP_CLOSE,
} HTTP_State_t;

typedef enum
{
    HTTP_GET  = 0,
    HTTP_POST = 1
} HTTP_Method_t;

typedef struct
{
    int status_code;
    int body_len;
    uint8_t* body;
    int content_length;
} http_response_t;



int app_w5500_http_get(const char *domain, uint16_t port, const char *path, const char *query, const char *headers);
int app_w5500_http_post(const char *domain, uint16_t port, const char *path, const char *query, const char *content_type, const uint8_t *data, uint16_t data_len, const char *headers);
int app_w5500_http_get_response(http_response_t *resp);
HTTP_State_t app_w5500_http_get_state(void);
void app_w5500_http_abort(void);
void app_w5500_http_proc(void);

#endif
