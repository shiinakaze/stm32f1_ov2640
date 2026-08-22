#include "ov2640.h"

/* 帧缓冲由上层提供（ov2640_set_frame_buffer），驱动只持有指针。
 * UVC 流水线为串行：采集（USB 中断关闭）→ USB 发送 → 再采集，
 * 单缓冲即可，采集时缓冲不会被发送侧读取。 */
static uint8_t *jpeg_buf = NULL;
static uint32_t jpeg_buf_size = 0;

/* 最近一次采集好的 JPEG 帧信息 */
static uint8_t *ready_frame_ptr = NULL;
static uint32_t ready_frame_len = 0;

void ov2640_set_frame_buffer(uint8_t *buf, uint32_t size)
{
    jpeg_buf = buf;
    jpeg_buf_size = size;
}

void ov2640_hw_reset(void)
{
    // Reset Camera
    GPIO_ResetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, RESET_PIN);
    delay_ms(10);
    GPIO_SetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, RESET_PIN);
    delay_ms(10);
}

void ov2640_set_power_down_mode(BitAction bit_val)
{
    // Set Device into Normal Mode
    if (bit_val)
    {
        GPIO_SetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, PWDN_PIN);
    }
    else
    {
        GPIO_ResetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, PWDN_PIN);
    }
}

uint16_t ov2640_get_pid(void)
{
    SCCB_Write(0xFF, 0x01);
    uint16_t pid = SCCB_Read(OV2640_SENSOR_PIDH);
    pid <<= 8;
    pid |= SCCB_Read(OV2640_SENSOR_PIDL);
    return pid;
}

uint16_t ov2640_get_mid(void)
{
    SCCB_Write(0xFF, 0x01);
    uint16_t mid = SCCB_Read(OV2640_SENSOR_MIDH);
    mid <<= 8;
    mid |= SCCB_Read(OV2640_SENSOR_MIDL);
    return mid;
}

/**
 * @brief  通用 OV2640 寄存器初始化函数（支持延时）
 * @param  cfg  配置数组指针
 * @param  len  数组长度
 */
void ov2640_init_config(const ov2640_cfg_item_t *cfg, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        SCCB_Write(cfg[i].reg, cfg[i].val);
        if (cfg[i].delay_ms)
        {
            delay_ms(cfg[i].delay_ms);
        }
    }
}

void ov2640_io_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB_OV2640, ENABLE);

    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    // RESET, output
    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_struct.GPIO_Pin = RESET_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &gpio_init_struct);
    // HREF, VSYNC, input
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init_struct.GPIO_Pin = HREF_PIN | VSYNC_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &gpio_init_struct);
    // PCLK, input
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init_struct.GPIO_Pin = PCLK_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &gpio_init_struct);
    // PWDN, output
    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_struct.GPIO_Pin = PWDN_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &gpio_init_struct);
    // DATA, input
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init_struct.GPIO_Pin = DATA0_PIN | DATA1_PIN | DATA2_PIN | DATA3_PIN | DATA4_PIN | DATA5_PIN | DATA6_PIN | DATA7_PIN;
    GPIO_Init(DATA_GPIO, &gpio_init_struct);
}

void ov2640_set_output_jpeg(void)
{
    // ov2640_init_config(atk_mc2640_set_yuv422_cfg, sizeof(atk_mc2640_set_yuv422_cfg) / sizeof(ov2640_cfg_item_t));
    // ov2640_init_config(atk_mc2640_set_jpeg_cfg, sizeof(atk_mc2640_set_jpeg_cfg) / sizeof(ov2640_cfg_item_t));
    ov2640_init_config(ov2640_set_jpeg_cfg, sizeof(ov2640_set_jpeg_cfg) / sizeof(ov2640_cfg_item_t));
}

void ov2640_set_output_size(uint16_t width, uint16_t height)
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

void ov2640_capture(void)
{
    uint32_t idx = 0;
    uint32_t jpeg_start, jpeg_end;
    uint8_t end_found;
    uint8_t *buf = jpeg_buf;

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

    /* 记录待发送帧；帧缓冲由上层在发送完成后再复用 */
    ready_frame_ptr = buf + jpeg_start;
    ready_frame_len = jpeg_end - jpeg_start;
}

uint8_t *ov2640_get_ready_frame(uint32_t *len)
{
    if (len != NULL)
    {
        *len = ready_frame_len;
    }
    return ready_frame_ptr;
}

void ov2640_init(void)
{
    ov2640_io_init();
    ov2640_hw_reset();
    sw_sccb_init();

    // ov2640_init_config(atk_mc2640_init_uxga_cfg, sizeof(atk_mc2640_init_uxga_cfg) / sizeof(ov2640_cfg_item_t));
    ov2640_init_config(svga_rgb565_25fps_cfg, sizeof(svga_rgb565_25fps_cfg) / sizeof(ov2640_cfg_item_t));

    ov2640_set_output_jpeg();

    SCCB_Write(0XFF, 0x01);
    SCCB_Write(0X11, 0x00); // CLKRC
    SCCB_Write(0XFF, 0x00);
    SCCB_Write(0XD3, 0x64); // R_DVP_SP

    ov2640_set_output_size(320, 240);
}
