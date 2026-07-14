#include "wiz_platform.h"
#include "spi.h"
#include "lfs_user.h"

extern uint8_t tcp_send_flag;

wiz_NetInfo default_net_info = {
    .mac = {0x00, 0x08, 0xdc, 0x12, 0x22, 0x12},
    .ip = {192, 168, 60, 30},
    .gw = {192, 168, 60, 1},
    .sn = {255, 255, 255, 0},
    .dns = {8, 8, 8, 8},
    .dhcp = NETINFO_DHCP}; 
uint8_t ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {0};


static void wizchip_version_check(void)
{
    uint8_t error_count = 0;
    while (1)
    {
        HAL_Delay(1000);
        if (getVERSIONR() != W5500_VERSION)
        {
            error_count++;
            if (error_count > 5)
            {
                printf("error, %s version is 0x%02x, but read %s version value = 0x%02x\r\n", _WIZCHIP_ID_, W5500_VERSION, _WIZCHIP_ID_, getVERSIONR());
                while (1)
                    ;
            }
        }
        else
        {
					printf("w5500 version is :%d" , getVERSIONR());
            break;
        }
    }
}

static void wizchip_select(void)
{
	 HAL_GPIO_WritePin(W5500_CS_GPIO_Port,W5500_CS_Pin , GPIO_PIN_RESET);
}

static void wizchip_deselect(void)
{
	 HAL_GPIO_WritePin(W5500_CS_GPIO_Port,W5500_CS_Pin , GPIO_PIN_SET);
}

static uint8_t wizchip_read_byte(void)
{
	uint8_t data = 0;
	HAL_SPI_Receive(&hspi1, &data, sizeof(uint8_t),HAL_MAX_DELAY);
	
	return data;
}

static void wizchip_write_byte(uint8_t dat)
{
	 HAL_SPI_Transmit(&hspi1, &dat, sizeof(uint8_t), HAL_MAX_DELAY);
}

static void wizchip_read_buff(uint8_t *buf, uint16_t len)
{
    uint16_t idx = 0;
    for (idx = 0; idx < len; idx++)
    {
        buf[idx] = wizchip_read_byte();
    }
}

static void wizchip_write_buff(uint8_t *buf, uint16_t len)
{
    uint16_t idx = 0;
    for (idx = 0; idx < len; idx++)
    {
        wizchip_write_byte(buf[idx]);
    }
}

static void wizchip_cris_enter(void)
{
    __disable_irq();
}

static void wizchip_cris_exit(void)
{
    __enable_irq();
}

static void wizchip_spi_cb_reg(void)
{
    reg_wizchip_cris_cbfunc(wizchip_cris_enter, wizchip_cris_exit);
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read_byte, wizchip_write_byte);
    reg_wizchip_spiburst_cbfunc(wizchip_read_buff, wizchip_write_buff);
}

static void wizchip_reset(void)
{
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port,W5500_RST_Pin , GPIO_PIN_SET);
    HAL_Delay(60);
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port,W5500_RST_Pin , GPIO_PIN_RESET);
    HAL_Delay(60);
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port,W5500_RST_Pin , GPIO_PIN_SET);
    HAL_Delay(60);
}

/*��·��ʼ��*/
void wizchip_initialize(void)
{
    uint8_t txsize[_WIZCHIP_SOCK_NUM_] = {2, 2, 2, 2, 2, 2, 2, 2};
    uint8_t rxsize[_WIZCHIP_SOCK_NUM_] = {2, 2, 2, 2, 2, 2, 2, 2};

    /* reg wizchip spi */
    wizchip_spi_cb_reg();

    /* Reset the wizchip */
    wizchip_reset();

    /* Initialize wizchip socket TX/RX buffer
       CRITICAL: Without this, Sn_TXBUF_SIZE/Sn_RXBUF_SIZE default to 0,
       causing getSn_TxMAX() to return 0 and all send operations to fail. */
    wizchip_init(txsize, rxsize);

    /* Read version register */
    wizchip_version_check();

    /* Check PHY link status, causes PHY to start normally */
//    wiz_phy_link_check();

}


/*������ʼ��*/
static uint8_t wiz_dhcp_process(uint8_t sn, uint8_t *buffer)
{
    wiz_NetInfo conf_info;
    uint8_t dhcp_run_flag = 1;
    uint8_t dhcp_ok_flag = 0;
    /* Registration DHCP_time_handler to 1 second timer */
    DHCP_init(sn, buffer);
    printf("DHCP running\r\n");
    while (1)
    {
        switch (DHCP_run()) // Do the DHCP client
        {
        case DHCP_IP_LEASED: // DHCP Acquiring network information successfully
        {
            if (dhcp_ok_flag == 0)
            {
                dhcp_ok_flag = 1;
                dhcp_run_flag = 0;
            }
            break;
        }
        case DHCP_FAILED:
        {
            dhcp_run_flag = 0;
            break;
        }
        }
        if (dhcp_run_flag == 0)
        {
            printf("DHCP %s!\r\n", dhcp_ok_flag ? "success" : "fail");
            DHCP_stop();

            /*DHCP obtained successfully, cancel the registration DHCP_time_handler*/

            if (dhcp_ok_flag)
            {
                getIPfromDHCP(conf_info.ip);
                getGWfromDHCP(conf_info.gw);
                getSNfromDHCP(conf_info.sn);
                getDNSfromDHCP(conf_info.dns);
                conf_info.dhcp = NETINFO_DHCP;
                getSHAR(conf_info.mac);
                wizchip_setnetinfo(&conf_info); // Update network information to network information obtained by DHCP
                return 1;
            }
            return 0;
        }
    }
}

static void print_network_information(void)
{
    wiz_NetInfo net_info;
    wizchip_getnetinfo(&net_info); // Get chip configuration information

    if (net_info.dhcp == NETINFO_DHCP)
    {
        printf("====================================================================================================\r\n");
        printf(" %s network configuration : DHCP\r\n\r\n", _WIZCHIP_ID_);
    }
    else
    {
        printf("====================================================================================================\r\n");
        printf(" %s network configuration : static\r\n\r\n", _WIZCHIP_ID_);
    }

    printf(" MAC         : %02X:%02X:%02X:%02X:%02X:%02X\r\n", net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5]);
    printf(" IP          : %d.%d.%d.%d\r\n", net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
    printf(" Subnet Mask : %d.%d.%d.%d\r\n", net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
    printf(" Gateway     : %d.%d.%d.%d\r\n", net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
    printf(" DNS         : %d.%d.%d.%d\r\n", net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3]);
    printf("====================================================================================================\r\n\r\n");
}

void network_init(void)
{
    int ret;
    wizchip_setnetinfo(&default_net_info); // Configuring Network Information
    if (default_net_info.dhcp == NETINFO_DHCP)
    {
        ret = wiz_dhcp_process(0, ethernet_buf);
        if (ret == 0)
        {
            default_net_info.dhcp = NETINFO_STATIC;
            wizchip_setnetinfo(&default_net_info);
        }
    }
    print_network_information();
		
//		setSn_KPALVTR(TCP_SOCKET, 6); // 30s keepalive
//		setSn_KPALVTR(UDP_SOCKET, 6); // 30s keepalive

}
