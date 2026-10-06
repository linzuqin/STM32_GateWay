#include "main.h"
#include "mqtt.h"
#include "msh.h"
#include "app_w5500_ntp.h"
#include "app_flashdb.h"
#include "cJSON.h"
#include "ota.h"
#include "digital.h"

#define DEBUG_ENABLE 1
#define DEBUG_LOG "[ MQTT ]"
#include "debug_print.h"

extern void (*mqttCallback)(int argc, char *argv);

mqtt_message_stuct mqtt_message;
char *MQTT_OneNet_BoardInfoRefresh(void)
{
	char *result = NULL;
	cJSON *root = cJSON_CreateObject();
	if(root == NULL)
	{
		DEBUG_PRINT("root malloc fail\r\n");
	}
	else
	{
		cJSON_AddStringToObject(root , "id" , "123");
//		cJSON_AddStringToObject(root , "version" , VERSION);
		cJSON *params_js = cJSON_CreateObject();
		if(params_js == NULL)
		{
				DEBUG_PRINT("params malloc fail\r\n");
		}
		else
		{
			char temp[64] = {0};

			if(app_w5500_ntp_get_str(temp) == 1)
			{
					cJSON *time_js = cJSON_CreateObject();
					cJSON_AddStringToObject(time_js , "value" , temp);
					cJSON_AddItemToObject(params_js , "BoardTime" , time_js);   
			}

			cJSON *temp_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(temp_js , "value" , board_info.board_temp); 
			cJSON_AddItemToObject(params_js , "BoardTemp" , temp_js);            

			cJSON *env_temp_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(env_temp_js , "value" , board_info.env_temp); 
			cJSON_AddItemToObject(params_js , "envTemp" , env_temp_js);   
						
			cJSON *boot_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(boot_js , "value" , board_info.boot_count);
			cJSON_AddItemToObject(params_js , "BoardBOOT" , boot_js);

			cJSON *uid_js = cJSON_CreateObject();
			cJSON_AddStringToObject(uid_js , "value" , board_info.mcu_id);
			cJSON_AddItemToObject(params_js , "BoardUID" , uid_js);
			
			cJSON *fanSpeed_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(fanSpeed_js , "value" , board_info.fanSpeed);
			cJSON_AddItemToObject(params_js , "fanSpeed" , fanSpeed_js);
			
			//电压上报
			cJSON *voltage_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(voltage_js , "value" , board_info.busVoltage);
			cJSON_AddItemToObject(params_js , "voltage" , voltage_js);

			//电流上报
			cJSON *current_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(current_js , "value" , board_info.current);
			cJSON_AddItemToObject(params_js , "current" , current_js);

			//功率上报
			cJSON *power_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(power_js , "value" , board_info.power);
			cJSON_AddItemToObject(params_js , "power" , power_js);
			
			//继电器状态上报
			cJSON *relay_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(relay_js , "value" , Read_ALL_digital_state_by_bit());
			cJSON_AddItemToObject(params_js , "relay" , relay_js);
			
			//报警状态
			cJSON *alarm_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(alarm_js , "value" , board_info.alarmFlag);
			cJSON_AddItemToObject(params_js , "alarmFlag" , alarm_js);
			
			//温度阈值上限
			cJSON *tempThresholdMax_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(tempThresholdMax_js , "value" , board_envThreshold.tempThresholdMax);
			cJSON_AddItemToObject(params_js , "tempThresholdMax" , tempThresholdMax_js);
			
			//温度回差
			cJSON *tempHysteresis_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(tempHysteresis_js , "value" , board_envThreshold.tempHysteresis);
			cJSON_AddItemToObject(params_js , "tempHysteresis" , tempHysteresis_js);			
			
			//电压阈值上限
			cJSON *voltageThresholdMax_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(voltageThresholdMax_js , "value" , board_envThreshold.voltageThresholdMax);
			cJSON_AddItemToObject(params_js , "voltageThresholdMax" , voltageThresholdMax_js);
			
			//电压阈值下限
			cJSON *voltageThresholdMin_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(voltageThresholdMin_js , "value" , board_envThreshold.voltageThresholdMin);
			cJSON_AddItemToObject(params_js , "voltageThresholdMin" , voltageThresholdMin_js);
			
			//电流阈值上限
			cJSON *currentThresholdMax_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(currentThresholdMax_js , "value" , board_envThreshold.currentThresholdMax);
			cJSON_AddItemToObject(params_js , "currentThresholdMax" , currentThresholdMax_js);
			
			//温度控制模式
			cJSON *tempControlMode_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(tempControlMode_js , "value" , board_info.tempControlMode);
			cJSON_AddItemToObject(params_js , "tempControlMode" , tempControlMode_js);
			
			//MQTT上报间隔
			cJSON *mqttRefreshInterval_js = cJSON_CreateObject();
			cJSON_AddNumberToObject(mqttRefreshInterval_js , "value" , board_envThreshold.mqttRefreshInterval);
			cJSON_AddItemToObject(params_js , "mqttRefreshInterval" , mqttRefreshInterval_js);

		}
		cJSON_AddItemToObject(root , "params" , params_js);
		result = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
	}
	return result;
}

char *MQTT_OneNet_MsgAck(char *id , char *msg , int code)
{
	char *result = NULL;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root , "id" , id);
	cJSON_AddStringToObject(root , "msg" , msg);
	cJSON_AddNumberToObject(root , "code" , code);
	
	result = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return result;
}

void MQTT_OneNet_MsgParse(char *payload)
{
	cJSON *root = cJSON_Parse(payload);
	if(root != NULL)
	{
		cJSON *id_js = cJSON_GetObjectItem(root , "id");
		if(id_js!= NULL)
		{
				memcpy(mqtt_message.id , id_js->valuestring , strlen(id_js->valuestring));
		}

		cJSON *version_js = cJSON_GetObjectItem(root , "version");
		if(version_js!= NULL)
		{
				memcpy(mqtt_message.version , version_js->valuestring , strlen(version_js->valuestring));
		}

		cJSON *params_js = cJSON_GetObjectItem(root , "params");
		if(params_js != NULL)
		{
			cJSON *update_js = cJSON_GetObjectItem(params_js , "update");
			if(update_js != NULL)
			{
					ota_set_start();
			}
			
			//温度控制模式
			cJSON *tempControlMode_js = cJSON_GetObjectItem(params_js , "tempControlMode");
			if(tempControlMode_js != NULL)
			{
					board_info.tempControlMode = (tempControlMode_t)tempControlMode_js->valueint;
			}
						
			//风速控制
			cJSON *fanSpeed_js = cJSON_GetObjectItem(params_js , "fanSpeed");
			if(fanSpeed_js != NULL)
			{
				if(board_info.tempControlMode == MANUAL)
				{
					board_info.fanSpeed = fanSpeed_js->valueint;
				}
			}
			
			//继电器控制
			cJSON *relay_js = cJSON_GetObjectItem(params_js , "relay");
			if(relay_js != NULL)
			{
				Set_digital(0 , (relay_js->valueint >> 0) & 0x01);
				Set_digital(1 , (relay_js->valueint >> 1) & 0x01);
			}
			
			//电流阈值上限
			cJSON *currentThresholdMax_js = cJSON_GetObjectItem(params_js , "currentThresholdMax");
			if(currentThresholdMax_js != NULL)
			{
					board_envThreshold.currentThresholdMax = currentThresholdMax_js->valueint;
					board_envThreshold.saveFlag = 1;
			}
			
			//电压阈值上限
			cJSON *voltageThresholdMax_js = cJSON_GetObjectItem(params_js , "voltageThresholdMax");
			if(voltageThresholdMax_js != NULL)
			{
					board_envThreshold.voltageThresholdMax = voltageThresholdMax_js->valueint;
					board_envThreshold.saveFlag = 1;
			}			
			
			//电压阈值下限
			cJSON *voltageThresholdMin_js = cJSON_GetObjectItem(params_js , "voltageThresholdMin");
			if(voltageThresholdMin_js != NULL)
			{
					board_envThreshold.voltageThresholdMin = voltageThresholdMin_js->valueint;
					board_envThreshold.saveFlag = 1;
			}
			
			//温度阈值上限
			cJSON *tempThresholdMax_js = cJSON_GetObjectItem(params_js , "tempThresholdMax");
			if(tempThresholdMax_js != NULL)
			{
					board_envThreshold.tempThresholdMax = tempThresholdMax_js->valueint;
					board_envThreshold.saveFlag = 1;
			}
			
			//msh指令下发
			cJSON *msh_js = cJSON_GetObjectItem(params_js , "msh");
			if(msh_js != NULL)
			{
				uint8_t i = 0;
				for(i = 0;i<msh_table_len;i++)
				{
					if(strcmp(msh_js->valuestring , msh_cmd_table[i].cmd) == 0)
					{
						if(msh_cmd_table[i].callback != NULL)
						{
							mqttCallback = msh_cmd_table[i].callback;
						}
					}
				}
			}
			
			//温度回差设置
			cJSON *tempHysteresis_js = cJSON_GetObjectItem(params_js , "tempHysteresis");
			if(tempHysteresis_js != NULL)
			{
					board_envThreshold.tempHysteresis = tempHysteresis_js->valueint;
					board_envThreshold.saveFlag = 1;
			}
			
			//MQTT上报间隔
			cJSON *mqttRefreshInterval_js = cJSON_GetObjectItem(params_js , "mqttRefreshInterval");
			if(mqttRefreshInterval_js != NULL)
			{
					board_envThreshold.mqttRefreshInterval = mqttRefreshInterval_js->valueint;
					board_envThreshold.saveFlag = 1;
			}			
			
			sprintf(mqtt_message.ack_message , "succ");
			mqtt_message.ack_code = 200;
			mqtt_message.need_ack = 1;
		}
		else
		{
			sprintf(mqtt_message.ack_message , "no params");
			mqtt_message.ack_code = 200;
			mqtt_message.need_ack = 1;
		}
		cJSON_Delete(root);
	}	
}
