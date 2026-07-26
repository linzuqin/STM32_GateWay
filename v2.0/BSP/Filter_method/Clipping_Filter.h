#ifndef _CLIPPING_FILTER_H_
#define _CLIPPING_FILTER_H_
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Filter_data_type.h"

typedef struct
{
    filter_data_t last_value; //上次采样值
    filter_data_t High_limit_value; //采样值上限
    filter_data_t Low_limit_value; //采样值下限
    uint8_t error_count;//异常数据次数 若连续异常超过五次 则排除噪声干扰 归为实际读数
    filter_data_t clipping_filter_result; //限幅滤波结果
}clipping_filter_info_t;

extern clipping_filter_info_t clipping_filter_info;

void clipping_filter_info_init(void);
void clipping_filter_proc_data(filter_data_t data);
filter_data_t clipping_filter_get_result(void);

#endif
