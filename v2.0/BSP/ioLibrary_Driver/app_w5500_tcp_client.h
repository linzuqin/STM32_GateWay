#ifndef _APP_W5500_TCP_CLIENT_H_
#define _APP_W5500_TCP_CLIENT_H_
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

extern uint8_t tcp_client_dest_ip[4];
extern uint16_t tcp_client_dest_port;

void network_tcp_client_proc(void);
void tcp_client_1s_callback(void);


#endif
