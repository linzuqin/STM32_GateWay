/*
 * @Description:定义了与modbus设备相关的参数
 * @Author: linzuqin
 * @Date: 2025-11-10 16:20:49
 * @LastEditTime: 2025-11-27 15:27:37
 * @LastEditors: linzuqin
 */
#ifndef _MB_H_
#define _MB_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MB_MAX_SIZE        512
#define MB_MIN_SIZE        4

#define MB_SLAVE_NUM        2
#define MB_MASTER_NUM        1

#define MAX_ERROR_COUNT    10
// 错误码定义
#define MB_EXCEPTION_ILLEGAL_FUNCTION      0x01
#define MB_EXCEPTION_ILLEGAL_DATA_ADDRESS  0x02
#define MB_EXCEPTION_ILLEGAL_DATA_VALUE    0x03
#define MB_EXCEPTION_SLAVE_DEVICE_FAILURE  0x04


// modbus从机寄存器大小定义
#define MB_S_COIL_SIZE        125            // 线圈寄存器大小
#define MB_S_DISC_SIZE        125            // 离散量寄存器大小
#define MB_S_HOLD_SIZE        125             // 保持寄存器大小
#define MB_S_INPUT_SIZE       125             // 输入寄存器大小

// modbus主机寄存器大小定义
#define MB_M_COIL_SIZE        125            // 线圈寄存器大小
#define MB_M_DISC_SIZE        125            // 离散量寄存器大小
#define MB_M_HOLD_SIZE        125             // 保持寄存器大小
#define MB_M_INPUT_SIZE       125             // 输入寄存器大小

// 功能码类型定义
typedef uint8_t mb_func_code_t;

#define MB_FUNC_READ_COILS        ((mb_func_code_t)0x01)
#define MB_FUNC_READ_DISCRETE     ((mb_func_code_t)0x02)
#define MB_FUNC_READ_HOLDING      ((mb_func_code_t)0x03)
#define MB_FUNC_READ_INPUT        ((mb_func_code_t)0x04)
#define MB_FUNC_WRITE_SINGLE_COIL ((mb_func_code_t)0x05)
#define MB_FUNC_WRITE_SINGLE_REGISTER ((mb_func_code_t)0x06)
#define MB_FUNC_WRITE_MULTIPLE_COILS ((mb_func_code_t)0x0F)
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS ((mb_func_code_t)0x10)

#define MB_ADDR_BIT          0                  // modbus帧中地址对应的bit位
#define MB_FUNC_BIT          1                  // modbus帧中功能码对应的bit位
#define MB_REGH_ADDR_BIT     2                  // modbus帧中寄存器起始地址高位对应的bit位
#define MB_REGL_ADDR_BIT     3                  // modbus帧中寄存器起始地址低位对应的bit位
#define MB_REGH_COUNT_BIT    4                  // 寄存器数量高位
#define MB_REGL_COUNT_BIT    5                  // 寄存器数量低位
#define MB_BYTE_COUNT_BIT    6                  // 字节数

#define MB_VALUEH_BIT    4                  // 写入寄存器值高位
#define MB_VALUEL_BIT    5                  // 写入寄存器值低位

//modbus错误码
typedef enum
{
    PARSE_OK = 0x00,
    Illegal_Function = 0x01,
    Illegal_Data_Address = 0x02,
    Illegal_Data_Value = 0x03,
    Slave_Device_Failure = 0x04,
    Acknowledge = 0x05,
    Slave_Device_Busy = 0x06,
    Negative_Acknowledge = 0x07,
    Memory_Parity_Error = 0x08,
}mb_err_code_t;

//modbus设备类型
typedef enum
{
    MB_MASTER = 0,
    MB_SLAVE,
} mb_dev_type_t;

//modbus函数返回值
typedef enum
{
    MB_OK = 0,
    MB_ERR_SIZE,
    MB_ERR_CRC,
    MB_ERR_ADDR,
    MB_ERR_FUNC,
    MB_ERR_DATA,
    MB_ERR_DEVICE,
    MB_ERR_TIMEOUT,
    MB_ERR_BUILD,
    MB_ERR_MEMORY,
} mb_err_t;

//这个结构体是主机部分特有 主要是为了能够根据标志位自动执行下发的操作 
typedef struct
{
    uint16_t value;
    uint8_t setFlag;
}mb_m_map;

typedef enum
{
    MB_IDLE = 0,
    MB_PARSE,//从机接收到modbus设备信息 正在解析
    MB_RESP,//从机的应答
    MB_GET,//主机的读取操作
    MB_SET,//主机的写入操作
    MB_ERROR,//modbus设备通信发生错误
    MB_OFFLINE,
    MB_WAIT,
}mb_state_t;

typedef struct{
    void (*write)(uint8_t *buf , uint16_t len);
    void (*read)(uint8_t *buf , uint16_t len);
    uint32_t (*handle)(void);
}mb_opt;

typedef struct
{
    // 寄存器数据指针
    uint8_t  *mb_coil_reg;
    uint8_t  *mb_disc_reg;
    uint16_t *mb_hold_reg;
    uint16_t *mb_input_reg;

    // 寄存器配置
    uint16_t coil_start_addr;
    uint16_t coil_read_size;
    uint16_t disc_start_addr;
    uint16_t disc_read_size;
    uint16_t hold_start_addr;
    uint16_t hold_read_size;
    uint16_t input_start_addr;
    uint16_t input_read_size;
		uint8_t setFlag;
}mb_reg_info;

typedef struct
{
    uint8_t addr;
    uint8_t uartid;
    mb_dev_type_t dev_type;

    mb_reg_info *reg_info;

    // 状态与错误
    uint32_t error_count;
    mb_state_t mb_state;

    // 收发缓冲区
    uint8_t  rx_buffer[MB_MAX_SIZE];
    uint8_t  tx_buffer[MB_MAX_SIZE];

    uint16_t rx_size;
    uint16_t tx_size;

    // 回调函数
    mb_opt *opt;
    void (*coil_write_cb)(uint16_t addr, uint16_t val);
    void (*hold_write_cb)(uint16_t addr, uint16_t val);

    // 主机写映射表
    mb_m_map *coil_map;
    mb_m_map *hold_map;

    uint32_t tick;
    mb_func_code_t index_func_code;
    uint8_t dev_online;
} mb_dev_t;



uint16_t usMBCRC16( uint8_t * pucFrame, uint16_t usLen );
uint32_t mb_get_tick(void);
void mb_clean(mb_dev_t *dev);
mb_err_t mb_data_get(mb_dev_t *mb_devs , uint8_t uart_id , uint8_t *data_buf , uint16_t data_len);


#endif /* _MB_H_ */
