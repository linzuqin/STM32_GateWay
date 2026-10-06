#ifndef _ENV_H_
#define _ENV_H_
#include "stdint.h"

typedef struct
{
	float board_temp;
	float env_temp;
	uint8_t fanSpeed;
}board_info_t;
extern board_info_t board_info;

void board_info_refresh(void);

#endif
