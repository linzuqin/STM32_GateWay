#include "app_w5500.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ W5500 ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)

network_report_info_t network_report_info;

static int phy_state  = 0;

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
		setSn_KPALVTR(TCP_SOCKET, 30);       // 30s keepalive
		setSn_KPALVTR(UDP_SOCKET, 30);       // 30s keepalive
		setSn_KPALVTR(TCP_SERVER_SOCKET, 30); // 30s keepalive
		setSn_KPALVTR(MQTT_SOCKET, 30);
		setSn_KPALVTR(DNS_SOCKET, 30);

		close(TCP_SOCKET);
		close(UDP_SOCKET);
		close(TCP_SERVER_SOCKET);
		close(MQTT_SOCKET);
		close(DNS_SOCKET);

	}
	else
	{
//			printf("PHY no link\r\n");
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
		network_tcp_client_proc();
		network_tcp_server_proc(); // TCP服务器处理
		network_udp_proc();        // UDP服务器处理
		app_w5500_mqtt_proc();
	}
}
