/**
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    readme.txt
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Description of the USB Virtual_COM_Port Demo.
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
In modern PCs, USB is the standard communication port for almost all peripherals. 
However many industrial software applications still use the classic COM Port (UART).
The Virtual COM Port Demo provides a simple solution to bypass this problem; it
uses the USB-FS-Device as a COM port by affecting the legacy PC application designed for
COM Port communication.
This Virtual COM Port Demo provides the firmware examples for the STM32F10xxx,STM32L15xxx,
STM32F30xxx and STM32F37xxx families.

8MHZ Cristal should be mounted on boards to recognize USB device.

Note that with this demo only the following configuration are supported:
-> 7bit data with parity (Even or Odd).
-> 8bit data with and without parity.

This demo is using two different methods for the IN and OUT transfers in order 
to manage the data rate difference between USB and USART buses:

 - OUT transfers (from Host to Device):
     When a packet is received from the host on the OUT pipe (EP3), the Endpoint
     callback function is called when the transfer is complete and all the received
     data are then sent through USART peripheral in polling mode. This allows all
     incoming OUT packets to be NAKed till the current packet is completely transferred
     through the USART interface.
 
 - IN transfers (from Device to Host):
     For IN data, a large circular buffer is used. USART and USB respectively write
     and read to/from this buffer independently.
     USART RXNE interrupt is used to write data into the buffer. This interrupt 
     has the highest priority, which allows to avoid overrun and data loss conditions.
     USB IN endpoint (EP1) is written with the received data into the Timer interrupt 
     callback (used instead of SOF)
     Into this callback, a frame counter counts the number of passed frames since 
     the last time IN endpoint was written. This allows to write IN endpoint at
     separated frames when enough data are present in the data buffer. 
     To modify the number of frame between IN write operations, modify the value 
     of the define "VCOMPORT_IN_FRAME_INTERVAL" in "usb_endp.c" file.
     To allow high data rate performance. USART is configure to send data using DMA     


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

To select the STMicroelectronics evaluation board used to run the example, uncomment
the corresponding line in platform_config.h file.

  - STM3210B-EVAL Set-up 
     - Jumper JP1 (USB disconnect) should be connected in position 2-3.
     - Connect a null-modem female/female RS232 cable between the DB9 connector 
      CN6 and Host (PC) serial port.

  - STM3210E-EVAL Set-up 
     - Jumper JP14 (USB disconnect) should be connected in position 2-3.
     - Connect a null-modem female/female RS232 cable between the DB9 connector
      CN12 and Host (PC) serial port. 

  - STM32L152-EVAL Set-up 
     - Jumper JP19 should be not connected.
     - Jumper JP5 should be connected in 2-3 position (RS232)
     - Connect a null-modem female/female RS232 cable between the DB9 connector
       CN2 and Host (PC) serial port.
     - The signal Bootloader_RESET (shared with CTS signal) HENSE USART cable with 
      RX,TX and GND lines ONLY is mandatory
  
  - STM32L152D-EVAL Set-up 
	- Jumper JP4 should be connected in 2-3 position (RS232)
	- Connect a null-modem female/female RS232 cable between the DB9 connector	
      CN1 and Host (PC) serial port.
	- In STM32L152D-EVAL RevB CTS pin is connected directly (without jumper) to MCU Reset: HENSE USART cable with 
      RX,TX and GND lines ONLY is mandatory. When using EVAL Rev A Put JP3 to position 2-3	
    
  - STM32F373C_EVAL  Set-up 
	- Jumper JP6 should be connected in 2-3 position (RS232)
        - Jumper JP7 should be connected in 1-2 position.
        - Connect a null-modem female/female RS232 cable between the DB9 connector
          CN12 and Host (PC) serial port.

  - STM32F303C_EVAL  Set-up 
	- Jumper JP13 should be connected in 2-3 position (RS232)
        - Connect a null-modem female/female RS232 cable between the DB9 connector
          CN8 and Host (PC) serial port.
		  
  - STM32F303E_EVAL  Set-up 
	- Jumper JP13 should be connected in 2-3 position (RS232)
        - Connect a null-modem female/female RS232 cable between the DB9 connector
          CN8 and Host (PC) serial port.
		  

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
 - Run the application

************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE******
