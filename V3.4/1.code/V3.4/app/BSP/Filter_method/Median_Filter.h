#ifndef _MEDIAN_FILTER_H_
#define _MEDIAN_FILTER_H_
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Filter_data_type.h"

#define MEDIAN_SAMPLING_MAX    5

typedef struct
{
    filter_data_t median_buf[MEDIAN_SAMPLING_MAX];   // 采样缓冲区
    filter_data_t sort_buf[MEDIAN_SAMPLING_MAX];     // 排序缓冲区（不破坏原始数据顺序）
    uint8_t sampling_count;                  // 当前采样数量
    filter_data_t median_filter_result;              // 中值滤波结果
} median_filter_info_t;

extern median_filter_info_t median_filter_info;

void median_filter_info_init(void);
void median_filter_proc_data(filter_data_t data);
filter_data_t median_filter_get_result(void);

#endif
