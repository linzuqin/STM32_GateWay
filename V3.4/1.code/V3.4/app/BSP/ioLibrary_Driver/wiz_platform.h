#ifndef _WIZ_PLATFORM_H_
#define _WIZ_PLATFORM_H_
#include "stdint.h"
#include "wizchip_conf.h"
#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "dhcp.h"

#define W5500_VERSION 0x04

#define SOCKET_ID 0
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)
extern uint8_t ethernet_buf[ETHERNET_BUF_MAX_SIZE];
void wizchip_initialize(void);
void network_init(void);


#endif
