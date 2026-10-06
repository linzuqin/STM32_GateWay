#ifndef _ENV_H_
#define _ENV_H_
#include "stdint.h"

#define ALARM_FLAG_OVER_TEMP	0x01	//高温报警
#define ALARM_FLAG_OVER_VOLTAGE	0x02	//高压报警
#define ALARM_FLAG_OVER_CURRENT	0x04	//过流报警
#define ALARM_FLAG_LOWTEMP	0x08	//低压报警

typedef enum
{
	AUTO = 0,
	MANUAL,
}tempControlMode_t;

typedef struct
{
	int tempThresholdMax; //温度阈值上限
	int voltageThresholdMax; //电压阈值上限
	int currentThresholdMax; //电流阈值上限
	int voltageThresholdMin; //电压阈值下限
	int tempHysteresis;//温度回差
	uint8_t saveFlag;
	uint16_t mqttRefreshInterval; //MQTT上报间隔

}board_envThreshold_t;

typedef struct
{
	float board_temp;
	float env_temp;
	uint8_t fanSpeed;
	uint32_t boot_count;
	char mcu_id[32];
	
	float busVoltage;    //总线电压,V
	float shuntVoltage;  //采样电阻两端电压,V
	float current;       //电流,A
	float power;         // 功率,W
	
	tempControlMode_t tempControlMode; //温控模式 0:自动  1:手动
	
	uint8_t alarmFlag;
	
}board_info_t;

extern board_info_t board_info;
extern board_envThreshold_t board_envThreshold;
void board_info_Init(void);
void board_info_refresh(void);
void env_proc(void);

#endif
