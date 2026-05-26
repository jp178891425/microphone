/**
 **************************************************************************************
 * @file    i2s_api.h
 * @brief
 *
 * @author
 * @version
 *
 * $Created:
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __I2S_MIX_AUDIO_MODE_H__
#define __I2S_MIX_AUDIO_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus
//-------------------------------------------------//
#include "type.h"
#include "rtos_api.h"
#include "app_config.h"
#include "resampler.h"
#include "resampler_polyphase.h"
#include "sra.h"
#include "mcu_circular_buf.h"
//-----------------------------------------//
//#define    RESAMPLE_FIFO_LEN  CFG_PARA_MAX_SAMPLES_PER_FRAME*4*2

#define    I2S_SRC_1_EN //

typedef struct _SRC_SRA_
{
	uint8_t 			 OutTempBuf[128*4+32];
	uint8_t 			 InTempBuf[128*4+32];
    MCU_CIRCULAR_CONTEXT InFifo;
    MCU_CIRCULAR_CONTEXT OutFifo;
    uint8_t              *OutSrcFifo;//[RESAMPLE_FIFO_LEN];
    uint8_t              *OutSrcBuff;//[RESAMPLE_FIFO_LEN];
#ifdef I2S_SRC_1_EN
    ResamplerContext	         *ResampleObj;//High memory usage,Any sampling rate
#else
    ResamplerPolyphaseContext	 *ResampleObj;//Low memory usage,limitation sampling rate
#endif
    SRAContext           SraObj;
    MCU_CIRCULAR_CONTEXT SraFifo;
    uint8_t              *OutSraFifo;//[RESAMPLE_FIFO_LEN];
}MODULE_SRC_SRA;
//----------------------------------------------------//
void AudioI2S_ResMalloc(uint16_t SampleLen);
void AudioI2S_Release(void);
void AudioI2S_HwInit(void);
void AudioI2S_ConfigInit(void);
void AudioI2S_PlayDeInit(void);
void AudioI2S0_HWInit(void);
void AudioI2S1_HWInit(void);

void  AudioI2S_DataProcess(void);

void AudioI2S0_DataInProcess(void);
void AudioI2S1_DataInProcess(void);

void AudioI2S0_DataOutProcess(void);
void AudioI2S1_DataOutProcess(void);

void AudioI2S0_OutResampleInit(void);
void AudioI2S1_OutResampleInit(void);
void AudioI2S0_InResampleInit(void);
void AudioI2S1_InResampleInit(void);

uint16_t AudioI2S0_DataSetX(void *Buf, uint16_t Len);
uint16_t AudioI2S1_DataSetX(void *Buf, uint16_t Len);
uint16_t AudioI2S0_DataSpaceLenGetX(void);
uint16_t AudioI2S1_DataSpaceLenGetX(void);

uint16_t AudioI2S0_DataLenGetX(void);
uint16_t AudioI2S0_DataGetX(void* Buf, uint16_t Len);
uint16_t AudioI2S1_DataLenGetX(void);
uint16_t AudioI2S1_DataGetX(void* Buf, uint16_t Len);



#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __USB_AUDIO_MODE_H__

