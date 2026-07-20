#include "app_w5500_udp.h"
#include "app_w5500.h"
#include "lfs_user.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ UDP ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)


/* UDP Server (Socket 1) */
static uint16_t udp_server_port = 9988;         // UDP服务器监听端口
static uint8_t udp_client_ip[4] = {0};           // 最近一次收到UDP数据的客户端IP
static uint16_t udp_client_port = 0;             // 最近一次收到UDP数据的客户端端口

/**
 * 
 * @param socket_id tcp server 对应的socket
 * @param buf 发送数据
 * @param len 数据长度
 * @param ip 目标IP
 * @param port 目标端口
 * @brief udp server专用的发送函数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-19 19:58:36
 * @copyright Copyright (c) 2026
 */
static int app_w5500_udp_send(uint8_t socket_id , uint8_t *buf , uint16_t len , uint8_t *ip , uint16_t port)
{	
	int ret = -1;

	ret = sendto(socket_id, (uint8_t *)buf, len, ip, port);
	return ret;
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
		if(sscanf(ip_js->valuestring , "%d.%d.%d.%d" , (int *)&receive_ip[0] , (int *)&receive_ip[1] , (int *)&receive_ip[2] , (int *)&receive_ip[3]) == 4)
		{
			memcpy(dest_ip , receive_ip , sizeof(receive_ip));
			*dest_port = port_js->valueint;
			ret = 1;
		}
		else 
		{
			DEBUG_PRINT("ip parse fail\r\n");
			ret = -1;
		}

	}
	else {
		DEBUG_PRINT("receive connect format error\r\n");
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
 * @brief tcp client连接应答的报文
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
		cJSON_AddStringToObject(root , "network" , "UDP");
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
			ret = app_w5500_udp_send(UDP_SOCKET, (uint8_t *)str, strlen(str), IP, port);
			DEBUG_PRINT("send connect ack : %s\r\n" , str);

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
 * @param id 消息id
 * @param IP 目标ip
 * @param port 目标端口号
 * @return int 
 * @brief tcp server连接应答报文
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-14 17:14:40
 * @copyright Copyright (c) 2026
 */
static int udp_tcp_server_connect_request_ack_proc(char *id , uint8_t *IP , uint16_t port)
{
	int ret = -1;
    wiz_NetInfo net_info;
    wizchip_getnetinfo(&net_info); // Get net info 
	char ip[16];
	sprintf(ip , "%d.%d.%d.%d" , net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);

	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		ret = -1;
	}
	else 
	{
		/*会上报本机地址 以及tcp server监听的端口号*/
		cJSON_AddStringToObject(root , "type" , TCP_SERVER_CONNECT_REQUEST);
		cJSON_AddStringToObject(root , "msg" , "success");
		cJSON_AddStringToObject(root , "ip" , ip);
		cJSON_AddNumberToObject(root , "port" , tcp_server_port);
		cJSON_AddNumberToObject(root , "code" , 200);

		char *str = cJSON_PrintUnformatted(root);
		if(str == NULL)
		{
			ret = -2;
		}
		else 
		{
			ret = app_w5500_udp_send(UDP_SOCKET, (uint8_t *)str, strlen(str), IP, port);
			DEBUG_PRINT("send connect ack : %s\r\n" , str);

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

static int udp_error_ack(char *id , uint8_t *IP , uint16_t port , char *msg)
{
	int ret = -1;
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		ret = -1;
	}
	else 
	{
		cJSON_AddStringToObject(root , "network" , "UDP");
		cJSON_AddStringToObject(root , "id" , id);
		cJSON_AddStringToObject(root , "msg" , msg);
		cJSON_AddNumberToObject(root , "code" , 0);

		char *str = cJSON_PrintUnformatted(root);
		if(str == NULL)
		{
			ret = -2;
		}
		else 
		{
			ret = app_w5500_udp_send(UDP_SOCKET, (uint8_t *)str, strlen(str), IP, port);
			DEBUG_PRINT("send error ack : %s\r\n" , str);
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
	int ret = 1;
	cJSON *root = cJSON_Parse((char *)buf);
	if(root == NULL)
	{
		DEBUG_PRINT("message parse fail!\r\n");
		ret = -1;
	}
	else 
	{
		/*先获取消息类型 与 消息id 这两个是必须有的*/
		cJSON *message_type_js = cJSON_GetObjectItem(root , "type");
		cJSON *message_id_js = cJSON_GetObjectItem(root , "id");
		if(message_id_js != NULL)
		{
			memcpy(message_id , message_id_js->valuestring , strlen(message_id_js->valuestring));
			message_id[strlen(message_id_js->valuestring)] = '\0';

		}
		else
		{
			ret = -2;
		}
		if(message_type_js != NULL && ret == 1)
		{
			DEBUG_PRINT("recv type:%s  %s\r\n" , message_type_js->valuestring , TCP_SERVER_CONNECT_REQUEST);
			if(strcmp(message_type_js->valuestring , TCP_CLIENT_CONNECT_REQUEST) == 0)//tcp客户端连接请求消息
			{
				int ip_int[4];
				cJSON *ip_js = cJSON_GetObjectItem(root , "ip");
				if(ip_js != NULL)
				{
					sscanf(ip_js->valuestring , "%d.%d.%d.%d" , &ip_int[0] ,&ip_int[1] ,&ip_int[2] ,&ip_int[3] );
					memcpy(dest_ip , ip_int , sizeof(uint8_t) * 4);
				}
				
				cJSON *port_js = cJSON_GetObjectItem(root , "port");
				if(port_js != NULL)
				{
					(*dest_port) = port_js->valueint;
				}	
				ret = NETWORK_TCP_CLIENT;
			}
			else if(strcmp(message_type_js->valuestring , TCP_SERVER_CONNECT_REQUEST) == 0)//tcp服务端连接请求消息
			{

				ret = NETWORK_TCP_SERVER;
			}
			else //其他消息 后期扩展
			{
				ret = 0xff;
			}
		}
		else
		{
			ret = -4;
			DEBUG_PRINT("message type unknow\r\n");
		}
	}
	if(root != NULL)
	{
		cJSON_Delete(root);
	}
	return ret;
}

void network_udp_proc(void)
{
	uint8_t udp_state_now;
	uint8_t recv_buf[256];
	uint8_t remote_ip[4];//临时保存解析出来的ip地址
	uint16_t remote_port;//临时保存解析出来的端口
	int32_t recv_len;

	char id[32];

	udp_state_now = getSn_SR(UDP_SOCKET);

	if(udp_state_now == SOCK_CLOSED)
	{
		close(UDP_SOCKET);
		if(socket(UDP_SOCKET, Sn_MR_UDP, udp_server_port, 0x00) != UDP_SOCKET)
		{
			return;
		}
		DEBUG_PRINT("listening on port %d\r\n", udp_server_port);
		return;
	}

	if(udp_state_now == SOCK_UDP && getSn_RX_RSR(UDP_SOCKET) > 0)
	{
		recv_len = recvfrom(UDP_SOCKET, recv_buf, sizeof(recv_buf) - 1, udp_client_ip, &udp_client_port);
		if(recv_len > 0)
		{
			DEBUG_PRINT("recv from %d.%d.%d.%d:%d -> %s\r\n",udp_client_ip[0], udp_client_ip[1], udp_client_ip[2], udp_client_ip[3], udp_client_port, recv_buf);

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
						if(udp_tcp_client_connect_request_ack_proc(id , udp_client_ip , udp_client_port) > 0)
						{
							/*这里只有当发送完成后 才会将解析出来的ip和端口 赋值给tcp连接的目标端口 若收到的端口与当前的不同 则重置状态*/
							if((memcmp(tcp_client_dest_ip, remote_ip, 4) != 0) || (tcp_client_dest_port != remote_port))
							{
								DEBUG_PRINT("receive new tcp client ip info , ip:%d:%d:%d:%d , port%d\r\n" ,remote_ip[0] , remote_ip[1] , remote_ip[2] , remote_ip[3] ,remote_port  );
								memcpy(tcp_client_dest_ip, remote_ip, sizeof(tcp_client_dest_ip));
								tcp_client_dest_port = remote_port;

								/* 重置TCP客户端连接，下一次循环会重新发起连接 */
								close(TCP_SOCKET);
//								tcp_send_flag = 0;

								lfs_user_save_tcp_client_dest_info(tcp_client_dest_ip , tcp_client_dest_port);
							}
						}
						break;
					}

					case NETWORK_TCP_SERVER:
					{
						if(memcmp(tcp_server_connect_ip , udp_client_ip , sizeof(udp_client_ip)) == 0)//若收到信息的ip与tcp server连接的ip一致 则不作为应答
						{
						
						}
						else
						{
							udp_tcp_server_connect_request_ack_proc(id , udp_client_ip , udp_client_port);
						}
						break;
					}

					default: //错误类型
					{
						udp_error_ack(id , udp_client_ip , udp_client_port  , "error net type");
						break;
					}
				}
			}
			else
			{
				udp_error_ack(id , udp_client_ip , udp_client_port  , "error info");
			}
		}
	}
}
