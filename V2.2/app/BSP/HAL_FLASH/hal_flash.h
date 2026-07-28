#ifndef _HAL_FLASH_H_
#define _HAL_FLASH_H_

#include "stdint.h"


#define APP_START_ADDRESS 0x08000000+10*1024  //Ó¦ÓÃ³ÌĞòÆğÊ¼µØÖ· ¸ù¾İÊµ¼ÊÇé¿öĞŞ¸Ä
#define APP_SIZE          20*1024  //Ó¦ÓÃ³ÌĞò´óĞ¡ ¸ù¾İÊµ¼ÊÇé¿öĞŞ¸Ä
#define DOWNLOAD_START_ADDRESS APP_START_ADDRESS + APP_SIZE  //ÏÂÔØÇøÆğÊ¼µØÖ· ¸ù¾İÊµ¼ÊÇé¿öĞŞ¸Ä

#define RAM_START_ADDR          0x20000000    // RAMÆğÊ¼µØÖ·
#define RAM_SIZE                64*1024    // RAMµÄ´óĞ¡

//ÒÔÏÂºê¶¨ÒåÖ÷ÒªÊÇÓÃÓÚflash²Ù×÷Ê±µÄµØÖ·Ğ£Ñé
#define FLASH_BASE_ADDR        0x08000000   // FlashÆğÊ¼µØÖ·
#define FLASH_TOTAL_SIZE       64 * 1024    // Flash×Ü´óĞ¡
#define PAGE_SIZE              1024 // Flashé¡µå¤§å°ï¼ˆSTM32F103C8T6 æ¯é¡µ 1KBï¼ŒåŸå€¼ 2048 å¯¼è‡´æ“¦é™¤ç®—é”™åœ°å€ï¼‰
#define FLASH_MAX_ADDR         (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE - 1) // Flash×î´óµØÖ·

uint8_t flash_write(uint32_t address, uint8_t *data, uint32_t size); 
uint8_t flash_erase(uint32_t address, uint32_t size);
uint8_t flash_read(uint32_t address, uint8_t *data, uint32_t size);








#endif
