#ifndef _IIR_FILTER_H_
#define _IIR_FILTER_H_
#include <stdint.h>
#include "Filter_data_type.h"


#define IIR_SHIFT_BITS      2   /*表示右移的位数 替代乘法*/

typedef struct
{
    filter_data_t IIR_filter_laster_result;
    filter_data_t IIR_filter_result;
}IIR_filter_info_t;
extern IIR_filter_info_t IIR_filter_info;

filter_data_t IIR_filter_get_result(void);
void IIR_filter_proc_data(filter_data_t data);
void IIR_filter_info_init(void);


#endif
