#include "app_w5500.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ W5500 ]"
#include "debug_print.h"

#define MAX_KEEP_COUNT	5

network_report_info_t network_report_info;
static uint8_t phy_stable = 0;       
static uint8_t phy_raw_last = 0;    
static uint8_t state_keep_count = 0;

socket_manage_t socket_manage[8] = 
{
	[TCP_SOCKET] = {.identifier = "TCP SOCKET" , .func = network_tcp_client_proc , .interval = 30},
	[UDP_SOCKET] = {.identifier = "UDP SOCKET" , .func = network_udp_proc , .interval = 30},
	[TCP_SERVER_SOCKET] = {.identifier = "TCP SERVER SOCKET" , .func = network_tcp_server_proc , .interval = 30},
	[MQTT_SOCKET] = {.identifier = "MQTT SOCKET" , .func = app_w5500_mqtt_proc , .interval = 30},
	[DNS_SOCKET] = {.identifier = "DNS SOCKET" , .func = NULL , .interval = 30},
	[HTTP_SOCKET] = {.identifier = "HTTP SOCKET" , .func = app_w5500_http_proc , .interval = 30},
	[NTP_SOCKET] = {.identifier = "NTP SOCKET" , .func = app_w5500_ntp_proc , .interval = 30},
	[TCP_SERVER_MODBUS_SLAVE_SOCKET] = {.identifier = "TCP SERVER MB S SOCKET" , .func = network_tcp_server_modbus_slave_proc , .interval = 30},
};

static void socket_close_all(void)
{
	uint8_t i = 0;
	for(i = 0;i<8;i++)
	{
		close(i);
	}
}

static void socket_proc(void)
{
	uint8_t i = 0;
	for(i = 0;i<8;i++)
	{
		if(socket_manage[i].func != NULL)
		{
			socket_manage[i].func();
		}
	}	
}

void network_proc(void)
{
	uint8_t index_state = 0;
	ctlwizchip(CW_GET_PHYLINK, (void *)&index_state);

	uint8_t new_stable = phy_stable;
	if(index_state == phy_raw_last)
	{
		if(state_keep_count < MAX_KEEP_COUNT)
		{
			state_keep_count ++;
		}
		else
		{
			new_stable = index_state; //状态消抖
		}
	}
	else
	{
		state_keep_count = 0;
	}
	phy_raw_last = index_state;

	if(new_stable != phy_stable)
	{
		if(new_stable == PHY_LINK_ON)
		{
			socket_close_all();
			network_init();
		}
		else
		{
			socket_close_all();
		}
		phy_stable = new_stable;
	}

	if(phy_stable == PHY_LINK_ON)
	{
		socket_proc();
		ota_proc();
	}
}
