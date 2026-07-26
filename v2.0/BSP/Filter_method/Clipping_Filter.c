#include "Clipping_Filter.h"
#include "Filter_manager.h"

/**
 * 
 * @brief 限幅滤波 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 09:41:34
 * @copyright Copyright (c) 2026
 */

#define DEFAULT_HIGH_LIMIT 100
#define DEFAULT_LOW_LIMIT 0
#define ERROR_COUNT_MAX 5

clipping_filter_info_t clipping_filter_info;

/**
 * 
 * @brief 限幅滤波参数初始化
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 11:46:13
 * @copyright Copyright (c) 2026
 */
void clipping_filter_info_init(void)
{
    clipping_filter_info_t *a = &clipping_filter_info;
    a->clipping_filter_result = FILTER_ZERO;
    a->error_count = 0;
    a->High_limit_value = INT_TO_FILTER(DEFAULT_HIGH_LIMIT);
    a->Low_limit_value = INT_TO_FILTER(DEFAULT_LOW_LIMIT);
    a->last_value = FILTER_ZERO;
}

/**
 * 
 * @param data 采样数据
 * @brief 限幅滤波 若数据连续几次为非法数据 则直接返回 若未达次数上限，则返回上次存储的有效值
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 11:45:16
 * @copyright Copyright (c) 2026
 */
void clipping_filter_proc_data(filter_data_t data)
{
    uint8_t Valid = 0;
    clipping_filter_info_t *a = &clipping_filter_info;

    //进来先判断 数据是否在范围内
    if(data > a->High_limit_value || data < a->Low_limit_value)
    {
        Valid = 0;
        a->error_count ++;
    }
    else
    {
        Valid = 1;
        a->error_count = 0;
    }

    //如果连续读取非法数据超过次数上限 则直接返回非法值 非法值不做存储
    if(a->error_count > ERROR_COUNT_MAX)
    {
        a->clipping_filter_result = data;
    }
    else if(Valid == 0) //数据非法 但是在错误范围内 将上次存储的数据作为返回值
    {
        a->clipping_filter_result = a->last_value;
    }
    else //数据合法 将当前数据作为结果 并存储
    {
        a->clipping_filter_result = data;
        a->last_value = data;
    }
}

/**
 * 
 * @return filter_data_t 
 * @brief 返回限幅滤波结果
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:35:11
 * @copyright Copyright (c) 2026
 */
filter_data_t clipping_filter_get_result(void)
{
    return clipping_filter_info.clipping_filter_result;
}

/*限幅滤波算法功能函数*/
filter_func_type_t clipping_filter_func = 
{
    .filter_init = clipping_filter_info_init,
    .filter_proc = clipping_filter_proc_data,
    .filter_get = clipping_filter_get_result
};
