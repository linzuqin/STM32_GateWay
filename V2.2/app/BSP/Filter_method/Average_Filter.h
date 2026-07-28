#ifndef _AVERAGE_FILTER_H_
#define _AVERAGE_FILTER_H_
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Filter_data_type.h"

#define SAMPLING_MAX    4
typedef struct
{
    filter_data_t average_buf[SAMPLING_MAX]; //采样数组 bit0为最早的数据
    uint8_t sampling_count; //采样数量
    filter_data_t average_filter_sum;//总和
    filter_data_t average_filter_result; //平均滤波结果
}average_filter_info_t;
extern average_filter_info_t average_filter_info;

void average_filter_info_init(void);
void average_filter_proc_data(filter_data_t data);
filter_data_t average_filter_get_result(void);

#endif
