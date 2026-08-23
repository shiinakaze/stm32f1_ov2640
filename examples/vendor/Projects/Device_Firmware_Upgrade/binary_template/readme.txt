/**
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    readme.txt
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Description of the binary directory.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */


Description
===================
This directory contains a set of sources files that build the application to be
loaded into Flash memory using device firmware upgrade (DFU).

To build such application, some special configuration has to be preformed: 
1. Set the program load address at 0x08003000, using your toolchain linker file
2. Relocate the vector table at address 0x08003000, using the "NVIC_SetVectorTable"
   function.
 
The SysTick example provided within the STM32F10xxx,STM32L152xxx, STM32F30xxx and STM32F37xxx Standard 
Peripheral examples is used as illustration. 
   
This example configures the SysTick to generate a time base equal to 1 ms.
The system clock is set to 72 MHz for STM32F10xxx, STM32F30xxx  and STM32F37xxx devices and to 32MHz for 
STM32L15xxx devices, the SysTick is clocked by the AHB clock (HCLK)

A "Delay" function is implemented based on the SysTick end-of-count event.
Four LEDs are toggled with a timing defined by the Delay function.


Directory contents
==================

  + binary_template\EWARM: This folder contains a preconfigured project 
                           for EWARM toolchain that produces a binary image of 
			   SysTick example to be loaded with DFU.

  + binary_template\MDK-ARM: This folder contains a preconfigured project for 
                            MDK-ARM toolchain that produces a binary image of 
		            SysTick example to be loaded with DFU.
						   
 + \binary_template\SW4STM32: This folder contains a preconfigured project for  
                            STM32 System Workbench that produces a binary image of SysTick 
			    example to be loaded with DFU.                                      
                  
  + binary_template\inc: contains the binary_template firmware header files 
    - stm32f10x_conf.h    Library Configuration file for STM32F10xxx devices
    - stm32_it.h      	  Header for stm32_it.c
    - stm32l15xx_conf.h    Library Configuration file for STM32L15xxx devices
    - stm32f37x_conf.h    Library Configuration file for STM32F37xxx devices
    - stm32f30x_conf.h    Library Configuration file for STM32F30xxx devices
    - main.h              Header for main.c
    - platform_config:    Evaluation board specific configuration file

  + binary_template\src: contains the binary_template firmware source files 
    - main.c              Main program
    - stm32_it.c      	  Interrupt handlers for STM32F10xxx,STM32L15xxx, STM32F30xxx and STM32F37xxx devices


Hardware environment
====================
This example runs on STMicroelectronics STM3210B-EVAL, STM3210E-EVAL, STM32L152-EVAL,STM32F303C_EVAL 
,STM32F373C_EVAL, STM32F302R8-Nucleo, STM32F303xE_EVAL and STM32L152D-EVAL evaluation boards and can be easily 
tailored to any other hardware.

 + STM3210B-EVAL 
    - Use LD1, LD2, LD3 and LD4 leds connected respectively to PC.06, PC.07, PC.08
      and PC.09 pins
 
 + STM3210E-EVAL
    - Use LD1, LD2, LD3 and LD4 leds connected respectively to PF.06, PF.07, PF.08
      and PF.09 pins

 + STM32L152-EVAL
    - Use LD1, LD2, LD3 and LD4 leds connected respectively to PD.00, PD.01, PD.02
      and PC.12 pins
 
 + STM32L152D-EVAL
	- Use LD1, LD2, LD3 and LD4 leds connected respectively to PD.03, PD.07, PG.14
      and PG.15 pins

 + STM32F373C_EVAL
	- Use LD1, LD2, LD3 and LD4 leds connected respectively to PC.0, PC.01, PC.02
      and PC.03 pins 
 
 + STM32F303C_EVAL
	- Use LD1, LD2, LD3 and LD4 leds connected respectively to PE.08, PE.09, PE.10
      and PE.11 pins

 + STM32F303E_EVAL
	- Use LD1, LD2, LD3 and LD4 leds connected respectively to PE.08, PE.09, PE.10
      and PE.11 pins 	  
 
 + STM32F302R8-Nucleo
	- Use LD2
 
How to use it
=============

In order to make the program work, you must do the following :
 - Open your preferred toolchain 
 - Rebuild all files and load your image into target memory
 - In the workspace toolbar select the project configuration:
        - STM3210B-EVAL: to configure the project for STM32 Medium-density devices
        - STM3210E-EVAL: to configure the project for STM32 High-density devices
        - STM3210E-EVAL_XL: to configure the project for STM32 XL-density devices
        - STM32L152-EVAL: to configure the project for STM32 Medium-Density Low-Power devices
        - STM32L152D-EVAL: to configure the project for STM32 High-Density Low-Power devices
        - STM32373C-EVAL:  to configure the project for STM32F37xxx devices
        - STM32303C-EVAL:  to configure the project for STM32F303xx devices.
        - STM32303E-EVAL:  to configure the project for STM32F303xE devices
        - STM32F302x8_Nucleo: to configure the project for STM32F302x8 devices
 - Run the application
 - The generated .bin file should be converted to the DFU format using the “DFU File
   Manager Tool” included in the “DfuSe” PC software install. For more details on
   how to convert a .bin file to DFU format please refer to the UM0412 user manual
   “Getting started with DfuSe USB device firmware upgrade STMicroelectronics extension”
   available from the STMicroelectronics microcontroller website www.st.com.
 
 
************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE******
