#include <stddef.h>

#include "ov2640.h"
#include "sccb.h"
#include "delay.h"

/* 寄存器组编号（写入 RA_DLMT 0xFF 的值，DS §8） */
#define OV2640_BANK_DSP 0x00
#define OV2640_BANK_SENSOR 0x01

/* 同步等待的轮询次数上限：72MHz 下单次循环约 0.1us，
 * 2000 万次约 2 秒，覆盖 25fps 帧间隔（40ms）的充分余量。
 * 超时视为传感器信号丢失（如摄像头拔掉），避免主循环永久挂死。 */
#define OV2640_SYNC_TIMEOUT 20000000UL

/*
 * DVP 输出速度分频（写入 R_DVP_SP，位[7]=0 手动模式 + 位[6:0]=分频值）：
 *   PCLK = 48MHz / OV2640_DVP_SPEED_DIV
 * 采集为 GPIO 位扫描轮询，实测反汇编（72MHz/-O1）每字节 CPU 耗时
 * 约 700~840ns（两次 PCLK 沿轮询 + 读存储 + HREF 检查），
 * 字节周期必须有约 1.5 倍以上裕量，即 PCLK ≤ 约 900kHz：
 *   64 → 750kHz（默认，字节周期 1333ns，裕量约 1.6 倍）
 *   50 → 960kHz（上限附近，裕量约 1.2 倍，需上板验证）
 *   32 → 1.5MHz（不可用：超出轮询能力，画面损坏全丢帧）
 *   100 → 480kHz（历史保守值）
 * 取值范围 1~127（≥128 会撞上位[7] 自动模式标志）。
 */
#define OV2640_DVP_SPEED_DIV 64

/*
 * JPEG 模式下 DSP 输入窗口（与 ov2640_jpeg_mode_cfg 的设定一致）：
 * 在 800x600 SVGA 画面中取左上 640x480，宽高比 4:3。
 * set_output_size() 据此窗口做缩放/裁剪。
 */
#define OV2640_INPUT_WIN_WIDTH 640
#define OV2640_INPUT_WIN_HEIGHT 480

/* 帧缓冲由上层提供（ov2640_set_frame_buffer），驱动只持有指针 */
static uint8_t *jpeg_buf = NULL;
static uint32_t jpeg_buf_size = 0;

/* 最近一次采集成功的 JPEG 帧信息 */
static uint8_t *ready_frame_ptr = NULL;
static uint32_t ready_frame_len = 0;

/* ---------------- 底层寄存器访问 ---------------- */

/* 当前寄存器组缓存：0xFF 为无效初值，首次访问必然先写组切换寄存器。
 * 初始化表（ov2640_load_config）中的 0xFF 写入也会同步更新此状态。 */
static uint8_t current_bank = 0xFF;

/**
 * @brief  写传感器寄存器，按需先切换寄存器组（0xFF / RA_DLMT）
 * @retval 0 成功；1 SCCB 无应答
 */
static uint8_t ov2640_write_reg(uint8_t bank, uint8_t reg, uint8_t val)
{
    uint8_t status = SCCB_OK;

    if (bank != current_bank)
    {
        status |= sw_sccb_write_reg(OV2640_SCCB_ADDR, OV2640_DSP_RA_DLMT, bank);
        current_bank = bank;
    }
    status |= sw_sccb_write_reg(OV2640_SCCB_ADDR, reg, val);

    return status ? SCCB_FAIL : SCCB_OK;
}

/**
 * @brief  读传感器寄存器，按需先切换寄存器组
 * @param  val 输出读到的值（仅成功时有效）
 * @retval 0 成功；1 SCCB 无应答
 */
static uint8_t ov2640_read_reg(uint8_t bank, uint8_t reg, uint8_t *val)
{
    uint8_t status = SCCB_OK;

    if (bank != current_bank)
    {
        status |= sw_sccb_write_reg(OV2640_SCCB_ADDR, OV2640_DSP_RA_DLMT, bank);
        current_bank = bank;
    }
    if (status == SCCB_OK)
    {
        status = sw_sccb_read_reg(OV2640_SCCB_ADDR, reg, val);
    }

    return status;
}

/**
 * @brief  按序下载寄存器初始化表（表内含 0xFF 组切换项与延时项）
 * @note   写入不做逐项错误检查，由调用方在下载完成后回读关键寄存器校验
 */
static void ov2640_load_config(const ov2640_cfg_item_t *cfg, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        sw_sccb_write_reg(OV2640_SCCB_ADDR, cfg[i].reg, cfg[i].val);
        if (cfg[i].reg == OV2640_DSP_RA_DLMT)
        {
            current_bank = cfg[i].val;
        }
        if (cfg[i].delay_ms)
        {
            delay_ms(cfg[i].delay_ms);
        }
    }
}

/* ---------------- 初始化 ---------------- */

/**
 * @brief  摄像头引脚初始化
 * @note   VSYNC/HREF 使用 PB3/PB4，依赖关闭 JTAG 的重映射
 *         （GPIO_Remap_SWJ_JTAGDisable，在 drivers/i2c.c 的软件 I2C
 *         初始化中执行），因此 main() 必须先 oled_init() 再 ov2640_init()
 */
static void ov2640_io_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB_OV2640, ENABLE);

    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;

    /* 控制引脚（GPIOB）：RESET/PWDN 推挽输出，VSYNC/HREF/PCLK 上拉输入 */
    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_struct.GPIO_Pin = RESET_PIN | PWDN_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &gpio_init_struct);
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init_struct.GPIO_Pin = VSYNC_PIN | HREF_PIN | PCLK_PIN;
    GPIO_Init(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, &gpio_init_struct);

    /* 数据引脚 D0-D7（GPIOA 低 8 位）上拉输入 */
    gpio_init_struct.GPIO_Pin = DATA0_PIN | DATA1_PIN | DATA2_PIN | DATA3_PIN | DATA4_PIN | DATA5_PIN | DATA6_PIN | DATA7_PIN;
    GPIO_Init(DATA_GPIO, &gpio_init_struct);

    /* PWDN 拉低 = 正常工作模式（DS §3.21），显式驱动避免依赖上电默认值 */
    GPIO_ResetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, PWDN_PIN);
}

void ov2640_hw_reset(void)
{
    /* 硬件复位（DS §3.20）：RESETB 拉低复位全部寄存器，DS 规定建立时间
     * < 1ms，这里各保持 10ms 留足余量 */
    GPIO_ResetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, RESET_PIN);
    delay_ms(10);
    GPIO_SetBits(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, RESET_PIN);
    delay_ms(10);
}

ov2640_err_t ov2640_get_pid(uint16_t *pid)
{
    uint8_t pidh, pidl;

    if (ov2640_read_reg(OV2640_BANK_SENSOR, OV2640_SENSOR_PIDH, &pidh) != SCCB_OK ||
        ov2640_read_reg(OV2640_BANK_SENSOR, OV2640_SENSOR_PIDL, &pidl) != SCCB_OK)
    {
        return OV2640_ERR_SCCB;
    }
    if (pid != NULL)
    {
        *pid = ((uint16_t)pidh << 8) | pidl;
    }
    return OV2640_OK;
}

ov2640_err_t ov2640_get_mid(uint16_t *mid)
{
    uint8_t midh, midl;

    if (ov2640_read_reg(OV2640_BANK_SENSOR, OV2640_SENSOR_MIDH, &midh) != SCCB_OK ||
        ov2640_read_reg(OV2640_BANK_SENSOR, OV2640_SENSOR_MIDL, &midl) != SCCB_OK)
    {
        return OV2640_ERR_SCCB;
    }
    if (mid != NULL)
    {
        *mid = ((uint16_t)midh << 8) | midl;
    }
    return OV2640_OK;
}

ov2640_err_t ov2640_init(void)
{
    uint16_t pid;
    uint8_t image_mode;
    ov2640_err_t err = OV2640_ERR_SCCB;

    ov2640_io_init();
    ov2640_hw_reset();
    sw_sccb_init();

    /* 先校验器件存在（读 ID 带重试）：失败时立即返回，
     * 不对无应答的总线做约 200 次无用的寄存器写入 */
    for (uint8_t retry = 0; retry < 3; retry++)
    {
        pid = 0;
        if (ov2640_get_pid(&pid) == OV2640_OK)
        {
            err = ((pid >> 8) == OV2640_PID_EXPECT_H) ? OV2640_OK : OV2640_ERR_PID;
            if (err == OV2640_OK)
            {
                break;
            }
        }
        delay_ms(5);
    }
    if (err != OV2640_OK)
    {
        return err;
    }

    /* SVGA RGB565 25fps 基础配置（App Notes 1.03 §13.2，含软复位） */
    ov2640_load_config(svga_rgb565_25fps_cfg, svga_rgb565_25fps_cfg_len);

    /* 切换为 JPEG 压缩输出 */
    ov2640_load_config(ov2640_jpeg_mode_cfg, ov2640_jpeg_mode_cfg_len);

    /*
     * 传感器时钟 CLKRC：基础表已写 0x00 = 不分频，24MHz 晶振下传感器
     * 时钟达到最大（App Notes §3.3 25fps 基准配置，无需再调）。
     * CLKRC 位[7] 倍频虽可进一步提高时钟，但会同时改变帧率与 AEC/AGC
     * 时基（超出官方参考配置范围），不建议使用；降帧率应改用 FLL。
     *
     * R_DVP_SP：DVP 输出速度手动分频（见 OV2640_DVP_SPEED_DIV），
     * PCLK = 48MHz/32 = 1.5MHz，与 GPIO 位扫描采集速度匹配
     * （有意覆盖配置表中的 0x82 自动模式）。
     */
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_R_DVP_SP, OV2640_DVP_SPEED_DIV);

    /* 默认输出 320x240 */
    ov2640_set_output_size(320, 240);

    /* 回读校验：IMAGE_MODE 应为 JPEG 输出使能（0x10），
     * 确认配置表确实写入生效 */
    if (ov2640_read_reg(OV2640_BANK_DSP, OV2640_DSP_IMAGE_MODE, &image_mode) != SCCB_OK ||
        image_mode != 0x10)
    {
        return OV2640_ERR_CONFIG;
    }

    return OV2640_OK;
}

/* ---------------- 输出设置 ---------------- */

void ov2640_set_frame_buffer(uint8_t *buf, uint32_t size)
{
    jpeg_buf = buf;
    jpeg_buf_size = size;
}

void ov2640_set_output_size(uint16_t width, uint16_t height)
{
    /* DSP 输入窗口（HSIZE1/VSIZE1/XOFFL/YOFFL，单位 real/4，DS 表 12）：
     * 默认取完整 640x480（4:3）；输出宽高比 ≠ 4:3 时（如 640x360），
     * 按输出比例在输入窗口内居中裁剪，保持画面比例不变形 */
    uint16_t in_w = OV2640_INPUT_WIN_WIDTH;
    uint16_t in_h = OV2640_INPUT_WIN_HEIGHT;
    uint16_t x_off = 0;
    uint16_t y_off = 0;

    if ((uint32_t)width * 3 > (uint32_t)height * 4)
    {
        /* 输出比 4:3 更宽（如 16:9）：裁剪输入高度并垂直居中 */
        in_h = (uint16_t)(((uint32_t)in_w * height / width) & ~3u);
        y_off = (OV2640_INPUT_WIN_HEIGHT - in_h) / 2;
    }
    else if ((uint32_t)width * 3 < (uint32_t)height * 4)
    {
        /* 输出比 4:3 更窄：裁剪输入宽度并水平居中 */
        in_w = (uint16_t)(((uint32_t)in_h * width / height) & ~3u);
        x_off = (OV2640_INPUT_WIN_WIDTH - in_w) / 2;
    }

    /* DSP 缩放输出窗口（ZMOW/ZMOH/ZMHH），单位 real/4（DS 表 12）：
     * 写入实际宽高的 1/4；ZMHH 位域：Bit[1:0]=OUTW[9:8]，Bit[2]=OUTH[8] */
    uint16_t out_w = width >> 2;
    uint16_t out_h = height >> 2;

    /* 缩放参数更新前后复位 DVP 输出（RESET 位[2]），保证时序重建
     * （窗口与尺寸寄存器都小于 256，高位寄存器 VHYX/TEST 无需写入） */
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_RESET, 0x04);
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_HSIZE1, (uint8_t)(in_w >> 2));
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_VSIZE1, (uint8_t)(in_h >> 2));
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_XOFFL, (uint8_t)(x_off >> 2));
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_YOFFL, (uint8_t)(y_off >> 2));
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_ZMOW, (uint8_t)(out_w & 0x00FF));
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_ZMOH, (uint8_t)(out_h & 0x00FF));
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_ZMHH,
                     ((uint8_t)(out_w >> 8) & 0x03) | ((uint8_t)(out_h >> 6) & 0x04));
    ov2640_write_reg(OV2640_BANK_DSP, OV2640_DSP_RESET, 0x00);
}

/* ---------------- 帧采集 ---------------- */

/**
 * @brief  等待当前帧结束并捕捉新帧 VSYNC 上升沿（帧起始）
 * @retval 0 成功对齐帧起始；1 超时（VSYNC 信号丢失）
 */
static uint8_t ov2640_wait_frame_start(void)
{
    uint32_t timeout = OV2640_SYNC_TIMEOUT;

    /* 先等当前帧结束，确保不会从帧中途开始采集（避免撕裂/闪烁） */
    while (OV2640_VSYNC == 1)
    {
        if (timeout-- == 0)
        {
            return 1;
        }
    }
    /* 等待新帧 VSYNC 上升沿 */
    timeout = OV2640_SYNC_TIMEOUT;
    while (OV2640_VSYNC == 0)
    {
        if (timeout-- == 0)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief  VSYNC 高电平期间按行读整帧：HREF 高电平为有效行，
 *         PCLK 上升沿锁存一个字节
 * @return 读到的总字节数；超出缓冲容量的字节只计数不保存，
 *         避免大帧污染有效数据之外的缓冲
 * @note   各等待共用一个约 2 秒的轮询预算，任一信号丢失即提前返回
 */
static uint32_t ov2640_read_frame(uint8_t *buf, uint32_t buf_size)
{
    uint32_t idx = 0;
    uint32_t timeout = OV2640_SYNC_TIMEOUT;

    while (OV2640_VSYNC == 1)
    {
        while (OV2640_HREF == 1) /* HREF 为高，读取一行 */
        {
            while (OV2640_PCLK == 0) /* 等待 PCLK 上升沿 */
            {
                if (timeout-- == 0)
                {
                    return idx;
                }
            }
            if (idx < buf_size)
            {
                buf[idx] = OV2640_READ_DATA();
            }
            idx++;
            while (OV2640_PCLK == 1) /* 等待 PCLK 下降沿（数据更新） */
            {
                if (timeout-- == 0)
                {
                    return idx;
                }
            }
        }
        if (timeout-- == 0)
        {
            break;
        }
    }
    return idx;
}

/**
 * @brief  在采集数据中定位 JPEG SOI（FF D8）/ EOI（FF D9）标记，
 *         成功时记录待发送帧；标记缺失（不完整/被截断的帧）则丢弃
 */
static void ov2640_locate_jpeg(uint8_t *buf, uint32_t len)
{
    uint32_t start, end;

    /* 定位 JPEG 起始标记 FF D8 */
    for (start = 0; start + 1 < len; start++)
    {
        if (buf[start] == 0xFF && buf[start + 1] == 0xD8)
        {
            break;
        }
    }
    if (start + 1 >= len)
    {
        /* 未找到 JPEG 起始，丢弃本帧 */
        ready_frame_ptr = NULL;
        ready_frame_len = 0;
        return;
    }

    /* 定位 JPEG 结束标记 FF D9 */
    for (end = start + 1; end + 1 < len; end++)
    {
        if (buf[end] == 0xFF && buf[end + 1] == 0xD9)
        {
            end += 2; /* 包含 FF D9 */
            ready_frame_ptr = buf + start;
            ready_frame_len = end - start;
            return;
        }
    }
    /* 未找到结束标记（JPEG 不完整/被截断），丢弃本帧避免解码异常 */
    ready_frame_ptr = NULL;
    ready_frame_len = 0;
}

void ov2640_capture(void)
{
    uint32_t idx;
    uint8_t *buf = jpeg_buf;

    if (buf == NULL || jpeg_buf_size == 0)
    {
        ready_frame_ptr = NULL;
        ready_frame_len = 0;
        return;
    }

    if (ov2640_wait_frame_start() != 0)
    {
        /* 帧同步超时（信号丢失），丢弃本帧 */
        ready_frame_ptr = NULL;
        ready_frame_len = 0;
        return;
    }

    idx = ov2640_read_frame(buf, jpeg_buf_size);
    /* 缓冲溢出时扫描范围截断到实际写入的部分 */
    if (idx > jpeg_buf_size)
    {
        idx = jpeg_buf_size;
    }
    ov2640_locate_jpeg(buf, idx);
}

uint8_t *ov2640_get_ready_frame(uint32_t *len)
{
    if (len != NULL)
    {
        *len = ready_frame_len;
    }
    return ready_frame_ptr;
}
