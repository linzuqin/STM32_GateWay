#include "digital.h"
#include "main.h"   /* 平台相关头文件，仅在 .c 中隔离 */

static digital_info_t digital_params[digital_NUM] = 
{
	[0] = 
	{
		.identifier = "IO1",
		.read_state = DIGITAL_PIN_RESET , 
		.set_state = DIGITAL_PIN_SET,
		.port = (void *)OUT1_GPIO_Port,
		.pin = OUT1_Pin,
		.type = OUTPUT_IO,
		.enable = 1,
	},
	[1] = 
	{
		.identifier = "IO2",
		.read_state = DIGITAL_PIN_RESET , 
		.set_state = DIGITAL_PIN_RESET,
		.port = (void *)OUT2_GPIO_Port,
		.pin = OUT2_Pin,
		.type = OUTPUT_IO,
		.enable = 1,
	},
	[2] = 
	{
		.identifier = "BEEP",
		.read_state = DIGITAL_PIN_RESET , 
		.set_state = DIGITAL_PIN_RESET,
		.port = (void *)BEEP_GPIO_Port,
		.pin = BEEP_Pin,
		.type = OUTPUT_IO,
		.enable = 1,
	},	
	[3] = 
	{
		.identifier = "LED",
		.read_state = DIGITAL_PIN_RESET , 
		.set_state = DIGITAL_PIN_RESET,
		.port = (void *)USER_LED_GPIO_Port,
		.pin = USER_LED_Pin,
		.type = OUTPUT_IO,
		.enable = 1,
	},
	[4] = 
	{
		.identifier = "KEY",
		.read_state = DIGITAL_PIN_RESET , 
		.set_state = DIGITAL_PIN_RESET,
		.port = (void *)KEY_GPIO_Port,
		.pin = KEY_Pin,
		.type = INPUT_IO,
		.enable = 1,
	},	

};

/*需要根据硬件平台手动定义一下读取函数*/
static digital_pin_state_t digital_read_func(void *port, uint16_t pin)
{
	return (digital_pin_state_t)HAL_GPIO_ReadPin((GPIO_TypeDef *)port, pin);
}

/*需要根据硬件平台手动定义一下写入函数*/
static void digital_write_func(void *port, uint16_t pin, digital_pin_state_t state)
{
	HAL_GPIO_WritePin((GPIO_TypeDef *)port, pin, (GPIO_PinState)state);
}

/**
 * 使能通道
 * @param ch 通道下标 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:21:08
 * @copyright Copyright (c) 2026
 */
void digital_enable(uint16_t ch)
{
	digital_params[ch].enable = 1;
}

/**
 * 失能通道
 * @param ch 通道下标 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:21:19
 * @copyright Copyright (c) 2026
 */
void digital_disable(uint16_t ch)
{
	digital_params[ch].enable = 0;
}

/**
 * 注册通道回调函数
 * @param ch 需要注册的通道下标
 * @param read_callback 注册的读取回调函数
 * @param set_callback 注册的写入回调函数
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:16:31
 * @copyright Copyright (c) 2026
 */
digital_err_t digital_callback_register(uint16_t ch , digital_callback read_callback , digital_callback set_callback)
{
	digital_info_t *dev = &digital_params[ch];
	digital_err_t ret = digital_OK;

	dev->read_callback = read_callback;
	dev->set_callback = set_callback;

	return ret;
}

/**
 * 设置单个通道状态(跳过失能的通道)
 * @param ch 对应通道的下标
 * @param state 需要设置的通道状态
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:17:05
 * @copyright Copyright (c) 2026
 */
digital_err_t Set_digital(uint16_t ch , uint8_t state)
{
	digital_err_t ret = digital_OK;
	digital_info_t *dev = &digital_params[ch];
	if(ch > digital_NUM){
		ret = digital_CH_ERROR;
	}
	else if(dev->type != OUTPUT_IO){
		ret = digital_TYPE_ERROR;
	}
	else if(dev->enable == 0)
	{
		ret = digital_ENABLE_ERROR;
	}
	else{
		digital_params[ch].set_state = (digital_pin_state_t)state;
	}
	return ret;
}

/**
 * 调用底层digital_read_func 读取单个通道状态并返回
 * @param ch 通道下标
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:18:03
 * @copyright Copyright (c) 2026
 */
digital_pin_state_t Read_digital_State(uint16_t ch)
{
	digital_info_t *dev = &digital_params[ch];;
	digital_pin_state_t ret = DIGITAL_PIN_RESET;

	if(dev->enable == 0)
	{
		/*引脚未使能*/
		
	}
	else 
	{
		dev->read_state = digital_read_func(dev->port , dev->pin);
		if(dev->read_callback == NULL){

		}
		else {
			dev->read_callback(ch , NULL);
		}
	}
	ret = dev->read_state;
	return ret;
}

/**
 * 读取所有通道状态 并更新到read_state(跳过失能的通道)
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:18:27
 * @copyright Copyright (c) 2026
 */
digital_err_t Read_ALL_digital_State(void)
{
	uint16_t i = 0;
	digital_err_t ret = digital_OK;

	for(i = 0;i<digital_NUM;i++)
	{
		Read_digital_State(i);
	}
	return ret;
}

/**
 * 调用底层digital_write_func 写入状态(跳过失能的通道)
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:18:55
 * @copyright Copyright (c) 2026
 */
digital_err_t Set_digital_State_refresh(void)
{
	uint16_t i = 0;
	digital_info_t *dev = NULL;
	digital_err_t ret = digital_OK;

	for(i = 0;i<digital_NUM;i++)
	{
		dev = &digital_params[i];
		if(dev->enable == 0)
		{
			continue;
		}
		if(dev->read_state != dev->set_state)
		{
			digital_write_func(dev->port , dev->pin , dev->set_state);
			dev->read_state = digital_read_func(dev->port , dev->pin);
			if(dev->set_callback == NULL){

			}
			else {
				ret = dev->set_callback(i , NULL);
			}
		}
	}
	return ret;
}
