#ifndef _BOARD_CONFIG_H
#define _BOARD_CONFIG_H

#include "stm32f10x.h"

/*
 * Blue Pill（STM32F103C8T6 最小系统板）板级连接关系
 *
 *   USART1  PA9/PA10    → 调试 / 图像传输串口（TX 走 DMA1_Channel4）
 *   DVP     PA0-PA7     → OV2640 并行数据 D0-D7
 *           PB3/PB4/PB5 → OV2640 VSYNC/HREF/RESET（需关闭 JTAG-DP 释放 PB3/PB4）
 *           PB8/PB9     → OV2640 PCLK/PWDN
 *   SCCB    PB10/PB11   → OV2640 寄存器配置总线（SIO_C/SIO_D）
 *   I2C     PB6/PB7     → OLED 显示屏（SSD1306，软件 I2C）
 *   I2C     PB4/PB5     → SHT30（驱动为空占位，未接入编译）
 */

/* ---------------- USART1 → 调试 / 图像传输串口 ---------------- */

#define RCC_APB_UART1_GPIO      RCC_APB2Periph_GPIOA
#define RCC_APB_UART1           RCC_APB2Periph_USART1
#define RCC_AHB_UART1_DMA       RCC_AHBPeriph_DMA1

#define UART1_GPIO              GPIOA
#define UART1_TX_PIN            GPIO_Pin_9
#define UART1_RX_PIN            GPIO_Pin_10

#define UART1_TX_DMA_CHANNEL    DMA1_Channel4   /* USART1_TX -> DMA1 Channel4 */

/* ---------------- SCCB（PB10/PB11）→ OV2640 ---------------- */

#define RCC_APB_SCCB_GPIO       RCC_APB2Periph_GPIOB
#define SCCB_GPIO               GPIOB

#define SIO_C_PIN               GPIO_Pin_10
#define SIO_D_PIN               GPIO_Pin_11

/* ---------------- DVP 并口 → OV2640 ---------------- */

/* 时钟：GPIOA + GPIOB（数据口 GPIOA，控制口/SCCB GPIOB） */
#define RCC_APB_OV2640          (RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB)

/* 控制引脚（GPIOB）。SCCB 引脚 SIO_C/SIO_D 定义在 sccb.h */
#define PCLK_PWDN_HREF_RESET_VSYNC_GPIO GPIOB

#define VSYNC_PIN               GPIO_Pin_3
#define HREF_PIN                GPIO_Pin_4
#define RESET_PIN               GPIO_Pin_5
#define PCLK_PIN                GPIO_Pin_8
#define PWDN_PIN                GPIO_Pin_9

/* 数据引脚 D0-D7（GPIOA 低 8 位） */
#define DATA_GPIO               GPIOA
#define DATA0_PIN               GPIO_Pin_0
#define DATA1_PIN               GPIO_Pin_1
#define DATA2_PIN               GPIO_Pin_2
#define DATA3_PIN               GPIO_Pin_3
#define DATA4_PIN               GPIO_Pin_4
#define DATA5_PIN               GPIO_Pin_5
#define DATA6_PIN               GPIO_Pin_6
#define DATA7_PIN               GPIO_Pin_7

/* ---------------- 软件 I2C（PB6/PB7）→ OLED（SSD1306）---------------- */

#define SW_I2C_RCC_APB_GPIO (RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO)
#define SW_I2C_GPIO               GPIOB
#define SW_I2C_SCL_PIN             GPIO_Pin_6
#define SW_I2C_SDA_PIN             GPIO_Pin_7

/* ---------------- SHT30 预留引脚（驱动为空占位） ---------------- */

#define SHT30_SCL_PIN           GPIO_Pin_4
#define SHT30_SDA_PIN           GPIO_Pin_5

#endif /* _BOARD_CONFIG_H */
