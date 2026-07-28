#ifndef _FILTER_MANAGER_H_
#define _FILTER_MANAGER_H_
#include "Filter_data_type.h"
#include "Average_Filter.h" //均值滤波头文件
#include "Clipping_Filter.h" //限幅滤波头文件
#include "Median_Filter.h" //中值滤波头文件
#include "IIR_Filter.h" //一阶低通滤波头文件
#include "Compensation_Filter.h"

/*算法参数管理*/
typedef struct
{
    average_filter_info_t *average;
    clipping_filter_info_t *clipping;
    median_filter_info_t *median;
    IIR_filter_info_t *IIR;
    compensation_filter_info_t *compensation;
}filter_info_managet_t;

/*算法功能函数管理*/
/*统一功能函数类型*/
typedef struct
{
    void (*filter_init)(void);
    void (*filter_proc)(filter_data_t data);
    filter_data_t (*filter_get)(void);
}filter_func_type_t;

typedef struct
{
    filter_func_type_t *average;
    filter_func_type_t *clipping;
    filter_func_type_t *median;
    filter_func_type_t *iir;
    filter_func_type_t *compensation;
}filter_func_manager_t;

extern filter_func_manager_t filter_func_manager;

void filter_info_init(void);
filter_data_t temp_Optimize_filter(filter_data_t data);

#endif
