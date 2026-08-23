/**
  ******************************************************************************
  * @file    usb_prop.h
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   All processing related to Mass Storage Demo (Endpoint 0)
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


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __usb_prop_H
#define __usb_prop_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Speaker_init(void);
void Speaker_Reset(void);
void Speaker_SetConfiguration(void);
void Speaker_SetDeviceAddress (void);
void Speaker_Status_In (void);
void Speaker_Status_Out (void);
RESULT Speaker_Data_Setup(uint8_t);
RESULT Speaker_NoData_Setup(uint8_t);
RESULT Speaker_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *Speaker_GetDeviceDescriptor(uint16_t );
uint8_t *Speaker_GetConfigDescriptor(uint16_t);
uint8_t *Speaker_GetStringDescriptor(uint16_t);
uint8_t *Mute_Command(uint16_t Length);

/* Exported define -----------------------------------------------------------*/
#define Speaker_GetConfiguration          NOP_Process
//#define Speaker_SetConfiguration          NOP_Process
#define Speaker_GetInterface              NOP_Process
#define Speaker_SetInterface              NOP_Process
#define Speaker_GetStatus                 NOP_Process
#define Speaker_ClearFeature              NOP_Process
#define Speaker_SetEndPointFeature        NOP_Process
#define Speaker_SetDeviceFeature          NOP_Process
//#define Speaker_SetDeviceAddress          NOP_Process
#define GET_CUR                           0x81
#define SET_CUR                           0x01

#endif /* __usb_prop_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

