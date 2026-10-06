#include "mb_master_app.h"
#include "mb_master_proc.h"
#include "mb_master_port.h"

//modbus主机专属的全局变量,专门用来存放需要下发数据的数组
static mb_m_map mb_m_coil_map[MB_M_COIL_SIZE];
static mb_m_map mb_m_hold_map[MB_M_HOLD_SIZE];

//modbus主机专属的全局变量,专门用来存放读取到的数据
static uint8_t mb_m_coil_buf[MB_M_COIL_SIZE];
static uint8_t mb_m_disc_buf[MB_M_DISC_SIZE];
static uint16_t mb_m_hold_buf[MB_M_HOLD_SIZE];
static uint16_t mb_m_input_buf[MB_M_INPUT_SIZE];

static mb_reg_info info = 
{
    .mb_coil_reg = mb_m_coil_buf,
    .mb_disc_reg = mb_m_disc_buf,
    .mb_hold_reg = mb_m_hold_buf,
    .mb_input_reg = mb_m_input_buf,
    
    .coil_start_addr = 0,//线圈的起始地址
    .coil_read_size = sizeof(mb_m_coil_buf),//线圈的数量
        
    .disc_start_addr = 0,//离散的起始地址
    .disc_read_size = sizeof(mb_m_disc_buf),//离散的数量
        
    .hold_start_addr = 0,
    .hold_read_size = sizeof(mb_m_hold_buf)/2,//mb_s_hold_buf是uint16_t类型的 sizeof算出来的长度会是实际长度的2倍
        
    .input_start_addr = 0,
    .input_read_size = sizeof(mb_m_input_buf)/2,//mb_s_hold_buf是uint16_t类型的 sizeof算出来的长度会是实际长度的2倍
};

// modbus主机设备列表
//   以下为默认配置 后续添加设备可以按照此参数配置
mb_dev_t mb_master_devs[MB_MASTER_NUM]	= 
{
    [0] = {
        .addr = 1,
        .uartid = 1,
        .dev_type = MB_MASTER,
//        .opt = &mb_master_opt,
        .reg_info = &info,

        .hold_write_cb = mbMaster_hold_set_callback,//保持寄存器设置回调函数
        .coil_write_cb = mbMaster_coil_set_callback,//线圈寄存器设置回调函数
        
        .coil_map = mb_m_coil_map,//主机线圈映射表
        .hold_map = mb_m_hold_map,//主机保持映射表
        
        .index_func_code = MB_FUNC_READ_COILS,
        .mb_state = MB_IDLE,
        .tick = 0,
        .dev_online = 0,
    }
};

void mb_m_poll(void)
{
    for(uint8_t i = 0; i < MB_MASTER_NUM; i++)
    {
        mb_dev_t *dev = &mb_master_devs[i];
        
        switch(dev->mb_state)
        {
            case MB_IDLE:
            {
                uint32_t current_time = dev->opt->handle();
                
                if(mb_map_check(dev))
                {
                    dev->mb_state = MB_SET;
                }
                else if(current_time - dev->tick >= MB_MASTER_POLL_INTERVAL)
                {
                    dev->tick = current_time;
                    dev->mb_state = MB_GET;
                }
                break;
            }

            case MB_GET:
            {
                mb_m_get(dev);

                break;
            }

            case MB_SET:
            {
                mb_m_set(dev);

                break;
            }
            
            case MB_WAIT:
            {
                if(dev->opt->handle() - dev->tick >= MB_MASTER_POLL_INTERVAL * 3) //应答超时
                {
                    dev->mb_state = MB_OFFLINE;
                    mb_clean(dev);
                }
                else if(dev->rx_size != 0)
                {
                    if(mb_m_check_ack(dev) == MB_OK)
                    {
                        dev->error_count = 0;
                        dev->mb_state = MB_IDLE;
                        dev->dev_online = 1;

                    }
                    else
                    {
                        dev->mb_state = MB_OFFLINE;
                    }
                    mb_clean(dev);
                }
                break;
            }

            case MB_OFFLINE:
            {
                dev->error_count++;
                if(dev->error_count > 5)
                {
                    dev->dev_online = 0;
                }
                else
                {

                }                
                dev->mb_state = MB_IDLE;
                break;
            }

            default:
                dev->mb_state = MB_IDLE;  // 未知状态回到IDLE
                break;
        }
    }
}
