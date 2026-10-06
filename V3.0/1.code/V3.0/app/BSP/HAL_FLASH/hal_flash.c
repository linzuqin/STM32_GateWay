#include "hal_flash.h"
#include "main.h"
#include "string.h"

//关闭全局中断函数
static void disable_interrupts(void) 
{
     __disable_irq();
}

//开启全局中断函数
static void enable_interrupts(void) 
{
     __enable_irq();
}

//读取flash函数 实现 0:成功 1:失败
uint8_t flash_read(uint32_t address, uint8_t *data, uint32_t size) 
{
    if (address < FLASH_BASE_ADDR || (address + size) > (FLASH_MAX_ADDR + 1) || size == 0)
    {
        return 1;
    }

    disable_interrupts();

    const volatile uint8_t *flash_src = (const volatile uint8_t *)address;//const是为了保护指向地址的内容不被修改 volatile是为了保证在读取的时候不被编译器优化
    memcpy(data, (const void *)flash_src, size);
    
    enable_interrupts();
    return 0; // 读取成功
}

uint8_t flash_erase(uint32_t address, uint32_t size)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    uint32_t PageError = 0;
    uint32_t page_start_addr, page_end_addr, nb_pages;
    uint8_t ret = 1;

    if (size == 0 || address == 0)
    {
        return 1;
    }
    if (address < FLASH_BASE_ADDR || (address + size) > (FLASH_MAX_ADDR + 1))
    {
        return 1;
    }
    if ((address % PAGE_SIZE) != 0)
    {
        return 1;
    }

    disable_interrupts();

    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY));
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    page_start_addr = address;
    page_end_addr = address + size - 1;
    nb_pages = ((page_end_addr - page_start_addr) / PAGE_SIZE) + 1;

    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = page_start_addr;
    EraseInitStruct.NbPages     = nb_pages;

    HAL_FLASH_Unlock();

    hal_status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    HAL_FLASH_Lock();

    if (hal_status == HAL_OK)
    {
        ret = 0;
    }

    /* 确保 Flash 操作完成，刷新指令预取缓冲 */
    __DSB();
    __ISB();

    enable_interrupts();
    return ret;
}

//写入flash函数 实现 0:成功 1:失败
uint8_t flash_write(uint32_t address, uint8_t *data, uint32_t size) 
{
    //1.对操作地址大小进行校验
    if (address < FLASH_BASE_ADDR || (address + size) > (FLASH_MAX_ADDR + 1) || size == 0)
    {
        return 1;
    }

    //2.对操作地址的有效性进行校验
    if ((address % 2 != 0) || (size % 2 != 0) || data == NULL)
    {
        return 1;
    }

    //3.关闭中断 防止在执行flash操作的时候被中断打断
    disable_interrupts();

    //4.执行flash操作
    HAL_StatusTypeDef hal_status = HAL_OK;

    uint32_t write_addr = address;
    uint16_t *p_halfword = (uint16_t *)data;
    uint32_t halfword_count = size / 2;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    for (uint32_t i = 0; i < halfword_count; i++)
    {
        hal_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, write_addr, p_halfword[i]);

        if (hal_status != HAL_OK)
        {
            HAL_FLASH_Lock();
            enable_interrupts();
            return 1;
        }

        write_addr += 2;
    }

    HAL_FLASH_Lock();

    /* 确保 Flash 操作完成，刷新指令预取缓冲 */
    __DSB();
    __ISB();

    //6.开启中断
    enable_interrupts();
    return 0;
}
