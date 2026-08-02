#include "bootloader.h"
#include "main.h"
#include "fal.h"
#include "app_flashdb.h"
#include <string.h>
#include "hal_flash.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ BOOTLOADER ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)

/* ---- 升级缓冲区（从 W25Q 读取固件的中转 buffer）---- */
static uint8_t chunk_buf[UPGRADE_CHUNK_SIZE];

/* ============================== CRC32 ============================== */

static uint32_t crc32_calc(uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }
    return ~crc;
}

/* ============================== 升级标志 ============================== */

/**
 * @brief 获取升级标志位
 * @return 1: 需要升级  0: 不需要
 * @note  从 FlashDB 中读取 key="upgrade_flag" 的值
 */
static uint8_t get_upgrade_flag(void)
{
    uint8_t flag = 0;
    app_flashdb_get("upgrade_flag", &flag, sizeof(flag));
    return flag;
}

/**
 * @brief 清除升级标志
 * @note  将 FlashDB 中 key="upgrade_flag" 清零
 */
static void clear_upgrade_flag(void)
{
    uint8_t flag = 0;
    app_flashdb_set("upgrade_flag", &flag, sizeof(flag));
}


/**
 * @brief 从 W25Q download 分区搬运固件到片内 APP 区
 * @note  APP_SIZE(240KB) < download 分区大小(2MB)，只复制前 APP_SIZE 字节
 * @return 0: 成功  1: 失败
 */
static uint8_t upgrade_firmware(void)
{
    const struct fal_partition *dl_part = fal_partition_find(DOWNLOAD_PART_NAME);
    if (dl_part == NULL)
    {
        DEBUG_PRINT("FAL partition '%s' not found", DOWNLOAD_PART_NAME);
        return 1;
    }

    DEBUG_PRINT("upgrade start: w25q '%s' -> internal flash 0x%08X, size=%dKB",
                DOWNLOAD_PART_NAME, (unsigned int)APP_START_ADDRESS, APP_SIZE / 1024);

    /* ---- 1. 擦除片内 APP 区 ---- */
    DEBUG_PRINT("erasing APP area...");
    if (flash_erase(APP_START_ADDRESS, APP_SIZE) != 0)
    {
        DEBUG_PRINT("erase failed");
        return 1;
    }

    /* ---- 2. 分块从 W25Q 读取并写入片内 Flash ---- */
    DEBUG_PRINT("writing firmware...");
    for (uint32_t offset = 0; offset < APP_SIZE; offset += UPGRADE_CHUNK_SIZE)
    {
        uint32_t chunk = ((APP_SIZE - offset) > UPGRADE_CHUNK_SIZE)
                       ? UPGRADE_CHUNK_SIZE
                       : (APP_SIZE - offset);

        /* 从 W25Q download 分区读取 */
        if (fal_partition_read(dl_part, offset, chunk_buf, chunk) < 0)
        {
            DEBUG_PRINT("read W25Q failed at offset=%lu", offset);
            return 1;
        }

        /* 写入片内 Flash */
        if (flash_write(APP_START_ADDRESS + offset, chunk_buf, chunk) != 0)
        {
            DEBUG_PRINT("write internal flash failed at offset=%lu", offset);
            return 1;
        }
    }

    /* ---- 3. 回读校验：对比 W25Q 与片内 Flash 前 256 字节 ---- */
    {
        uint8_t verify_w25q[256];
        uint8_t verify_flash[256];

        if (fal_partition_read(dl_part, 0, verify_w25q, sizeof(verify_w25q)) < 0)
        {
            DEBUG_PRINT("verify: read W25Q failed");
            return 1;
        }
        if (flash_read(APP_START_ADDRESS, verify_flash, sizeof(verify_flash)) != 0)
        {
            DEBUG_PRINT("verify: read internal flash failed");
            return 1;
        }
        if (memcmp(verify_w25q, verify_flash, sizeof(verify_w25q)) != 0)
        {
            DEBUG_PRINT("verify: MISMATCH! W25Q != internal flash");
            for (int i = 0; i < 256; i++)
            {
                if (verify_w25q[i] != verify_flash[i])
                {
                    DEBUG_PRINT("  offset 0x%02X: W25Q=0x%02X, Flash=0x%02X",
                                i, verify_w25q[i], verify_flash[i]);
                    break;
                }
            }
            return 1;
        }
        DEBUG_PRINT("verify: first 256 bytes OK");
    }

    return 0;
}

/* ============================== 跳转 APP ============================== */

static void bootloader_jump_to_app(void)
{
    /* 1. 校验 APP 区是否有有效程序 */
    uint32_t msp_address = *(volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_address = *(volatile uint32_t *)(APP_START_ADDRESS + 4);

    DEBUG_PRINT("MSP=0x%08lX, ResetVector=0x%08lX", msp_address, app_address);

    if (msp_address < RAM_START_ADDR || msp_address >= (RAM_START_ADDR + RAM_SIZE))
    {
        DEBUG_PRINT("invalid MSP: 0x%08lX, stay in bootloader", msp_address);
        return;
    }
    if (app_address < APP_START_ADDRESS || app_address >= (APP_START_ADDRESS + APP_SIZE))
    {
        DEBUG_PRINT("invalid reset vector: 0x%08lX, stay in bootloader", app_address);
        return;
    }

    /* 2. 关闭 bootloader 使用的外设，避免干扰 APP 启动 */
    /* 停止 TIM1（bootloader 的 timebase），否则 APP 开中断后会触发 Default_Handler 死循环 */
    TIM1->CR1  &= ~TIM_CR1_CEN;
    TIM1->DIER &= ~TIM_DIER_UIE;
    NVIC_DisableIRQ(TIM1_UP_IRQn);
		DEBUG_PRINT("DISABLE TIM1\r\n");
		
    /* 复位 SysTick 到安全状态 */
    SysTick->CTRL  = 0;
    SysTick->LOAD  = 0;
    SysTick->VAL   = 0;
		DEBUG_PRINT("DISABLE systick\r\n");

    /* 清除所有挂起的中断 */
    NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
    NVIC_ClearPendingIRQ(SysTick_IRQn);
		DEBUG_PRINT("DISABLE NVIC\r\n");

    /* 3. 关全局中断，设置 MSP，跳转 */
    __disable_irq();
		DEBUG_PRINT("DISABLE IRQ\r\n");

    __set_MSP(msp_address);
		DEBUG_PRINT("SET MSP\r\n");

    SCB->VTOR = APP_START_ADDRESS;
		DEBUG_PRINT("SET VTOR\r\n");

    DEBUG_PRINT("jumping to 0x%08lX...", app_address);
    void (*app_entry)(void) = (void (*)(void))app_address;
    app_entry();

    /* 理论上不会到这里 */
    while (1) {}
}

/* ============================== 入口 ============================== */

void bootloader_poll(void)
{
    /* 检查是否需要升级 */
    if (get_upgrade_flag() == 1)
    {
        DEBUG_PRINT("upgrade flag detected, start firmware upgrade");

        if (upgrade_firmware() == 0)
        {
            /* 升级成功，清除标志 */
            clear_upgrade_flag();
            DEBUG_PRINT("upgrade done, clear flag, jumping to app");
        }
        else
        {
            /* 升级失败，不清除标志，尝试跳转（可能旧固件仍可用） */
            DEBUG_PRINT("upgrade failed, try to jump to existing app");
        }
    }

    /* 跳转到 APP */
    bootloader_jump_to_app();

    /* 跳转失败，停留在 bootloader */
    DEBUG_PRINT("jump failed, stay in bootloader");
    while (1)
    {
        /* 闪烁 LED 或打印日志表示跳转失败 */
        HAL_Delay(500);
    }
}
