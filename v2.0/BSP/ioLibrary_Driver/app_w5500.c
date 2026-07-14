#include "app_w5500.h"
#include "lfs_user.h"
#include "cJSON_fieldManage.h"
#include <inttypes.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

/* TCP Server (Socket 0) */
int tcp_state = 0 , udp_state = 0 , phy_state = 0;
uint16_t tcp_send_interval = 10;//tcp client send interval ,s
uint8_t socket_0_tcp_dest_ip[4] = {192, 168, 1, 10};
uint16_t socket_0_tcp_dest_port = 2345;
static uint16_t tcp_send_flag = 0;

/* TCP Server (Socket 2) */
static uint16_t tcp_server_port = 8080;          // TCP服务器监听端口
static uint8_t tcp_server_client_ip[4] = {0};    // 已连接客户端IP
static uint16_t tcp_server_client_port = 0;      // 已连接客户端端口

/* UDP Server (Socket 1) */
static uint16_t udp_server_port = 50001;         // UDP服务器监听端口
static uint8_t udp_client_ip[4] = {0};           // 最近一次收到UDP数据的客户端IP
static uint16_t udp_client_port = 0;             // 最近一次收到UDP数据的客户端端口

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
		setSn_KPALVTR(TCP_SOCKET, 6);       // 30s keepalive
		setSn_KPALVTR(UDP_SOCKET, 6);       // 30s keepalive
		setSn_KPALVTR(TCP_SERVER_SOCKET, 6); // 30s keepalive

		close(TCP_SERVER_SOCKET);
		if(socket(TCP_SERVER_SOCKET, Sn_MR_TCP, tcp_server_port, 0x00) == TCP_SERVER_SOCKET)
		{
			if(listen(TCP_SERVER_SOCKET) == SOCK_OK)
			{
				printf("TCP Server: listening on port %d\r\n", tcp_server_port);
			}
		}

	}
	else
	{
//			printf("PHY no link\r\n");
	}
}

/**
 * 
 * @param root 需要解析的json字符串
 * @param dest_ip 目标ip保存区
 * @param dest_port 目标端口保存区
 * @return int 小于0:解析失败 1:解析成功
 * @brief 连接请求解析函数 解析出目标ip 与 目标端口
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-14 10:59:28
 * @copyright Copyright (c) 2026
 */
static int udp_connect_request_proc(cJSON *root, uint8_t *dest_ip , uint16_t *dest_port)
{
	int ret = -1;
	uint8_t receive_ip[4];
	cJSON *ip_js = cJSON_GetObjectItem(root , "ip");
	cJSON *port_js = cJSON_GetObjectItem(root , "port");

	
	if(ip_js != NULL && port_js != NULL)
	{
		if(sscanf(ip_js->valuestring , "%d:%d:%d:%d" , (int *)&receive_ip[0] , (int *)&receive_ip[1] , (int *)&receive_ip[2] , (int *)&receive_ip[3]) == 4)
		{
			memcpy(dest_ip , receive_ip , sizeof(uint8_t) * 4);
			*dest_port = port_js->valueint;
			ret = 1;
		}
		else 
		{
			printf("ip parse fail\r\n");
			ret = -1;
		}

	}
	else {
		printf("udp receive connect format error\r\n");
		ret = -1;
	}
	return ret;
}

/**
 * 
 * @param id 消息ID
 * @param IP 目标设备IP
 * @param port 目标设备端口
 * @return int 小于0:发送失败 1:发送成功
 * @brief 连接应答的报文
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-14 10:31:05
 * @copyright Copyright (c) 2026
 */
static int udp_tcp_client_connect_request_ack_proc(char *id , uint8_t *IP , uint16_t port)
{
	int ret = -1;
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		ret = -1;
	}
	else 
	{
		cJSON_AddStringToObject(root , "id" , id);
		cJSON_AddStringToObject(root , "type" , TCP_CLIENT_CONNECT_REQUEST);
		cJSON_AddStringToObject(root , "msg" , "success");
		cJSON_AddNumberToObject(root , "code" , 200);

		char *str = cJSON_PrintUnformatted(root);
		if(str == NULL)
		{
			ret = -2;
		}
		else 
		{
			ret = sendto(UDP_SOCKET, (uint8_t *)str, strlen(str), IP, port);
		}
		if(str != NULL)
		{
			free(str);
		}
	}
	if(root != NULL)
	{
		cJSON_Delete(root);
	}
	return ret;
}

/**
 * 
 * @param buf 需要处理的数据
 * @param message_id 解析出来的消息id
 * @param dest_ip 解析出来的目标ip
 * @param dest_port 解析出来的目标端口
 * @return int 小于0:解析失败 >=0:返回消息的类型
 * @brief 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-14 11:03:07
 * @copyright Copyright (c) 2026
 */
static int udp_message_proc(uint8_t *buf , char *message_id , uint8_t *dest_ip , uint16_t *dest_port)
{
	int ret = -1;
	cJSON *root = cJSON_Parse((char *)buf);
	if(root == NULL)
	{
		printf("udp message parse fail!\r\n");
		ret = -1;
	}
	else 
	{
		/*先获取消息类型 与 消息id 这两个是必须有的*/
		cJSON *message_type_js = cJSON_GetObjectItem(root , "type");
		cJSON *message_id_js = cJSON_GetObjectItem(root , "id");

		if(message_type_js != NULL && message_id_js != NULL)
		{
			if(strcmp(message_type_js->valuestring , TCP_CLIENT_CONNECT_REQUEST) == 0)//tcp客户端连接请求消息
			{
				if(udp_connect_request_proc(root , dest_ip , dest_port) == 1)
				{
					ret = NETWORK_TCP_CLIENT;
					memcpy(message_id , message_id_js->valuestring , strlen(message_id_js->valuestring));
				}
				else 
				{
					ret = -2;
				}
			}
			else if(strcmp(message_type_js->valuestring , TCP_SERVER_CONNECT_REQUEST) == 0)//tcp服务端连接请求消息
			{

				ret = NETWORK_TCP_SERVER;
			}
			else //其他消息 后期扩展
			{
			
			}
		}
		else
		{
			ret = -3;
			printf("udp message type unknow\r\n");
		}
	}
	if(root != NULL)
	{
		cJSON_Delete(root);
	}
	return ret;
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

static void network_tcp_server_proc(void)
{
	uint8_t server_state;
	uint8_t tmp_ip[4];
	uint16_t tmp_port;
	
	server_state = getSn_SR(TCP_SERVER_SOCKET);
	
	switch(server_state)
	{
		case SOCK_CLOSE_WAIT:
		{
			// 客户端主动断开连接
			if(disconnect(TCP_SERVER_SOCKET) != SOCK_OK)
			{
				return;
			}
			printf("TCP Server: client disconnected\r\n");
			break;
		}
		
		case SOCK_INIT:
		{
			// 开始监听
			setSn_KPALVTR(TCP_SERVER_SOCKET, 6); // 30s keepalive
			if(listen(TCP_SERVER_SOCKET) != SOCK_OK)
			{
				return;
			}
			printf("TCP Server: listening on port %d\r\n", tcp_server_port);
			break;
		}
		
		case SOCK_CLOSED:
		{
			// 关闭后重新打开socket
			close(TCP_SERVER_SOCKET);
			if(socket(TCP_SERVER_SOCKET, Sn_MR_TCP, tcp_server_port, 0x00) != TCP_SERVER_SOCKET)
			{
				return;
			}
			break;
		}
		
		case SOCK_LISTEN:
		{
			// 等待客户端连接，硬件自动处理
			break;
		}
		
		case SOCK_ESTABLISHED:
		{
			// 检查连接中断（首次进入时获取客户端信息）
			if(getSn_IR(TCP_SERVER_SOCKET) & Sn_IR_CON)
			{
				setSn_IR(TCP_SERVER_SOCKET, Sn_IR_CON);
				getSn_DIPR(TCP_SERVER_SOCKET, tmp_ip);
				tmp_port = getSn_DPORT(TCP_SERVER_SOCKET);
				memcpy(tcp_server_client_ip, tmp_ip, 4);
				tcp_server_client_port = tmp_port;
				printf("TCP Server: client connected - %d.%d.%d.%d:%d\r\n",
					tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3], tmp_port);
			}
			
			// 接收客户端数据
			if(getSn_RX_RSR(TCP_SERVER_SOCKET) > 0)
			{
				uint8_t recv_buf[256];
				int32_t recv_len = recv(TCP_SERVER_SOCKET, recv_buf, sizeof(recv_buf) - 1);
				if(recv_len > 0)
				{
					recv_buf[recv_len] = '\0';
					printf("TCP Server recv(%d): %s\r\n", recv_len, recv_buf);
					
					// 回显数据给客户端 (Echo)
					send(TCP_SERVER_SOCKET, recv_buf, recv_len);
				}
			}
			break;
		}
		
		default:
			break;
	}
}

static void network_udp_proc(void)
{
	uint8_t udp_state_now;
	uint8_t recv_buf[256];
	uint8_t remote_ip[4];//临时保存解析出来的ip地址
	uint16_t remote_port;//临时保存解析出来的端口
	int32_t recv_len;

	char id[16];

	udp_state_now = getSn_SR(UDP_SOCKET);

	if(udp_state_now == SOCK_CLOSED)
	{
		close(UDP_SOCKET);
		if(socket(UDP_SOCKET, Sn_MR_UDP, udp_server_port, 0x00) != UDP_SOCKET)
		{
			return;
		}
		printf("UDP Server: listening on port %d\r\n", udp_server_port);
		return;
	}

	if(udp_state_now == SOCK_UDP && getSn_RX_RSR(UDP_SOCKET) > 0)
	{
		recv_len = recvfrom(UDP_SOCKET, recv_buf, sizeof(recv_buf) - 1, udp_client_ip, &udp_client_port);
		if(recv_len > 0)
		{
			printf("UDP recv from %d.%d.%d.%d:%d -> %s\r\n",udp_client_ip[0], udp_client_ip[1], udp_client_ip[2], udp_client_ip[3], udp_client_port, recv_buf);

			/*这里udp收到数据的时候虽然可以直接获取发送设备的ip和端口号 但是不直接使用 只使用解析出来的参数*/
			recv_buf[recv_len] = '\0';
			// memcpy(udp_client_ip, remote_ip, 4);
			// udp_client_port = remote_port;
			int proc_ret = udp_message_proc(recv_buf , id , remote_ip , &remote_port);
			if(proc_ret >= 0)
			{
				switch(proc_ret)
				{
					case NETWORK_TCP_CLIENT:
					{
						if(udp_tcp_client_connect_request_ack_proc(id , remote_ip , remote_port) > 0)
						{
							/*这里只有当发送完成后 才会将解析出来的ip和端口 赋值给tcp连接的目标端口 若收到的端口与当前的不同 则重置状态*/
							if((memcmp(socket_0_tcp_dest_ip, remote_ip, 4) != 0) || (socket_0_tcp_dest_port != remote_port))
							{
								printf("udp receive new tcp client ip connect request , ip:%d:%d:%d:%d , port%d\r\n" ,remote_ip[0] , remote_ip[1] , remote_ip[2] , remote_ip[3] ,remote_port  );
								memcpy(socket_0_tcp_dest_ip, remote_ip, sizeof(socket_0_tcp_dest_ip));
								socket_0_tcp_dest_port = remote_port;

								/* 重置TCP客户端连接，下一次循环会重新发起连接 */
								close(TCP_SOCKET);
								tcp_send_flag = 0;

								lfs_user_save_tcp_client_dest_info(socket_0_tcp_dest_ip , socket_0_tcp_dest_port);
							}
						}
						break;
					}

					case NETWORK_TCP_SERVER:
					{

						break;
					}
				}
			}
		}
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
		network_tcp_server_proc(); // TCP服务器处理
		network_udp_proc();        // UDP服务器处理
	}
}
