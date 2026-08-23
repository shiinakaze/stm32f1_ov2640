#ifndef _OV2640_CONFIG_H
#define _OV2640_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app.h"

/*
 * OV2640 寄存器地址映射（命名对应 DS V1.6 表 12 / 表 13）。
 * 器件分两个寄存器组，由 0xFF（RA_DLMT）选择：
 *   0xFF = 0x00 → DSP 寄存器组（表 12）
 *   0xFF = 0x01 → 传感器寄存器组（表 13）
 */

/* DSP 寄存器组（0xFF = 0x00，DS 表 12） */
#define OV2640_DSP_R_BYPASS 0x05    /* R_BYPASS: Bit[0] 旁路 DSP */
#define OV2640_DSP_QS 0x44          /* Qs: JPEG 量化系数 */
#define OV2640_DSP_CTRLl 0x50       /* CTRLl: H/V 分频器 */
#define OV2640_DSP_HSIZE1 0x51      /* HSIZE[7:0]，单位 real/4 */
#define OV2640_DSP_VSIZE1 0x52      /* VSIZE[7:0]，单位 real/4 */
#define OV2640_DSP_XOFFL 0x53       /* XOFFL: 水平偏移低 8 位 */
#define OV2640_DSP_YOFFL 0x54       /* YOFFL: 垂直偏移低 8 位 */
#define OV2640_DSP_VHYX 0x55        /* VHYX: 尺寸/偏移高位比特 */
#define OV2640_DSP_DPRP 0x56        /* DPRP: DP 选择 */
#define OV2640_DSP_TEST 0x57        /* TEST: Bit[7] H_SIZE[9] */
#define OV2640_DSP_ZMOW 0x5A        /* ZMOW: 输出宽 OUTW[7:0]，单位 real/4 */
#define OV2640_DSP_ZMOH 0x5B        /* ZMOH: 输出高 OUTH[7:0]，单位 real/4 */
#define OV2640_DSP_ZMHH 0x5C        /* ZMHH: Bit[2] OUTH[8]，Bit[1:0] OUTW[9:8] */
#define OV2640_DSP_BPADDR 0x7C      /* BPADDR: SDE 间接寄存器地址 */
#define OV2640_DSP_BPDATA 0x7D      /* BPDATA: SDE 间接寄存器数据 */
#define OV2640_DSP_CTRL2 0x86       /* CTRL2: DCW/SDE/UV_ADJ/UV_AVG/CMX 使能 */
#define OV2640_DSP_CTRL3 0x87       /* CTRL3: BPC/WPC 使能 */
#define OV2640_DSP_SIZEL 0x8C       /* SIZEL: HSIZE/VSIZE 低位比特 */
#define OV2640_DSP_HSIZE2 0xC0      /* HSIZE8: HSIZE[10:3] */
#define OV2640_DSP_VSIZE2 0xC1      /* VSIZE8: VSIZE[10:3] */
#define OV2640_DSP_CTRL0 0xC2       /* CTRL0: AEC/YUV422/YUV/RGB/RAW 使能 */
#define OV2640_DSP_CTRL1 0xC3       /* CTRL1: CIP/GMA/DG/AWB/AWB_GAIN/LENC/PRE 使能 */
#define OV2640_DSP_R_DVP_SP 0xD3    /* R_DVP_SP: Bit[7] 自动模式，Bit[6:0] DVP 输出速度分频 */
#define OV2640_DSP_IMAGE_MODE 0xDA  /* IMAGE_MODE: Bit[4] JPEG 输出，Bit[3:2] 输出格式 */
#define OV2640_DSP_RESET 0xE0       /* RESET: Bit[4] JPEG 引擎复位 */
#define OV2640_DSP_MS_SP 0xF0       /* MS_SP: SCCB 主机速度 */
#define OV2640_DSP_SS_ID 0x7F       /* SS_ID: SCCB 从机 ID */
#define OV2640_DSP_SS_CTRL 0xF8     /* SS_CTRL: SCCB 从机控制 */
#define OV2640_DSP_MC_BIST 0xF9     /* MC_BIST: 微控制器 BIST */
#define OV2640_DSP_MC_AL 0xFA       /* MC_AL: 程序存储器指针低字节 */
#define OV2640_DSP_MC_AH 0xFB       /* MC_AH: 程序存储器指针高字节 */
#define OV2640_DSP_MC_D 0xFC        /* MC_D: 程序存储器访问 */
#define OV2640_DSP_P_STATUS 0xFE    /* P_STATUS: SCCB 协议状态 */
#define OV2640_DSP_RA_DLMT 0xFF     /* RA_DLMT: Bit[0] 寄存器组选择 */

/* 传感器寄存器组（0xFF = 0x01，DS 表 13） */
#define OV2640_SENSOR_GAIN 0x00     /* GAIN: AGC 增益 */
#define OV2640_SENSOR_COM1 0x03     /* COM1: 垂直开窗边界低位 */
#define OV2640_SENSOR_REG04 0x04    /* REG04: 镜像/翻转 + AEC[1:0] */
#define OV2640_SENSOR_REG08 0x08    /* REG08: 帧曝光预充电行数 */
#define OV2640_SENSOR_COM2 0x09     /* COM2: 待机/输出驱动能力 */
#define OV2640_SENSOR_PIDH 0x0A     /* PIDH: 产品 ID 高字节（只读，= 0x26） */
#define OV2640_SENSOR_PIDL 0x0B     /* PIDL: 产品 ID 低字节（只读，= 0x41） */
#define OV2640_SENSOR_COM3 0x0C     /* COM3: 频闪设置 */
#define OV2640_SENSOR_COM4 0x0D     /* COM4: 掉电引脚状态 */
#define OV2640_SENSOR_AEC 0x10      /* AEC: 曝光控制 AEC[9:2] */
#define OV2640_SENSOR_CLKRC 0x11    /* CLKRC: Bit[7] 倍频，Bit[5:0] 时钟分频 */
#define OV2640_SENSOR_COM7 0x12     /* COM7: Bit[7] 软复位，Bit[6:4] 分辨率选择 */
#define OV2640_SENSOR_COM8 0x13     /* COM8: 频闪滤波/AGC/AEC 使能 */
#define OV2640_SENSOR_COM9 0x14     /* COM9: AGC 增益上限 */
#define OV2640_SENSOR_COM10 0x15    /* COM10: PCLK 边沿/门控等 DVP 选项 */
#define OV2640_SENSOR_HREFST 0x17   /* HREFST: 水平开窗起点 */
#define OV2640_SENSOR_HREFEND 0x18  /* HREFEND: 水平开窗终点 */
#define OV2640_SENSOR_VSTART 0x19   /* VSTRT: 垂直开窗起点 */
#define OV2640_SENSOR_VEND 0x1A     /* VEND: 垂直开窗终点 */
#define OV2640_SENSOR_MIDH 0x1C     /* MIDH: 厂商 ID 高字节（只读，= 0x7F） */
#define OV2640_SENSOR_MIDL 0x1D     /* MIDL: 厂商 ID 低字节（只读，= 0xA2） */
#define OV2640_SENSOR_AEW 0x24      /* AEW: AEC/AGC 亮度上限 */
#define OV2640_SENSOR_AEB 0x25      /* AEB: AEC/AGC 亮度下限 */
#define OV2640_SENSOR_VV 0x26       /* VV: AEC/AGC 快速模式阈值 */
#define OV2640_SENSOR_REG2A 0x2A    /* REG2A: 行间隔调整高位 */
#define OV2640_SENSOR_FRARL 0x2B    /* FRARL: 行间隔调整低位（虚拟像素） */
#define OV2640_SENSOR_ADDVSL 0x2D   /* ADDVSL: VSYNC 脉宽调整低字节 */
#define OV2640_SENSOR_ADDVHS 0x2E   /* ADDVSH: VSYNC 脉宽调整高字节 */
#define OV2640_SENSOR_YAVG 0x2F     /* YAVG: 平均亮度（自动更新） */
#define OV2640_SENSOR_REG32 0x32    /* REG32: PCLK 分频 + 开窗边界低位 */
#define OV2640_SENSOR_ARCOM2 0x34   /* ARCOM2: 缩放窗口水平起点 */
#define OV2640_SENSOR_REG45 0x45    /* REG45: AGC[9:8] + AEC[15:10] */
#define OV2640_SENSOR_FLL 0x46      /* FLL: 帧长调整低字节（虚拟行） */
#define OV2640_SENSOR_FLH 0x47      /* FLH: 帧长调整高字节 */
#define OV2640_SENSOR_COM19 0x48    /* COM19: 缩放模式垂直窗口起点低位 */
#define OV2640_SENSOR_ZOOMS 0x49    /* ZOOMS: 缩放模式垂直窗口起点高位 */
#define OV2640_SENSOR_COM22 0x4B    /* COM22: 闪光灯控制 */
#define OV2640_SENSOR_COM25 0x4E    /* COM25: 频闪 AEC 高位 */
#define OV2640_SENSOR_BD50 0x4F     /* BD50: 50Hz 频闪滤波 AEC 低 8 位 */
#define OV2640_SENSOR_BD60 0x50     /* BD60: 60Hz 频闪滤波 AEC 低 8 位 */
#define OV2640_SENSOR_REG5D 0x5D    /* REG5D: 16 区平均权重选择 */
#define OV2640_SENSOR_REG5E 0x5E    /* REG5E: 16 区平均权重选择 */
#define OV2640_SENSOR_REG5F 0x5F    /* REG5F: 16 区平均权重选择 */
#define OV2640_SENSOR_REG60 0x60    /* REG60: 16 区平均权重选择 */
#define OV2640_SENSOR_HISTO_LOW 0x61  /* HISTO_LOW: 直方图算法低电平 */
#define OV2640_SENSOR_HISTO_HIGH 0x62 /* HISTO_HIGH: 直方图算法高电平 */

/* 期望的器件识别值（DS 表 13：PID = 0x2641，MID = 0x7FA2） */
#define OV2640_PID_EXPECT_H 0x26

/*
 * 寄存器初始化表项：写入 reg = val 后，若 delay_ms 非 0 则延时
 * （用于软复位、JPEG 引擎重启等需要等待生效的寄存器）
 */
typedef struct {
    uint8_t reg;        // 寄存器地址
    uint8_t val;        // 要写入的值
    uint16_t delay_ms;  // 写入后延时的毫秒数（0 表示不延时）
} ov2640_cfg_item_t;

/* SVGA RGB565 25fps 基础初始化表（来源：OV2640 Software Application Notes 1.03 §13.2） */
extern const ov2640_cfg_item_t svga_rgb565_25fps_cfg[];
extern const uint16_t svga_rgb565_25fps_cfg_len;

/* JPEG 输出模式配置表（在基础表之后下载） */
extern const ov2640_cfg_item_t ov2640_jpeg_mode_cfg[];
extern const uint16_t ov2640_jpeg_mode_cfg_len;

#ifdef __cplusplus
}
#endif

#endif // _OV2640_CONFIG_H
