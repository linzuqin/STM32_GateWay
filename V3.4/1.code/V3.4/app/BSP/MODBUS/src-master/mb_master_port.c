#include "mb_master_port.h"

//串口发送函数 需要根据实际修改
static void mb_m_send(uint8_t *buf, uint16_t len)
{
    // test code for one slave device

	// HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

//	HAL_UART_Transmit(&huart1 , buf , len , 1000);
	// HAL_UART_Transmit_IT(&huart1, buf, len);

	// HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

//保持寄存器写入回调函数
void mbMaster_hold_set_callback(uint16_t addr, uint16_t val)
{

}

//线圈寄存器写入回调函数
void mbMaster_coil_set_callback(uint16_t addr, uint16_t val)
{

}

//mb_opt mb_master_opt =
//{
//	.write = mb_m_send,
//	.handle = HAL_GetTick,
//}; 
