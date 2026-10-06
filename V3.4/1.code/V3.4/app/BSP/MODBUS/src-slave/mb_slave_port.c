#include "mb_slave_port.h"
#include "usart.h"
#include "app_w5500.h"

static void mb_s_send(uint8_t *buf, uint16_t len)
{
    // test code for one slave device
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

    HAL_UART_Transmit(&huart3 , buf , len , 1000);

    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

static void mb_s_tcp_send(uint8_t *buf, uint16_t len)
{
    // test code for one slave device
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
		send(TCP_SERVER_MODBUS_SLAVE_SOCKET, (uint8_t *)buf, len);

    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

void mbSlave_hold_set_callback(uint16_t addr, uint16_t val)
{
}

void mbSlave_coil_set_callback(uint16_t addr, uint16_t val)
{
}

mb_opt mb_slave_opt =
{
	.write = mb_s_send,
	.handle = HAL_GetTick,
};

mb_opt mb_slave_tcp_opt =
{
	.write = mb_s_tcp_send,
	.handle = HAL_GetTick,
};
