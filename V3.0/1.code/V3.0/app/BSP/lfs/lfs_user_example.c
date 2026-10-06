/**
 * @file    lfs_user_example.c
 * @brief   littlefs + hal_flash STM32F103C8T6 使用示例
 * @note    配置和底层驱动已封装在 lfs_user.c 中，本例仅演示如何使用 API
 */

#include "lfs_user.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 写一个字符串到文件，再读出来对比
 */
static int test_read_write(void)
{
    const char *test_str = "Hello from STM32F103C8T6 + littlefs!";
    char        buf[64]  = {0};
    int         ret;

    ret = lfs_user_write_file("hello.txt", test_str, strlen(test_str));
    if (ret < 0)
    {
        printf("[FAIL] lfs_user_write_file: err=%d\n", ret);
        return ret;
    }
    printf("[OK]  写入 %d 字节\n", ret);

    ret = lfs_user_read_file("hello.txt", buf, sizeof(buf));
    if (ret < 0)
    {
        printf("[FAIL] lfs_user_read_file: err=%d\n", ret);
        return ret;
    }
    printf("[OK]  读取 %d 字节: %s\n", ret, buf);

    if (strcmp(test_str, buf) != 0)
    {
        printf("[FAIL] 数据对比不一致!\n");
        return -1;
    }
    printf("[OK]  数据对比一致\n");

    return 0;
}

/**
 * @brief 测试写入一个结构体并读取
 */
typedef struct
{
    uint32_t count;
    uint32_t crc;
    char     tag[16];
} sys_config_t;

static int test_struct(void)
{
    sys_config_t cfg_write =
    {
        .count = 42,
        .crc   = 0xA5A5A5A5,
        .tag   = "littlefs",
    };
    sys_config_t cfg_read = {0};
    int ret;

    ret = lfs_user_write_file("config.bin", &cfg_write, sizeof(cfg_write));
    if (ret < 0)
        return ret;

    ret = lfs_user_read_file("config.bin", &cfg_read, sizeof(cfg_read));
    if (ret < 0)
        return ret;

    printf("[OK]  结构体: count=%lu, crc=0x%08lX, tag=%s\n",
           (unsigned long)cfg_read.count,
           (unsigned long)cfg_read.crc,
           cfg_read.tag);
    return 0;
}

/*=============================================================================
 * main 函数
 *============================================================================*/

int main(void)
{
    int ret;

    /* HAL 库初始化（STM32 标准启动流程） */
    HAL_Init();
    SystemClock_Config();  /* 用户需实现时钟配置 */

    /* 初始化串口（用于 printf 输出） */
    MX_USART1_UART_Init(); /* 用户需实现串口初始化 */

    printf("\n===== littlefs STM32F103C8T6 Demo (hal_flash) =====\n\n");

    /* ---- 1. 初始化文件系统（使用 lfs_user.c 内部的配置） ---- */
    ret = lfs_info_init();
    if (ret != LFS_ERR_OK)
    {
        Error_Handler();
    }

    /* ---- 2. 基本读写测试 ---- */
    ret = test_read_write();
    if (ret != 0) goto exit;

    printf("\n");

    /* ---- 3. 结构体读写测试 ---- */
    ret = test_struct();
    if (ret != 0) goto exit;

    printf("\n");

    /* ---- 4. 创建文件夹 + 写入文件测试（实现在 lfs_user.c 中） ---- */
    ret = test_folder_and_file();
    if (ret != 0) goto exit;

exit:
    /* ---- 5. 反初始化 ---- */
    lfs_user_deinit();
    printf("\n[OK]  文件系统已卸载\n");
    printf("===== Demo 结束 =====\n");

    while (1)
    {
        /* 空循环或进入低功耗 */
    }
}

/**
 * @brief  HAL 断言失败回调
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Assert failed: %s, line %lu\n", (char *)file, (unsigned long)line);
    while (1);
}
