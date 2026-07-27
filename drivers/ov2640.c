#include "ov2640.h"
#include <stdio.h>

uint8_t jpeg_buffer[16 * 1024] = {0};

volatile uint32_t g_href_count = 0;

static void TIM2_PCLK_Counter_Init(void);
static void TIM4_TimeBase_Init(void);
static void HREF_EXTI_Init(void);
static void Counter_Reset_All(void);
static void Counter_Start_All(void);
static void Counter_Stop_All(void);

void OV2640_HW_Reset(void)
{
    GPIO_ResetBits(CTRL_GPIO, RESET_PIN);
    Delay_ms(10);
    GPIO_SetBits(CTRL_GPIO, RESET_PIN);
    Delay_ms(10);
}

void OV2640_SetPowerDownMode(BitAction BitVal)
{
    if (BitVal)
    {
        GPIO_SetBits(CTRL_GPIO, PWDN_PIN);
    }
    else
    {
        GPIO_ResetBits(CTRL_GPIO, PWDN_PIN);
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

    // RESET, PWDN output
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = RESET_PIN | PWDN_PIN;
    GPIO_Init(CTRL_GPIO, &GPIO_InitStructure);

    // VSYNC, HREF input
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = VSYNC_PIN | HREF_PIN;
    GPIO_Init(CTRL_GPIO, &GPIO_InitStructure);

    // PCLK input
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = PCLK_PIN;
    GPIO_Init(PCLK_GPIO, &GPIO_InitStructure);

    // DATA input
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = DATA0_PIN | DATA1_PIN | DATA2_PIN | DATA3_PIN |
                                  DATA4_PIN | DATA5_PIN | DATA6_PIN | DATA7_PIN;
    GPIO_Init(DATA_GPIO, &GPIO_InitStructure);

    // TIM2 partial remap: CH1 -> PA15
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM2, ENABLE);
}

void OV2640_Set_Output_JPEG(void)
{
    // 1) 确保走DSP
    SCCB_Write(0xFF, 0x00);
    SCCB_Write(0x05, 0x00); // R_BYPASS = 0, use DSP
    // 2) 先配置为 YUV422 路径
    OV2640_Init_Config(atk_mc2640_set_yuv422_cfg,
                       sizeof(atk_mc2640_set_yuv422_cfg) / sizeof(ov2640_cfg_item_t));
    // 3) 配置 JPEG 相关寄存器
    OV2640_Init_Config(atk_mc2640_set_jpeg_cfg,
                       sizeof(atk_mc2640_set_jpeg_cfg) / sizeof(ov2640_cfg_item_t));
    // 4) JPEG量化参数，可按需覆盖
    SCCB_Write(0xFF, 0x00);
    SCCB_Write(0x44, 0x0C); // Qs
    // 5) IMAGE_MODE: JPEG enable
    SCCB_Write(0xDA, 0x12); // 或 0x12
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

    while (OV2640_VSYNC == 0)
    {
    }
    while (OV2640_VSYNC == 1)
    {
        while (OV2640_HREF == 1)
        {
            while (OV2640_PCLK == 0)
            {
            }
            jpeg_buffer[buffer_inedex] = OV2640_READ_DATA();
            buffer_inedex++;
            if (buffer_inedex >= sizeof(jpeg_buffer))
                break;
            while (OV2640_PCLK == 1)
            {
            }
        }
        if (buffer_inedex >= sizeof(jpeg_buffer))
            break;
    }

    for (jpeg_valid_start = 0; jpeg_valid_start + 1 < buffer_inedex; jpeg_valid_start++)
    {
        if (jpeg_buffer[jpeg_valid_start] == 0xFF && jpeg_buffer[jpeg_valid_start + 1] == 0xD8)
        {
            for (jpeg_valid_end = jpeg_valid_start + 1; jpeg_valid_end < buffer_inedex; jpeg_valid_end++)
            {
                if (jpeg_buffer[jpeg_valid_end] == 0xD9 && jpeg_buffer[jpeg_valid_end - 1] == 0xFF)
                {
                    UART1_Transmit(jpeg_buffer + jpeg_valid_start, jpeg_valid_end - jpeg_valid_start + 1);
                    break;
                }
            }
            break;
        }
    }
}

/*-------------------- 时序测试相关 --------------------*/

static void TIM2_PCLK_Counter_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_DeInit(TIM2);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFFFFFF;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_TIxExternalClockConfig(TIM2, TIM_TIxExternalCLK1Source_TI1, TIM_ICPolarity_Rising, 0x00);

    TIM_SetCounter(TIM2, 0);
    TIM_Cmd(TIM2, DISABLE);
}

static void TIM4_TimeBase_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_DeInit(TIM4);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 71; // 72MHz / (71+1) = 1MHz
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_SetCounter(TIM4, 0);
    TIM_Cmd(TIM4, DISABLE);
}

static void HREF_EXTI_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource13);

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line13;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void OV2640_TimerMeasure_Init(void)
{
    TIM2_PCLK_Counter_Init();
    TIM4_TimeBase_Init();
    HREF_EXTI_Init();
}

static void Counter_Reset_All(void)
{
    g_href_count = 0;
    TIM_SetCounter(TIM2, 0);
    TIM_SetCounter(TIM4, 0);
}

static void Counter_Start_All(void)
{
    TIM_Cmd(TIM2, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}

static void Counter_Stop_All(void)
{
    TIM_Cmd(TIM2, DISABLE);
    TIM_Cmd(TIM4, DISABLE);
}

void OV2640_Test_FrameSignal(OV2640_SignalMeasure_t *result)
{
    uint32_t time_us;
    uint32_t href_cnt;
    uint32_t pclk_cnt;

    if (result == 0)
        return;

    while (OV2640_VSYNC == 1)
        ;
    while (OV2640_VSYNC == 0)
        ;

    Counter_Reset_All();
    Counter_Start_All();

    while (OV2640_VSYNC == 1)
        ;

    Counter_Stop_All();

    time_us = TIM_GetCounter(TIM4);
    href_cnt = g_href_count;
    pclk_cnt = TIM_GetCounter(TIM2);

    result->vsync_high_us = time_us;
    result->href_count = href_cnt;
    result->pclk_count = pclk_cnt;

    if (time_us == 0)
    {
        result->href_freq = 0.0f;
        result->pclk_freq = 0.0f;
    }
    else
    {
        result->href_freq = (float)href_cnt * 1000000.0f / (float)time_us;
        result->pclk_freq = (float)pclk_cnt * 1000000.0f / (float)time_us;
    }
}

void OV2640_Init(void)
{
    OV2640_IO_Init();
    OV2640_HW_Reset();
    SW_SCCB_Init();

    // OV2640_Init_Config(atk_mc2640_init_uxga_cfg, sizeof(atk_mc2640_init_uxga_cfg) / sizeof(ov2640_cfg_item_t));

    OV2640_Init_Config(svga_rgb565_25fps_cfg, sizeof(svga_rgb565_25fps_cfg) / sizeof(ov2640_cfg_item_t));
    OV2640_Init_Config(ov2640_set_jpeg_320x240_cfg, sizeof(ov2640_set_jpeg_320x240_cfg) / sizeof(ov2640_cfg_item_t));

    // OV2640_Set_Output_JPEG();

    SCCB_Write(0xFF, 0x00);
    SCCB_Write(0x05, 0x00); // R_BYPASS = 0, use DSP

    SCCB_Write(0XFF, 0x01);
    SCCB_Write(0X11, 0x00); // CLKRC
    SCCB_Write(0XFF, 0x00);
    SCCB_Write(0XD3, 0x64); // R_DVP_SP

    // 4) JPEG量化参数，可按需覆盖
    SCCB_Write(0xFF, 0x00);
    SCCB_Write(0x44, 0x0C); // Qs
    // 5) IMAGE_MODE: JPEG enable
    SCCB_Write(0xDA, 0x12);

    OV2640_Set_Output_Size(320, 240);
}

/* EXTI13: HREF 上升沿计数 */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
        if (OV2640_VSYNC == 1)
        {
            g_href_count++;
        }
        EXTI_ClearITPendingBit(EXTI_Line13);
    }
}
