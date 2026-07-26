#include "Average_Filter.h" 
#include "Filter_manager.h"

/**
 * 
 * @brief 滑动平均滤波 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 09:39:36
 * @copyright Copyright (c) 2026
 */
average_filter_info_t average_filter_info;

/**
 * 
 * @brief 初始化算法参数 
 * @author LinZuQin (190449930t6@qq.com)
 * @date 2026-07-21 11:30:59
 * @copyright Copyright (c) 2026
 */
void average_filter_info_init(void)
{
    average_filter_info.average_filter_result = FILTER_ZERO;
    average_filter_info.average_filter_sum = FILTER_ZERO;
    average_filter_info.sampling_count = 0;
    memset(average_filter_info.average_buf , 0 , sizeof(average_filter_info.average_buf));
}
 
/**
 * 
 * @param data 采样数据 
 * @brief 获取采样数据
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 11:31:13
 * @copyright Copyright (c) 2026
 */
void average_filter_proc_data(filter_data_t data)
{
    average_filter_info_t *a = &average_filter_info;
    if(average_filter_info.sampling_count < SAMPLING_MAX)
    {
        a->average_buf[a->sampling_count ++] = data;
        a->average_filter_sum = FILTER_ADD(a->average_filter_sum, data);
    }
    else
    {
        //先处理总和
        a->average_filter_sum = FILTER_SUB(a->average_filter_sum, a->average_buf[0]);
        a->average_filter_sum = FILTER_ADD(a->average_filter_sum, data);

        //再处理数组 数据左移
        for(uint8_t i = 0;i<SAMPLING_MAX - 1;i++)
        {
            a->average_buf[i] = a->average_buf[i+1];
        }

        //存入新数据
        a->average_buf[SAMPLING_MAX - 1] = data;
    }
}

/**
 * 
 * @return filter_data_t 
 * @brief 返回平均采样结果
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 11:27:24
 * @copyright Copyright (c) 2026
 */
filter_data_t average_filter_get_result(void)
{
    /* Q16.16定点除法: 分母为整数，sum / count 自动保持Q格式 */
    average_filter_info.average_filter_result = average_filter_info.average_filter_sum / average_filter_info.sampling_count;
    return average_filter_info.average_filter_result;
}

/*均值滤波算法功能函数*/
filter_func_type_t average_filter_func = 
{
    .filter_init = average_filter_info_init,
    .filter_proc = average_filter_proc_data,
    .filter_get = average_filter_get_result
};
