#include "msh.h"
#include "main.h"
#include "usart.h"
#include "app_w25qxx.h"
#include "lfs_user.h"
#include "wiz_platform.h"
#include "app_w5500.h"
#include "temp.h"

#define MCU_UID_BASE    ((uint32_t)0x1FFFF7E8)
typedef struct
{
  uint32_t uid0;
  uint32_t uid1;
  uint32_t uid2;
} mcu_uid_t;
#define MCU_UID         (*(mcu_uid_t *)MCU_UID_BASE)

static uint8_t msh_buf[MSH_CMD_MAX_LEN];
static void msh_help_callback(int argc, char *argv);
static void msh_reboot_callback(int argc, char *argv);
static void msh_test_callback(int argc, char *argv);
static void msh_clean_flash_callback(int argc, char *argv);
static void msh_ls_flash_callback(int argc, char *argv);
static void msh_cat_callback(int argc, char *argv);
static void msh_set_dest_ip_callback(int argc, char *argv);
static void msh_set_dest_port_callback(int argc, char *argv);
static void msh_find_callback(int argc, char *argv);
static void msh_ipconfig_callback(int argc, char *argv);
static void msh_board_info_callback(int argc, char *argv);

static uint8_t msh_recv_flag = 0;
extern uint32_t boot;

static char cmd[MSH_CMD_MAX_LEN] = {0};
static char arg[MSH_PROFILE_MAX_LEN] = {0};

msh_cmd_table_t msh_cmd_table[] = {
	{"help" , "show all msh help" , msh_help_callback},
	{"ls" , "ls <dir> - list directory" , msh_ls_flash_callback},
	{"reboot" , "reboot system" , msh_reboot_callback},
	{"test" , "process test demo" , msh_test_callback},
	{"cat" , "cat <file> - print file content" , msh_cat_callback},
	{"clean_flash" , "clean extern flash" , msh_clean_flash_callback},
	{"find" , "find params" , msh_find_callback},
	{"ipconfig" , "show network config" , msh_ipconfig_callback},
	{"set_dest_ip" , "set dest IP address" , msh_set_dest_ip_callback},
	{"set_dest_port" , "set dest port" , msh_set_dest_port_callback},
	{"board_info" , "get board info params" , msh_board_info_callback}
};

/**
 * 
 * @brief 数据接收函数 
 * @param data 需要处理的数据
 * @param size 数据长度
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-28 15:02:10
 */
void msh_rx_data(uint8_t *data , uint16_t size)
{
    if(size > MSH_CMD_MAX_LEN){
        return;
    }else{
        memcpy(msh_buf , data , size);
			msh_recv_flag = 1;
    }
}

/**
 * 
 * @brief 文本输出函数 需要根据硬件自定义
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-28 15:06:41
 * @copyright Copyright (c) 2026
 */
__attribute__((weak)) void msh_putc(char c)
{
    (void)c;
//    #error please set putc function first
    HAL_UART_Transmit(&huart1 , (uint8_t *)&c , 1 , 1000);
}

/**
 * 
 * @brief 系统重启函数 需要根据硬件自定义
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-28 15:06:51
 * @copyright Copyright (c) 2026
 */
void system_reboot(void)
{
    HAL_NVIC_SystemReset();
}

void msh_printf(const char *fmt, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, fmt);
	vsprintf(String, fmt, arg);
	va_end(arg);
	HAL_UART_Transmit(&huart1, (uint8_t *)String, strlen(String), HAL_MAX_DELAY);

}

static void msh_default_callback(int argc, char *argv)
{
    msh_printf("cmd process finish\r\n");
}

static void msh_help_callback(int argc, char *argv)
{
	uint16_t i =0;
	uint16_t size = sizeof(msh_cmd_table) / sizeof(msh_cmd_table_t);
	if(argv[0] != 0)
	{
		for (i = 0; i < size; i++)
		{
			msh_cmd_table_t *p = &msh_cmd_table[i];
			if (strcmp(p->cmd, argv) == 0)
			{
				msh_printf("%s : %s\r\n" , p->cmd , p->desc);
				break;

			}
		}
		if(i == size)
		{
			msh_printf("未找到指定路径\r\n");
		}
	}
	else
	{
		msh_printf("\r\n");
		msh_printf("================= MSH Command List =================\r\n");
		for (i = 0; i < size; i++)
		{
			msh_cmd_table_t *p = &msh_cmd_table[i];
			if (strcmp(p->cmd, "\r\n") == 0)
				continue;
			msh_printf("  %-12s - %s\r\n", p->cmd, p->desc);
		}
		msh_printf("====================================================\r\n");
	}

}

static void msh_reboot_callback(int argc, char *argv)
{
    msh_printf("system reboot.....\r\n");
    system_reboot();
}

static void msh_test_callback(int argc, char *argv)
{
    msh_printf("syetem test start.....\r\n");
}

static void msh_clean_flash_callback(int argc, char *argv)
{
	app_w25qxx_chip_erase();
	printf("flash earse finish ，reboot...\r\n");
	system_reboot();
}

static void msh_ls_flash_callback(int argc, char *argv)
{
	int ret;
	char list[256];

	if(argv[0] == 0)
	{
		/*无参数时默认列出根目录*/
		ret = lfs_user_listdir("/", list, sizeof(list));
		if (ret >= 0)
			msh_printf("内容:\n%s", list);
		else
			msh_printf("列出根目录失败: err=%d\r\n", ret);
		return;
	}

	/*列出指定目录*/
	char *file = argv;

	ret = lfs_user_isdir(file);
	if (ret != 1)
	{
		msh_printf("%s 目录不存在\r\n", file);
		return;
	}

	memset(list, 0, sizeof(list));
	ret = lfs_user_listdir(file, list, sizeof(list));
	if (ret >= 0)
	{
		msh_printf("%s 内容:\n%s", file, list);
	}
}

static void msh_cat_callback(int argc, char *argv)
{
	int ret;
	uint8_t buf[512];

	/* 检查是否有传入路径 */
	if (argv[0] == 0)
	{
		msh_printf("用法: cat <file>\r\n");
		return;
	}

	char *file = argv;

	/*检查文件是否存在*/
	ret = lfs_user_exist(file);
	if (ret == 0)
	{
		msh_printf("%s 不存在\r\n", file);
		return;
	}

	ret = lfs_user_isdir(file);
	if (ret == 1)
	{
		msh_printf("%s 传入的文件路径异常\r\n", file);
		return;
	}

	/*读取文件内容*/
	memset(buf, 0, sizeof(buf));
	ret = lfs_user_read_file(file, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		msh_printf("读取%s失败: err=%d\r\n", file, ret);
		return;
	}
}

static void msh_set_dest_ip_callback(int argc, char *argv)
{
	if (argv[0] == 0)
	{
		msh_printf("当前目的ip:%d.%d.%d.%d\r\n" , socket_0_tcp_dest_ip[0] , socket_0_tcp_dest_ip[1] , socket_0_tcp_dest_ip[2] , socket_0_tcp_dest_ip[3]);
		msh_printf("用法: set_dest_ip <ip_address>\r\n");
		return;
	}
	int octet[4] = {0};
	int ret = sscanf(argv , "%d.%d.%d.%d" , &octet[0] , &octet[1] , &octet[2] , &octet[3]);
	if(ret != 4)
	{
		msh_printf("IP地址格式错误\r\n");
		return;
	}

	ret = lfs_user_isdir(dir_path);
	if (ret == 1)
	{
		msh_printf("[OK]  确认 %s 是目录\n" , dir_path);
	}
	else
	{
		ret = lfs_user_mkdir(dir_path);
		if (ret != LFS_ERR_OK)
		{
			msh_printf("[FAIL] 创建文件夹 %s 失败: err=%d\n",dir_path , ret);
			return;
		}
	}

	if(lfs_user_exist(dest_ip_path) == 0)
	{
		msh_printf("未检测到%s 创建%s\r\n", dest_ip_path, dest_ip_path);
	}
	uint8_t ip[4] = {(uint8_t)octet[0], (uint8_t)octet[1], (uint8_t)octet[2], (uint8_t)octet[3]};
	ret = lfs_user_write_file(dest_ip_path, ip, sizeof(ip));
	if (ret < 0)
	{
		msh_printf("[FAIL] 写入 %s 失败: err=%d\n", dest_ip_path , ret);
		return;
	}
	msh_printf("ip地址设置成功：%d.%d.%d.%d" , ip[0] , ip[1] , ip[2] , ip[3]);
}

static void msh_set_dest_port_callback(int argc, char *argv)
{
	if (argv[0] == 0)
	{
		msh_printf("当前目的端口:%d\r\n" , socket_0_tcp_dest_port);
		msh_printf("用法: set_dest_port <port>\r\n");
		return;
	}
	uint16_t port = 0;
	int ret = sscanf(argv , "%hu" , &port);
	if(ret != 1)
	{
		msh_printf("端口格式错误\r\n");
		return;
	}

	ret = lfs_user_isdir(dir_path);
	if (ret == 1)
	{
		msh_printf("[OK]  确认 %s 是目录\n" , dir_path);
	}
	else
	{
		ret = lfs_user_mkdir(dir_path);
		if (ret != LFS_ERR_OK)
		{
			msh_printf("[FAIL] 创建文件夹 %s 失败: err=%d\n",dir_path , ret);
			return;
		}
	}

	if(lfs_user_exist(dest_port_path) == 0)
	{
		msh_printf("未检测到%s 创建%s\r\n", dest_port_path, dest_port_path);
	}
	ret = lfs_user_write_file(dest_port_path, &port, sizeof(port));
	if (ret < 0)
	{
		msh_printf("[FAIL] 写入 %s 失败: err=%d\n", dest_port_path , ret);
		return;
	}
	
	msh_printf("端口设置成功：%d" , socket_0_tcp_dest_port);

}

static void msh_find_callback(int argc, char *argv)
{
	if (argv[0] == 0)
	{
		msh_printf("用法: find <params>\r\n");
		msh_printf("for example:find ip\r\n");
		return;
	}

	if(strstr(&argv[0] , "ip"))
	{
		msh_printf("当前目的ip:%d.%d.%d.%d\r\n" , socket_0_tcp_dest_ip[0] , socket_0_tcp_dest_ip[1] , socket_0_tcp_dest_ip[2] , socket_0_tcp_dest_ip[3]);
	}
	else if(strstr(&argv[0] , "port"))
	{
		msh_printf("当前目的端口:%d\r\n" , socket_0_tcp_dest_port);
	}
	else if(strstr(&argv[0] , "boot"))
	{
		msh_printf("当前开机次数:%d\r\n" , boot);
	}
	else
	{
		msh_printf("参数异常\r\n");
	}
}

static void msh_ipconfig_callback(int argc, char *argv)
{
    wiz_NetInfo info;
    wizchip_getnetinfo(&info);
    msh_printf(" MAC         : %02X:%02X:%02X:%02X:%02X:%02X\r\n",info.mac[0], info.mac[1], info.mac[2],info.mac[3], info.mac[4], info.mac[5]);
    msh_printf(" IP          : %d.%d.%d.%d\r\n",info.ip[0], info.ip[1], info.ip[2], info.ip[3]);
    msh_printf(" Subnet Mask : %d.%d.%d.%d\r\n",info.sn[0], info.sn[1], info.sn[2], info.sn[3]);
    msh_printf(" Gateway     : %d.%d.%d.%d\r\n",info.gw[0], info.gw[1], info.gw[2], info.gw[3]);
    msh_printf(" DNS         : %d.%d.%d.%d\r\n",info.dns[0], info.dns[1], info.dns[2], info.dns[3]);
    msh_printf(" DHCP        : %s\r\n",info.dhcp == NETINFO_DHCP ? "DHCP" : "Static");
    msh_printf("--------------------------------------------\r\n");
    msh_printf(" Dest IP     : %d.%d.%d.%d\r\n",socket_0_tcp_dest_ip[0], socket_0_tcp_dest_ip[1],socket_0_tcp_dest_ip[2], socket_0_tcp_dest_ip[3]);
    msh_printf(" Dest Port   : %d\r\n", socket_0_tcp_dest_port);
}

static void msh_sudo_callback(char *buf)
{
    if(strstr(buf ,"reboot")!= NULL)
    {
        msh_printf("force reboot\r\n");
        msh_reboot_callback(0 , NULL);
    }
    else if(strstr(buf , "clean_flash") != NULL)
    {
        msh_printf("force clean_flash\r\n");
        msh_clean_flash_callback(0 , NULL);
    }
    else
    {
        msh_printf("未知管理员指令\r\n");
    }
}

static void msh_board_info_callback(int argc, char *argv)
{
    RCC_ClkInitTypeDef clk_cfg;
    uint32_t flash_latency;
    // 读取系统时钟配置
    HAL_RCC_GetClockConfig(&clk_cfg, &flash_latency);

    // 计算各总线时钟 MHz
    uint32_t sysclk_mhz = clk_cfg.SYSCLKSource / 1000000U;
    uint32_t hclk_mhz   = clk_cfg.AHBCLKDivider / 1000000U;
    uint32_t pclk1_mhz  = clk_cfg.APB1CLKDivider / 1000000U;
    uint32_t pclk2_mhz  = clk_cfg.APB2CLKDivider / 1000000U;

    // 读取内部芯片温度
    float chip_temp = board_temp_get();

    // 读取STM32原厂96位唯一UID
    mcu_uid_t uid = MCU_UID;

    msh_printf("=================== BOARD INFO ===================\r\n");
    msh_printf("Hardware Model:    STM32F103RET6-W5500-V1.0\r\n");
    msh_printf("MCU Part Number:   STM32F103RET6\r\n");
    msh_printf("Chip Unique UID:   %08X-%08X-%08X\r\n", uid.uid0, uid.uid1, uid.uid2);
    msh_printf("Boot Counter:      %lu\r\n", boot);
    msh_printf("MCU Internal Temp: %.2f ℃\r\n", chip_temp);
    msh_printf("\r\nSystem Clock Info:\r\n");
    msh_printf("SYSCLK Core:       %lu MHz\r\n", sysclk_mhz);
    msh_printf("AHB HCLK:          %lu MHz\r\n", hclk_mhz);
    msh_printf("APB1 PCLK1:        %lu MHz\r\n", pclk1_mhz);
    msh_printf("APB2 PCLK2:        %lu MHz\r\n", pclk2_mhz);
    msh_printf("Flash Latency WS:  %lu\r\n", flash_latency);
    msh_printf("\r\nTCP Client Default Config:\r\n");
    msh_printf("Dest TCP IP:       %d.%d.%d.%d\r\n",
        socket_0_tcp_dest_ip[0], socket_0_tcp_dest_ip[1],
        socket_0_tcp_dest_ip[2], socket_0_tcp_dest_ip[3]);
    msh_printf("Dest TCP Port:     %u\r\n", socket_0_tcp_dest_port);
    msh_printf("===================================================\r\n");
}

/**
 * 
 * @brief msh处理函数放在线程中循环调用
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-28 15:13:24
 * @copyright Copyright (c) 2026
 */
void msh_process(void)
{
	if(msh_recv_flag == 0)
	{
		return;
	}
	msh_recv_flag = 0;
	size_t len = strlen((const char *)msh_buf);
	if (len == 0 || strspn((const char *)msh_buf, " \t\r\n") == len) {
			msh_printf("\r\nmsh> ");
			memset(msh_buf, 0, sizeof(msh_buf));
			return;
	}

	arg[0] = 0;
	sscanf((char *)msh_buf , "%s %s" , cmd , arg);
	uint16_t size = sizeof(msh_cmd_table) / sizeof(msh_cmd_table_t);
	for (uint16_t i = 0; i < size; i++)
	{
		if(strstr((char *)msh_buf , "sudo") != NULL)
		{
			msh_sudo_callback((char *)&msh_buf[0 + strlen("sudo ")]);
			memset(msh_buf, 0, sizeof(msh_buf));
		}
		else if (strcmp(cmd, msh_cmd_table[i].cmd) == 0)
		{
				if (msh_cmd_table[i].callback != NULL)
				{
					msh_cmd_table[i].callback(0, arg);

				}
				else
				msh_default_callback(0, msh_cmd_table[i].cmd);
				memset(msh_buf, 0, sizeof(msh_buf));
				msh_printf("\r\nmsh> ");
				break;
		}

	}
}

void msh_init(void)
{
	msh_printf("MSH initialized.\r\n");
	msh_printf("msh> ");
}
