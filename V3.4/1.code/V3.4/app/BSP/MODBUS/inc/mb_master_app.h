/*
 * @Description: modbus主机头文件 定义了主机帧格式 声明了主要的两个函数
 * @Author: linzuqin
 * @Date: 2025-11-20 17:15:46
 * @LastEditTime: 2025-11-23 22:28:10
 * @LastEditors: linzuqin
 */
#ifndef _MB_MASTER_APP_H_
#define _MB_MASTER_APP_H_

#include "mb.h"

#define MB_MASTER_POLL_INTERVAL 1000 //主机轮询间隔 单位ms

void mb_m_poll(void);

// 主机函数声明
extern mb_dev_t mb_master_devs[MB_MASTER_NUM];

#endif /* _MB_MASTER_H_ */
