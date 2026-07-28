#ifndef _APP_W25QXX_H_
#define _APP_W25QXX_H_
#include "driver_w25qxx.h"

/* W25Q128 容量及擦除单位 */
#define W25Q128_CAPACITY        16777216    /* 16 MB */
#define W25QXX_SECTOR_SIZE      4096        /* 4 KB （最小擦除单位） */
#define W25QXX_BLOCK32_SIZE     32768       /* 32 KB */
#define W25QXX_BLOCK64_SIZE     65536       /* 64 KB */

uint8_t app_w25qxx_init(void);
uint8_t app_w25qxx_page_program(uint32_t addr, uint8_t *data, uint16_t len);
uint8_t app_w25qxx_write(uint32_t addr, uint8_t *data, uint32_t len);
uint8_t app_w25qxx_read(uint32_t addr, uint8_t *data, uint32_t len);
uint8_t app_w25qxx_erase(uint32_t addr, uint32_t len);
uint8_t app_w25qxx_chip_erase(void);

#endif
