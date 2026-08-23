/**
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    readme.txt
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Description of the USB Audio_Speaker Demo.
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
The USB-FS-Device Audio Speaker demo gives examples of how to use the STM32F10xxx 
and STM32L15xxx USB-FS-Device peripheral to communicate with the PC host in the 
isochronous transfer mode.
It provides a demonstration of the correct method for configuring an isochronous
endpoint, receiving or transmitting data from/to the host.

8MHZ Cristal should be mounted on boards to recognize USB device.

The purpose of the USB audio speaker demo is to receive the audio stream (data) 
from a PC host using the USB and to play it back via the STM32 MCU,the  audio 
stream is output on-board Speaker.

Please note that Win 7 doesn't support 8-bit audio by default. You have to disable 
enhancements in "Enhancements" tab of speaker properties.

More details about this Demo implementation is given in the User manual 
"UM0424 STM32F10xxx USB development kit", available for download from the ST
microcontrollers website: www.st.com/stm32


Directory contents
==================
 + \inc: contains the Demo firmware header files
 + \EWARM: contains preconfigured projects for EWARM toolchain
 + \MDK-ARM: contains preconfigured projects for MDK-ARM toolchain
 + \SW4STM32: contains preconfigured projects for STM32 System Workbench for toolchain          
 + \src: contains the Demo firmware source files

Hardware environment
====================
This example runs on STMicroelectronics STM3210B-EVAL, STM3210E-EVAL and 
STM32L152-EVAL evaluation boards and can be easily tailored to any other hardware.

  - STM3210B-EVAL Set-up 
     - Jumper JP1 (USB disconnect) should be connected in position 2-3.
     - Jumper JP6 should be connected.
      - Connect the STM3210B-EVAL board to the PC through 'USB micro A-Male to A-Male' cable to CN1 connector

  - STM3210E-EVAL Set-up 
     - Jumper JP14 (USB disconnect) should be connected in position 2-3.
     - Jumper JP18 (MCKI) should be connected in position 2-3.
     - Connect the STM3210E-EVALL board to the PC through 'USB micro A-Male to A-Male' cable to CN14 connector 
  
  - STM32L152-EVAL Set-up 
     - Jumper JP16 (Audio Out) should be not connected.
     - Connect the STM32L152-EVALL board to the PC through 'USB micro A-Male to A-Male' cable to CN1 connector

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
 - Run the application

NOTE:
 - Low-density devices are STM32F101xx, STM32F102xx and STM32F103xx 
   microcontrollers where the Flash memory density ranges between 16 and 32 Kbytes.
 - Medium-density devices are STM32F101xx, STM32F102xx and STM32F103xx 
   microcontrollers where the Flash memory density ranges between 64 and 128 Kbytes.
 - High-density devices are STM32F101xx and STM32F103xx microcontrollers where
   the Flash memory density ranges between 256 and 512 Kbytes.
 - XL-density devices are STM32F101xx and STM32F103xx microcontrollers where
   the Flash memory density ranges between 512 and 1024 Kbytes.
 - Medium-density Low-Power devices are STM32L15xx microcontrollers where the flash
   memory density ranges between 64 and 128 Kbytes.
   
************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE******
