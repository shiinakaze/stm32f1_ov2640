#include "ov2640.h"

/* 双缓冲：缓冲区由上层提供（OV2640_SetFrameBuffer），驱动只持有指针。
 * 采集写入一块缓冲时，DMA 正在发送另一块，互不冲突。 */
static uint8_t *jpeg_buf[2] = {NULL, NULL};
static uint32_t jpeg_buf_size = 0;

/* 当前写入的缓冲索引；采集完成后指向刚写满并待发送的帧 */
static uint8_t capture_idx = 0;

/* 最近一次采集好的 JPEG 帧信息 */
static uint8_t *ready_frame_ptr = NULL;
static uint32_t ready_frame_len = 0;

void OV2640_SetFrameBuffer(uint8_t *buf0, uint8_t *buf1, uint32_t size)
{
    jpeg_buf[0] = buf0;
    jpeg_buf[1] = buf1;
    jpeg_buf_size = size;
}

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
    // OV2640_Init_Config(atk_mc2640_set_yuv422_cfg, sizeof(atk_mc2640_set_yuv422_cfg) / sizeof(ov2640_cfg_item_t));
    // OV2640_Init_Config(atk_mc2640_set_jpeg_cfg, sizeof(atk_mc2640_set_jpeg_cfg) / sizeof(ov2640_cfg_item_t));
    OV2640_Init_Config(ov2640_set_jpeg_cfg, sizeof(ov2640_set_jpeg_cfg) / sizeof(ov2640_cfg_item_t));
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

void OV2640_Capture(void)
{
    uint32_t idx = 0;
    uint32_t jpeg_start, jpeg_end;
    uint8_t end_found;
    uint8_t *buf = jpeg_buf[capture_idx];

    /* 先等当前帧结束（确保不会从帧中途开始采集，避免撕裂/闪烁） */
    while (OV2640_VSYNC == 1)
    {
    }
    /* 等待新帧 VSYNC 上升沿 */
    while (OV2640_VSYNC == 0)
    {
    }
    /* VSYNC 高电平期间按行读取整帧 */
    while (OV2640_VSYNC == 1)
    {
        while (OV2640_HREF == 1) // 当 HREF 为高，读取一行
        {
            while (OV2640_PCLK == 0) // 等待 PCLK 上升沿并读取数据
            {
            }
            if (idx < jpeg_buf_size)
            {
                buf[idx] = OV2640_READ_DATA();
            }
            idx++;
            while (OV2640_PCLK == 1) // 等待 PCLK 下降沿，数据更新
            {
            }
        }
    }

    /* 防止缓冲溢出：超过容量的数据被丢弃，但 idx 继续计数 */
    if (idx > jpeg_buf_size)
    {
        idx = jpeg_buf_size;
    }

    /* 定位 JPEG 起始标记 FF D8 */
    for (jpeg_start = 0; jpeg_start + 1 < idx; jpeg_start++)
    {
        if (buf[jpeg_start] == 0xFF && buf[jpeg_start + 1] == 0xD8)
        {
            break;
        }
    }
    if (jpeg_start + 1 >= idx)
    {
        /* 未找到 JPEG 起始，丢弃本帧 */
        ready_frame_ptr = NULL;
        ready_frame_len = 0;
        return;
    }

    /* 定位 JPEG 结束标记 FF D9 */
    end_found = 0;
    for (jpeg_end = jpeg_start + 1; jpeg_end + 1 < idx; jpeg_end++)
    {
        if (buf[jpeg_end] == 0xFF && buf[jpeg_end + 1] == 0xD9)
        {
            jpeg_end += 2; // 包含 FF D9
            end_found = 1;
            break;
        }
    }
    if (!end_found)
    {
        /* 未找到结束标记（JPEG 不完整/被截断），丢弃本帧避免解码异常 */
        ready_frame_ptr = NULL;
        ready_frame_len = 0;
        return;
    }

    /* 记录待发送帧，并切换缓冲：下一帧写入另一块，
     * 这样 DMA 发送本帧时不会与下一帧采集冲突 */
    ready_frame_ptr = buf + jpeg_start;
    ready_frame_len = jpeg_end - jpeg_start;
    capture_idx ^= 1;
}

uint8_t *OV2640_GetReadyFrame(uint32_t *len)
{
    if (len != NULL)
    {
        *len = ready_frame_len;
    }
    return ready_frame_ptr;
}

void OV2640_Init(void)
{
    OV2640_IO_Init();
    OV2640_HW_Reset();
    SW_SCCB_Init();

    // OV2640_Init_Config(atk_mc2640_init_uxga_cfg, sizeof(atk_mc2640_init_uxga_cfg) / sizeof(ov2640_cfg_item_t));
    OV2640_Init_Config(svga_rgb565_25fps_cfg, sizeof(svga_rgb565_25fps_cfg) / sizeof(ov2640_cfg_item_t));

    OV2640_Set_Output_JPEG();

    SCCB_Write(0XFF, 0x01);
    SCCB_Write(0X11, 0x00); // CLKRC
    SCCB_Write(0XFF, 0x00);
    SCCB_Write(0XD3, 0x64); // R_DVP_SP

    OV2640_Set_Output_Size(640, 360);
}
