#ifndef _OV2640_H
#define _OV2640_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include "main.h"
#include "delay.h"
#include "sccb.h"
#include "ov2640_config.h"

#define OV2640_DEVICE_ADDRESS 0x60

#define RCC_APB_OV2640 (RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC)

// Control Pin
// SIO_C and SIO_D are defined in sccb.h

#define PCLK_PWDN_HREF_RESET_VSYNC_GPIO GPIOB

#define VSYNC_PIN GPIO_Pin_3
#define HREF_PIN GPIO_Pin_4
#define RESET_PIN GPIO_Pin_5
#define PCLK_PIN GPIO_Pin_8
#define PWDN_PIN GPIO_Pin_9

// Data Pin
#define DATA_GPIO GPIOA
#define DATA0_PIN GPIO_Pin_0
#define DATA1_PIN GPIO_Pin_1
#define DATA2_PIN GPIO_Pin_2
#define DATA3_PIN GPIO_Pin_3
#define DATA4_PIN GPIO_Pin_4
#define DATA5_PIN GPIO_Pin_5
#define DATA6_PIN GPIO_Pin_6
#define DATA7_PIN GPIO_Pin_7

#define OV2640_READ_DATA() (GPIOA->IDR & 0x00FF)
#define OV2640_PCLK GPIO_ReadInputDataBit(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, PCLK_PIN)
#define OV2640_HREF GPIO_ReadInputDataBit(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, HREF_PIN)
#define OV2640_VSYNC GPIO_ReadInputDataBit(PCLK_PWDN_HREF_RESET_VSYNC_GPIO, VSYNC_PIN)

#define SCCB_Write(sub_address, data) sw_sccb_write_reg(OV2640_DEVICE_ADDRESS, sub_address, data)
#define SCCB_Read(sub_address) sw_sccb_read_reg(OV2640_DEVICE_ADDRESS, sub_address)

void ov2640_hw_reset(void);
uint16_t ov2640_get_pid(void);
uint16_t ov2640_get_mid(void);
void ov2640_init(void);
void ov2640_init_config(const ov2640_cfg_item_t *cfg, uint16_t len);
void ov2640_set_output_size(uint16_t width, uint16_t height);
void ov2640_set_frame_buffer(uint8_t *buf0, uint8_t *buf1, uint32_t size);
void ov2640_capture(void);
uint8_t *ov2640_get_ready_frame(uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif // _OV2640_H
