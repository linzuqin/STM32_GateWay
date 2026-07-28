#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

// bootloader部分程序主要执行的两个步骤:(1).将download中的程序复制到app区 (2).关闭全局中断后跳转到app区执行程序
// 跳转app区主要分为两个部分:
// (1).关闭全局中断
// (2).设置msp栈顶指针。msp栈顶指针规定了程序运行的栈空间的起始地址,栈是存储在RAM里的,所以msp指针指向的地址的值为栈在RAM里的起始地址,bootloader和app不通用,所以跳转前需要重新设置 
// (3).设置PC指针。PC指针指向的是程序的入口地址,bootloader和app不通用,所以跳转前需要重新设置,程序入口地址为栈顶指针下一个地址,即地址+4。

#include <stdint.h>

#define APP_START_ADDRESS 0x08000000+32*1024  //应用程序起始地址 根据实际情况修改
#define APP_SIZE          240*1024  //应用程序大小 根据实际情况修改
#define DOWNLOAD_START_ADDRESS APP_START_ADDRESS + APP_SIZE  //下载区起始地址 根据实际情况修改

#define RAM_START_ADDR          0x20000000    // RAM起始地址
#define RAM_SIZE                64*1024    // RAM的大小

//以下宏定义主要是用于flash操作时的地址校验
#define FLASH_BASE_ADDR        0x08000000   // Flash起始地址
#define FLASH_TOTAL_SIZE       512 * 1024    // Flash总大小
#define PAGE_SIZE              2048 // Flash页大小
#define FLASH_MAX_ADDR         (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE - 1) // Flash最大地址

void bootloader_poll(void);

#endif
