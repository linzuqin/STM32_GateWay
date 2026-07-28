#include "bootloader.h"
#include "main.h"
#include <string.h>

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ BOOTLOADER ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)

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

//获取升级标志位函�? 需要自定义 1:需要升�? 0:不需要升�?
static uint8_t get_upgrade_flag(void) 
{

    return 0; // 临时返回0，需�?定义
}

//清除升级标志位函�? 需要自定义
static void clear_upgrade_flag(void) 
{

}

//关闭全局�?�?函数
static void disable_interrupts(void) 
{
    __disable_irq();
}

//开�?全局�?�?函数
static void enable_interrupts(void) 
{
    __enable_irq();
}

//设置msp栈顶指针函数
static void set_msp(uint32_t addr) 
{
    __set_MSP(addr);
}

//读取flash函数 实现 0:成功 1:失败
static uint8_t flash_read(uint32_t address, uint8_t *data, uint32_t size) 
{
    if (address < FLASH_BASE_ADDR || (address + size) > (FLASH_MAX_ADDR + 1) || address == 0 || size == 0)
    {
        return 1;
    }

    disable_interrupts();

    const volatile uint8_t *flash_src = (const volatile uint8_t *)address;//const�?为了保护指向地址的内容不�?�?�? volatile�?为了保证在�?�取的时候不�?编译器优�?
    memcpy(data, (const void *)flash_src, size);
   
    enable_interrupts();
    return 0; // 读取成功
}

//擦除flash函数 实现 0:成功 1:失败
static uint8_t flash_erase(uint32_t address, uint32_t size) 
{
    //1.对操作地址大小进�?�校�?
    if (address < FLASH_BASE_ADDR || (address + size) > (FLASH_MAX_ADDR + 1) || address == 0 || size == 0)
    {
        return 1;
    }

    //2.关闭�?�? 防�?�在执�?�flash操作的时候�??�?�?打断
    disable_interrupts();

    //3.执�?�flash操作
    HAL_StatusTypeDef hal_status = HAL_OK;
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};

    uint32_t offset = address - FLASH_BASE_ADDR;
    uint32_t page_start_addr = FLASH_BASE_ADDR + ((offset / PAGE_SIZE) * PAGE_SIZE);
    uint32_t page_end_addr = FLASH_BASE_ADDR + (((offset + size - 1) / PAGE_SIZE) * PAGE_SIZE);
    uint32_t nb_pages = ((page_end_addr - page_start_addr) / PAGE_SIZE) + 1;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = page_start_addr;
    EraseInitStruct.NbPages = nb_pages;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR | FLASH_FLAG_BSY);

    hal_status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    if (hal_status != HAL_OK)
    {
        HAL_FLASH_Lock();
        enable_interrupts();
        return 1;
    }

    HAL_FLASH_Lock();

    //4.开�?�?�?
    enable_interrupts();
    return 0; // 成功    
}

//写入flash函数 实现 0:成功 1:失败
static uint8_t flash_write(uint32_t address, uint8_t *data, uint32_t size) 
{
    //1.对操作地址大小进�?�校�?
    if (address < FLASH_BASE_ADDR || (address + size) > (FLASH_MAX_ADDR + 1) || address == 0 || size == 0)
    {
        return 1;
    }

    //2.对操作地址的有效性进行校�?
    if ((address % 2 != 0) || (size % 2 != 0) || data == NULL)
    {
        return 1;
    }

    //3.关闭�?�? 防�?�在执�?�flash操作的时候�??�?�?打断
    disable_interrupts();

    //4.执�?�flash操作
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

    //6.开�?�?�?
    enable_interrupts();
    return 0;
}

static void bootloader_jump_to_app(void)
{
    //1. 获取app区的程序入口
    uint32_t app_address = *(volatile uint32_t *)(APP_START_ADDRESS + 4); //这里�?4�?因为根据stm32的内存划�? �?一�?地址存储的是msp栈顶指针,下一�?地址才是复位�?�?向量

    //校验指向复位向量地址的指针是否在app的范围内
    if(app_address < APP_START_ADDRESS || app_address >= (APP_START_ADDRESS + APP_SIZE)) 
    {
        return;
    }

    //2. 设置msp栈顶指针
    uint32_t msp_address = *(volatile uint32_t *)APP_START_ADDRESS; //获取msp栈顶指针的�?

    //校验msp指针所对应的值是否在RAM的范围内
    if (msp_address < RAM_START_ADDR || msp_address >= RAM_START_ADDR + RAM_SIZE)
    {
        return;
    }

    //3. 设置栈顶指针 并�?�用所有中�?
    disable_interrupts(); //禁用所有中�?
    set_msp(msp_address); //设置栈顶指针

    //4. 跳转到APP入口地址
//    DEBUG_PRINT("jump to app , address:%x\r\n" , app_address);
		SCB->VTOR = FLASH_BASE | 0x00008000U;

    void (*app_entry)(void) = (void (*)(void))app_address;
    app_entry();
}

void bootloader_poll(void)
{
    if (get_upgrade_flag() == 1) 
    {
        disable_interrupts();

        //1. 擦除APP�?
        if(flash_erase(APP_START_ADDRESS, APP_SIZE) != 0)
        {
            enable_interrupts();
            return; 
        }

        //2. 从download区�?�制数据到APP�?
        if(flash_write(APP_START_ADDRESS, (uint8_t *)DOWNLOAD_START_ADDRESS, APP_SIZE) != 0) //写入失败
        {
            enable_interrupts();
            return; 
        }

        //3. crc32校验
        uint8_t *download_data = (uint8_t *)DOWNLOAD_START_ADDRESS;
        uint32_t download_crc = crc32_calc(download_data, APP_SIZE);
        uint32_t app_crc = crc32_calc((uint8_t *)APP_START_ADDRESS, APP_SIZE);
        if (download_crc != app_crc)//校验失败
        {
            enable_interrupts();
            return; 
        }

        //3. 清除升级标志�?
        clear_upgrade_flag();

        //4. 恢�?�中�?
				  SysTick->CTRL  = 0;       // 关闭SysTick定时器、关闭中断
					SysTick->LOAD  = 0;       // 清空重装载值
					SysTick->VAL   = 0;       // 清空当前计数器
        enable_interrupts();
    }
    //5. 执�?�跳�?
    bootloader_jump_to_app();
}
