#ifndef _OV2640_H
#define _OV2640_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app.h"
#include "board_config.h"
#include "ov2640_config.h"

/* SCCB 器件地址（DS §8：写 0x60 / 读 0x61） */
#define OV2640_SCCB_ADDR 0x60

/* 驱动错误码 */
typedef enum {
    OV2640_OK = 0,      /* 成功 */
    OV2640_ERR_SCCB,    /* SCCB 总线通信失败（传感器未连接或未应答） */
    OV2640_ERR_PID,     /* 器件 ID 与 OV2640 不符 */
    OV2640_ERR_CONFIG,  /* 配置下载后回读校验失败 */
} ov2640_err_t;

/* DVP 数据/控制引脚与时钟见 boards/bluepill/board_config.h */

#define OV2640_READ_DATA() ((uint8_t)(DATA_GPIO->IDR & 0x00FF))

/* 直接读 IDR 位（0/1），避免 GPIO_ReadInputDataBit 的函数调用开销：
 * 采集热路径对 PCLK 的轮询必须快于 PCLK 半周期，否则丢字节 */
#define OV2640_PCLK ((PCLK_PWDN_HREF_RESET_VSYNC_GPIO->IDR & PCLK_PIN) != 0)
#define OV2640_HREF ((PCLK_PWDN_HREF_RESET_VSYNC_GPIO->IDR & HREF_PIN) != 0)
#define OV2640_VSYNC ((PCLK_PWDN_HREF_RESET_VSYNC_GPIO->IDR & VSYNC_PIN) != 0)

/**
 * @brief  初始化摄像头：GPIO/复位/SCCB → 校验器件 ID → 下载寄存器配置，
 *         进入 SVGA 采样 + JPEG 压缩输出，默认分辨率 320x240
 * @retval OV2640_OK 成功；否则为失败原因（传感器缺失/ID 不符/配置校验失败）
 */
ov2640_err_t ov2640_init(void);

/**
 * @brief  硬件复位：RESET 引脚拉低再拉高（DS §3.20），各保持 10ms
 */
void ov2640_hw_reset(void);

/**
 * @brief  读取产品 ID（PIDH/PIDL，传感器组只读寄存器）
 * @param  pid 输出 16 位 ID（DS 表 13：OV2640 为 0x2641）
 * @retval OV2640_OK 成功；OV2640_ERR_SCCB 总线通信失败
 */
ov2640_err_t ov2640_get_pid(uint16_t *pid);

/**
 * @brief  读取厂商 ID（MIDH/MIDL，传感器组只读寄存器）
 * @param  mid 输出 16 位 ID（DS 表 13：OmniVision 为 0x7FA2）
 * @retval OV2640_OK 成功；OV2640_ERR_SCCB 总线通信失败
 */
ov2640_err_t ov2640_get_mid(uint16_t *mid);

/**
 * @brief  设置采集帧缓冲（由上层提供，驱动只持有指针）
 * @note   UVC 流水线为串行：采集（USB 中断关闭）→ USB 发送 → 再采集，
 *         单缓冲即可，采集时缓冲不会被发送侧读取
 */
void ov2640_set_frame_buffer(uint8_t *buf, uint32_t size);

/**
 * @brief  设置输出分辨率（DSP 缩放输出窗口，单位 real/4，DS 表 12）
 * @param  width/height 实际输出宽高（如 320x240），可被 UVC 主机
 *         commit 请求动态切换
 */
void ov2640_set_output_size(uint16_t width, uint16_t height);

/**
 * @brief  采集一帧：等待帧起始 → 按位扫描读入 → 定位 JPEG SOI/EOI 标记。
 *         采集期间调用方需关闭会打断位扫描的中断（如 USB 中断）
 * @note   结果通过 ov2640_get_ready_frame() 获取；标记缺失/超时时
 *         得到 NULL（该帧被丢弃）
 */
void ov2640_capture(void);

/**
 * @brief  取最近一次采集成功的 JPEG 帧
 * @param  len 输出帧长度（字节）
 * @retval 帧起始指针；上一帧无效时返回 NULL
 */
uint8_t *ov2640_get_ready_frame(uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif // _OV2640_H
