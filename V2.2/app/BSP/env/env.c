#include "env.h"
#include "temp.h"
#include "STTS22HTR.h"
#include "tim.h"
#include "app_flashdb.h"

board_info_t board_info;


typedef struct
{
  uint32_t uid0;
  uint32_t uid1;
  uint32_t uid2;
} mcu_uid_t;

void board_info_refresh(void)
{
	board_info.board_temp = board_temp_get();
	board_info.env_temp = Get_temp();
	board_info.boot_count = Get_Boot();
	__HAL_TIM_SetCompare(&htim2 , TIM_CHANNEL_2 , board_info.fanSpeed);
}

void board_info_Init(void)
{
	mcu_uid_t uid = MCU_UID;
	sprintf(board_info.mcu_id , "%08X-%08X-%08X", uid.uid0, uid.uid1, uid.uid2);
}
