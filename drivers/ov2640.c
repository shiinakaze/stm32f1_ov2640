#include "ov2640.h"

uint8_t jpeg_buffer[16 * 1024] = {0};

void OV2640_HW_Reset(void)
{
    // Reset Camera
    GPIO_ResetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, RESET_PIN);
    Delay_ms(10);
    GPIO_SetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, RESET_PIN);
    Delay_ms(10);
}

void OV2640_SetPowerDownMode(BitAction BitVal)
{
    // Set Device into Normal Mode
    if (BitVal)
    {
        GPIO_SetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, PWDN_PIN);
    }
    else
    {
        GPIO_ResetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, PWDN_PIN);
    }
}

uint16_t OV2640_GetPID(void)
{
    SCCB_Write(0xFF, 0x01);
    uint16_t PID = SCCB_Read(OV2640_SENSOR_PIDH);
    PID <<= 8;
    PID |= SCCB_Read(OV2640_SENSOR_PIDL);
    return PID;
}

uint16_t OV2640_GetMID(void)
{
    SCCB_Write(0xFF, 0x01);
    uint16_t MID = SCCB_Read(OV2640_SENSOR_MIDH);
    MID <<= 8;
    MID |= SCCB_Read(OV2640_SENSOR_MIDL);
    return MID;
}

/**
 * @brief  通用 OV2640 寄存器初始化函数（支持延时）
 * @param  cfg  配置数组指针
 * @param  len  数组长度
 */
void OV2640_Init_Config(const ov2640_cfg_item_t *cfg, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        SCCB_Write(cfg[i].reg, cfg[i].val);
        if (cfg[i].delay_ms)
        {
            Delay_ms(cfg[i].delay_ms);
        }
    }
}

void OV2640_IO_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB_OV2640, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    // RESET, output
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = RESET_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &GPIO_InitStructure);
    // HREF, VSYNC, input
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = HREF_PIN | VSYNC_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &GPIO_InitStructure);
    // PCLK, input
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = PCLK_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &GPIO_InitStructure);
    // PWDN, output
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = PWDN_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &GPIO_InitStructure);
    // DATA, input
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = DATA0_PIN | DATA1_PIN | DATA2_PIN | DATA3_PIN | DATA4_PIN | DATA5_PIN | DATA6_PIN | DATA7_PIN;
    GPIO_Init(DATA_GPIO, &GPIO_InitStructure);
}

void OV2640_Set_Output_JPEG(void)
{
    OV2640_Init_Config(atk_mc2640_set_yuv422_cfg, sizeof(atk_mc2640_set_yuv422_cfg) / sizeof(ov2640_cfg_item_t));
    OV2640_Init_Config(atk_mc2640_set_jpeg_cfg, sizeof(atk_mc2640_set_jpeg_cfg) / sizeof(ov2640_cfg_item_t));
}

void OV2640_Set_Output_Size(uint16_t width, uint16_t height)
{
    uint16_t output_width;
    uint16_t output_height;

    output_width = width >> 2;
    output_height = height >> 2;

    SCCB_Write(0xFF, 0x00);
    SCCB_Write(OV2640_DSP_RESET, 0x04);
    SCCB_Write(OV2640_DSP_ZMOW, (uint8_t)(output_width & 0x00FF));
    SCCB_Write(OV2640_DSP_ZMOH, (uint8_t)(output_height & 0x00FF));
    SCCB_Write(OV2640_DSP_ZMHH, ((uint8_t)(output_width >> 8) & 0x03) | ((uint8_t)(output_height >> 6) & 0x04));
    SCCB_Write(OV2640_DSP_RESET, 0x00);
}

void OV2640_Test_Capture_UART(void)
{
    uint32_t buffer_inedex = 0;
    uint32_t jpeg_valid_start, jpeg_valid_end = 0;
    while (OV2640_VSYNC == 0) // wait for new frame VSYNC rising edge
    {
    }
    while (OV2640_VSYNC == 1) // wait for new frame VSYNC falling edge
    {
        while (OV2640_HREF == 1) // when HREF high, read row
        {
            while (OV2640_PCLK == 0) // wait for PCLK rising edge and read data
            {
            }
            jpeg_buffer[buffer_inedex] = OV2640_READ_DATA();
            buffer_inedex++;
            while (OV2640_PCLK == 1) //  wait for PCLK falling edge, update data
            {
            }
        }
    }

    for (jpeg_valid_start = 0; jpeg_valid_start < buffer_inedex; jpeg_valid_start++)
    {
        if (jpeg_buffer[jpeg_valid_start] == 0xFF && jpeg_buffer[jpeg_valid_start + 1] == 0xD8)
        {

            for (jpeg_valid_end = jpeg_valid_start; jpeg_valid_end < buffer_inedex; jpeg_valid_end++)
            {
                if (jpeg_buffer[jpeg_valid_end] == 0xD9 && jpeg_buffer[jpeg_valid_end - 1] == 0xFF)
                {
                    UART1_Transmit(jpeg_buffer + jpeg_valid_start, jpeg_valid_end - jpeg_valid_start + 1);
                    break;
                }
            }
        }
    }
}

void OV2640_Init(void)
{
    OV2640_IO_Init();
    OV2640_HW_Reset();
    SW_SCCB_Init();

    OV2640_Init_Config(atk_mc2640_init_uxga_cfg, sizeof(atk_mc2640_init_uxga_cfg) / sizeof(ov2640_cfg_item_t));

    OV2640_Set_Output_JPEG();

    SCCB_Write(0XFF, 0x01);
    SCCB_Write(0X11, 0x00); // CLKRC
    SCCB_Write(0XFF, 0x00);
    SCCB_Write(0XD3, 0x64); // R_DVP_SP

    OV2640_Set_Output_Size(320, 240);
}
