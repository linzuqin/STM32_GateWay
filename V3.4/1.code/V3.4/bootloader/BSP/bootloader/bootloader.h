#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H
#include "hal_flash.h"

// bootloader部分程序主要执行的两个步骤:(1).将download中的程序复制到app区 (2).关闭全局中断后跳转到app区执行程序
// 跳转app区主要分为两个部分:
// (1).关闭全局中断
// (2).设置msp栈顶指针。msp栈顶指针规定了程序运行的栈空间的起始地址,栈是存储在RAM里的,所以msp指针指向的地址的值为栈在RAM里的起始地址,bootloader和app不通用,所以跳转前需要重新设置 
// (3).设置PC指针。PC指针指向的是程序的入口地址,bootloader和app不通用,所以跳转前需要重新设置,程序入口地址为栈顶指针下一个地址,即地址+4。

#include <stdint.h>



void bootloader_poll(void);

#endif
