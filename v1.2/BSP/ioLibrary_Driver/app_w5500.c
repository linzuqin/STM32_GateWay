#include "app_w5500.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int tcp_state = 0 , udp_state = 0 , phy_state = 0;
uint16_t tcp_send_interval = 10;//tcp主动上报间隔 单位:s
uint8_t socket_0_tcp_dest_ip[4] = {192, 168, 1, 10};
uint16_t socket_0_tcp_dest_port = 2345;
static uint16_t tcp_send_flag = 0;

void tcp_1s_callback(void)
{
	static uint16_t tcp_send_count = 0;
	tcp_send_count ++;
	if(tcp_send_count % tcp_send_interval == 0)
	{
			tcp_send_flag  = 1;
	}
}

static void network_phy_proc(void)
{
	ctlwizchip(CW_GET_PHYLINK, (void *)&phy_state);
	if (phy_state == PHY_LINK_ON)
	{
		printf("PHY link\r\n");
		uint8_t get_phy_conf;
		get_phy_conf = getPHYCFGR();
		printf("The current Mbtis speed : %dMbps\r\n", get_phy_conf & 0x02 ? 100 : 10);
		printf("The current Duplex Mode : %s\r\n", get_phy_conf & 0x04 ? "Full-Duplex" : "Half-Duplex");
		network_init();
		setSn_KPALVTR(TCP_SOCKET, 6); // 30s keepalive
		setSn_KPALVTR(UDP_SOCKET, 6); // 30s keepalive


	}
	else
	{
//			printf("PHY no link\r\n");
	}
}

static void network_tcp_proc(void)
{
	static uint16_t any_port = 50000;
	tcp_state = getSn_SR(TCP_SOCKET);
	switch(tcp_state)
	{
		case SOCK_CLOSE_WAIT :
		{
			if(disconnect(TCP_SOCKET) != SOCK_OK) 
			{
				
				return ;
			}
			break;
		}
		
		case SOCK_INIT ://socket params init
		{
			setSn_KPALVTR(TCP_SOCKET, 6); // 30s keepalive
			setSn_DIPR(TCP_SOCKET, socket_0_tcp_dest_ip);
			setSn_DPORT(TCP_SOCKET, socket_0_tcp_dest_port);
			setSn_CR(TCP_SOCKET, Sn_CR_CONNECT);
			while(getSn_CR(TCP_SOCKET));
			break;
		}
		
		case SOCK_CLOSED:
		{
			close(TCP_SOCKET);
			if(socket(TCP_SOCKET, Sn_MR_TCP, any_port++, 0x00) != TCP_SOCKET){
			if(any_port == 0xffff) any_port = 50000;
			return ;
			} 
			break;
		}
		
		case SOCK_ESTABLISHED :
		if(getSn_IR(TCP_SOCKET) & Sn_IR_CON)	// Socket n interrupt register mask; TCP CON interrupt = connection with peer is successful
		{
			setSn_IR(TCP_SOCKET, Sn_IR_CON);  // this interrupt should be write the bit cleared to '1'
			printf("tcp have connected to dest ip \r\n");
		}
		
	 if(getSn_RX_RSR(TCP_SOCKET) > 0) // receive data by tcp
	 {
		
	 }
	 else
	 {
		if(tcp_send_flag == 1)
		{
			char buf[] = "hello world\r\n";

			tcp_send_flag = 0;
			send(TCP_SOCKET , (uint8_t *)buf , strlen(buf));
		}	 
	 
	 }

		
		break;
	}
}

void network_proc(void)
{
	if(phy_state != PHY_LINK_ON)
	{
		network_phy_proc();
	}
	else
	{
		network_tcp_proc();
	}
}
