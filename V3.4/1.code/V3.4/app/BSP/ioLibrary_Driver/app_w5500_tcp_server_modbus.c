#include "app_w5500_tcp_server_modbus.h"
#include "app_w5500.h"
#include "lfs_user.h"
#include "main.h"
#include "crc32.h"
#include "tiny_md5.h"
#include "mb_slave_app.h"
#include "main.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ TCP-SERVER-MB-S ]"
#include "debug_print.h"


/* TCP Server (Socket 2) */
uint16_t tcp_server_modbus_slave_port = 2222;          // TCP服务器监听端口
uint8_t tcp_server_modbus_slave_ip[4] = {0};

static uint8_t tcp_server_state;

/**
 * 
 * @brief tcp server处理函数 用来执行对应的收发状态机 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-19 20:42:00
 * @copyright Copyright (c) 2026
 */
void network_tcp_server_modbus_slave_proc(void)
{
	
	tcp_server_state = getSn_SR(TCP_SERVER_MODBUS_SLAVE_SOCKET);
	
	switch(tcp_server_state)
	{
		case SOCK_CLOSE_WAIT:
		{
			// 客户端主动断开连接
			if(disconnect(TCP_SERVER_MODBUS_SLAVE_SOCKET) != SOCK_OK)
			{
				return;
			}
			memset(tcp_server_modbus_slave_ip , 0 , sizeof(tcp_server_modbus_slave_ip));
			DEBUG_PRINT("client disconnected\r\n");
			break;
		}
		
		case SOCK_INIT:
		{
			// 开始监听
			setSn_KPALVTR(TCP_SERVER_MODBUS_SLAVE_SOCKET, socket_manage[TCP_SERVER_MODBUS_SLAVE_SOCKET].interval);
			if(listen(TCP_SERVER_MODBUS_SLAVE_SOCKET) != SOCK_OK)
			{
				return;
			}
			DEBUG_PRINT("listening on port %d\r\n", tcp_server_modbus_slave_port);
			memset(tcp_server_modbus_slave_ip , 0 , sizeof(tcp_server_modbus_slave_ip));
			break;
		}
		
		case SOCK_CLOSED:
		{
			// 关闭后重新打开socket
			close(TCP_SERVER_MODBUS_SLAVE_SOCKET);
			if(socket(TCP_SERVER_MODBUS_SLAVE_SOCKET, Sn_MR_TCP, tcp_server_modbus_slave_port, 0x00) != TCP_SERVER_MODBUS_SLAVE_SOCKET)
			{
				
			}
			else
			{
				setSn_KPALVTR(TCP_SERVER_MODBUS_SLAVE_SOCKET, socket_manage[TCP_SERVER_MODBUS_SLAVE_SOCKET].interval);
				memset(tcp_server_modbus_slave_ip , 0 , sizeof(tcp_server_modbus_slave_ip));				
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
			if(getSn_IR(TCP_SERVER_MODBUS_SLAVE_SOCKET) & Sn_IR_CON)
			{
				setSn_IR(TCP_SERVER_MODBUS_SLAVE_SOCKET, Sn_IR_CON);
				getSn_DIPR(TCP_SERVER_MODBUS_SLAVE_SOCKET, tcp_server_modbus_slave_ip);
				DEBUG_PRINT("client connected - %d.%d.%d.%d:%d\r\n",tcp_server_modbus_slave_ip[0], tcp_server_modbus_slave_ip[1], tcp_server_modbus_slave_ip[2], tcp_server_modbus_slave_ip[3], tcp_server_modbus_slave_port);
			}
			
			// 接收客户端数据
			if(getSn_RX_RSR(TCP_SERVER_MODBUS_SLAVE_SOCKET) > 0)
			{
				uint8_t recv_buf[256];
				int32_t recv_len = recv(TCP_SERVER_MODBUS_SLAVE_SOCKET, recv_buf, sizeof(recv_buf) - 1);
				if(recv_len > 0)
				{
					recv_buf[recv_len] = '\0';
//					DEBUG_PRINT("recv(%d): %s\r\n", recv_len, recv_buf);
					
					mb_data_get(mb_slave_devs , 0xff , recv_buf , recv_len);
					// send(TCP_SERVER_MODBUS_SLAVE_SOCKET, recv_buf, recv_len);
				}
			}

			break;
		}
		
		default:
			break;
	}
}
