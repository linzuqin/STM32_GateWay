#include "env.h"
#include "temp.h"
#include "STTS22HTR.h"
#include "tim.h"
#include "app_flashdb.h"
#include "INA226.h"

#define DEBUG_ENABLE 1
#define DEBUG_LOG "[ ENV ]"
#include "debug_print.h"

board_info_t board_info;
board_envThreshold_t board_envThreshold = 
{
	.tempThresholdMax = 35,
	.voltageThresholdMax = 15,
	.currentThresholdMax = 3,
	.voltageThresholdMin = 10,
	.tempHysteresis = 5,
	.mqttRefreshInterval = 60,
};

void board_info_refresh(void)
{
	INA226_ReadData();

	board_info.board_temp = board_temp_get();
	board_info.env_temp = Get_temp();
	board_info.boot_count = Get_Boot();
}

void board_info_Init(void)
{
	uint32_t uid[3];
	uid[0] = HAL_GetUIDw0();
	uid[1] = HAL_GetUIDw0();
	uid[2] = HAL_GetUIDw0();

	sprintf(board_info.mcu_id , "%08X-%08X-%08X", uid[0], uid[1], uid[2]);
	
	board_envThreshold_t temp;
	if(app_flashdb_get("env" , &temp , sizeof(temp)) > 0)
	{
		board_envThreshold = temp;
	}
	DEBUG_PRINT("tempThresholdMax:%d  voltageThresholdMax:%d  currentThresholdMax:%d  voltageThresholdMin:%d  tempHysteresis:%d  mqttRefreshInterval:%d\r\n" ,board_envThreshold.tempThresholdMax,board_envThreshold.voltageThresholdMax , board_envThreshold.currentThresholdMax , board_envThreshold.voltageThresholdMin , board_envThreshold.tempHysteresis , board_envThreshold.mqttRefreshInterval );
}

void env_proc(void)
{
	/* 1.先对手自动模式下的风扇转速做一个判断 */
	switch(board_info.tempControlMode)
	{
		case AUTO:
		{
			if((board_info.board_temp > board_envThreshold.tempThresholdMax) || (board_info.env_temp > board_envThreshold.tempThresholdMax) )
			{
				board_info.fanSpeed = 100;
			}
			else if((board_info.board_temp < (board_envThreshold.tempThresholdMax - board_envThreshold.tempHysteresis)) && (board_info.env_temp < (board_envThreshold.tempThresholdMax - board_envThreshold.tempHysteresis))) //这里减1做一个回差
			{
				board_info.fanSpeed = 0;
			}
			break;
		}
		
		case MANUAL:
		{
			break;
		}
	}
	
	/* 2.报警数据判断 */
	if(board_info.busVoltage > board_envThreshold.voltageThresholdMax) //过压判断
	{
		board_info.alarmFlag |= ALARM_FLAG_OVER_VOLTAGE;
		board_info.fanSpeed = 0;
	}
	else
	{
		board_info.alarmFlag &= ~ALARM_FLAG_OVER_VOLTAGE;
	}
	
	if(board_info.busVoltage < board_envThreshold.voltageThresholdMin) //欠压判断
	{
		board_info.alarmFlag |= ALARM_FLAG_LOWTEMP;
		board_info.fanSpeed = 0;
	}
	else
	{
		board_info.alarmFlag &= ~ALARM_FLAG_LOWTEMP;
	}
	
	if(board_info.current > board_envThreshold.currentThresholdMax) //过流判断
	{
		board_info.alarmFlag |= ALARM_FLAG_OVER_CURRENT;
		board_info.fanSpeed = 0;
	}
	else
	{
		board_info.alarmFlag &= ~ALARM_FLAG_OVER_CURRENT;
	}
	
	if((board_info.board_temp > board_envThreshold.tempThresholdMax) || (board_info.env_temp > board_envThreshold.tempThresholdMax) ) //环境高温判断
	{
		board_info.alarmFlag |= ALARM_FLAG_OVER_TEMP;
	}
	else
	{
		board_info.alarmFlag &= ~ALARM_FLAG_OVER_TEMP;
	}
	
	__HAL_TIM_SetCompare(&htim2 , TIM_CHANNEL_2 , board_info.fanSpeed);
	
	/* 3.判断是否需要保存 */
	if(board_envThreshold.saveFlag ==1)
	{
		app_flashdb_set("env" , &board_envThreshold , sizeof(board_envThreshold_t));
		board_envThreshold.saveFlag = 0;
	}
}