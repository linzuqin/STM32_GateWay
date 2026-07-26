#ifndef _COMPENSATION_FILTER_H_
#define _COMPENSATION_FILTER_H_
#include "Filter_data_type.h"

typedef struct
{
    filter_data_t add_factor;     // 加法因数
    filter_data_t mul_factor;     // 乘法因数
    filter_data_t compensation_filter_result;        // 自定义补偿算法结果
} compensation_filter_info_t;

extern compensation_filter_info_t compensation_filter_info;


#endif
