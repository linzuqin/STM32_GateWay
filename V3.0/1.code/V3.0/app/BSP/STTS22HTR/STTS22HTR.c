/**
  ******************************************************************************
  * @file           : STTS22HTR.c
  * @brief          : STTS22HTR 温度传感器驱动函数
  * @description    : 基于 STM32 HAL 库的 I2C 驱动
  *                   硬件接口: I2C1 - SCL:PB6, SDA:PB7
  *                   ADDR引脚接GND, 7位I2C地址: 0x3F
  *                   主控芯片: STM32F103RET6
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "STTS22HTR.h"
#include <stdint.h>
#include "i2c.h"
#include "Filter_manager.h"

#define DEBUG_ENABLE    1
#define DEBUG_LOG "[ STTS22HTR ]"
#include "debug_print.h"

#define STTS22HTR_I2C_TIMEOUT           100   /*!< I2C 通信超时时间(ms) */

/* 阈值转换公式: reg = (temp_C / 0.64f) + 63 */
#define STTS22HTR_TEMP_TO_REG(temp)     ((uint8_t)(((temp) / 0.64f) + 63.0f))

float temp = 0.0;

static STTS22HTR_Status_t STTS22HTR_WriteReg(I2C_HandleTypeDef *hi2c,uint8_t reg,uint8_t data);

static STTS22HTR_Status_t STTS22HTR_ReadRegs(I2C_HandleTypeDef *hi2c,uint8_t reg,uint8_t *pData,uint16_t len);

static STTS22HTR_Status_t STTS22HTR_WriteReg(I2C_HandleTypeDef *hi2c,uint8_t reg,uint8_t data)
{
    HAL_StatusTypeDef halStatus;
    uint16_t devAddr = (STTS22HTR_I2C_ADDR << 1);  /* HAL库使用8位地址(左移1位) */

    halStatus = HAL_I2C_Mem_Write(hi2c,devAddr,reg,I2C_MEMADD_SIZE_8BIT,&data,1,STTS22HTR_I2C_TIMEOUT);

    if (halStatus == HAL_OK)
    {
        return STTS22HTR_OK;
    }
    else if (halStatus == HAL_TIMEOUT)
    {
        return STTS22HTR_TIMEOUT;
    }
    else
    {
        return STTS22HTR_ERROR;
    }
}

static STTS22HTR_Status_t STTS22HTR_ReadRegs(I2C_HandleTypeDef *hi2c,uint8_t reg,uint8_t *pData,uint16_t len)
{
    HAL_StatusTypeDef halStatus;
    uint16_t devAddr = (STTS22HTR_I2C_ADDR << 1);

    halStatus = HAL_I2C_Mem_Read(hi2c,devAddr,reg,I2C_MEMADD_SIZE_8BIT,pData,len,STTS22HTR_I2C_TIMEOUT);

    if (halStatus == HAL_OK)
    {
        return STTS22HTR_OK;
    }
    else if (halStatus == HAL_TIMEOUT)
    {
        return STTS22HTR_TIMEOUT;
    }
    else
    {
        return STTS22HTR_ERROR;
    }
}

STTS22HTR_Status_t STTS22HTR_Init(I2C_HandleTypeDef *hi2c)
{
    STTS22HTR_Status_t status;

    if (hi2c == NULL)
    {
        return STTS22HTR_ERROR;
    }

    status = STTS22HTR_CheckWhoAmI(hi2c);
    if (status != STTS22HTR_OK)
    {
        return status;
    }

    status = STTS22HTR_SetMode(hi2c, STTS22HTR_MODE_25HZ, STTS22HTR_AVG_1);
    if (status != STTS22HTR_OK)
    {
        return status;
    }

    return STTS22HTR_OK;
}

STTS22HTR_Status_t STTS22HTR_CheckWhoAmI(I2C_HandleTypeDef *hi2c)
{
    uint8_t whoami = 0;
    STTS22HTR_Status_t status;

    if (hi2c == NULL)
    {
        return STTS22HTR_ERROR;
    }

    status = STTS22HTR_ReadRegs(hi2c,STTS22HTR_REG_WHO_AM_I,&whoami,1);
    if (status != STTS22HTR_OK)
    {
        return status;
    }

    if (whoami != STTS22HTR_WHO_AM_I_VALUE)
    {
        return STTS22HTR_ERROR;
    }

    return STTS22HTR_OK;
}

/**
  * @brief  设置传感器工作模式
  * @note   根据 ODR 模式组合表配置 CTRL 寄存器:
  *         - POWER_DOWN: one_shot=0, freerun=0, low_odr_start=0
  *         - ONE_SHOT:   one_shot=1, freerun=0, low_odr_start=0
  *         - 1Hz:        one_shot=0, freerun=0, low_odr_start=1
  *         - 25Hz:       one_shot=0, freerun=1, low_odr_start=0, avg=00
  *         - 50Hz:       one_shot=0, freerun=1, low_odr_start=0, avg=01
  *         - 100Hz:      one_shot=0, freerun=1, low_odr_start=0, avg=10
  *         - 200Hz:      one_shot=0, freerun=1, low_odr_start=0, avg=11
  * @param  hi2c: I2C 句柄指针
  * @param  mode: 工作模式 (STTS22HTR_Mode_t)
  * @param  avg: 平均采样数 (STTS22HTR_Avg_t)
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_SetMode(I2C_HandleTypeDef *hi2c,STTS22HTR_Mode_t mode,STTS22HTR_Avg_t avg)
{
    uint8_t ctrl = 0;

    if (hi2c == NULL)
    {
        return STTS22HTR_ERROR;
    }

    ctrl |= STTS22HTR_CTRL_BDU;
    ctrl |= STTS22HTR_CTRL_IF_ADD_INC;

    ctrl |= (uint8_t)((uint8_t)avg << STTS22HTR_CTRL_AVG_SHIFT);

    switch (mode)
    {
        case STTS22HTR_MODE_POWER_DOWN:
            break;

        case STTS22HTR_MODE_ONE_SHOT:
            ctrl |= STTS22HTR_CTRL_ONE_SHOT;
            break;

        case STTS22HTR_MODE_1HZ:
            ctrl |= STTS22HTR_CTRL_LOW_ODR_START;
            break;

        case STTS22HTR_MODE_25HZ:
            ctrl |= STTS22HTR_CTRL_FREERUN;
            break;

        case STTS22HTR_MODE_50HZ:
            ctrl |= STTS22HTR_CTRL_FREERUN;
            break;

        case STTS22HTR_MODE_100HZ:
            ctrl |= STTS22HTR_CTRL_FREERUN;
            break;

        case STTS22HTR_MODE_200HZ:
            ctrl |= STTS22HTR_CTRL_FREERUN;
            break;

        default:
            return STTS22HTR_ERROR;
    }

    return STTS22HTR_WriteReg(hi2c, STTS22HTR_REG_CTRL, ctrl);
}

STTS22HTR_Status_t STTS22HTR_TriggerOneShot(I2C_HandleTypeDef *hi2c)
{
    uint8_t ctrl = 0;
    STTS22HTR_Status_t status;

    if (hi2c == NULL)
    {
        return STTS22HTR_ERROR;
    }

    status = STTS22HTR_ReadCtrlReg(hi2c, &ctrl);
    if (status != STTS22HTR_OK)
    {
        return status;
    }

    ctrl |= STTS22HTR_CTRL_ONE_SHOT;

    return STTS22HTR_WriteReg(hi2c, STTS22HTR_REG_CTRL, ctrl);
}

STTS22HTR_Status_t STTS22HTR_ReadRawTemp(I2C_HandleTypeDef *hi2c,int16_t *rawTemp)
{
    uint8_t tempBuf[2] = {0};
    STTS22HTR_Status_t status;

    if ((hi2c == NULL) || (rawTemp == NULL))
    {
        return STTS22HTR_ERROR;
    }

    status = STTS22HTR_ReadRegs(hi2c,
                                STTS22HTR_REG_TEMP_L_OUT,
                                tempBuf,
                                2);
    if (status != STTS22HTR_OK)
    {
        return status;
    }

    *rawTemp = (int16_t)(((uint16_t)tempBuf[1] << 8) | tempBuf[0]);

    return STTS22HTR_OK;
}


STTS22HTR_Status_t STTS22HTR_ReadTemp(I2C_HandleTypeDef *hi2c,float *temperature)
{
    int16_t rawTemp = 0;
    STTS22HTR_Status_t status;

    if ((hi2c == NULL) || (temperature == NULL))
    {
        return STTS22HTR_ERROR;
    }

    status = STTS22HTR_ReadRawTemp(hi2c, &rawTemp);
    if (status != STTS22HTR_OK)
    {
        return status;
    }

    *temperature = (float)rawTemp / 100.0f;

    return STTS22HTR_OK;
}

STTS22HTR_Status_t STTS22HTR_ReadStatus(I2C_HandleTypeDef *hi2c,
                                        uint8_t *status)
{
    if ((hi2c == NULL) || (status == NULL))
    {
        return STTS22HTR_ERROR;
    }

    return STTS22HTR_ReadRegs(hi2c,STTS22HTR_REG_STATUS,status,1);
}

STTS22HTR_Status_t STTS22HTR_IsBusy(I2C_HandleTypeDef *hi2c,uint8_t *isBusy)
{
    uint8_t status = 0;
    STTS22HTR_Status_t ret;

    if ((hi2c == NULL) || (isBusy == NULL))
    {
        return STTS22HTR_ERROR;
    }

    ret = STTS22HTR_ReadStatus(hi2c, &status);
    if (ret != STTS22HTR_OK)
    {
        return ret;
    }

    *isBusy = (status & STTS22HTR_STATUS_BUSY) ? 1 : 0;

    return STTS22HTR_OK;
}

STTS22HTR_Status_t STTS22HTR_SetHighTempLimit(I2C_HandleTypeDef *hi2c,float threshold)
{
    uint8_t regValue;

    if (hi2c == NULL)
    {
        return STTS22HTR_ERROR;
    }

    /* 将温度值转换为寄存器值 */
    regValue = STTS22HTR_TEMP_TO_REG(threshold);

    return STTS22HTR_WriteReg(hi2c,
                              STTS22HTR_REG_TEMP_H_LIMIT,
                              regValue);
}

STTS22HTR_Status_t STTS22HTR_SetLowTempLimit(I2C_HandleTypeDef *hi2c,float threshold)
{
    uint8_t regValue;

    if (hi2c == NULL)
    {
        return STTS22HTR_ERROR;
    }

    regValue = STTS22HTR_TEMP_TO_REG(threshold);

    return STTS22HTR_WriteReg(hi2c,
                              STTS22HTR_REG_TEMP_L_LIMIT,
                              regValue);
}

STTS22HTR_Status_t STTS22HTR_ReadCtrlReg(I2C_HandleTypeDef *hi2c,uint8_t *ctrl)
{
    if ((hi2c == NULL) || (ctrl == NULL))
    {
        return STTS22HTR_ERROR;
    }

    return STTS22HTR_ReadRegs(hi2c,
                              STTS22HTR_REG_CTRL,
                              ctrl,
                              1);
}

STTS22HTR_Status_t STTS22HTR_Reset(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL)
    {
        return STTS22HTR_ERROR;
    }

    /* 写入 0x00 恢复默认配置 */
    return STTS22HTR_WriteReg(hi2c,STTS22HTR_REG_CTRL,0x00);
}


STTS22HTR_Status_t STTS22HTR_Optimize_Init(STTS22HTR_OptimizeState_t *pState)
{
    uint8_t i;

    if (pState == NULL)
        return STTS22HTR_ERROR;

    pState->lastRaw      = 0.0f;
    pState->medianIdx    = 0;
    pState->avgIdx       = 0;
    pState->avgCount     = 0;
    pState->lowpassPrev  = 0.0f;
    pState->sumBuf       = 0.0f;
    pState->initialized  = 0;

    for (i = 0; i < 5; i++)
        pState->medianBuf[i] = 0.0f;

    for (i = 0; i < 32; i++)
        pState->avgBuf[i] = 0.0f;

    return STTS22HTR_OK;
}

STTS22HTR_Status_t STTS22HTR_Optimize_Reset(STTS22HTR_OptimizeState_t *pState)
{
    return STTS22HTR_Optimize_Init(pState);
}

STTS22HTR_Status_t STTS22HTR_Optimize_Process(STTS22HTR_OptimizeState_t  *pState,const STTS22HTR_OptimizeConfig_t *pConfig,float  rawTemp,float *outTemp)
{
    if ((pState == NULL) || (pConfig == NULL) || (outTemp == NULL))
		{
				return STTS22HTR_ERROR;
		}
    filter_data_t result = temp_Optimize_filter(FLOAT_TO_FILTER(rawTemp));
    *outTemp = FILTER_TO_FLOAT(result);
    // DEBUG_PRINT("out temp :%.1f\r\n" , *outTemp);
    return STTS22HTR_OK;
}

STTS22HTR_Status_t STTS22HTR_ReadTempOptimized(
    I2C_HandleTypeDef *hi2c,
    STTS22HTR_OptimizeState_t  *pState,
    const STTS22HTR_OptimizeConfig_t *pConfig,
    float *outTemp)
{
    float rawTemp = 0.0f;
    STTS22HTR_Status_t status;
    STTS22HTR_OptimizeConfig_t defaultConfig = STTS22HTR_OPTIMIZE_CONFIG_DEFAULT;

    if ((hi2c == NULL) || (pState == NULL) || (outTemp == NULL))
        return STTS22HTR_ERROR;

    /* 读取原始温度 */
    status = STTS22HTR_ReadTemp(hi2c, &rawTemp);
    if (status != STTS22HTR_OK)
        return status;

    /* 若未传入配置则使用默认配置 */
    if (pConfig == NULL)
        pConfig = &defaultConfig;

    /* 自动初始化状态(首次使用时) */
    if (!pState->initialized)
        STTS22HTR_Optimize_Init(pState);

    /* 执行精度优化滤波链 */
    return STTS22HTR_Optimize_Process(pState, pConfig, rawTemp, outTemp);
}

float Get_temp(void)
{
    static STTS22HTR_OptimizeState_t  optState;
    static STTS22HTR_OptimizeConfig_t optConfig = STTS22HTR_OPTIMIZE_CONFIG_DEFAULT;
    static float optimizedTemp = 0.0f;
    static uint8_t init_done = 0;

    if (!init_done)
    {
        if (STTS22HTR_Init(&hi2c1) != STTS22HTR_OK)
        {
            DEBUG_PRINT("init failed\r\n");
            return 0.0f;
        }
        init_done = 1;
        HAL_Delay(50);  // 等待传感器第一个数据就绪
    }

    if (STTS22HTR_ReadTemp(&hi2c1, &temp) == STTS22HTR_OK)
    {
        STTS22HTR_Optimize_Process(&optState, &optConfig, temp, &optimizedTemp);
    }
    return optimizedTemp;
}
