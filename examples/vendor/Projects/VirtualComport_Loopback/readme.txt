/**
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    readme.txt
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Description of the USB VirtualComport_Loopback  Demo.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************


Example description
===================
The aim of this example is to read data from and write data to USB devices using the CDC protocol. 
It makes the USB device look like a serial port (NO serial cable connectors:You can see the data
transferred to and from via USB instead of USB-to-USART bridge connection). 
This example loops back the contents of a text file over usb port. To run the example, Type a message using the Pc's keyboard.
Any data that shows in HyperTerminal is received from the device.
This VirtualComport_Loopback Demo provides the firmware examples for the STM32F10xxx, STM32L15xxx,
STM32F30xxx and STM32F37xxx families.

8MHZ Cristal should be mounted on boards to recognize USB device.

- OUT transfers (receive the data from the PC to STM32):
	When a packet is received from the PC on the OUT pipe (EP3), EP3_IN_Callback will process the receive data.
	To receive something to the STM32 data are stored in the Receive_Buffer[] by calling CDC_Receive_DATA() 
 
- IN transfers (to send the data received from the STM32 to the PC):
	When a packet is sent from the STM32 on the IN pipe (EP1), EP1_IN_Callback will process the send data.
	To send something to the HOST we put the data in to the Send_Buffer[] buffer and call CDC_Send_DATA() 

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
This example runs these STMicroelectronics evaluation boards and can be easily tailored to any other hardware: 

- STM3210B-EVAL
- STM3210E-EVAL
- STM32L152-EVAL
- STM32F373C_EVAL
- STM32F303C_EVAL
- STM32F303E_EVAL
- STM32L152D-EVAL
- STM32F302R8-Nucleo
 
- STM3210B-EVAL Set-up 
   - Jumper JP1 (USB disconnect) should be connected in position 2-3.

- STM3210E-EVAL Set-up 
   - Jumper JP14 (USB disconnect) should be connected in position 2-3.

- STM32L152-EVAL Set-up 
   - Jumper JP19 should be not connected. 

- STM32F302R8-Nucleo Rev C Set-up
      - Since there is no USB 2.0 Full speed connector (Type B) on the nucleo board, user has to make 
      his own USB shield daughter board with the a USB connector and plug it on top of Ardiuno connectorsthe 
      of STM32F302R8-Nucleo Rev C board. The USB connector has to be connected to the USB device associated GPIOs as follows:
       - DP (D+ of the USB connector) <======> PA12 (Nucleo board)
       - DM (D- of the USB connector) <======> PA11 (Nucleo board)
      - External USB 1.5k  resistor pull-ups is required on the USB D+ Line and VDD (3V3).
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
        - STM32303E-EVAL:  to configure the project for STM32F303xE devices.
        - STM32302x8-Nucleo:  to configure the project for STM32F302x8 devices
 - Run the application	  
	 
************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE******
