/**
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    readme.txt
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Description of the USB Device_Firmware_Upgrade Demo.
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


Example description
===================
This Demo presents the implementation of a device firmware upgrade (DFU) capability
in the STM32F10xxx, STM32F37xxx, STM32F30xxx and STM32L15xxx microcontrollers. It follows the DFU class 
specification defined by the USB Implementers Forum for reprogramming an application 
through USB-FS-Device. 
The DFU principle is particularly well suited to USB-FS-Device applications that 
need to be reprogrammed in the field.

8MHZ Cristal should be mounted on boards to recognize USB device.

More details about this Demo implementation is given in the User manual 
"UM0424 STM32F10xxx USB development kit", available for download from the ST
microcontrollers website: www.st.com/stm32


Directory contents
==================
 + \binary_template: contains a set of sources files that build the application 
                     to be loaded with DFU 
 + \inc: contains the Demo firmware header files
 + \EWARM: contains preconfigured project for EWARM toolchain
 + \MDK-ARM: contains preconfigured project for MDK-ARM toolchain 
 + \SW4STM32 : contains preconfigured projects for TM32 System Workbench toolchain          
 + \src: contains the Demo firmware source files


Hardware environment
====================
This example runs these STMicroelectronics evaluation boards and can be easily tailored to any other hardware: 

- STM3210B-EVAL
- STM3210E-EVAL
- STM32L152-EVAL
- STM32F373C_EVAL
- STM32F303C_EVAL
- STM32F303E_EVAL
- STM32L152D-EVAL
- STM32F303R8-Nucleo

Note: After each device reset, hold down the Key push-button on the STM3210B-EVAL 
      or STM3210E-EVAL or STM32L152D-EVAL or STM32F373C-EVAL evaluation boards or the Up push button 
      on STM32L152-EVAL evaluation board to enter the DFU mode.

  - STM3210B-EVAL Set-up 
     - Jumper JP1 (USB disconnect) should be connected in position 2-3.

  - STM3210E-EVAL Set-up 
     - Jumper JP14 (USB disconnect) should be connected in position 2-3.

  - STM32L152-EVAL Set-up 
     - Note that the JoyStick Up push button is used to enter DFU mode.

  - STM32F302R8-Nucleo Rev C Set-up
      - Since there is no USB 2.0 Full speed connector (Type B) on the nucleo board, user has to make 
      his own USB shield daughter board with the a USB connector and plug it on top of Ardiuno connectorsthe 
      of STM32F302R8-Nucleo Rev C board. The USB connector has to be connected to the USB device associated GPIOs as follows:
       - DP (D+ of the USB connector) <======> PA12 (Nucleo board)
       - DM (D- of the USB connector) <======> PA11 (Nucleo board)
      - External USB 1.5k  resistor pull-ups is required on the USB D+ Line and VDD (3V3).
		It is mandatory to add a logic (Transistor/IO) to control this pull up, otherwise after downloading the binary image, 
		leaving DFU mode will not be correctly completed. 
      - To improve EMC performance (noise immunity and signal integrity), it is recommended to connect a 100nF
      ceramic capacitor to the USB VDD pin.	 
     
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

************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE******
