#include "INA226.h"
#include "i2c.h"
#include "env.h"

#define INA226_I2C_TIMEOUT           100   /*!< I2C 通信超时时间(ms) */
#define INA226_I2C_ADDR 						 0x40

#define INA226_FACTORY_ID						 0x5449
#define INA226_CHIP_ID						   0x2260

#define INA226_RSHUNT_OHM            0.001f		//采样电阻阻值0.001Ω
#define INA226_CURRENT_LSB_A         0.0001f  //电流 LSB = 0.1mA, 满量程 3.2768A
#define INA226_POWER_LSB_W           0.0025f  //功率 LSB = 2.5mW (25 x Current_LSB)
#define INA226_VBUS_LSB_V            0.00125f  //总线电压 LSB = 1.25mV
#define INA226_VSHUNT_LSB_V          0.0000025f//分流电压 LSB = 2.5uV

/* CONFIG: AVG=64次, VBUS转换1.1ms, VSH转换1.1ms, PGA +-40.96mV, 连续测量分流+总线 */
#define INA226_CONFIG_VALUE          0x3907
#define INA226_CAL_VALUE             0xC800    /* 51200 */

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ INA226 ]"
#include "debug_print.h"

static INA226_Status_t INA226_ReadRegs(I2C_HandleTypeDef *hi2c,uint8_t reg,uint8_t *pData,uint16_t len)
{
    if(hi2c == NULL || pData == NULL)
    {
        return INA226_ERROR;
    }
    HAL_StatusTypeDef halStatus;
    uint16_t devAddr = (INA226_I2C_ADDR << 1);

    halStatus = HAL_I2C_Mem_Read(hi2c,devAddr,reg,I2C_MEMADD_SIZE_8BIT,pData,len,INA226_I2C_TIMEOUT);

    if (halStatus == HAL_OK)
    {
        return INA226_OK;
    }
    else if (halStatus == HAL_TIMEOUT)
    {
        return INA226_TIMEOUT;
    }
    else
    {
        return INA226_ERROR;
    }
}

static INA226_Status_t INA226_WriteRegs(I2C_HandleTypeDef *hi2c,uint8_t reg, uint16_t val)
{
    if(hi2c == NULL)
    {
        return INA226_ERROR;
    }
    HAL_StatusTypeDef halStatus;
    uint16_t devAddr = (INA226_I2C_ADDR << 1);
		uint8_t buf[2];
    buf[0] = (val >> 8) & 0xFF;
    buf[1] = val & 0xFF;
		
    halStatus = HAL_I2C_Mem_Write(hi2c, devAddr, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, INA226_I2C_TIMEOUT);

    if (halStatus == HAL_OK)
    {
        return INA226_OK;
    }
    else if (halStatus == HAL_TIMEOUT)
    {
        return INA226_TIMEOUT;
    }
    else
    {
        return INA226_ERROR;
    }
}


void INA226_Init(void)
{
	uint16_t chipID , factoryID;
	uint8_t buf[2];
	INA226_Status_t ret = INA226_ReadRegs(&hi2c1 , INA226_CHIP_ID_REG , buf , 2);
	if(ret == INA226_OK)
	{
		chipID = (uint16_t)(buf[0] << 8 | buf[1]);
	}
	else
	{
		
	}

	ret = INA226_ReadRegs(&hi2c1 , INA226_FACTORY_ID_REG , buf , 2);
	if(ret == INA226_OK)
	{
		factoryID = (uint16_t)(buf[0] << 8 | buf[1]);
	}
	else
	{
		
	}
	if(ret == INA226_OK)
	{
		DEBUG_PRINT("INA226 Init Finish , Factory:%c%c ,ID:0x%04x , CHIP ID:0x%04x\r\n" ,(factoryID >> 8)&0xff , factoryID & 0xff , factoryID , chipID);
    if (INA226_WriteRegs(&hi2c1, INA226_SET_REG, INA226_CONFIG_VALUE) != INA226_OK ||INA226_WriteRegs(&hi2c1, INA226_CAL_REG, INA226_CAL_VALUE) != INA226_OK)
    {
        DEBUG_PRINT("INA226 config write failed\r\n");
    }
		else
		{
				DEBUG_PRINT("INA226 CONFIG success, CONFIG=0x%04x CAL=0x%04x\r\n",INA226_CONFIG_VALUE, INA226_CAL_VALUE);
		}
	}
	else
	{
		DEBUG_PRINT("INA226 Init Fail\r\n");
	}
}

INA226_Status_t INA226_ReadData(void)
{
    uint8_t buf[2];
    uint16_t raw;
    int16_t sraw;
    INA226_Status_t status;

    status = INA226_ReadRegs(&hi2c1, INA226_CURRENT_F_REG, buf, 2);
    if (status != INA226_OK) return status;
    sraw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    board_info.shuntVoltage = (float)sraw * INA226_VSHUNT_LSB_V;

    status = INA226_ReadRegs(&hi2c1, INA226_VOLTAGE_REG, buf, 2);
    if (status != INA226_OK) return status;
    raw = ((uint16_t)buf[0] << 8) | buf[1];
    board_info.busVoltage = (float)raw * INA226_VBUS_LSB_V;

    status = INA226_ReadRegs(&hi2c1, INA226_POWER_REG, buf, 2);
    if (status != INA226_OK) return status;
    sraw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    board_info.power = (float)sraw * INA226_POWER_LSB_W;

    status = INA226_ReadRegs(&hi2c1, INA226_CURRENT_REG, buf, 2);
    if (status != INA226_OK) return status;
    sraw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    board_info.current = (float)sraw * INA226_CURRENT_LSB_A;

//		DEBUG_PRINT("ina226 shuntVoltage:%.3f   busVoltage:%.3f   power:%.3f   current:%.3f\r\n" , board_info.shuntVoltage , board_info.busVoltage , board_info.power , board_info.current);
    return INA226_OK;
}
