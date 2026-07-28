#include "app_w25qxx.h"
#include "spi.h"
#include "main.h"

#define USER_FLASH_START    0x00000000U
static w25qxx_handle_t gs_handle;        /**< w25qxx handle */

#define W25QXX_HANDLE   hspi2
#define W25QXX_CS_PIN_PORT  SPI2_CS_GPIO_Port
#define W25QXX_CS_PIN_PIN   SPI2_CS_Pin
#define w25qxx_spi_debug printf
uint8_t w25qxx_spi_init(void)
{
	
	return 0;
}

uint8_t w25qxx_spi_deinit(void)
{
	
	return 0;
}

static uint8_t spi_send(uint8_t *buf , uint16_t len)
{
    return HAL_SPI_Transmit(&W25QXX_HANDLE, buf, len, 1000);
}

static uint8_t spi_recv(uint8_t *buf , uint16_t len)
{
    return HAL_SPI_Receive(&W25QXX_HANDLE, buf, len, 1000);
}

static uint8_t spi_cs_set(uint8_t flag)
{
    HAL_GPIO_WritePin(W25QXX_CS_PIN_PORT , W25QXX_CS_PIN_PIN , (GPIO_PinState)flag);
	return 0;
}

uint8_t w25qxx_spi_write_read(uint8_t instruction, uint8_t instruction_line,
                                uint32_t address, uint8_t address_line, uint8_t address_len,
                                uint32_t alternate, uint8_t alternate_line, uint8_t alternate_len,
                                uint8_t dummy, uint8_t *in_buf, uint32_t in_len,
                                uint8_t *out_buf, uint32_t out_len, uint8_t data_line)
{
    if ((instruction_line != 0) || (address_line != 0) || (alternate_line != 0) || (dummy != 0) || (data_line != 1))
    {
        return 1;
    }
    
    uint8_t res;
    
    /* set cs low */
    spi_cs_set(0);
    
    /* if in_len > 0 */
    if (in_len > 0)
    {
        /* transmit the input buffer */
        res = spi_send(in_buf, in_len);
        if (res != HAL_OK)
        {
            /* set cs high */
            spi_cs_set(1);
            return 1;
        }
    }
    
    /* if out_len > 0 */
    if (out_len > 0)
    {
        /* transmit to the output buffer */
        res = spi_recv(out_buf, out_len);
        if (res != HAL_OK)
        {
            /* set cs high */
            spi_cs_set(1);
           
            return 1;
        }
    }
    
    /* set cs high */
    spi_cs_set(1);
    
    return 0;
} 

void w25qxx_spi_delayms(uint32_t ms)
{
    HAL_Delay(ms);
}

void w25qxx_spi_delayus(uint32_t ms)
{

}

//void w25qxx_spi_debug(const char *const fmt, ...)
//{
//	char String[100];

//	va_list arg;
//	va_start(arg, fmt);
//	vsprintf(String, fmt, arg);
//	va_end(arg);
//	printf("%s" , String);
//}

uint8_t w25qxx_basic_init(w25qxx_type_t type, w25qxx_interface_t interface, w25qxx_bool_t dual_quad_spi_enable)
{
    uint8_t res;
    
    /* link interface function */
    DRIVER_W25QXX_LINK_INIT(&gs_handle, w25qxx_handle_t);
    DRIVER_W25QXX_LINK_SPI_QSPI_INIT(&gs_handle, w25qxx_spi_init);
    DRIVER_W25QXX_LINK_SPI_QSPI_DEINIT(&gs_handle, w25qxx_spi_deinit);
    DRIVER_W25QXX_LINK_SPI_QSPI_WRITE_READ(&gs_handle, w25qxx_spi_write_read);
    DRIVER_W25QXX_LINK_DELAY_MS(&gs_handle, w25qxx_spi_delayms);
    DRIVER_W25QXX_LINK_DELAY_US(&gs_handle, w25qxx_spi_delayus);
    DRIVER_W25QXX_LINK_DEBUG_PRINT(&gs_handle, w25qxx_spi_debug);
    
    /* set chip type */
    res = w25qxx_set_type(&gs_handle, type);
    if (res != 0)
    {
        w25qxx_spi_debug("w25qxx: set type failed.\n");
       
        return 1;
    }
    
    /* set chip interface */
    res = w25qxx_set_interface(&gs_handle, interface);
    if (res != 0)
    {
        w25qxx_spi_debug("w25qxx: set interface failed.\n");
       
        return 1;
    }
    
    /* set dual quad spi */
    res = w25qxx_set_dual_quad_spi(&gs_handle, dual_quad_spi_enable);
    if (res != 0)
    {
        w25qxx_spi_debug("w25qxx: set dual quad spi failed.\n");
        (void)w25qxx_deinit(&gs_handle);
       
        return 1;
    }
    
    /* chip init */
    res = w25qxx_init(&gs_handle);
    if (res != 0)
    {
        w25qxx_spi_debug("w25qxx: init failed.\n");
       
        return 1;
    }
    else
    {
        if (type >= W25Q256)
        {
            res = w25qxx_set_address_mode(&gs_handle, W25QXX_ADDRESS_MODE_4_BYTE);
            if (res != 0)
            {
                w25qxx_spi_debug("w25qxx: set address mode failed.\n");
                (void)w25qxx_deinit(&gs_handle);
               
                return 1;
            }
        }
        
        return 0;
    }
}

uint8_t app_w25qxx_init(void)
{
    uint8_t res = 1;
    w25qxx_type_t chip_type = W25Q128;
    w25qxx_interface_t interface = W25QXX_INTERFACE_SPI;
    res = w25qxx_basic_init(chip_type, interface, W25QXX_BOOL_FALSE);

    return res;
}

/**
 * @brief     纯页写入（不擦除），供 LFS 的 prog 回调使用
 * @param[in] addr 写入地址
 * @param[in] data 数据缓冲区
 * @param[in] len  写入长度（<= 256，且不能跨页）
 * @return    状态码
 *            - 0 成功
 *            - 1 失败
 * @note      与 app_w25qxx_write 不同，本函数不会自动擦除。
 *            必须先擦除再写入，由 LFS 自己管理擦除周期。
 */
uint8_t app_w25qxx_page_program(uint32_t addr, uint8_t *data, uint16_t len)
{
    addr = addr + USER_FLASH_START;
    return w25qxx_page_program(&gs_handle, addr, data, len);
}

uint8_t app_w25qxx_write(uint32_t addr, uint8_t *data, uint32_t len)
{
    addr = addr + USER_FLASH_START;
	return w25qxx_write(&gs_handle, addr, data, len);
}

uint8_t app_w25qxx_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    addr = addr + USER_FLASH_START;
	return w25qxx_read(&gs_handle, addr, data, len);
}

/**
 * @brief     擦除 W25QXX 指定地址范围的存储空间
 * @param[in] addr 起始地址（无需对齐，函数自动处理）
 * @param[in] len  擦除长度（字节）
 * @return    状态码
 *            - 0 成功
 *            - 1 地址范围超出芯片容量
 *            - 2 擦除失败（底层驱动错误）
 * @note      自动按 64KB → 32KB → 4KB 优先使用大块擦除，效率更高
 *            擦除范围会按扇区/块边界自动对齐
 */
uint8_t app_w25qxx_erase(uint32_t addr, uint32_t len)
{
    uint8_t res;
    uint32_t end_addr;
    uint32_t remaining;

    if (len == 0) return 0;
    addr = addr + USER_FLASH_START;

    end_addr = addr + len;

    /* 校验地址范围 */
    if (end_addr > W25Q128_CAPACITY)
    {
        printf("w25qxx: erase range overflow (0x%08X > 0x%08X).\n",
               (unsigned int)end_addr, (unsigned int)W25Q128_CAPACITY);
        return 1;
    }

    while (addr < end_addr)
    {
        remaining = end_addr - addr;

        /* 优先使用 64KB 块擦除（地址对齐且剩余足够） */
        if ((addr % W25QXX_BLOCK64_SIZE == 0) && (remaining >= W25QXX_BLOCK64_SIZE))
        {
            res = w25qxx_block_erase_64k(&gs_handle, addr);
            if (res != 0)
            {
                printf("w25qxx: block erase 64k failed at 0x%08X.\n", (unsigned int)addr);
                return 2;
            }
            addr += W25QXX_BLOCK64_SIZE;
        }
        /* 其次使用 32KB 块擦除 */
        else if ((addr % W25QXX_BLOCK32_SIZE == 0) && (remaining >= W25QXX_BLOCK32_SIZE))
        {
            res = w25qxx_block_erase_32k(&gs_handle, addr);
            if (res != 0)
            {
                printf("w25qxx: block erase 32k failed at 0x%08X.\n", (unsigned int)addr);
                return 2;
            }
            addr += W25QXX_BLOCK32_SIZE;
        }
        /* 4KB 扇区擦除（通用后备） */
        else
        {
            res = w25qxx_sector_erase_4k(&gs_handle, addr);
            if (res != 0)
            {
                printf("w25qxx: sector erase 4k failed at 0x%08X.\n", (unsigned int)addr);
                return 2;
            }
            addr += W25QXX_SECTOR_SIZE;
        }
    }

    return 0;
}

/**
 * @brief     擦除 W25QXX 整片
 * @return    状态码
 *            - 0 成功
 *            - 1 擦除失败
 * @note      整片擦除比逐扇区擦除快得多，约 40~100s
 */
uint8_t app_w25qxx_chip_erase(void)
{
    uint8_t res;

    printf("w25qxx: chip erase start (may take up to 100s)...\n");

    res = w25qxx_chip_erase(&gs_handle);
    if (res != 0)
    {
        printf("w25qxx: chip erase failed.\n");
        return 1;
    }

    printf("w25qxx: chip erase done.\n");
    return 0;
}
