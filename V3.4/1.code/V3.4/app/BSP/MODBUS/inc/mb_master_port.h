/*
 * @Description: modbus主机头文件 定义了主机帧格式 声明了主要的两个函数
 * @Author: linzuqin
 * @Date: 2025-11-20 17:15:46
 * @LastEditTime: 2025-11-23 22:28:10
 * @LastEditors: linzuqin
 */
#ifndef _MB_MASTER_PORT_H_
#define _MB_MASTER_PORT_H_
#include "mb.h"
#include "stdint.h"

//保持寄存器写入回调函数
void mbMaster_hold_set_callback(uint16_t addr, uint16_t val);
//线圈寄存器写入回调函数
void mbMaster_coil_set_callback(uint16_t addr, uint16_t val);

extern const mb_opt mb_master_opt;

#endif /* _MB_MASTER_H_ */
