#include "temp.h"
#include "adc.h"

#define TS_CAL1 (*(uint16_t*)0x1FFFF7B8) // 30℃校准ADC值
#define TS_CAL2 (*(uint16_t*)0x1FFFF7C2) // 110℃校准ADC值
#define TS_CAL1_TEMP 30
#define TS_CAL2_TEMP 110
uint16_t adc_raw;



uint32_t ADC_Get_Average(uint8_t ch, uint8_t times)
{
    ADC_ChannelConfTypeDef sConfig; // 通道初始化
    uint32_t value_sum = 0;
    uint8_t i;
    switch (ch) // 选择ADC通道
    {
    case 0:
        sConfig.Channel = ADC_CHANNEL_0;
        break;
    case 1:
        sConfig.Channel = ADC_CHANNEL_1;
        break;
    case 2:
        sConfig.Channel = ADC_CHANNEL_2;
        break;
    case 3:
        sConfig.Channel = ADC_CHANNEL_3;
        break;
    case 4:
        sConfig.Channel = ADC_CHANNEL_4;
        break;
    case 5:
        sConfig.Channel = ADC_CHANNEL_5;
        break;
    case 6:
        sConfig.Channel = ADC_CHANNEL_6;
        break;
    case 7:
        sConfig.Channel = ADC_CHANNEL_7;
        break;
    case 8:
        sConfig.Channel = ADC_CHANNEL_8;
        break;
    case 9:
        sConfig.Channel = ADC_CHANNEL_9;
        break;
		case 0xff:
			sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
			break;
    }
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; // 采用周期239.5周期
    sConfig.Rank = 1;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    for (i = 0; i < times; i++)
    {
        HAL_ADC_Start(&hadc1);                 // 启动转换
        HAL_ADC_PollForConversion(&hadc1, 30); // 等待转化结束
        value_sum += HAL_ADC_GetValue(&hadc1); // 求和
        HAL_ADC_Stop(&hadc1);                  // 停止转换
    }
    return value_sum / times; // 返回平均值
}

static void board_temp_adc_get(void)
{
	adc_raw = ADC_Get_Average(0xff ,ADC_CHANNEL_TEMPSENSOR);
}

void board_temp_init(void)
{
	SET_BIT(hadc1.Instance->CR2, ADC_CR2_TSVREFE);
	HAL_Delay(10);

	HAL_ADCEx_Calibration_Start(&hadc1);
}

float board_temp_get(void)
{
	board_temp_adc_get();
	float vsense = adc_raw * 3.3f / 4095.0f;
	float temp = 25.0f - (vsense - 1.43f) / 0.0043f;
	return temp;
}
