#ifndef _INA226_H_
#define _INA226_H_
#include "main.h"

#define INA226_SET_REG						0x00
#define INA226_CURRENT_F_REG			0x01
#define INA226_VOLTAGE_REG				0x02
#define INA226_POWER_REG					0x03
#define INA226_CURRENT_REG				0x04
#define INA226_CAL_REG						0x05
#define INA226_ENABLE_REG					0x06
#define INA226_SET_ALARM_REG			0x07
#define INA226_FACTORY_ID_REG			0xFE
#define INA226_CHIP_ID_REG				0xFF

typedef enum
{
    INA226_OK       = 0x00,  //操作成功
    INA226_ERROR    = 0x01,  //操作失败
    INA226_TIMEOUT  = 0x02,  //超时
    INA226_BUSY     = 0x03   //设备忙
} INA226_Status_t;

void INA226_Init(void);
INA226_Status_t INA226_ReadData(void);





#endif
