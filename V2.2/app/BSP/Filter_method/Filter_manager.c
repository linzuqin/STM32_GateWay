#include "Filter_manager.h"
/**
 * 
 * @brief 算法控制器 用来组合各种算法 以及统一管理会用到的参数变量 
 * @author LinZuQin (1904499306@qq.com)
 * @date 2026-07-21 09:45:33
 * @copyright Copyright (c) 2026
 */
#define DEBUG_ENABLE    0
#define DEBUG_LOG "[ FILTER ]"
#define DEBUG_PRINT(fmt, ...) do {if (DEBUG_ENABLE) printf(DEBUG_LOG "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)

extern filter_func_type_t clipping_filter_func;
extern filter_func_type_t median_filter_func;
extern filter_func_type_t average_filter_func;
extern filter_func_type_t IIR_filter_func;
extern filter_func_type_t compensation_filter_func;

/*所有算法的功能函数*/
filter_func_manager_t filter_func_manager = 
{
    .average = &average_filter_func,
    .clipping = &clipping_filter_func,
    .median = &median_filter_func,
    .iir = &IIR_filter_func,
    .compensation = &compensation_filter_func,
};

/*所有的算法参数*/
filter_info_managet_t filter_info_manager = 
{
    .average = &average_filter_info,
    .clipping = &clipping_filter_info,
    .IIR = &IIR_filter_info,
    .median = &median_filter_info,
};

/*初始化所有算法的参数*/
void filter_info_init(void)
{
    filter_func_manager.average->filter_init();
    filter_func_manager.clipping->filter_init();
    filter_func_manager.iir->filter_init();
    filter_func_manager.median->filter_init();
    filter_func_manager.compensation->filter_init();
}

/*针对温度读取优化的算法组合*/
filter_data_t temp_Optimize_filter(filter_data_t data)
{
    filter_data_t result = FILTER_ZERO;
    filter_data_t after_clipping  = FILTER_ZERO;
    filter_data_t after_median    = FILTER_ZERO;
    filter_data_t after_average   = FILTER_ZERO;
    filter_data_t after_iir       = FILTER_ZERO;

    /*限幅滤波处理*/
    filter_func_manager.clipping->filter_proc(data);
    after_clipping = filter_func_manager.clipping->filter_get();

    /*中值滤波处理*/
    filter_func_manager.median->filter_proc(after_clipping);
    after_median = filter_func_manager.median->filter_get();

    /*平均滤波处理*/
    filter_func_manager.average->filter_proc(after_median);
    after_average = filter_func_manager.average->filter_get();

    /*一阶低通IIR滤波(EMA)*/
    filter_func_manager.iir->filter_proc(after_average);
    after_iir = filter_func_manager.iir->filter_get();

    /*自定义校正*/
    filter_func_manager.compensation->filter_proc(after_iir);
    result = filter_func_manager.compensation->filter_get();

#if FILTER_USE_FLOAT
    DEBUG_PRINT("data:%.5f , after_clipping:%.5f , after_median:%.5f , after_average:%.5f , after_iir:%.5f , result:%.5f\r\n" ,
        FILTER_TO_FLOAT(data), FILTER_TO_FLOAT(after_clipping), FILTER_TO_FLOAT(after_median),
        FILTER_TO_FLOAT(after_average), FILTER_TO_FLOAT(after_iir), FILTER_TO_FLOAT(result));
#else
    DEBUG_PRINT("data:%d , after_clipping:%d , after_median:%d , after_average:%d , after_iir:%d , result:%d\r\n" ,
        data, after_clipping, after_median, after_average, after_iir, result);
#endif
    return result;
}
