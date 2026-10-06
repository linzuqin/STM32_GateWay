#include "IIR_Filter.h"
#include "Filter_manager.h"

/**
 * 
 * @brief 一阶低通IIR滤波(EMA)

 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 09:44:30
 */

IIR_filter_info_t IIR_filter_info;

/**
 * 
 * @brief 一阶低通滤波参数初始化 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 11:59:54
 * @copyright Copyright (c) 2026
 */
void IIR_filter_info_init(void)
{
    IIR_filter_info_t *a = &IIR_filter_info;
    a->IIR_filter_laster_result = FILTER_ZERO;
    a->IIR_filter_result = FILTER_ZERO;
}

/**
 * 
 * @param data 采样数据(Q16.16)
 * @brief 一阶低通滤波
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:36:03
 */
void IIR_filter_proc_data(filter_data_t data)
{
    IIR_filter_info_t *a = &IIR_filter_info;
    filter_data_t diff;
    
    if (a->IIR_filter_laster_result == FILTER_ZERO)
    {
        /* 首次滤波，直接取当前值 */
        a->IIR_filter_result = data;
        a->IIR_filter_laster_result = a->IIR_filter_result;
    }
    else
    {
        /* Y n=α × X n +(1−α) × Y n−1 */
        /* Y n=α × (X n - Y n−1) + Y n−1 */

        diff = FILTER_SUB(data, a->IIR_filter_laster_result);
        
#if IIR_SHIFT_BITS > 0
        if (diff >= 0)
            a->IIR_filter_result = a->IIR_filter_laster_result + (diff >> IIR_SHIFT_BITS);
        else
            a->IIR_filter_result = a->IIR_filter_laster_result - ((-diff) >> IIR_SHIFT_BITS);
#else
        a->IIR_filter_result = a->IIR_filter_laster_result + diff;
#endif
        a->IIR_filter_laster_result = a->IIR_filter_result;
    }
}

/**
 * 
 * @return filter_data_t 
 * @brief 返回一阶低通滤波滤波结果
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:35:45
 * @copyright Copyright (c) 2026
 */
filter_data_t IIR_filter_get_result(void)
{
    return IIR_filter_info.IIR_filter_result;
}

/*一阶低通滤波算法功能函数*/
filter_func_type_t IIR_filter_func = 
{
    .filter_init = IIR_filter_info_init,
    .filter_proc = IIR_filter_proc_data,
    .filter_get = IIR_filter_get_result
};
