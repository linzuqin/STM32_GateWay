#include "Oversample_Filter.h"
#include "Filter_manager.h"

/**
 * @brief 过采样+抽取滤波器 (Oversampling & Decimation)
 * @description 积累 N 次采样后取平均输出一次
 *              原理: 对同一信号多次采样累加，噪声随机分布会相互抵消一部分
 *                   信噪比提升 = 20·log10(√N) dB
 *              参考: ADI 过采样技术笔记、HP/Agilent 万用表数字滤波
 * @author Generated for STTS22HTR precision optimization
 * @date 2026-07-22
 */

oversample_filter_info_t oversample_filter_info;

void oversample_filter_info_init(void)
{
    oversample_filter_info_t *a = &oversample_filter_info;
    a->sum        = FILTER_ZERO;
    a->count      = 0;
    a->rate       = OVERSAMPLE_RATE;
    a->last_output = FILTER_ZERO;
    a->initialized = 0;
}

void oversample_filter_proc_data(filter_data_t data)
{
    oversample_filter_info_t *a = &oversample_filter_info;

    /* 累加采样值 */
    a->sum = FILTER_ADD(a->sum, data);
    a->count++;

    /* 达到过采样率 → 输出平均值并重置 */
    if (a->count >= a->rate)
    {
        /* sum为Q格式, rate为整数, 直接除法保持Q格式 */
        a->last_output = a->sum / a->rate;
        a->sum   = FILTER_ZERO;
        a->count = 0;
        a->initialized = 1;
    }
}

filter_data_t oversample_filter_get_result(void)
{
    oversample_filter_info_t *a = &oversample_filter_info;

    /* 若尚未完成一次完整过采样周期,返回当前部分累加均值 */
    if (!a->initialized && (a->count > 0))
    {
        return a->sum / a->count;
    }

    /* 已完成过采样 → 返回最后输出的均值 */
    return a->last_output;
}
