#ifndef _HAL_FLASH_H_
#define _HAL_FLASH_H_

#include "stdint.h"


#define FLASH_BASE_ADDR        0x08000000      // Flash 起始地址
#define BOOTLOADER_SIZE        (64 * 1024)     // Bootloader 占用 32KB
#define APP_START_ADDRESS      (FLASH_BASE_ADDR + BOOTLOADER_SIZE)  // APP 起始地址 0x08010000
#define APP_SIZE               (448 * 1024)    // APP 最大 240KB
#define FLASH_TOTAL_SIZE       (512 * 1024)    // 片内 Flash 总大小 512KB
#define FLASH_MAX_ADDR         (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE - 1)
#define PAGE_SIZE              2048            // 片内 Flash 页大小（STM32F1 为 2KB）

#define RAM_START_ADDR          0x20000000
#define RAM_SIZE                (64 * 1024)    // STM32F103RE 有 64KB SRAM

#define DOWNLOAD_START_ADDRESS APP_START_ADDRESS + APP_SIZE  //下载区起始地址 根据实际情况修改

#define DOWNLOAD_PART_NAME     "download"      // W25Q 中存放固件的分区名

#define UPGRADE_CHUNK_SIZE     4096

uint8_t flash_write(uint32_t address, uint8_t *data, uint32_t size); 
uint8_t flash_erase(uint32_t address, uint32_t size);
uint8_t flash_read(uint32_t address, uint8_t *data, uint32_t size);








#endif
