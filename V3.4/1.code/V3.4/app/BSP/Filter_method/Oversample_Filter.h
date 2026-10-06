#ifndef _OVERSAMPLE_FILTER_H_
#define _OVERSAMPLE_FILTER_H_
#include <stdint.h>
#include "Filter_data_type.h"

/**
 * @brief 过采样+抽取滤波器 (Oversampling & Decimation)
 * @description 积累 N 次采样后取平均输出一次，提升信噪比 √N 倍
 *              参考: 高精度ADC过采样技术、6位半万用表信号处理
 *              放置在滤波链最前端效果最佳
 */

/* 过采样率: 积累多少次采样输出一次平均值 */
#define OVERSAMPLE_RATE     8

typedef struct
{
    filter_data_t sum;              /*!< 累加和 */
    uint16_t count;                 /*!< 当前累计次数 */
    uint16_t rate;                  /*!< 过采样率 */
    filter_data_t last_output;      /*!< 上一次完整输出 */
    uint8_t initialized;            /*!< 是否已初始化 */
} oversample_filter_info_t;

extern oversample_filter_info_t oversample_filter_info;

void oversample_filter_info_init(void);
void oversample_filter_proc_data(filter_data_t data);
filter_data_t oversample_filter_get_result(void);

#endif
