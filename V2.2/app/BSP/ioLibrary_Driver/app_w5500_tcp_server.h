#ifndef _APP_W5500_TCP_SERVER_H_
#define _APP_W5500_TCP_SERVER_H_
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "cJSON.h"
#include "cJSON_fieldManage.h"

#include "wiz_platform.h"
#include "socket.h"
#include "dhcp.h"
#include "loopback.h"
#include "w5500.h"

extern uint16_t tcp_server_port;          // TCP服务器监听端口
extern uint8_t tcp_server_connect_ip[4];

void network_tcp_server_proc(void);
void tcp_server_1s_callback(void);

#endif
