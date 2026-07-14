#ifndef _digital_H_
#define _digital_H_

#include <stdint.h>

#define digital_NUM	4

typedef enum{
	OUTPUT_IO = 0,
	INPUT_IO,
}digital_type_t;

typedef enum{
	DIGITAL_PIN_RESET = 0,
	DIGITAL_PIN_SET
}digital_pin_state_t;

typedef enum
{
	digital_OK = 0,
	digital_CH_ERROR,
	digital_TYPE_ERROR,
	digital_ENABLE_ERROR,

}digital_err_t;

typedef digital_err_t(*digital_callback)(uint16_t ch , void *arg);

typedef struct
{
	char *identifier;
	digital_pin_state_t read_state;
	digital_pin_state_t set_state;
	void *port;
	uint16_t pin;
	digital_type_t type;
	digital_callback read_callback;
	digital_callback set_callback;
	uint8_t enable;
}digital_info_t;

/**
 * 使能通道
 * @param ch 通道下标 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:21:08
 * @copyright Copyright (c) 2026
 */
void digital_enable(uint16_t ch);

/**
 * 失能通道
 * @param ch 通道下标 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:21:19
 * @copyright Copyright (c) 2026
 */
void digital_disable(uint16_t ch);

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
digital_err_t digital_callback_register(uint16_t ch , digital_callback read_callback , digital_callback set_callback);

/**
 * 设置单个通道状态(跳过失能的通道)
 * @param ch 对应通道的下标
 * @param state 需要设置的通道状态
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:17:05
 * @copyright Copyright (c) 2026
 */
digital_err_t Set_digital(uint16_t ch , uint8_t state);

/**
 * 调用底层digital_read_func 读取单个通道状态并更新到read_state(跳过失能的通道)
 * @param ch 通道下标
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:18:03
 * @copyright Copyright (c) 2026
 */
digital_err_t Read_digital_State(uint16_t ch);

/**
 * 读取所有通道状态 并更新到read_state(跳过失能的通道)
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:18:27
 * @copyright Copyright (c) 2026
 */
digital_err_t Read_ALL_digital_State(void);

/**
 * 调用底层digital_write_func 写入状态(跳过失能的通道)
 * @return digital_err_t 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-06-06 23:18:55
 * @copyright Copyright (c) 2026
 */
digital_err_t Set_digital_State_refresh(void);
#endif
