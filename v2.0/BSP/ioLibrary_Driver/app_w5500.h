#ifndef _APP_W5500_H_
#define _APP_W5500_H_

#include <stdint.h>
#include "wiz_platform.h"
#include "socket.h"
#include "dhcp.h"
#include "loopback.h"
#include "w5500.h"
#include "socket.h"

#define TCP_SOCKET	0
#define UDP_SOCKET  1
#define TCP_SERVER_SOCKET 2

typedef enum
{
    NETWORK_TCP_CLIENT = 0,
    NETWORK_TCP_SERVER,
    NETWORK_UDP,
}network_con_type;


extern uint8_t socket_0_tcp_dest_ip[4];
extern uint16_t socket_0_tcp_dest_port;

void tcp_1s_callback(void);
void network_proc(void);

#endif
