branch:

- main: Main branch
- stm32f1_ov2640_test: Test signal
- stm32f1_ov2640_dma: legacy

```
$ tree -L 2
.
|-- DebugConfig
|                   `-- Target_1_STM32F103C8_1.0.0.dbgconf
|-- Listings
|                   `-- stm32f1_ov2640.map
|-- Objects
|   |               `-- core_cm3.crf
|   |               `-- core_cm3.d
|   |               `-- core_cm3.o
|   |               `-- delay.crf
|   |               `-- delay.d
|   |               `-- delay.o
|   |               `-- i2c.crf
|   |               `-- i2c.d
|   |               `-- i2c.o
|   |               `-- main.crf
|   |               `-- main.d
|   |               `-- main.o
|   |               `-- main_test.d
|   |               `-- misc.crf
|   |               `-- misc.d
|   |               `-- misc.o
|   |               `-- oled.crf
|   |               `-- oled.d
|   |               `-- oled.o
|   |               `-- oled_font.crf
|   |               `-- oled_font.d
|   |               `-- oled_font.o
|   |               `-- ov2640.crf
|   |               `-- ov2640.d
|   |               `-- ov2640.o
|   |               `-- ov2640_config.crf
|   |               `-- ov2640_config.d
|   |               `-- ov2640_config.o
|   |               `-- sccb.crf
|   |               `-- sccb.d
|   |               `-- sccb.o
|   |               `-- sht30.crf
|   |               `-- sht30.d
|   |               `-- sht30.o
|   |               `-- startup_stm32f10x_md.d
|   |               `-- startup_stm32f10x_md.o
|   |               `-- stm32f10x_adc.crf
|   |               `-- stm32f10x_adc.d
|   |               `-- stm32f10x_adc.o
|   |               `-- stm32f10x_bkp.crf
|   |               `-- stm32f10x_bkp.d
|   |               `-- stm32f10x_bkp.o
|   |               `-- stm32f10x_can.crf
|   |               `-- stm32f10x_can.d
|   |               `-- stm32f10x_can.o
|   |               `-- stm32f10x_cec.crf
|   |               `-- stm32f10x_cec.d
|   |               `-- stm32f10x_cec.o
|   |               `-- stm32f10x_crc.crf
|   |               `-- stm32f10x_crc.d
|   |               `-- stm32f10x_crc.o
|   |               `-- stm32f10x_dac.crf
|   |               `-- stm32f10x_dac.d
|   |               `-- stm32f10x_dac.o
|   |               `-- stm32f10x_dbgmcu.crf
|   |               `-- stm32f10x_dbgmcu.d
|   |               `-- stm32f10x_dbgmcu.o
|   |               `-- stm32f10x_dma.crf
|   |               `-- stm32f10x_dma.d
|   |               `-- stm32f10x_dma.o
|   |               `-- stm32f10x_exti.crf
|   |               `-- stm32f10x_exti.d
|   |               `-- stm32f10x_exti.o
|   |               `-- stm32f10x_flash.crf
|   |               `-- stm32f10x_flash.d
|   |               `-- stm32f10x_flash.o
|   |               `-- stm32f10x_fsmc.crf
|   |               `-- stm32f10x_fsmc.d
|   |               `-- stm32f10x_fsmc.o
|   |               `-- stm32f10x_gpio.crf
|   |               `-- stm32f10x_gpio.d
|   |               `-- stm32f10x_gpio.o
|   |               `-- stm32f10x_i2c.crf
|   |               `-- stm32f10x_i2c.d
|   |               `-- stm32f10x_i2c.o
|   |               `-- stm32f10x_it.crf
|   |               `-- stm32f10x_it.d
|   |               `-- stm32f10x_it.o
|   |               `-- stm32f10x_iwdg.crf
|   |               `-- stm32f10x_iwdg.d
|   |               `-- stm32f10x_iwdg.o
|   |               `-- stm32f10x_pwr.crf
|   |               `-- stm32f10x_pwr.d
|   |               `-- stm32f10x_pwr.o
|   |               `-- stm32f10x_rcc.crf
|   |               `-- stm32f10x_rcc.d
|   |               `-- stm32f10x_rcc.o
|   |               `-- stm32f10x_rtc.crf
|   |               `-- stm32f10x_rtc.d
|   |               `-- stm32f10x_rtc.o
|   |               `-- stm32f10x_sdio.crf
|   |               `-- stm32f10x_sdio.d
|   |               `-- stm32f10x_sdio.o
|   |               `-- stm32f10x_spi.crf
|   |               `-- stm32f10x_spi.d
|   |               `-- stm32f10x_spi.o
|   |               `-- stm32f10x_tim.crf
|   |               `-- stm32f10x_tim.d
|   |               `-- stm32f10x_tim.o
|   |               `-- stm32f10x_usart.crf
|   |               `-- stm32f10x_usart.d
|   |               `-- stm32f10x_usart.o
|   |               `-- stm32f10x_wwdg.crf
|   |               `-- stm32f10x_wwdg.d
|   |               `-- stm32f10x_wwdg.o
|   |               `-- stm32f1_ov2640.build_log.htm
|   |               `-- stm32f1_ov2640.htm
|   |               `-- stm32f1_ov2640.lnp
|   |               `-- stm32f1_ov2640.sct
|   |               `-- stm32f1_ov2640_Target_1.dep
|   |               `-- system_stm32f10x.crf
|   |               `-- system_stm32f10x.d
|   |               `-- system_stm32f10x.o
|   |               `-- uart.crf
|   |               `-- uart.d
|                   `-- uart.o
|-- README.md
|-- RTE
|                   `-- Device
|-- app
|   |               `-- main.c
|   |               `-- main.h
|   |               `-- stm32f10x_conf.h
|   |               `-- stm32f10x_it.c
|                   `-- stm32f10x_it.h
|-- drivers
|   |               `-- cmsis
|   |               `-- i2c.c
|   |               `-- i2c.h
|   |               `-- oled.c
|   |               `-- oled.h
|   |               `-- oled_font.c
|   |               `-- oled_font.h
|   |               `-- ov2640.c
|   |               `-- ov2640.h
|   |               `-- ov2640_config.c
|   |               `-- ov2640_config.h
|   |               `-- sccb.c
|   |               `-- sccb.h
|   |               `-- sht30.c
|   |               `-- sht30.h
|   |               `-- spl
|   |               `-- uart.c
|                   `-- uart.h
|-- middleware
|   |               `-- delay.c
|                   `-- delay.h
|-- stm32f1_ov2640.code-workspace
|-- stm32f1_ov2640.uvguix.shiinakaze
|-- stm32f1_ov2640.uvoptx
|-- stm32f1_ov2640.uvprojx
`-- tools
                    `-- main.py // Python script to capture images

11 directories, 147 files


```