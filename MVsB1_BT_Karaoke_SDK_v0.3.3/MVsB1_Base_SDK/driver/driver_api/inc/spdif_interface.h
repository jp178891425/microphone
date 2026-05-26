/**
 **************************************************************************************
 * @file    spdif_interface.h
 * @brief   audio spdif interface
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * $Created: 2015-10-21 10:01:05$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __SPDIF_INTERFACE_H__
#define __SPDIF_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
    
void AudioSpdifRx_DeInit(void);    
    
void AudioSpdifRx_Init(void);
    
uint16_t AudioSpdifRx_GetDataLen(void);
    
uint16_t AudioSpdifRx_GetData(void* Buf1, void *Buf2, uint16_t MaxLen);
    
uint16_t AudioSpdifGetData(int16_t** Buf, uint16_t MaxLen);     


uint16_t AudioSpdifTXDataSet(void* Buf, uint16_t Len);

uint16_t AudioSpdifTXDataSpaceLenGet(void);

void AudioSpdif_OutInit(GPIO_PortA gpio, uint32_t SampleRate, void *SpdifFifo, uint32_t  SPDIF_FIFO_LEN);

void AudioSpdif_OutDeInit(void);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__SPDIF_INTERFACE_H__