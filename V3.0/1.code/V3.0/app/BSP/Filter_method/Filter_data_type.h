#ifndef _FILTER_DATA_TYPE_H_
#define _FILTER_DATA_TYPE_H_

#include <stdint.h>

/**
 * @file    Filter_data_type.h
 * @brief   滤波算法数据类型抽象层
 * @author  Adapted from LinZuQin's float filter code
 * @date    2026-07-22
 * 
 * @details 提供编译开关，在 float 和 Q16.16 定点数之间切换：
 * 
 *          - FILTER_USE_FLOAT = 1: 使用 float（兼容原始代码，调试方便）
 *          - FILTER_USE_FLOAT = 0: 使用 int32_t Q16.16 定点数（适配8位单片机）
 * 
 *          Q16.16 定点格式说明：
 *          - 范围:      -32768 ~ 32767.999985
 *          - 精度:      1/65536 ≈ 0.000015
 *          - 存储:      int32_t，低16位为小数部分，高16位为整数部分
 *          - 示例:      1.5 → 1.5 × 65536 = 98304 (0x18000)
 *                     -40.25 → -40.25 × 65536 = -2637824 (0xFFD80000)
 */

/* ======================== 编译开关 ======================== */
#ifndef FILTER_USE_FLOAT
#define FILTER_USE_FLOAT    0   /* 0: 定点数模式; 1: float模式 */
#endif

/* ======================== 浮点模式 ======================== */
#if FILTER_USE_FLOAT

    typedef float filter_data_t;

    /** @def 浮点模式：数值转换宏（直接传递） */
    #define FLOAT_TO_FILTER(f)       (f)
    #define FILTER_TO_FLOAT(x)       (x)
    #define INT_TO_FILTER(i)         ((float)(i))

    /** @def 浮点模式：算术宏（直接使用运算符） */
    #define FILTER_ADD(a, b)         ((a) + (b))
    #define FILTER_SUB(a, b)         ((a) - (b))
    #define FILTER_MUL(a, b)         ((a) * (b))
    #define FILTER_DIV(a, b)         ((a) / (b))

    /** @def 浮点模式：常量 */
    #define FILTER_ONE               1.0f
    #define FILTER_ZERO              0.0f
    #define FILTER_HALF              0.5f

/* ======================== 定点模式(Q16.16) ======================== */
#else

    typedef int32_t filter_data_t;

    /** @def Q16.16 格式参数 */
    #define FILTER_Q_BITS            16
    #define FILTER_SCALE             ((int32_t)1 << FILTER_Q_BITS)  /* 65536 */

    /** @def 定点模式：常用常量 */
    #define FILTER_ONE               ((filter_data_t)(1 << FILTER_Q_BITS))         /* 1.0 */
    #define FILTER_ZERO              ((filter_data_t)0)                            /* 0.0 */
    #define FILTER_HALF              ((filter_data_t)(1 << (FILTER_Q_BITS - 1)))   /* 0.5 */

    /**
     * @def   定点模式：数值转换宏
     * @param  f    float 值
     * @param  x    filter_data_t 值
     * @param  i    整数
     */
    #define FLOAT_TO_FILTER(f)       ((filter_data_t)((f) * FILTER_SCALE + ((f) >= 0 ? 0.5f : -0.5f)))
    #define FILTER_TO_FLOAT(x)       ((float)(x) / FILTER_SCALE)
    #define INT_TO_FILTER(i)         ((filter_data_t)(i) * FILTER_SCALE)

    /**
     * @def   定点乘法: (a × b) >> 16
     * @note  使用 int64_t 中间结果防止溢出
     *        例: 1.5 × 2.0 = (98304 × 131072) >> 16 = 12884901888 >> 16 = 196608 = 3.0
     */
    #define FILTER_MUL(a, b)         ((filter_data_t)(((int64_t)(a) * (int64_t)(b)) >> FILTER_Q_BITS))

    /**
     * @def   定点除法: (a << 16) / b
     * @note  使用 int64_t 中间结果防止溢出
     *        例: 3.0 ÷ 2.0 = (196608 << 16) / 131072 = 12884901888 / 131072 = 98304 = 1.5
     */
    #define FILTER_DIV(a, b)         ((filter_data_t)((((int64_t)(a) << FILTER_Q_BITS) / (int64_t)(b))))

    /** @def 定点加减法（与普通整数加减相同） */
    #define FILTER_ADD(a, b)         ((a) + (b))
    #define FILTER_SUB(a, b)         ((a) - (b))

#endif /* FILTER_USE_FLOAT */

#endif /* _FILTER_DATA_TYPE_H_ */
