/**
 * @file    debug_print.h
 * @brief   统一调试打印宏，通过 Keil 全局宏 MSH_USE_RTT 切换 printf / RTT 输出
 * 
 * 使用方式：在 .c 文件中先定义 DEBUG_ENABLE 和 DEBUG_LOG，再 include 本文件
 *   #define DEBUG_ENABLE    1
 *   #define DEBUG_LOG       "[ MODULE_NAME ]"
 *   #include "debug_print.h"
 */
#ifndef _DEBUG_PRINT_H_
#define _DEBUG_PRINT_H_

#include <stdio.h>

#ifdef MSH_USE_RTT
#include "SEGGER_RTT.h"
#define DEBUG_PRINT(fmt, ...)  do { \
    if (DEBUG_ENABLE) SEGGER_RTT_printf(0, DEBUG_LOG "[%s:%d] " fmt "\n", \
        __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)
#else
#define DEBUG_PRINT(fmt, ...)  do { \
    if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", \
        __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)
#endif

#endif /* _DEBUG_PRINT_H_ */
