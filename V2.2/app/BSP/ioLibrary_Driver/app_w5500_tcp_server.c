#include "app_w5500_tcp_server.h"
#include "app_w5500.h"
#include "lfs_user.h"
#include "main.h"
#include "crc32.h"
#include "tiny_md5.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ TCP-SERVER ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)

uint16_t tcp_server_send_interval = 10;//tcp server send interval ,s

/* TCP Server (Socket 2) */
uint16_t tcp_server_port = 8080;          // TCP服务器监听端口
uint8_t tcp_server_connect_ip[4] = {0};

static uint16_t tcp_server_send_flag = 0;
static uint8_t tcp_server_state;

/**
 * 
 * @brief 1秒的回调函数 用来对tcp server主动上报定时
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-19 20:41:05
 * @copyright Copyright (c) 2026
 */
void tcp_server_1s_callback(void)
{
	static uint16_t tcp_send_count = 0;
	tcp_send_count ++;
	if(tcp_send_count % tcp_server_send_interval == 0)
	{
		tcp_server_send_flag  = 1;
	}
}


static void GetDev_ID(char *uid_str)
{
	uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw0();
    uid[2] = HAL_GetUIDw0();
    
    sprintf(uid_str, "%08X%08X%08X", uid[0], uid[1], uid[2]);
}

/**
 * 
 * @param socket_id tcp server 对应的socket
 * @param buf 发送数据
 * @param len 数据长度
 * @param ip 目标IP
 * @param port 目标端口
 * @brief tcp server专用的发送函数 会先计算出要发送的json字符串的md5 然后拆包并新增md5字段 在组包后发送
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-19 19:58:36
 * @copyright Copyright (c) 2026
 */
static int app_w5500_tcp_server_send(uint8_t *buf , uint16_t len , uint8_t *ip , uint16_t port)
{
	int ret = -1;
	uint8_t md5_output[16] = {0};
	if(tcp_server_state == SOCK_ESTABLISHED)
	{
		tiny_md5(buf , len , md5_output);
		cJSON *root = cJSON_Parse((char *)buf);
		if(root == NULL)
		{
			DEBUG_PRINT("send data format error\r\n");
			ret = -2;

		}
		else 
		{
			cJSON_AddStringToObject(root , "md5" , (char *)md5_output);
			char *str = cJSON_PrintUnformatted(root);
			if(str == NULL)
			{
				DEBUG_PRINT("send data malloc fail\r\n");
				ret = -3;
			}
			else
			{
				if (getSn_SR(TCP_SERVER_SOCKET) == SOCK_ESTABLISHED)
				{
					ret = send(TCP_SERVER_SOCKET, (uint8_t *)str, strlen(str));
				}
				else
				{
					ret = -1;
					DEBUG_PRINT("tcp server socket state error\r\n");
				}

				free(str);
			}
			cJSON_Delete(root);
		}
	}
	else 
	{
		DEBUG_PRINT("state error\r\n");
		ret = -1;
	}
	return ret;
}

/**
 * 
 * @return int -1:上报失败
 * @brief tcp server 主动上报数据
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-19 21:41:24
 * @copyright Copyright (c) 2026
 */
static int app_w5500_tcp_server_report(void)
{
	int ret = -1;
	char uid_str[32] = {0};
	static uint32_t msg_id = 1;
	GetDev_ID(uid_str);

	cJSON *root = cJSON_CreateObject();
	cJSON *Digital_js = cJSON_CreateObject();
	cJSON *Dev_info_js = cJSON_CreateObject();
	cJSON *Analog_js = cJSON_CreateObject();

	cJSON_AddNumberToObject(root , "id" , msg_id++);
	cJSON_AddStringToObject(root , "type" , TCP_SERVER_SEND_MSG_REQUEST);
	cJSON_AddStringToObject(root , "network" , "TCP SERVER");

	cJSON_AddStringToObject(Dev_info_js, "UID", uid_str);
	cJSON_AddNumberToObject(Dev_info_js , "temp" , network_report_info.temp);
	cJSON_AddNumberToObject(Dev_info_js , "interval" , tcp_server_send_interval);

	cJSON_AddBoolToObject(Digital_js , "DO1" , (_Bool)network_report_info.digital_out_1);
	cJSON_AddBoolToObject(Digital_js , "DO2" , (_Bool)network_report_info.digital_out_2);
	cJSON_AddBoolToObject(Digital_js , "DI1" , (_Bool)network_report_info.digital_in_1);
	cJSON_AddBoolToObject(Digital_js , "DI2" , (_Bool)network_report_info.digital_in_2);
	cJSON_AddBoolToObject(Digital_js , "Beep" , (_Bool)network_report_info.beep_stae);

	cJSON_AddNumberToObject(Analog_js , "AI1" , network_report_info.analog_in_1);
	cJSON_AddNumberToObject(Analog_js , "AI2" , network_report_info.analog_in_2);
	cJSON_AddNumberToObject(Analog_js , "AO2" , network_report_info.analog_out_1);

	cJSON_AddItemToObject(root, "DevInfo", Dev_info_js);
	cJSON_AddItemToObject(root, "Digital", Digital_js);
	cJSON_AddItemToObject(root, "Analog", Analog_js);

	char *str = cJSON_PrintUnformatted(root);
	ret = app_w5500_tcp_server_send((uint8_t *)str , strlen(str) , tcp_server_connect_ip , tcp_server_port);
	
	if(str != NULL)
	{
		free(str);
	}
	if(root != NULL)
	{
		cJSON_Delete(root);
	}
	return ret;
}

/**
 * 
 * @brief tcp server处理函数 用来执行对应的收发状态机 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-19 20:42:00
 * @copyright Copyright (c) 2026
 */
void network_tcp_server_proc(void)
{
	
	tcp_server_state = getSn_SR(TCP_SERVER_SOCKET);
	
	switch(tcp_server_state)
	{
		case SOCK_CLOSE_WAIT:
		{
			// 客户端主动断开连接
			if(disconnect(TCP_SERVER_SOCKET) != SOCK_OK)
			{
				return;
			}
			memset(tcp_server_connect_ip , 0 , sizeof(tcp_server_connect_ip));
			DEBUG_PRINT("client disconnected\r\n");
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
			DEBUG_PRINT("listening on port %d\r\n", tcp_server_port);
			memset(tcp_server_connect_ip , 0 , sizeof(tcp_server_connect_ip));
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
			memset(tcp_server_connect_ip , 0 , sizeof(tcp_server_connect_ip));
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
				getSn_DIPR(TCP_SERVER_SOCKET, tcp_server_connect_ip);
				DEBUG_PRINT("client connected - %d.%d.%d.%d:%d\r\n",tcp_server_connect_ip[0], tcp_server_connect_ip[1], tcp_server_connect_ip[2], tcp_server_connect_ip[3], tcp_server_port);
			}
			
			// 接收客户端数据
			if(getSn_RX_RSR(TCP_SERVER_SOCKET) > 0)
			{
				uint8_t recv_buf[256];
				int32_t recv_len = recv(TCP_SERVER_SOCKET, recv_buf, sizeof(recv_buf) - 1);
				if(recv_len > 0)
				{
					recv_buf[recv_len] = '\0';
					DEBUG_PRINT("recv(%d): %s\r\n", recv_len, recv_buf);
					
					app_w5500_tcp_server_send(recv_buf, recv_len , tcp_server_connect_ip , tcp_server_port);
					// send(TCP_SERVER_SOCKET, recv_buf, recv_len);
				}
			}

			//触发数据主动上报
			if(tcp_server_send_flag == 1)
			{
				if(app_w5500_tcp_server_report() != -1)
				{
					tcp_server_send_flag = 0;
				}
				else
				{
					DEBUG_PRINT("report fail\r\n");
				}
			}
			break;
		}
		
		default:
			break;
	}
}
