#ifndef _MB_SLAVE_PORT_H_
#define _MB_SLAVE_PORT_H_
#include "mb.h"
#include "stdint.h"

void mbSlave_hold_set_callback(uint16_t addr, uint16_t val);
void mbSlave_coil_set_callback(uint16_t addr, uint16_t val);

extern mb_opt mb_slave_opt;
extern mb_opt mb_slave_tcp_opt;

#endif

