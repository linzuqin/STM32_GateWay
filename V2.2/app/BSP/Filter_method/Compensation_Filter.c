#include "Compensation_Filter.h"
#include "Filter_manager.h"

compensation_filter_info_t compensation_filter_info;

/**
 * 
 * @brief 自定义修正算法参数初始化
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 17:42:35
 * @copyright Copyright (c) 2026
 */
static void compensation_filter_info_init(void)
{
    compensation_filter_info.compensation_filter_result = FILTER_ZERO;
    compensation_filter_info.add_factor = FILTER_ZERO;
    compensation_filter_info.mul_factor = FILTER_ONE;
}

/**
 * 
 * @param data 采样数据
 * @brief 自定义算法处理采样数据
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 17:42:52
 * @copyright Copyright (c) 2026
 */
static void compensation_filter_proc_data(filter_data_t data)
{
    compensation_filter_info_t *a = &compensation_filter_info;
    a->compensation_filter_result = FILTER_ADD(FILTER_MUL(data, a->mul_factor), a->add_factor);
}

/**
 * 
 * @return filter_data_t 
 * @brief 读取自定义算法计算结果 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 17:43:10
 * @copyright Copyright (c) 2026
 */
static filter_data_t compensation_filter_get_result(void)
{
    return compensation_filter_info.compensation_filter_result;
}

filter_func_type_t compensation_filter_func = 
{
    .filter_init = compensation_filter_info_init,
    .filter_proc = compensation_filter_proc_data,
    .filter_get = compensation_filter_get_result
};

/**
 * 
 * @brief 两点校准 - 自动计算补偿系数
 * 
 * 公式: result = data × mul + add
 * 
 * mul = (actual2 - actual1) / (input2 - input1)
 * add = actual1 - input1 × mul
 * 
 * @param input1  第一点输入值(Q16.16)
 * @param actual1 第一点实际值(Q16.16)
 * @param input2  第二点输入值(Q16.16)
 * @param actual2 第二点实际值(Q16.16)
 * 
 * @note 该函数只会在校准时调用一次，不影响运行时性能
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-229
 */
void compensation_filter_calibrate(filter_data_t input1, filter_data_t actual1,
                                    filter_data_t input2, filter_data_t actual2)
{
    filter_data_t delta_input = FILTER_SUB(input2, input1);
    filter_data_t delta_actual = FILTER_SUB(actual2, actual1);
    
    /* 防止除零 */
    if (delta_input == FILTER_ZERO)
    {
        compensation_filter_info.mul_factor = FILTER_ONE;
        compensation_filter_info.add_factor = FILTER_SUB(actual1, input1);
        return;
    }
    
    /* mul = (actual2 - actual1) / (input2 - input1) */
    compensation_filter_info.mul_factor = FILTER_DIV(delta_actual, delta_input);
    
    /* add = actual1 - input1 × mul */
    compensation_filter_info.add_factor = FILTER_SUB(actual1, FILTER_MUL(input1, compensation_filter_info.mul_factor));
}
