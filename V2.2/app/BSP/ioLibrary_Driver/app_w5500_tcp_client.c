#include "app_w5500_tcp_client.h"
#include "app_w5500.h"
#include "lfs_user.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ TCP-CLIENT ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)


uint16_t tcp_client_send_interval = 10;//tcp client send interval ,s

/* TCP Client (Socket 0) */
uint8_t tcp_client_dest_ip[4] = {192, 168, 1, 10};
uint16_t tcp_client_dest_port = 2345;
static uint16_t tcp_client_send_flag = 0;

/**
 * 
 * @param socket_id tcp server 对应的socket
 * @param buf 发送数据
 * @param len 数据长度
 * @param ip 目标IP
 * @param port 目标端口
 * @brief tcp server专用的发送函数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-19 19:58:36
 * @copyright Copyright (c) 2026
 */
static int app_w5500_tcp_client_send(uint8_t socket_id , uint8_t *buf , uint16_t len , uint8_t *ip , uint16_t port)
{
	int ret = -1;
	if (getSn_SR(TCP_SOCKET) == SOCK_ESTABLISHED)
	{
		ret = send(socket_id, (uint8_t *)buf, len);
	}
	else
	{
		DEBUG_PRINT("tcp client state error\r\n");
	}
	return ret;
}

void tcp_client_1s_callback(void)
{
	static uint16_t tcp_send_count = 0;
	tcp_send_count ++;
	if(tcp_send_count % tcp_client_send_interval == 0)
	{
			tcp_client_send_flag  = 1;
	}
}

void network_tcp_client_proc(void)
{
	static uint16_t any_port = 50000;
	int tcp_state = getSn_SR(TCP_SOCKET);
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
			setSn_DIPR(TCP_SOCKET, tcp_client_dest_ip);
			setSn_DPORT(TCP_SOCKET, tcp_client_dest_port);
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
			DEBUG_PRINT("tcp have connected to dest ip \r\n");
		}
		
	 if(getSn_RX_RSR(TCP_SOCKET) > 0) // receive data by tcp
	 {
		
	 }
	 else
	 {
		if(tcp_client_send_flag == 1)
		{
			char buf[] = "hello world\r\n";
			tcp_client_send_flag = 0;
			app_w5500_tcp_client_send(TCP_SOCKET , (uint8_t *)buf , strlen(buf) , tcp_client_dest_ip , tcp_client_dest_port);
		}	 
	 
	 }

		
		break;
	}
}
