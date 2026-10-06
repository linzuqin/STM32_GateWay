#include "app_can.h"
#include <string.h>
#include <stdint.h>
uint32_t  tx_mailbox;
void app_can_send(uint8_t *data)
{
    CAN_TxHeaderTypeDef tx_header;

    tx_header.StdId = 0x030;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 8;
    tx_header.TransmitGlobalTime = DISABLE;

    // 阻塞发送，超时100ms
    HAL_CAN_AddTxMessage(&hcan , &tx_header , data , &tx_mailbox);

}

