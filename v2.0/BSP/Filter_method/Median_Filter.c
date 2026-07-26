#include "Median_Filter.h"
#include "Filter_manager.h"

/**
 * 
 * @brief 中值滤波
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:38:29
 * @copyright Copyright (c) 2026
 */
median_filter_info_t median_filter_info;

/**
 * @brief 初始化中值滤波器参数
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:38:29
 * @copyright Copyright (c) 2026
 */
void median_filter_info_init(void)
{
    median_filter_info.median_filter_result = FILTER_ZERO;
    median_filter_info.sampling_count = 0;
    memset(median_filter_info.median_buf, 0, sizeof(median_filter_info.median_buf));
    memset(median_filter_info.sort_buf, 0, sizeof(median_filter_info.sort_buf));
}

/**
 * @brief 对指定数组进行从小到大的冒泡排序
 * @param buf   待排序数组
 * @param len   数组长度
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:38:29
 * @copyright Copyright (c) 2026
 */
static void bubble_sort(filter_data_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len - 1; i++) //循环遍历每一个数据
    {
        for (uint8_t j = 0; j < len - 1 - i; j++) //将当前数据中的最大值放到数组的末端
        {
            if (buf[j] > buf[j + 1])
            {
                filter_data_t temp = buf[j];
                buf[j] = buf[j + 1];
                buf[j + 1] = temp;
            }
        }
    }
}

/**
 * @param data 采样数据
 * @brief 处理采样数据
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:38:29
 * @copyright Copyright (c) 2026
 */
void median_filter_proc_data(filter_data_t data)
{
    median_filter_info_t *m = &median_filter_info;

    if (m->sampling_count < MEDIAN_SAMPLING_MAX)
    {
        /* 缓冲区未满，直接存入 */
        m->median_buf[m->sampling_count++] = data;
    }
    else
    {
        /* 缓冲区已满，数据左移，丢弃最旧数据，末尾存入新数据 */
        for (uint8_t i = 0; i < MEDIAN_SAMPLING_MAX - 1; i++)
        {
            m->median_buf[i] = m->median_buf[i + 1];
        }
        m->median_buf[MEDIAN_SAMPLING_MAX - 1] = data;
    }
}

/**
 * @return filter_data_t 中值滤波结果
 * @brief 对当前缓冲区排序后取中值并返回 若采样数据数量为奇数 则取中值 若为偶数 则取中间两个数据的平均值
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 13:38:29
 * @copyright Copyright (c) 2026
 */
filter_data_t median_filter_get_result(void)
{
    median_filter_info_t *m = &median_filter_info;

    if (m->sampling_count == 0)
    {
        return FILTER_ZERO;
    }

    memcpy(m->sort_buf, m->median_buf, m->sampling_count * sizeof(filter_data_t));

    bubble_sort(m->sort_buf, m->sampling_count);

    if (m->sampling_count % 2 == 1)
    {
        m->median_filter_result = m->sort_buf[m->sampling_count / 2];
    }
    else
    {
        m->median_filter_result = (m->sort_buf[m->sampling_count / 2 - 1] + m->sort_buf[m->sampling_count / 2]) / 2;
    }

    return m->median_filter_result;
}

/*中值滤波算法功能函数*/
filter_func_type_t median_filter_func = 
{
    .filter_init = median_filter_info_init,
    .filter_proc = median_filter_proc_data,
    .filter_get = median_filter_get_result
};
