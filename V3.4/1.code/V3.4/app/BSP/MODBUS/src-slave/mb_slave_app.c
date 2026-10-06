#include "mb_slave_app.h"

static uint8_t mb_s_coil_buf[MB_S_COIL_SIZE];
static uint8_t mb_s_disc_buf[MB_S_DISC_SIZE];
static uint16_t mb_s_hold_buf[MB_S_HOLD_SIZE];
static uint16_t mb_s_input_buf[MB_S_INPUT_SIZE];

static mb_reg_info info = 
{
    .mb_coil_reg = mb_s_coil_buf,
    .mb_disc_reg = mb_s_disc_buf,
    .mb_hold_reg = mb_s_hold_buf,
    .mb_input_reg = mb_s_input_buf,

    .coil_start_addr = 0,
    .coil_read_size = sizeof(mb_s_coil_buf),

    .disc_start_addr = 0,
    .disc_read_size = sizeof(mb_s_disc_buf),

    .hold_start_addr = 0,
    .hold_read_size = sizeof(mb_s_hold_buf)/2,//mb_s_hold_buf是uint16_t类型的 sizeof算出来的长度会是实际长度的2倍

    .input_start_addr = 0,
    .input_read_size = sizeof(mb_s_input_buf)/2,//mb_s_hold_buf是uint16_t类型的 sizeof算出来的长度会是实际长度的2倍
};


// mb_dev_t mb_slave_devs[MB_SLAVE_NUM];
// test code for one slave device
mb_dev_t mb_slave_devs[MB_SLAVE_NUM] =
{
    [0] = {
        .addr = 1,
        .uartid = 3,
        .dev_type = MB_SLAVE,

        .opt = &mb_slave_opt,
        .reg_info = &info,

        .hold_write_cb = mbSlave_hold_set_callback,
        .coil_write_cb = mbSlave_coil_set_callback,
    },
		[1] = {
        .addr = 1,
        .uartid = 0xff,
        .dev_type = MB_SLAVE,

        .opt = &mb_slave_tcp_opt,
        .reg_info = &info,

        .hold_write_cb = mbSlave_hold_set_callback,
        .coil_write_cb = mbSlave_coil_set_callback,
    },
};

void mb_s_poll(void)
{
    for(uint8_t i = 0;i<MB_SLAVE_NUM;i++)
    {
        mb_dev_t *dev = &mb_slave_devs[i];

        switch(dev->mb_state)
        {
            case MB_IDLE:
            {
                if(dev->opt->handle() - dev->tick > MB_SLAVE_POLL_INTERVAL)
                {
                    //超过预设的轮询时间 就判定为设备离线
                    dev->mb_state = MB_OFFLINE;
                }
                else if(dev->rx_size > 0)
                {
                    dev->mb_state = MB_PARSE;
                }
                break;
            }

            case MB_PARSE:
            {
                if(mb_s_parse(dev) == MB_OK)
                {
                    dev->dev_online = 1;//恢复为设备在线
                    dev->mb_state = MB_RESP;
                    dev->tick = dev->opt->handle();

                }
                else
                {
                    dev->mb_state = MB_ERROR;
                }
                break;
            }

            case MB_ERROR:
            {
                //这里可以添加自定义的错误处理

            }

            case MB_RESP:
            {
                if(dev->opt->write != NULL)
                {
                    dev->opt->write(dev->tx_buffer , dev->tx_size);
                }
                mb_clean(dev);
                dev->mb_state = MB_IDLE;
                break;
            }

            case MB_OFFLINE:
            {
                //设备离线处理
                dev->dev_online = 0;

                //自定义离线处理
                dev->tick = dev->opt->handle();
                dev->mb_state = MB_IDLE;
                break;
            }

            default:{
                //未知状态
                break;
            }
        }
    }
}
