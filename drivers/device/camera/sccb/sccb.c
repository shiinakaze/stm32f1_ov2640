#include "sccb.h"

/**
 * @brief  初始化 SCCB 总线引脚（推挽输出，起始/停止间隙总线上两线均为主机驱动）
 */
void sw_sccb_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB_SCCB_GPIO, ENABLE);

    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_struct.GPIO_Pin = SIO_C_PIN | SIO_D_PIN;
    GPIO_Init(SCCB_GPIO, &gpio_init_struct);
}

/** SIO_D 切换为上拉输入，用于读取从机应答/数据位 */
static void sw_sccb_sio_d_set_input(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init_struct.GPIO_Pin = SIO_D_PIN;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SCCB_GPIO, &gpio_init_struct);
}

/** SIO_D 切换回推挽输出 */
static void sw_sccb_sio_d_set_output(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_struct.GPIO_Pin = SIO_D_PIN;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SCCB_GPIO, &gpio_init_struct);
}

/** 起始条件：SIO_C 高电平期间 SIO_D 由高变低 */
static void sw_sccb_start(void)
{
    // make sure SIO_C SIO_D high
    SW_SIO_C_WRITE(Bit_SET);
    SW_SIO_D_WRITE(Bit_SET);
    SW_SCCB_DELAY();
    // SCCB Start condition
    SW_SIO_D_WRITE(Bit_RESET);
    SW_SCCB_DELAY();
    SW_SIO_C_WRITE(Bit_RESET);
    SW_SCCB_DELAY();
}

/** 停止条件：SIO_C 高电平期间 SIO_D 由低变高 */
static void sw_sccb_stop(void)
{
    // make sure SIO_D low
    SW_SIO_D_WRITE(Bit_RESET);
    SW_SCCB_DELAY();
    // SCCB Stop condition
    SW_SIO_C_WRITE(Bit_SET);
    SW_SCCB_DELAY();
    SW_SIO_D_WRITE(Bit_SET);
    SW_SCCB_DELAY();
}

/**
 * @brief  发送一个字节并检查第 9 位应答（X/Don't care 位）
 * @retval SCCB_OK 发送成功；SCCB_FAIL 无应答
 */
static uint8_t sw_sccb_write_byte(uint8_t byte)
{
    uint8_t x_bit;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (byte & 0x80)
        {
            SW_SIO_D_WRITE(Bit_SET);
        }
        else
        {
            SW_SIO_D_WRITE(Bit_RESET);
        }
        byte <<= 1;
        SW_SCCB_DELAY();
        SW_SIO_C_WRITE(Bit_SET);
        SW_SCCB_DELAY();
        SW_SIO_C_WRITE(Bit_RESET);
        SW_SCCB_DELAY();
    }
    sw_sccb_sio_d_set_input();
    SW_SCCB_DELAY();
    SW_SIO_C_WRITE(Bit_SET);
    x_bit = SW_SIO_D_READ(); // X/Don't care bit, typical value is 0
    SW_SCCB_DELAY();
    SW_SIO_C_WRITE(Bit_RESET);
    SW_SCCB_DELAY();
    sw_sccb_sio_d_set_output();

    // check sccb data
    if (x_bit != 0)
    {
        return SCCB_FAIL;
    }
    return SCCB_OK;
}

/**
 * @brief  读取一个字节，末位发送 NA（=1）表示主机不再继续读
 * @param  data 读到的字节（仅成功时有效）
 * @retval SCCB_OK 读取成功；SCCB_FAIL 无应答
 */
static uint8_t sw_sccb_read_byte(uint8_t *data)
{
    uint8_t na_bit;
    uint8_t byte = 0x00;
    sw_sccb_sio_d_set_input();
    for (uint8_t i = 0; i < 8; i++)
    {
        SW_SCCB_DELAY();
        SW_SIO_C_WRITE(Bit_SET);
        if (SW_SIO_D_READ())
        {
            byte |= (0x80 >> i);
        }
        SW_SCCB_DELAY();
        SW_SIO_C_WRITE(Bit_RESET);
        SW_SCCB_DELAY();
    }
    SW_SCCB_DELAY();
    SW_SIO_C_WRITE(Bit_SET);
    na_bit = SW_SIO_D_READ(); // NA bit, typical value is 1
    SW_SCCB_DELAY();
    SW_SIO_C_WRITE(Bit_RESET);
    SW_SCCB_DELAY();
    sw_sccb_sio_d_set_output();

    if (na_bit != 1)
    {
        return SCCB_FAIL;
    }
    *data = byte;
    return SCCB_OK;
}

/**
 * @brief  三相写：ID 地址 + 寄存器地址 + 数据
 * @param  id_address 器件写地址（OV2640 为 0x60）
 * @retval SCCB_OK 写入成功；SCCB_FAIL 任一阶段无应答
 */
uint8_t sw_sccb_write_reg(uint8_t id_address, uint8_t sub_address, uint8_t data)
{
    uint8_t status = SCCB_OK;

    sw_sccb_start();
    status |= sw_sccb_write_byte(id_address);
    status |= sw_sccb_write_byte(sub_address);
    status |= sw_sccb_write_byte(data);
    sw_sccb_stop();

    return status ? SCCB_FAIL : SCCB_OK;
}

/**
 * @brief  两相写寄存器地址 + 两相读数据
 * @param  id_address 器件写地址（OV2640 为 0x60）
 * @param  data 读到的寄存器值（仅成功时有效）
 * @retval SCCB_OK 读取成功；SCCB_FAIL 无应答
 */
uint8_t sw_sccb_read_reg(uint8_t id_address, uint8_t sub_address, uint8_t *data)
{
    uint8_t status = SCCB_OK;

    // 2-Phase Write
    sw_sccb_start();
    status |= sw_sccb_write_byte(id_address);
    status |= sw_sccb_write_byte(sub_address);
    sw_sccb_stop();

    // 2-Phase Read
    sw_sccb_start();
    status |= sw_sccb_write_byte(id_address | 0x01);
    if (status == SCCB_OK)
    {
        status = sw_sccb_read_byte(data);
    }
    sw_sccb_stop();

    return status;
}
