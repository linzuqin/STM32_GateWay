#ifndef _MB_SLAVE_PROC_H_
#define _MB_SLAVE_PROC_H_
#include "mb.h"


mb_err_code_t mb_slave_check(mb_dev_t *mb_dev, mb_func_code_t func_code, uint16_t start_addr, uint16_t quantity);
/*modbus从机对于读取指令的应答*/
mb_err_t mb_s_build_response(mb_dev_t *mb_dev, mb_func_code_t func_code, uint16_t start_addr, uint16_t reg_count, uint8_t *response, uint16_t response_size);
uint16_t mb_s_get_response_size(mb_func_code_t func_code, uint16_t quantity);
/*modbus从机对于写入指令的应答*/
mb_err_t mb_s_coil_parse(mb_dev_t *mb_dev, uint16_t start_addr, uint16_t val);
mb_err_t mb_s_hold_parse(mb_dev_t *mb_dev, uint16_t start_addr, uint16_t val);
mb_err_t mb_s_coils_parse(mb_dev_t *mb_dev, uint16_t start_addr, uint8_t *val, uint16_t quantity);
mb_err_t mb_s_holds_parse(mb_dev_t *mb_dev, uint16_t start_addr, uint8_t *val, uint16_t quantity);
mb_err_t mb_s_parse(mb_dev_t *mb_dev);

#endif
