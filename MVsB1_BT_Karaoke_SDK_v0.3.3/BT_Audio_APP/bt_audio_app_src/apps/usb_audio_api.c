/**
 **************************************************************************************
 * @file    usb_audio_api.c
 * @brief
 *
 * @author  Owen
 * @version V1.0.0
 *
 * $Created: 2018-04-27 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "rtos_api.h"
#include "main_task.h"
#include "app_message.h"
#include "app_config.h"
#include "debug.h"
#include "delay.h"
#include "dac.h"
#include "otg_device_hcd.h"
#include "otg_device_audio.h"
#include "otg_device_standard_request.h"
#include "mcu_circular_buf.h"
#include "audio_core_api.h"
#include "timer.h"
#include "resampler.h"
#include "ctrlvars.h"
#include "otg_device_audio.h"
#include "sra.h"
#include "usb_audio_api.h"

#ifdef CFG_FUNC_USB_MIX_MODE
//|---------extern---------------------------------|
extern int32_t AudioCoreServiceStatusGet(void);
void UsbAudioSrcInit(void);
int32_t Get_Resampler_Polyphase(uint32_t resampler);
//----------var----------------------------------------------//

SRAContext UsbSpeakerSraObj;//usb speaker软件微调pcm采样点数，结构体。
SRAContext UsbMicSraObj;//usb mic软件微调pcm采样点数，结构体。

int16_t UsbSraInBuf[SRA_BLOCK * 2  + 64];
int16_t UsbSraOutBuf[SRA_BLOCK * 2 + 64];


MCU_CIRCULAR_CONTEXT UsbMicBuf;//usb mic临时缓冲,采样率微调专用
MCU_CIRCULAR_CONTEXT UsbSpeakerBuf;//usb speaker临时缓冲,采样率微调专用

uint8_t *Mic_SraFifo;//[512*4*2];//最大采样点数 = 512
uint8_t *UsbSpeaker_Buf;//[512*4];//最大采样点数 = 512

uint16_t SraFifoLen;
//-----------------------------------------//
UsbAudio UsbAudioSpeaker;
UsbAudio UsbAudioMic;

uint32_t temp_speaker_fram;
uint32_t temp_mic_fram;
uint32_t FramCount = 0;
bool 	IsUsbAudioMode = FALSE;
#ifdef CFG_RES_SUPPORT_DYNAMIC_RAM
	//-------USB插拔动态分配及释放相关ram功能选项-------------//
bool   UsbDynmicRealeaseFlag = 0; //0=不支持动态释放， 1=支持动态释放
#endif

uint16_t  MicWaterLevelSpeed  = 0;
uint16_t  SpeakerWaterLevelSpeed  = 0;
uint16_t  SraSpeakDataLen = 0;
uint8_t   UsbSpeakerBuff[128*4+128];
#define  SPEAKER_SRA_SPEED     1000//MS
#define  USB_MIC_SRA_SPEED     50//MS
/*
****************************************************************
*
*
****************************************************************
*/
int32_t AudioCoreServiceStatusGet(void)
{
    //DBG("%s\n",__func__);
    //转采样
   if(SoftFlagGet(SoftFlagEffectChange))
	{
		return 0;//屏蔽
	}
#ifdef  CFG_RES_AUDIO_USB_IN_EN

  #ifdef CFG_RES_AUDIO_USB_IN_SRC_EN
	if(UsbAudioSpeaker.Resampler == NULL)
	{
		return 0;
	}
	#endif  ///end of CFG_RES_AUDIO_USB_SRC_EN

	//Speaker FIFO
	if(UsbAudioSpeaker.PCMBuffer == NULL)
	{
		return 0;
	}
	//Speaker fifo 2
	if(UsbSpeaker_Buf == NULL)
	{
		return 0;
	}
	//Speaker BUFF
	if(mainAppCt.SourceBuf_UsbIn == NULL)
	{
		return 0;
	}

#endif//end of CFG_RES_USB_IN_EN

	//转采样
#ifdef CFG_RES_AUDIO_USB_OUT_EN

   #ifdef CFG_RES_AUDIO_USB_OUT_SRC_EN
	if(UsbAudioMic.Resampler == NULL)
	{
		return 0;
	}
   #endif //end of #ifdef CFG_RES_AUDIO_USB_SRC_EN
	if(UsbAudioMic.SRCOutBuf == NULL)
	{
		return 0;
	}


	//MIC FIFO
	if(UsbAudioMic.PCMBuffer == NULL)
	{
		return 0;
	}
	//MIC BUFF
	if(mainAppCt.SinkBuf_UsbOut == NULL)
	{
		return 0;
	}
	//MIC SRA BUFF
	if(Mic_SraFifo == NULL)
	{
		return 0;
	}
#endif
	   return 1;
}
/*
****************************************************************
*
*USB声卡模式参数配置，资源反初始化
*
****************************************************************
*/
bool UsbDevicePlayDeInit(void)
{
     DBG("%s\n",__func__);
     UsbAudioSpeaker.AudioSampleRateFlag = FALSE;
     UsbAudioMic.AudioSampleRateFlag = FALSE;
     //转采样
#ifdef  CFG_RES_AUDIO_USB_IN_EN

   #ifdef CFG_RES_AUDIO_USB_IN_SRC_EN
	if(UsbAudioSpeaker.Resampler != NULL)
	{
		osPortFree(UsbAudioSpeaker.Resampler);
		UsbAudioSpeaker.Resampler = NULL;
	}
	#endif  ///end of CFG_RES_AUDIO_USB_SRC_EN
	if(UsbAudioSpeaker.SRCOutBuf != NULL)
	{
		osPortFree(UsbAudioSpeaker.SRCOutBuf);
		UsbAudioSpeaker.SRCOutBuf = NULL;
	}
	//Speaker FIFO
	if(UsbAudioSpeaker.PCMBuffer != NULL)
	{
		osPortFree(UsbAudioSpeaker.PCMBuffer);
		UsbAudioSpeaker.PCMBuffer = NULL;
	}
	//Speaker fifo 2
	if(UsbSpeaker_Buf != NULL)
	{
		osPortFree(UsbSpeaker_Buf);
		UsbSpeaker_Buf = NULL;
	}
	//Speaker BUFF
	if(mainAppCt.SourceBuf_UsbIn != NULL)
	{
		osPortFree(mainAppCt.SourceBuf_UsbIn);
		mainAppCt.SourceBuf_UsbIn = NULL;
	}
#endif//end of CFG_RES_USB_IN_EN

	//转采样
#ifdef CFG_RES_AUDIO_USB_OUT_EN

    #ifdef CFG_RES_AUDIO_USB_OUT_SRC_EN
	if(UsbAudioMic.Resampler != NULL)
	{
		osPortFree(UsbAudioMic.Resampler);
		UsbAudioMic.Resampler = NULL;
	}
   #endif //end of #ifdef CFG_RES_AUDIO_USB_SRC_EN
	if(UsbAudioMic.SRCOutBuf != NULL)
	{
		osPortFree(UsbAudioMic.SRCOutBuf);
		UsbAudioMic.SRCOutBuf = NULL;
	}


	//MIC FIFO
	if(UsbAudioMic.PCMBuffer != NULL)
	{
		osPortFree(UsbAudioMic.PCMBuffer);
		UsbAudioMic.PCMBuffer = NULL;
	}
	//MIC BUFF
	if(mainAppCt.SinkBuf_UsbOut != NULL)
	{
		osPortFree(mainAppCt.SinkBuf_UsbOut);
		mainAppCt.SinkBuf_UsbOut = NULL;
	}
	//MIC SRA BUFF
	if(Mic_SraFifo != NULL)
	{
		osPortFree(Mic_SraFifo);
		Mic_SraFifo = NULL;
	}

#endif///end of CFG_REGS_AUDIO_USB_OUT_EN
     return TRUE;
}
/*
****************************************************************
*
*USB声卡模式参数配置，资源初始化
*
****************************************************************
*/
bool UsbDevicePlayInit(uint16_t SamplesLen)
{
	DBG("%s\n",__func__);

	SraFifoLen = SamplesLen * 4 * 4 + 4 * 4;//sam modify 20210106

#ifdef  CFG_RES_AUDIO_USB_IN_EN

	//Speaker buff
	if(mainAppCt.SourceBuf_UsbIn==NULL)
	{
	   mainAppCt.SourceBuf_UsbIn = osPortMallocFromEnd(SamplesLen*2*2);
	}
	if(mainAppCt.SourceBuf_UsbIn == NULL)
	{
		DBG("mainAppCt.SourceBuf_UsbIn memory error\n");
		return FALSE;
	}

   #ifdef CFG_RES_AUDIO_USB_IN_SRC_EN
	//UsbAudioSpeaker.Resampler = (ResamplerContext *)osPortMallocFromEnd(sizeof(ResamplerContext));
	if(UsbAudioSpeaker.Resampler==NULL)
	{
	UsbAudioSpeaker.Resampler = (ResamplerPolyphaseContext *)osPortMallocFromEnd(sizeof(ResamplerPolyphaseContext));
	}
	if(UsbAudioSpeaker.Resampler == NULL)
	{
		DBG("UsbAudioSpeaker.Resampler memory error\n");
		return FALSE;
	}
    resampler_polyphase_init(UsbAudioSpeaker.Resampler, 2, Get_Resampler_Polyphase(UsbAudioSpeaker.AudioSampleRate));

	#endif  ///end of CFG_RES_AUDIO_USB_SRC_EN
	
	//Speaker FIFO
    if(UsbAudioSpeaker.PCMBuffer==NULL)
    {
	  UsbAudioSpeaker.PCMBuffer = osPortMallocFromEnd(SraFifoLen);
    }
	if(UsbAudioSpeaker.PCMBuffer == NULL)
	{
		DBG("UsbAudioSpeaker.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioSpeaker.PCMBuffer,0,SraFifoLen);
	MCUCircular_Config(&UsbAudioSpeaker.CircularBuf, UsbAudioSpeaker.PCMBuffer, SraFifoLen);
	//speaker sra FIFO
    if(UsbSpeaker_Buf==NULL)
    {
    	UsbSpeaker_Buf = osPortMallocFromEnd(SraFifoLen);
    }
	if(UsbSpeaker_Buf == NULL)
	{
		DBG("UsbSpeaker_Buf  memory error\n");
		return FALSE;
	}
	memset(UsbSpeaker_Buf,0,SraFifoLen);
	MCUCircular_Config(&UsbSpeakerBuf, UsbSpeaker_Buf, SraFifoLen);//sra
#endif//end of CFG_RES_USB_IN_EN

	//转采样
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	//mic buff
    if(mainAppCt.SinkBuf_UsbOut==NULL)
    {
    	mainAppCt.SinkBuf_UsbOut = osPortMallocFromEnd(SamplesLen*2*2);
    }

	if(mainAppCt.SinkBuf_UsbOut == NULL)
	{
		DBG("mainAppCt.SinkBuf_UsbOut memory error\n");
		return FALSE;
	}
    #ifdef CFG_RES_AUDIO_USB_OUT_SRC_EN
	//UsbAudioMic.Resampler = (ResamplerContext *)osPortMallocFromEnd(sizeof(ResamplerContext));
    if(UsbAudioMic.Resampler==NULL)
    {
    	UsbAudioMic.Resampler = (ResamplerPolyphaseContext *)osPortMallocFromEnd(sizeof(ResamplerPolyphaseContext));
    }

	if(UsbAudioMic.Resampler == NULL)
	{
		DBG("UsbAudioMic.Resampler memory error\n");
		return FALSE;
	}
	//if(IsUsbAudioMode == TRUE)

		//resampler_init(UsbAudioMic.Resampler, 2,CFG_PARA_SAMPLE_RATE, 48000, 0, 0);
	resampler_polyphase_init(UsbAudioMic.Resampler, 2, Get_Resampler_Polyphase(UsbAudioMic.AudioSampleRate));
    #endif //end of #ifdef CFG_RES_AUDIO_USB_SRC_EN
    if(UsbAudioMic.SRCOutBuf==NULL)
    {
    	UsbAudioMic.SRCOutBuf = osPortMallocFromEnd(USB_AUDIO_SRC_BUF_LEN);
    }
	if(UsbAudioMic.SRCOutBuf == NULL)
	{
		DBG("UsbAudioMic.SRCOutBuf memory error\n");
		return FALSE;
	}
	

	//MIC FIFO
    if(UsbAudioMic.PCMBuffer==NULL)
    {
    	UsbAudioMic.PCMBuffer = osPortMallocFromEnd(SraFifoLen);
    }
	if(UsbAudioMic.PCMBuffer == NULL)
	{
		DBG("UsbAudioMic.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioMic.PCMBuffer,0,SraFifoLen);
	MCUCircular_Config(&UsbAudioMic.CircularBuf, UsbAudioMic.PCMBuffer, SraFifoLen);

	//mic sra FIFO
    if(Mic_SraFifo==NULL)
    {
    	Mic_SraFifo = osPortMallocFromEnd(SraFifoLen);
    }

	if(Mic_SraFifo == NULL)
	{
		DBG("Usb Mic_Buf  memory error\n");
		return FALSE;
	}
	memset(Mic_SraFifo,0,SraFifoLen);
	MCUCircular_Config(&UsbMicBuf, Mic_SraFifo, SraFifoLen);//sra

#endif///end of CFG_REGS_AUDIO_USB_OUT_EN
#ifdef CFG_RES_AUDIO_USB_IN_EN
	mainAppCt.AudioCore->AudioSource[USB_MIX_SOURCE_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSource[USB_MIX_SOURCE_NUM].FuncDataGet = UsbAudioSpeakerDataGet;
	mainAppCt.AudioCore->AudioSource[USB_MIX_SOURCE_NUM].FuncDataGetLen = UsbAudioSpeakerDataLenGet;
	mainAppCt.AudioCore->AudioSource[USB_MIX_SOURCE_NUM].IsSreamData = 1;//TRUE;
	mainAppCt.AudioCore->AudioSource[USB_MIX_SOURCE_NUM].PcmFormat = 2;//
	mainAppCt.AudioCore->AudioSource[USB_MIX_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.SourceBuf_UsbIn;
#endif
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	mainAppCt.AudioCore->AudioSink[USB_AUDIO_SINK_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmFormat = 2;
	mainAppCt.AudioCore->AudioSink[USB_AUDIO_SINK_NUM].SreamDataState = 0;
	mainAppCt.AudioCore->AudioSink[USB_AUDIO_SINK_NUM].FuncDataSet = UsbAudioMicDataSet;
	mainAppCt.AudioCore->AudioSink[USB_AUDIO_SINK_NUM].FuncDataSpaceLenGet = UsbAudioMicSpaceLenGet;
	mainAppCt.AudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.SinkBuf_UsbOut; //sink audiocore 内buf 1 frame。
#endif
	UsbMicFifoCfg();

	UsbSpeakerFifoCfg();

	UsbAudioSrcInit();

	UsbAudioSraInit();

	SraSpeakDataLen = 0;

	memset(UsbSpeakerBuff,0,sizeof(UsbSpeakerBuff));

	return TRUE;
}
/*
****************************************************************
*
*
*
****************************************************************
*/
void UsbDevicePowerInit(void)
{
     memset(&UsbAudioSpeaker,0,sizeof(UsbAudio));
     memset(&UsbAudioMic,0,sizeof(UsbAudio));
     UsbAudioSpeaker.Channel    = 2;
     UsbAudioMic.Channel        = MIC_CH;
     UsbAudioSpeaker.LeftVol    = AUDIO_MAX_VOLUME;
     UsbAudioSpeaker.RightVol   = AUDIO_MAX_VOLUME;
     UsbAudioMic.LeftVol        = AUDIO_MAX_VOLUME;
     UsbAudioMic.RightVol       = AUDIO_MAX_VOLUME;
     UsbAudioSraInit();
#ifdef CFG_RES_SUPPORT_DYNAMIC_RAM
	//-------USB插拔动态分配及释放相关ram功能选项-------------//
     UsbDynmicRealeaseFlag = 0;
#endif
}
/*
****************************************************************
*
*
*
****************************************************************
*/
void UsbDeviceEnable(void)
{
	uint32_t mode = 0;
	
	DBG("UsbDevice:App enable\n");
#ifdef CFG_RES_SUPPORT_DYNAMIC_RAM
	IsUsbAudioMode = TRUE;
	if((SoftFlagGet(SoftFlagEffectChange)==0))
	{
		if(UsbDynmicRealeaseFlag)
		{
		  UsbDevicePlayInit(mainAppCt.SamplesPreFrame);//mainAppCt.SamplesPreFrame);
		}
	}	
#endif

#ifdef CFG_RES_AUDIO_USB_IN_EN
    #ifdef CFG_RES_AUDIO_USB_IN_SRC_EN
	if(UsbAudioSpeaker.Resampler)
	{
		//resampler_init(UsbAudioSpeaker.Resampler, 2, 48000, CFG_PARA_SAMPLE_RATE, 0, 0);
	  resampler_polyphase_init(UsbAudioSpeaker.Resampler, 2, Get_Resampler_Polyphase(UsbAudioSpeaker.AudioSampleRate));
	  UsbAudioSpeaker.AudioSampleRateFlag = TRUE;
	}
	#endif
#endif

#ifdef CFG_RES_AUDIO_USB_OUT_EN
   #ifdef CFG_RES_AUDIO_USB_OUT_SRC_EN
	if(UsbAudioMic.Resampler)
	{
		//resampler_init(UsbAudioMic.Resampler, 2,CFG_PARA_SAMPLE_RATE, 48000, 0, 0);
	  resampler_polyphase_init(UsbAudioMic.Resampler, 2, RESAMPLER_POLYPHASE_SRC_RATIO_160_147);//Get_Resampler_Polyphase(UsbAudioMic.AudioSampleRate));
	  UsbAudioMic.AudioSampleRateFlag = TRUE;
	}
	#endif
#endif

	IsUsbAudioMode = TRUE;
	OTG_DeviceModeSel(CFG_PARA_USB_MODE,0x1235,CFG_PARA_USB_MODE+1);
#ifdef CFG_RES_CARD_USE		
	mode = CFG_PARA_USB_MODE;
	if((mode == READER) || (mode == AUDIO_READER) || (mode == MIC_READER) || (mode == AUDIO_MIC_READER))
	{	
		if(ResourceValue(AppResourceCard))
		{
			CardPortInit(CFG_RES_CARD_GPIO);
			if(SDCard_Init() == NONE_ERR)
			{
				APP_DBG("SD INIT OK\n");
				//sd_link = 1;
			}
		}
#if 0
		if(sd_link == 0)
		{
			if(mode == READER)
			{
				APP_DBG("mode error\n");
			}
			else if(mode == AUDIO_READER)
			{
				OTG_DeviceModeSel(AUDIO_ONLY,0x0000,0x1234);
			}
			else if(mode == MIC_READER)
			{
				OTG_DeviceModeSel(MIC_ONLY,0x0000,0x1234);
			}
			else if(mode == AUDIO_MIC_READER)
			{
				OTG_DeviceModeSel(AUDIO_MIC,0x0000,0x1234);
			}
		}
#endif
	}	
#endif
	OTG_DeviceInit();
	NVIC_EnableIRQ(Usb_IRQn);
//	NVIC_SetPriority(Usb_IRQn,1);
//	NVIC_SetPriority(BT_IRQn,0);
//	NVIC_SetPriority(BLE_IRQn,0);
	DBG("RESET NVIC BT Priority\n");
}
/*
****************************************************************
*
*
*
****************************************************************
*/
void UsbDeviceDisable(void)
{
	DBG("UsbDevice:Appn disable\n");
	NVIC_DisableIRQ(Usb_IRQn);
	OTG_DeviceDisConnect();
	IsUsbAudioMode = FALSE;

    #ifdef CFG_RES_AUDIO_USB_IN_EN
	UsbAudioSpeaker.AltSet = 0;
	AudioCoreSourceDisable(USB_MIX_SOURCE_NUM);
    #endif

    #ifdef CFG_RES_AUDIO_USB_OUT_EN
	UsbAudioMic.AltSet = 0;
	AudioCoreSinkDisable(USB_AUDIO_SINK_NUM);
    #endif
    #ifdef CFG_RES_SUPPORT_DYNAMIC_RAM
	if(SoftFlagGet(SoftFlagEffectChange)==0)
	{
		if(UsbDynmicRealeaseFlag)
		{
		  UsbDevicePlayDeInit();
		}
	}
    #endif	
}
/*
****************************************************************
*
*
*
****************************************************************
*/
void UsbDeviceSleep(void)
{
	DBG("%s\n",__func__);

    #ifdef CFG_RES_AUDIO_USB_IN_EN
	UsbAudioSpeaker.AltSet = 0;
	AudioCoreSourceDisable(USB_MIX_SOURCE_NUM);
    #endif

    #ifdef CFG_RES_AUDIO_USB_OUT_EN
	UsbAudioMic.AltSet = 0;
	AudioCoreSinkDisable(USB_AUDIO_SINK_NUM);
    #endif
}
/*
****************************************************************
*
*
*
****************************************************************
*/
void UsbMicFifoCfg(void)
{
#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(UsbAudioMic.PCMBuffer)
	{
	  MCUCircular_Config(&UsbAudioMic.CircularBuf, UsbAudioMic.PCMBuffer, SraFifoLen);
	}
	if(Mic_SraFifo)
	{
      MCUCircular_Config(&UsbMicBuf, Mic_SraFifo, SraFifoLen);//sra
	}
    MicWaterLevelSpeed  = 0;
#endif //end of
}
/*
****************************************************************
*
*
*
****************************************************************
*/
void UsbSpeakerFifoCfg(void)
{
#ifdef CFG_RES_AUDIO_USB_IN_EN
	if(UsbAudioSpeaker.PCMBuffer)
	{
	  MCUCircular_Config(&UsbAudioSpeaker.CircularBuf, UsbAudioSpeaker.PCMBuffer, SraFifoLen);
	}
	if(UsbSpeaker_Buf)
	{
       MCUCircular_Config(&UsbSpeakerBuf, UsbSpeaker_Buf, SraFifoLen);//sra
	}
    SpeakerWaterLevelSpeed  = 0;
#endif
}
/*
****************************************************************
* USB软件采样率转换处理初始化函数
*
*
****************************************************************
*/
void UsbAudioSrcInit(void)
{
#ifdef CFG_RES_AUDIO_USB_OUT_SRC_EN
	if(UsbAudioMic.Resampler)
	  {
	    UsbAudioMic.AudioSampleRateFlag = TRUE;

		if (UsbAudioMic.AudioSampleRate == 48000)//44k->SampleRate(48k)
			{
			  resampler_polyphase_init(UsbAudioMic.Resampler, 2, RESAMPLER_POLYPHASE_SRC_RATIO_160_147);
			}
		if (UsbAudioMic.AudioSampleRate == 44100)//48k->SampleRate(44k)
			{
			  resampler_polyphase_init(UsbAudioMic.Resampler, 2, RESAMPLER_POLYPHASE_SRC_RATIO_147_160);
			}
		}
#endif

#ifdef CFG_RES_AUDIO_USB_IN_SRC_EN
	if(UsbAudioSpeaker.Resampler)
		{
		    UsbAudioSpeaker.AudioSampleRateFlag = TRUE;

			if (UsbAudioSpeaker.AudioSampleRate == 44100)//44k->SampleRate(48k)
			{
			  resampler_polyphase_init(UsbAudioSpeaker.Resampler, 2, RESAMPLER_POLYPHASE_SRC_RATIO_160_147);
			}
			if (UsbAudioSpeaker.AudioSampleRate == 48000)//48k->SampleRate(44k)
			{
			  resampler_polyphase_init(UsbAudioSpeaker.Resampler, 2, RESAMPLER_POLYPHASE_SRC_RATIO_147_160);
			}
		}
#endif
}
/*
****************************************************************
* USB软件微调处理初始化函数
*
*
****************************************************************
*/
void UsbAudioSraInit(void)
{
	sra_init(&UsbSpeakerSraObj,2);
	sra_init(&UsbMicSraObj,2);
}
/*
****************************************************************
*
*usb speaker 数据获取
*
****************************************************************
*/
uint16_t UsbAudioSpeakerDataGet(void *Buffer,uint16_t Len)
{
	uint16_t Length = 0;
	if(!UsbAudioSpeaker.PCMBuffer)
		return Len;
    Length = Len*4;
    Len = MCUCircular_GetData(&UsbAudioSpeaker.CircularBuf,Buffer,Length)/4;
	return Len;
}
/*
****************************************************************
*
*usb speaker 有效数据长度获取
*
****************************************************************
*/
uint16_t UsbAudioSpeakerDataLenGet(void)
{
	uint16_t Len;
	if(!UsbAudioSpeaker.PCMBuffer)
		return 0;
	Len = MCUCircular_GetDataLen(&UsbAudioSpeaker.CircularBuf)/4;
	return Len;
}
/*
****************************************************************
*
*usb mic 数据设置
*
****************************************************************
*/
uint16_t UsbAudioMicDataSet(void *Buffer,uint16_t Len)
{
	uint32_t i;
	uint32_t temp0;
	uint32_t space_len;// = MCUCircular_GetSpaceLen(&UsbAudioMic.CircularBuf);
	uint32_t one_fifo = SraFifoLen/8;
	uint16_t data_len;
	int32_t SRCDoneLen,n_inc_dec_o;
	int16_t *UsbMicData = (int16_t *)Buffer;

#ifdef CFG_RES_AUDIO_USB_OUT_EN
	//printf(">>%u\n",Len);
	if(!UsbAudioMic.PCMBuffer) return 0;
#ifdef CFG_RES_AUDIO_USB_OUT_SRC_EN
	if(UsbAudioMic.Resampler==NULL)return 0;
#endif
	if(!AudioCoreServiceStatusGet()) return 0;

    #ifdef CFG_RES_AUDIO_USB_OUT_SRC_EN
//-----------------SRC---------------------------------------------------------------------//
	//if(UsbAudioMic.AudioSampleRate == 48000)
	if(UsbAudioMic.AudioSampleRate != CFG_PARA_SAMPLE_RATE)
	{
		if(UsbAudioMic.Resampler == NULL)
		{
			return 0;
		}
		temp0 = Len/128;
		for(i=0;i<temp0;i++)
		{
			//SRCDoneLen = resampler_apply(UsbAudioMic.Resampler, (int16_t*)(UsbMicData+(i*128*2)), (int16_t*)UsbAudioMic.SRCOutBuf, 128);
			SRCDoneLen = resampler_polyphase_apply(UsbAudioMic.Resampler, (int16_t*)(UsbMicData+(i*128*2)), (int16_t*)UsbAudioMic.SRCOutBuf, 128);
			if(SRCDoneLen < 0) SRCDoneLen = 0;
			MCUCircular_PutData(&UsbMicBuf, UsbAudioMic.SRCOutBuf, SRCDoneLen * 2 * 2);
		}
	}
	else //if(UsbAudioMic.AudioSampleRate == 44100)
	#endif ///end of #ifdef CFG_RES_AUDIO_USB_SRC_EN
	{
		MCUCircular_PutData(&UsbMicBuf, UsbMicData, Len*4);
	}
//-----------------SRA------------------------------------------------------------------//
		data_len = MCUCircular_GetDataLen(&UsbMicBuf)/4;//usb mic临时缓冲
		//DBG("data_len = %d, %d\n", data_len, MCUCircular_GetSpaceLen(&UsbAudioMic.CircularBuf)/4);
		data_len /= 128;

	    for(i = 0; i < data_len; i++)
	    {
			space_len = MCUCircular_GetSpaceLen(&UsbAudioMic.CircularBuf);
			if(space_len / 4 <= 128 + 2)
			{
				//DBG("!!");
				break;
			}
			//DBG("len %dm \n", space_len/4);
			if((space_len >= one_fifo*6)&& (space_len < one_fifo*8))
				{
					n_inc_dec_o = 1;
					//DBG("+1\n");
				}
			else if(space_len >= one_fifo*4)// && (space_len < one_fifo*4))
				{
					n_inc_dec_o = 0;
					//DBG("0\n");
				}
			else
				{
					n_inc_dec_o = -1;
					//DBG("-1\n");
				}
			 //-------调节周期，影响了频率---------------//
//			if(MicWaterLevelSpeed != USB_MIC_SRA_SPEED)
//			{
//				n_inc_dec_o = 0;
//			}
           //------------------------------------------//
			MCUCircular_GetData(&UsbMicBuf,(uint8_t *)UsbAudioMic.SRCOutBuf,SRA_BLOCK*4); //

			if(sra_apply(&UsbMicSraObj, UsbAudioMic.SRCOutBuf, (int16_t *)UsbSraOutBuf, n_inc_dec_o)==SRA_ERROR_OK)//
			{
				SRCDoneLen = SRA_BLOCK + n_inc_dec_o;
			}
			else
			{
				SRCDoneLen = SRA_BLOCK;
				memcpy(UsbSraOutBuf,UsbAudioMic.SRCOutBuf,SRCDoneLen*4);
			}
//------------Put Data---------------------------------------------------------------------------------//
			MCUCircular_PutData(&UsbAudioMic.CircularBuf, UsbSraOutBuf, SRCDoneLen*4);
	     }
	    //-------调节周期，影响了频率----------------//
		if(MicWaterLevelSpeed == USB_MIC_SRA_SPEED)
		{
			MicWaterLevelSpeed++;
		}
		//-----------------------------------------//
#endif
	return Len;
}
/*
****************************************************************
*
*usb mic 数据缓存剩余空间获取
*
****************************************************************
*/
uint16_t UsbAudioMicSpaceLenGet(void)
{
	uint16_t Len;
	if(!UsbAudioMic.PCMBuffer) return 0;
	Len = MCUCircular_GetSpaceLen(&UsbAudioMic.CircularBuf);
	Len = Len/4;
	return Len;
}
/*
****************************************************************
*
*usb speaker 采样率转换   + 采样率微调 1
*
****************************************************************
*/

uint16_t usb_speaker_sra_state;

uint16_t usb_iso_speaker(uint8_t *InBuf, uint8_t *OutBuf,uint8_t Len)
{
	//static uint16_t DataLen = 0;
	uint16_t SRCDoneLen,n_inc_dec_o,i,space_len;
	uint16_t one_fifo;
	//static uint16_t WaterLevelCount;
	SRCDoneLen = 0;
	one_fifo = SraFifoLen/8;

	for(i= 0; i < Len;i++)
	{
		UsbSpeakerBuff[SraSpeakDataLen++] =InBuf[i];
		if(SraSpeakDataLen>=128*4)
		{
			SraSpeakDataLen = 0;
			space_len = MCUCircular_GetSpaceLen(&UsbAudioSpeaker.CircularBuf);
			
			//if((space_len >= one_fifo*6)&& (space_len <= one_fifo*7))
			if(space_len >= one_fifo*6)
			{
				n_inc_dec_o = 1;
				//OTG_DBG1("+1\n");
				usb_speaker_sra_state = 2;
			}
		  else if(space_len >= one_fifo*4)// && (space_len < one_fifo*4))
		    {
			   n_inc_dec_o = 0;
			   //OTG_DBG1("0\n");
			   usb_speaker_sra_state = 1;
		    }
		  else
		    {
			 n_inc_dec_o = -1;
			  //OTG_DBG1("-1\n");
			  usb_speaker_sra_state = 3;
		    }
		 //-------起动调节时间- 调节周期，影响了频响，速度慢频响好，速度快频响变化大---------------------//
//		   if(SpeakerWaterLevelSpeed < SPEAKER_SRA_SPEED)
//			 {
//				 n_inc_dec_o = 0;
//				 WaterLevelCount = 0;
//			 }

			if(sra_apply(&UsbSpeakerSraObj, (int16_t *)UsbSpeakerBuff, (int16_t *)OutBuf, n_inc_dec_o)==SRA_ERROR_OK)//
			{
				SRCDoneLen = SRA_BLOCK + n_inc_dec_o;
			}
		}
	}
  return SRCDoneLen*4;
}
/*
****************************************************************
*
*usb speaker 采样率转换   + 采样率微调 2
*
****************************************************************
*/
void usb_iso_data_proess(uint16_t SamplesLen)
{
	uint32_t i;
	uint16_t data_len,space_len;
	int32_t n_inc_dec_o;
	int32_t SRCDoneLen = 0;
	uint32_t one_fifo = SraFifoLen/8;
	static uint16_t WaterLevelCount;

	if(IsUsbAudioMode == FALSE)	return;

	if(UsbSpeaker_Buf==NULL) return;

	if(UsbAudioSpeaker.PCMBuffer==NULL) return;

	if(!AudioCoreServiceStatusGet()) return;

	data_len = MCUCircular_GetDataLen(&UsbAudioSpeaker.CircularBuf)/4;///来自USB AUDIO的数据

	if(data_len < SRA_BLOCK)
	{
		return;
	}
//-----------------SRA------------------------------------------------------------------//
	data_len /= 128;

    for(i = 0; i < data_len; i++)
    {
		 space_len = MCUCircular_GetSpaceLen(&UsbSpeakerBuf);

		if((space_len >= one_fifo*5)&& (space_len < one_fifo*7))
			{
				n_inc_dec_o = 1;
				//DBG("+1\n");
			}
		else if(space_len >= one_fifo*2)// && (space_len < one_fifo*4))
			{
				n_inc_dec_o = 0;
				//DBG("0\n");
			}
		else
			{
				n_inc_dec_o = -1;
				 //DBG("-1\n");
			}
        //-------起动调节时间----------------------------//
		if(SpeakerWaterLevelSpeed < SPEAKER_SRA_SPEED)
		{
			n_inc_dec_o = 0;
			WaterLevelCount = 0;
		}
		else//调节周期，影响了频响，速度慢频响好，速度快频响变化大
		{
			WaterLevelCount++;
			WaterLevelCount &= 0x0f;///128*16ms
			if(WaterLevelCount)
			{
				n_inc_dec_o = 0;
			}
		}

      //------------------------------------------------------//
		MCUCircular_GetData(&UsbAudioSpeaker.CircularBuf,(uint8_t *)UsbSraInBuf,SRA_BLOCK*4); //

		if(sra_apply(&UsbSpeakerSraObj, (int16_t *)UsbSraInBuf, (int16_t *)UsbSraOutBuf, n_inc_dec_o)==SRA_ERROR_OK)//
		{
			SRCDoneLen = SRA_BLOCK + n_inc_dec_o;
		}
		else
		{
			SRCDoneLen = SRA_BLOCK;
			memcpy(UsbSraOutBuf,(uint8_t *)&UsbSraInBuf,SRCDoneLen*4);
		}
//------------Put Data---------------------------------------------------------------------------------//
		MCUCircular_PutData(&UsbSpeakerBuf, (uint8_t *)&UsbSraInBuf, SRCDoneLen*4);
     }
}
/*
****************************************************************
*
*usb audio 状态检测
*
****************************************************************
*/
void UsbAudioTimer1msProcess(void)
{
	if(SpeakerWaterLevelSpeed < SPEAKER_SRA_SPEED)
		{
		  SpeakerWaterLevelSpeed++;
		}

	if(MicWaterLevelSpeed < USB_MIC_SRA_SPEED)
		{
		  MicWaterLevelSpeed++;
		}

	if(IsUsbAudioMode == FALSE)
	{
		return;
	}
	if(!AudioCoreServiceStatusGet())
	{
		AudioCoreSourceDisable(USB_MIX_SOURCE_NUM);
		AudioCoreSinkDisable(USB_AUDIO_SINK_NUM);
		SpeakerWaterLevelSpeed = 0;
		MicWaterLevelSpeed = 0;
		return ;
	}

	FramCount++;
	if(FramCount%2)//2ms
	{
		return;
	}
#ifdef CFG_RES_AUDIO_USB_IN_EN
	if(UsbAudioSpeaker.AltSet)//open stream
	{
		if(UsbAudioSpeaker.FramCount)//正在传数据 1-2帧数据
		{
			if(UsbAudioSpeaker.FramCount != temp_speaker_fram)
			{
				temp_speaker_fram = UsbAudioSpeaker.FramCount;
				if(AudioCore.AudioSource[USB_MIX_SOURCE_NUM].Enable == FALSE)
				{
				  AudioCoreSourceEnable(USB_MIX_SOURCE_NUM);
				}
			}
			else
			{
				AudioCoreSourceDisable(USB_MIX_SOURCE_NUM);
			}
		}
	}
	else
	{
		UsbAudioSpeaker.FramCount = 0;
		temp_speaker_fram = 0;
		AudioCoreSourceDisable(USB_MIX_SOURCE_NUM);
	}
#endif

#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(UsbAudioMic.AltSet)//open stream
	{
		if(UsbAudioMic.FramCount)//正在传数据 切传输了1-2帧数据
		{
			if(UsbAudioMic.FramCount != temp_mic_fram)
			{
				temp_mic_fram = UsbAudioMic.FramCount;
				if(AudioCore.AudioSink[USB_AUDIO_SINK_NUM].Enable == FALSE)
				{
					AudioCoreSinkEnable(USB_AUDIO_SINK_NUM);
				}
			}
		}
	}
	else
	{
		UsbAudioMic.FramCount = 0;
		temp_mic_fram = 0;
		AudioCoreSinkDisable(USB_AUDIO_SINK_NUM);
	}
#endif
}
#endif //end of CFG_APP_USB_AUDIO_MODE_EN

