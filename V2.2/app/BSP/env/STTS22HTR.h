/**
  ******************************************************************************
  * @file           : STTS22HTR.h
  * @brief          : STTS22HTR 温度传感器驱动程序头文件
  * @description    : 基于 STM32 HAL 库的 I2C 驱动
  *                   硬件接口: I2C1 - SCL:PB6, SDA:PB7
  *                   ADDR引脚接GND, 7位I2C地址: 0x3F
  ******************************************************************************
  */

#ifndef __STTS22HTR_H__
#define __STTS22HTR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* STTS22HTR I2C 地址 ---------------------------------------------------------*/
/* ADDR引脚接GND: 7位地址 = 0x3F */
#define STTS22HTR_I2C_ADDR              0x3F

/* 寄存器地址定义 ------------------------------------------------------------*/
#define STTS22HTR_REG_WHO_AM_I          0x01  /*!< 设备标识寄存器, 只读, 固定值 0xA0 */
#define STTS22HTR_REG_TEMP_H_LIMIT      0x02  /*!< 高温阈值寄存器, 读写 */
#define STTS22HTR_REG_TEMP_L_LIMIT      0x03  /*!< 低温阈值寄存器, 读写 */
#define STTS22HTR_REG_CTRL              0x04  /*!< 控制寄存器, 读写 */
#define STTS22HTR_REG_STATUS            0x05  /*!< 状态寄存器, 只读 */
#define STTS22HTR_REG_TEMP_L_OUT        0x06  /*!< 温度数据低字节, 只读 */
#define STTS22HTR_REG_TEMP_H_OUT        0x07  /*!< 温度数据高字节, 只读 */

/* WHO_AM_I 期望值 -----------------------------------------------------------*/
#define STTS22HTR_WHO_AM_I_VALUE        0xA0

/* CTRL 控制寄存器位定义 -----------------------------------------------------*/
#define STTS22HTR_CTRL_ONE_SHOT         (0x01 << 0)  /*!< 单次触发模式 */
#define STTS22HTR_CTRL_TIME_OUT_DIS     (0x01 << 1)  /*!< SMBus超时禁用 */
#define STTS22HTR_CTRL_FREERUN          (0x01 << 2)  /*!< 自由运行模式(连续测量) */
#define STTS22HTR_CTRL_IF_ADD_INC       (0x01 << 3)  /*!< 多字节地址自增 */
#define STTS22HTR_CTRL_AVG_SHIFT        4            /*!< 平均采样位数偏移 */
#define STTS22HTR_CTRL_AVG_MASK         (0x03 << 4)  /*!< 平均采样数掩码 */
#define STTS22HTR_CTRL_BDU              (0x01 << 6)  /*!< 块数据更新 */
#define STTS22HTR_CTRL_LOW_ODR_START    (0x01 << 7)  /*!< 低速启动模式 */

/* STATUS 状态寄存器位定义 ---------------------------------------------------*/
#define STTS22HTR_STATUS_BUSY           (0x01 << 0)  /*!< 忙标志 */
#define STTS22HTR_STATUS_OVER_THH       (0x01 << 1)  /*!< 超过高温阈值 */
#define STTS22HTR_STATUS_UNDER_THL      (0x01 << 2)  /*!< 低于低温阈值 */

/* 平均采样数枚举 ------------------------------------------------------------*/
typedef enum
{
    STTS22HTR_AVG_1    = 0x00,  /*!< 平均采样1次 */
    STTS22HTR_AVG_2    = 0x01,  /*!< 平均采样2次 */
    STTS22HTR_AVG_4    = 0x02,  /*!< 平均采样4次 */
    STTS22HTR_AVG_8    = 0x03   /*!< 平均采样8次 */
} STTS22HTR_Avg_t;

/* 工作模式枚举 --------------------------------------------------------------*/
typedef enum
{
    STTS22HTR_MODE_POWER_DOWN = 0x00,  /*!< 掉电模式 */
    STTS22HTR_MODE_ONE_SHOT   = 0x01,  /*!< 单次触发模式 */
    STTS22HTR_MODE_1HZ        = 0x02,  /*!< 1Hz 连续模式 */
    STTS22HTR_MODE_25HZ       = 0x03,  /*!< 25Hz 连续模式 */
    STTS22HTR_MODE_50HZ       = 0x04,  /*!< 50Hz 连续模式 */
    STTS22HTR_MODE_100HZ      = 0x05,  /*!< 100Hz 连续模式 */
    STTS22HTR_MODE_200HZ      = 0x06   /*!< 200Hz 连续模式 */
} STTS22HTR_Mode_t;

/* 返回状态枚举 --------------------------------------------------------------*/
typedef enum
{
    STTS22HTR_OK       = 0x00,  /*!< 操作成功 */
    STTS22HTR_ERROR    = 0x01,  /*!< 操作失败 */
    STTS22HTR_TIMEOUT  = 0x02,  /*!< 超时 */
    STTS22HTR_BUSY     = 0x03   /*!< 设备忙 */
} STTS22HTR_Status_t;

/* ---------------------------------------------------------------------------*/
/* 函数声明                                                                    */
/* ---------------------------------------------------------------------------*/

/**
  * @brief  初始化 STTS22HTR 传感器
  * @param  hi2c: I2C 句柄指针
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_Init(I2C_HandleTypeDef *hi2c);

/**
  * @brief  读取 WHO_AM_I 寄存器，验证器件
  * @param  hi2c: I2C 句柄指针
  * @retval STTS22HTR_Status_t: 操作状态 (OK 表示器件正常)
  */
STTS22HTR_Status_t STTS22HTR_CheckWhoAmI(I2C_HandleTypeDef *hi2c);

/**
  * @brief  设置传感器工作模式
  * @param  hi2c: I2C 句柄指针
  * @param  mode: 工作模式 (STTS22HTR_Mode_t)
  * @param  avg: 平均采样数 (STTS22HTR_Avg_t)
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_SetMode(I2C_HandleTypeDef *hi2c,
                                     STTS22HTR_Mode_t mode,
                                     STTS22HTR_Avg_t avg);

/**
  * @brief  触发单次转换 (仅在 One-Shot 模式下使用)
  * @param  hi2c: I2C 句柄指针
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_TriggerOneShot(I2C_HandleTypeDef *hi2c);

/**
  * @brief  读取温度值 (原始值)
  * @param  hi2c: I2C 句柄指针
  * @param  rawTemp: 输出原始温度值 (16位有符号)
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_ReadRawTemp(I2C_HandleTypeDef *hi2c,
                                         int16_t *rawTemp);

/**
  * @brief  读取温度值 (摄氏度)
  * @param  hi2c: I2C 句柄指针
  * @param  temperature: 输出温度值 (摄氏度, 分辨率 0.01°C)
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_ReadTemp(I2C_HandleTypeDef *hi2c,
                                      float *temperature);

/**
  * @brief  读取状态寄存器
  * @param  hi2c: I2C 句柄指针
  * @param  status: 输出状态寄存器值
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_ReadStatus(I2C_HandleTypeDef *hi2c,
                                        uint8_t *status);

/**
  * @brief  检查设备是否忙
  * @param  hi2c: I2C 句柄指针
  * @param  isBusy: 输出忙状态 (1=忙, 0=空闲)
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_IsBusy(I2C_HandleTypeDef *hi2c,
                                    uint8_t *isBusy);

/**
  * @brief  设置高温阈值
  * @param  hi2c: I2C 句柄指针
  * @param  threshold: 阈值温度 (摄氏度)
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_SetHighTempLimit(I2C_HandleTypeDef *hi2c,
                                              float threshold);

/**
  * @brief  设置低温阈值
  * @param  hi2c: I2C 句柄指针
  * @param  threshold: 阈值温度 (摄氏度)
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_SetLowTempLimit(I2C_HandleTypeDef *hi2c,
                                             float threshold);

/**
  * @brief  读取控制寄存器值
  * @param  hi2c: I2C 句柄指针
  * @param  ctrl: 输出控制寄存器值
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_ReadCtrlReg(I2C_HandleTypeDef *hi2c,
                                         uint8_t *ctrl);

/**
  * @brief  恢复默认配置 (将 CTRL 寄存器写 0x00)
  * @param  hi2c: I2C 句柄指针
  * @retval STTS22HTR_Status_t: 操作状态
  */
STTS22HTR_Status_t STTS22HTR_Reset(I2C_HandleTypeDef *hi2c);

float Get_temp(void);

/* ==========================================================================*/
/*                   精度优化模块 - 高精度温度算法                              */
/* ==========================================================================*/
/* 参考国内外高精度传感器算法:
 * - Sensirion SHT30: IIR数字低通滤波器, α可配置
 * - Bosch BME280:    IIR滤波器系数可调(0~16), 多级过采样
 * - TI TMP117:       多周期均值累加, 精度高达±0.1°C
 * - ADI ADT7420:     均值滤波+异常值剔除
 * 本模块实现: 限幅滤波 → 中值滤波 → 滑动平均 → 一阶低通IIR滤波链
 */

/** @brief 滤波模式枚举 */
typedef enum
{
    STTS22HTR_FILTER_LIMITING   = 0x01,  /*!< 限幅滤波: 剔除突变跳变值 */
    STTS22HTR_FILTER_MEDIAN     = 0x02,  /*!< 中值滤波: 窗口内取中值,去尖峰 */
    STTS22HTR_FILTER_AVERAGE    = 0x04,  /*!< 滑动平均滤波: 窗口累加平均 */
    STTS22HTR_FILTER_LOWPASS    = 0x08   /*!< 一阶低通IIR滤波(EMA): Y(n)=α*X(n)+(1-α)*Y(n-1) */
} STTS22HTR_FilterType_t;

/** @brief 精度优化配置结构体 */
typedef struct
{
    uint8_t     enableMask;         /*!< 使能滤波组合掩码 (FilterType_t 位或) */
    float       limitThreshold;     /*!< 限幅滤波: 相邻采样最大允许温差(°C), 默认 2.0f */
    uint8_t     medianWindowSize;   /*!< 中值滤波窗口大小 (3 或 5), 默认 3 */
    uint8_t     avgWindowSize;      /*!< 滑动平均窗口大小 (2~32), 默认 8 */
    float       lowpassAlpha;       /*!< 低通滤波系数 α (0.01~0.99), 越小越平滑, 默认 0.15f */
    float       calibSlope;         /*!< 校准斜率修正 (默认 1.0f, 即 y = k*x + b 中的 k) */
    float       calibOffset;        /*!< 校准偏置修正 (默认 0.0f, 即 y = k*x + b 中的 b) */
} STTS22HTR_OptimizeConfig_t;

/** @brief 精度优化内部状态结构体(用户无需直接操作) */
typedef struct
{
    float   lastRaw;            /*!< 上一次原始值, 用于限幅滤波 */
    float   medianBuf[5];       /*!< 中值滤波缓冲区 */
    uint8_t medianIdx;          /*!< 中值滤波缓冲区索引 */
    float   avgBuf[32];         /*!< 滑动平均缓冲区 */
    uint8_t avgIdx;             /*!< 滑动平均缓冲区索引 */
    uint8_t avgCount;           /*!< 滑动平均已填充计数 */
    float   lowpassPrev;        /*!< 上一帧低通输出值 */
    float   sumBuf;             /*!< 滑动平均累加和 (优化用) */
    uint8_t initialized;        /*!< 状态是否已初始化 */
} STTS22HTR_OptimizeState_t;

/* 默认精度优化配置 --------------------------------------------------------*/
#define STTS22HTR_OPTIMIZE_CONFIG_DEFAULT                                \
  {                                                                    \
      .enableMask      = (STTS22HTR_FILTER_LIMITING |                  \
                          STTS22HTR_FILTER_MEDIAN   |                  \
                          STTS22HTR_FILTER_AVERAGE  |                  \
                          STTS22HTR_FILTER_LOWPASS),                   \
      .limitThreshold  = 2.0f,                                         \
      .medianWindowSize= 3,                                            \
      .avgWindowSize   = 8,                                            \
      .lowpassAlpha    = 0.15f,                                        \
      .calibSlope      = 1.0f,                                         \
      .calibOffset     = 0.0f                                          \
  }

/* 精度优化函数声明 --------------------------------------------------------*/

/**
  * @brief  初始化精度优化模块状态
  * @param  pState: 状态结构体指针
  * @retval STTS22HTR_Status_t
  */
STTS22HTR_Status_t STTS22HTR_Optimize_Init(STTS22HTR_OptimizeState_t *pState);

/**
  * @brief  重置精度优化模块状态(清空历史缓冲区)
  * @param  pState: 状态结构体指针
  * @retval STTS22HTR_Status_t
  */
STTS22HTR_Status_t STTS22HTR_Optimize_Reset(STTS22HTR_OptimizeState_t *pState);

/**
  * @brief  单步执行精度优化滤波链
  * @param  pState:  状态结构体指针
  * @param  pConfig: 配置结构体指针
  * @param  rawTemp: 原始温度输入值 (°C)
  * @param  outTemp: 优化后温度输出值 (°C)
  * @retval STTS22HTR_Status_t
  */
STTS22HTR_Status_t STTS22HTR_Optimize_Process(
    STTS22HTR_OptimizeState_t  *pState,
    const STTS22HTR_OptimizeConfig_t *pConfig,
    float  rawTemp,
    float *outTemp);

/**
  * @brief  读取经精度优化的温度值(便捷函数,含传感器读取+滤波链)
  * @param  hi2c:    I2C句柄指针
  * @param  pState:  状态结构体指针
  * @param  pConfig: 配置结构体指针(传NULL使用默认配置)
  * @param  outTemp: 输出优化后温度值 (°C)
  * @retval STTS22HTR_Status_t
  */
STTS22HTR_Status_t STTS22HTR_ReadTempOptimized(
    I2C_HandleTypeDef *hi2c,
    STTS22HTR_OptimizeState_t  *pState,
    const STTS22HTR_OptimizeConfig_t *pConfig,
    float *outTemp);

#ifdef __cplusplus
}
#endif

#endif /* __STTS22HTR_H__ */
