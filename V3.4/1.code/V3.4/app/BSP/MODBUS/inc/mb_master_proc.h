/*
 * @Description: modbus主机头文件 定义了主机帧格式 声明了主要的两个函数
 * @Author: linzuqin
 * @Date: 2025-11-20 17:15:46
 * @LastEditTime: 2025-11-23 22:28:10
 * @LastEditors: linzuqin
 */
#ifndef _MB_MASTER_PROC_H_
#define _MB_MASTER_PROC_H_
#include "mb.h"

mb_err_t mb_m_send_request(mb_dev_t *mb_dev, mb_func_code_t func_code, uint16_t start_addr, uint16_t quantity , uint16_t *val);

// 检查是否有待下发的数据
uint8_t mb_map_check(mb_dev_t *dev);

/*modbus主机设置函数 根据标志位判断是否有下发任务*/
mb_err_t mb_m_set(mb_dev_t *dev);

mb_err_t mb_m_get(mb_dev_t *dev);
mb_err_t mb_m_check_ack(mb_dev_t *mb_dev);

#endif /* _MB_MASTER_H_ */
