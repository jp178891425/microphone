/*
 * spdif_mix_api.c
 *
 *  Created on: Mar 1, 2022
 *      Author: szsj-1
 */
#include <string.h>
#include "type.h"
#include "app_config.h"

#ifdef CFG_FUNC_SPDIF_MIX_MODE
//-------------------------------//
#include "timeout.h"
#include "app_message.h"
#include "debug.h"
#include "dma.h"
#include "clk.h"
#include "main_task.h"
#include "timer.h"
#include "watchdog.h"
#include "audio_adc.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "ctrlvars.h"
#include "delay.h"
#include "sra.h"
#include "spdif.h"
#include "mcu_circular_buf.h"
#include "resampler_polyphase.h"
#include "rtos_api.h"
#include "resampler_farrow.h"
#include "resampler.h"
//-----------------------------------//
#define CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN

#define SPDIF_RESAMPLER_OUT_LEN			(128 * 2 * 2 * 4)
#define PCM_REMAIN_SMAPLES				3//spdif数据丢声道时,返回数据会超过一帧128，用于缓存数据，试听感更加好
//recv, dma buf len,MAX_FRAME_SAMPLES * 2 * 2 * 2 * 2是基础，OS切换间隔实测需要加倍。
#define	SPDIF_FIFO_LEN					(128 * 2 * 2 * 2 * 2 * 2)
//由于采样值从32bit转换为16bit，可以使用同一个buf，否则要独立申请
#define SPDIF_CARRY_LEN					(128 * 2 * 2 * 2 + PCM_REMAIN_SMAPLES * 2 * 4)//支持192000输入 buf len for get data form dma fifo, deal + 6samples 用于缓存偶尔多余的数据，#9817
//转采样输出buf,如果spdif转采样提升大于四倍需要加大此SPDIF_CARRY_LEN，比如输入8000以下转48000,需要缩小单次carry帧大小或调大SPDIF_RESAMPLER_OUT_LEN、SPDIF_PCM_FIFO_LEN
#define SPDIF_RESAMPLER_OUT_LEN			(128 * 2 * 2 * 4)

#define SPDIF_MIX_SOURCE_SOFT_ADJUST_STEP 64
#define HighLevel   (mainAppCt.SamplesPreFrame * 2 * 2 * 2 * 2 / 4 - mainAppCt.SamplesPreFrame) * 3 / 4
#define LowLevel    (mainAppCt.SamplesPreFrame * 2 * 2 * 2 * 2 / 4 - mainAppCt.SamplesPreFrame) / 4
//------------------------------------//
typedef struct _SpdifPlayContext
{
	uint32_t			*SpdifPwcFIFO;		//
	int16_t 			*Source1Buf_Spdif;//
	uint32_t            *SpdifCarry;
	uint32_t			*SpdifPcmFifo;
	MCU_CIRCULAR_CONTEXT SpdifPcmCircularBuf;

	AudioCoreContext 	*AudioCoreSpdif;

	//play
	uint32_t 			SampleRate;

	ResamplerPolyphaseContext *ResamplerCt;
	uint32_t*			resampleOutBuf;

	uint32_t			SpdifSampleRate;

	uint32_t 			SpdifDmaWritePtr;
	uint32_t 			SpdifDmaReadPtr;
	uint32_t 			SpdifPreSample;
	uint8_t  			SpdifSampleRateCheckFlg;
	uint32_t 			SpdifSampleRateFromSW;

#ifdef CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
	int16_t 			pcmRemain[PCM_REMAIN_SMAPLES * 4];//用于缓存底层给上来超过128samples的数据
	int16_t 			pcmRemainLen;
	int16_t             *SraPcmOutnBuf;
	ResamplerFarrowContext 	SraResFarCt;//软件微调pcm采样点数，结构体。
	int32_t             SraDecIncNum;
#endif
}SpdifPlayContext;
//----------------------------------------//
static  SpdifPlayContext*		SpdifPlayCt = 0;
static uint16_t SpdifLockFlag = FALSE;
static uint32_t MixSamplerate=0;
static uint16_t AdjustLevelHigh	= 0;	//(CFG_PARA_SAMPLES_PER_FRAME * 3 / 4); //混音前buf两帧大小，获取时机是抽走一帧后。
static uint16_t AdjustLevelLow	= 0;	//(CFG_PARA_SAMPLES_PER_FRAME / 4);

static void SpdifSampleRateChange(void);
uint16_t AudioSpdif_DataGetX(void* Buf, uint16_t Len);
uint16_t AudioSpdif_DataLenGetX(void);
int32_t Get_Resampler_Polyphase(uint32_t resampler);
void SpdifDataCarry(void);
void AudioSpdif_SRAInit(void);
uint16_t AudioSpdif_SRAProcess(int16_t *InBuf, uint16_t InLen);
void AudioSpdif_SRAStepCnt(void);
uint8_t GetSysResMallocStatus(void);
//-------------------------------------------//
void AudioSpdif_ResMalloc(uint16_t SampleLen)
{
	DBG("%s\n",__func__);

	uint16_t FifoLenStereo = SampleLen * 2 * 2 * 2 * 2;//立体声8倍大小于帧长，单位byte
	uint16_t AudioCoreBufLen = SampleLen * 2 * 2;//AudioCore接口均按立体声来处理

	SpdifPlayCt = (SpdifPlayContext*)osPortMallocFromEnd(sizeof(SpdifPlayContext));
	if(SpdifPlayCt == NULL)
	{
		DBG("spdif res err\n");
		return;
	}
	memset(SpdifPlayCt, 0, sizeof(SpdifPlayContext));
	SpdifPlayCt->SpdifDmaWritePtr 		 = 0;
	SpdifPlayCt->SpdifDmaReadPtr  		 = 0;
	SpdifPlayCt->SpdifSampleRateCheckFlg = 0;
	SpdifPlayCt->SpdifSampleRateFromSW 	 = 0;
	SpdifPlayCt->SpdifPreSample			 = 0;

	//
	SpdifPlayCt->SampleRate = 0;//CFG_PARA_SAMPLE_RATE;

	//
	SpdifPlayCt->SpdifPwcFIFO = (uint32_t*)osPortMallocFromEnd(SPDIF_FIFO_LEN);
	if(SpdifPlayCt->SpdifPwcFIFO == NULL)
	{
		DBG("spdif res err\n");
		return;
	}
	memset(SpdifPlayCt->SpdifPwcFIFO, 0, SPDIF_FIFO_LEN);

	SpdifPlayCt->SpdifCarry = (uint32_t *)osPortMallocFromEnd(SPDIF_CARRY_LEN);
	if(SpdifPlayCt->SpdifCarry == NULL)
	{
		DBG("spdif res err\n");
		return;
	}
	memset(SpdifPlayCt->SpdifCarry, 0, SPDIF_CARRY_LEN);
#ifdef CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
	SpdifPlayCt->SraPcmOutnBuf = (int16_t *)osPortMallocFromEnd(128*8);
	if(SpdifPlayCt->SraPcmOutnBuf == NULL)
	{
		DBG("spdif res err\n");
		return;
	}
	memset(SpdifPlayCt->SraPcmOutnBuf, 0, 128*8);
#endif
	//Spdif数据Rx是32bit存储，取出过程帧空间需放大到帧* 8。
	SpdifPlayCt->Source1Buf_Spdif = (int16_t*)osPortMallocFromEnd(AudioCoreBufLen);//stereo
	if(SpdifPlayCt->Source1Buf_Spdif == NULL)
	{
		return ;
	}
	memset(SpdifPlayCt->Source1Buf_Spdif, 0, AudioCoreBufLen);

	SpdifPlayCt->SpdifPcmFifo = (uint32_t *)osPortMallocFromEnd(FifoLenStereo);
	if(SpdifPlayCt->SpdifPcmFifo == NULL)
	{
		return ;
	}
	MCUCircular_Config(&SpdifPlayCt->SpdifPcmCircularBuf, SpdifPlayCt->SpdifPcmFifo, FifoLenStereo);
	memset(SpdifPlayCt->SpdifPcmFifo, 0, FifoLenStereo);


	SpdifPlayCt->ResamplerCt = (ResamplerPolyphaseContext *)osPortMalloc(sizeof(ResamplerPolyphaseContext));
	if(SpdifPlayCt->ResamplerCt == NULL)
	{
		return;
	}

	SpdifPlayCt->resampleOutBuf = (uint32_t *)osPortMalloc(SPDIF_RESAMPLER_OUT_LEN);
	if(SpdifPlayCt->resampleOutBuf == NULL)
	{
		return;
	}

	AudioSpdif_SRAInit();

}

void AudioSpdif_Release(void)
{
	DBG("%s\n",__func__);

	SpdifLockFlag = 0;

	MixSamplerate = 0;
	AudioCoreSourceMute(SPDIF_MIX_SOURCE_NUM, TRUE, TRUE);
	AudioCoreSourceDisable(SPDIF_MIX_SOURCE_NUM);

	SPDIF_ModuleDisable();

	if(SpdifPlayCt==NULL)
	{
		return;
	}
	if(SpdifPlayCt->SpdifCarry != NULL)
	{
		osPortFree(SpdifPlayCt->SpdifCarry);
		SpdifPlayCt->SpdifCarry = NULL;
	}


	if(SpdifPlayCt->SpdifPwcFIFO != NULL)
	{
		osPortFree(SpdifPlayCt->SpdifPwcFIFO);
		SpdifPlayCt->SpdifPwcFIFO = NULL;
	}

	if(SpdifPlayCt->Source1Buf_Spdif != NULL)
	{
		APP_DBG("Source1Buf_Spdif\n");
		osPortFree(SpdifPlayCt->Source1Buf_Spdif);
		SpdifPlayCt->Source1Buf_Spdif = NULL;
	}

	if(SpdifPlayCt->SpdifPcmFifo != NULL)
	{
		APP_DBG("SpdifPcmFifo\n");
		osPortFree(SpdifPlayCt->SpdifPcmFifo);
		SpdifPlayCt->SpdifPcmFifo = NULL;
	}
	if(SpdifPlayCt->ResamplerCt != NULL)
	{
		APP_DBG("SpdifPlayCt->ResamplerCt\n");
		osPortFree(SpdifPlayCt->ResamplerCt);
		SpdifPlayCt->ResamplerCt = NULL;
	}

	if(SpdifPlayCt->resampleOutBuf != NULL)
	{
		APP_DBG("SpdifPlayCt->resampleOutBuf\n");
		osPortFree(SpdifPlayCt->resampleOutBuf);
		SpdifPlayCt->resampleOutBuf = NULL;
	}
#ifdef CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
	if(SpdifPlayCt->SraPcmOutnBuf != NULL)
	{
		APP_DBG("SpdifPlayCt->SraPcmOutnBuf\n");
		osPortFree(SpdifPlayCt->SraPcmOutnBuf);
		SpdifPlayCt->SraPcmOutnBuf = NULL;
	}
#endif
	if(SpdifPlayCt != NULL)
	{
		APP_DBG("SpdifPlayCt\n");
		osPortFree(SpdifPlayCt);
		SpdifPlayCt = NULL;
	}

}

void AudioSpdif_HWInit(void)
{
	uint8_t ch;
	//将SPDIF时钟切换到AUPLL
	Clock_SpdifClkSelect(APLL_CLK_MODE);

	GPIO_PortAModeSet(SPDIF_MIX_COAXIAL_INDEX, SPDIF_MIX_COAXIAL_PORT_MODE);
	SPDIF_AnalogModuleEnable(SPDIF_MIX_COAXIAL_PORT_ANA_INPUT, SPDIF_ANA_LEVEL_300mVpp);

	SPDIF_ModuleDisable();
	DMA_ChannelDisable(PERIPHERAL_ID_SPDIF_RX);
	//Reset_FunctionReset(DMAC_FUNC_SEPA);
	DMA_InterruptFlagClear(PERIPHERAL_ID_SPDIF_RX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_SPDIF_RX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_SPDIF_RX, DMA_ERROR_INT);

	SPDIF_RXInit(1, 1, 0);
	//使用
	DMA_CircularConfig(PERIPHERAL_ID_SPDIF_RX, SPDIF_FIFO_LEN / 2, (void*)SpdifPlayCt->SpdifPwcFIFO, SPDIF_FIFO_LEN);
	DMA_ChannelEnable(PERIPHERAL_ID_SPDIF_RX);
	SPDIF_ModuleEnable();

	ch = DMA_ChannelNumGet(PERIPHERAL_ID_SPDIF_RX);

	if(ch > 7 )
	{
		DBG("AudioSpdif DMA NOT OPEN!\n");
	}
	else
	{
		DBG("AudioSpdif DMA SET OK!\n");
	}

	SpdifLockFlag = 0;

	MixSamplerate = 0;
}

void AudioSpdif_ConfigInit(void)
{
	DBG("%s\n",__func__);
	mainAppCt.AudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].FuncDataGet = AudioSpdif_DataGetX;
	mainAppCt.AudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].FuncDataGetLen = AudioSpdif_DataLenGetX;
	mainAppCt.AudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].IsSreamData = 0;
	mainAppCt.AudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].PcmFormat = 2;//test mic audio effect
	mainAppCt.AudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].PcmInBuf = (int16_t *)SpdifPlayCt->Source1Buf_Spdif;

	AudioSpdif_HWInit();
}

/*
****************************************************************
*
*采样率转换   + 采样率微调 1ms执行一次
*
****************************************************************
*/
void AudioSpdif_DataInProcess(void)
{
	//---------------------//
    #define SPDIF_MUTE_DELAY 200
	if(GetSysResMallocStatus()) return;

	if(SpdifLockFlag)
	{
		if(SpdifLockFlag<SPDIF_MUTE_DELAY)
		{
			SpdifLockFlag++;
			if(SpdifLockFlag==SPDIF_MUTE_DELAY)
			{
				AudioCoreSourceUnmute(SPDIF_MIX_SOURCE_NUM, TRUE, TRUE);
				SpdifLockFlag++;
			}
		}
	}
	//----------------------------------//
	if(SpdifLockFlag && !SPDIF_FlagStatusGet(LOCK_FLAG_STATUS))
	{
		APP_DBG("SPDIF RX UNLOCK!\n");
		SpdifLockFlag = 0;
		AudioCoreSourceDisable(SPDIF_MIX_SOURCE_NUM);
		AudioCoreSourceMute(SPDIF_MIX_SOURCE_NUM, TRUE, TRUE);
	}

	if(!SpdifLockFlag && SPDIF_FlagStatusGet(LOCK_FLAG_STATUS))
	{
		APP_DBG("SPDIF RX LOCK!\n");
		SpdifLockFlag = 1;
		MixSamplerate = 0;
		AudioCoreSourceEnable(SPDIF_MIX_SOURCE_NUM);

	}

	//监控SPDIF RX采样率是否改变
	if(SpdifLockFlag)
	{
		if(MixSamplerate != SPDIF_SampleRateGet())
		{
			MixSamplerate = SPDIF_SampleRateGet();

			SpdifPlayCt->SpdifSampleRate = MixSamplerate;
			APP_DBG("Get SampleRate: %ld\n", SpdifPlayCt->SpdifSampleRate);
			SpdifSampleRateChange();
		}
		if(MixSamplerate)
		{
			SpdifDataCarry();
			AudioSpdif_SRAStepCnt();
		}
	}
}

static void SpdifSampleRateChange(void)
{
   resampler_polyphase_init(SpdifPlayCt->ResamplerCt, 2, Get_Resampler_Polyphase(SpdifPlayCt->SpdifSampleRate));
}

void SpdifDataCarry(void)
{
	int16_t pcm_space;
	uint16_t spdif_len;
	int16_t pcm_len;
	int16_t *pcmBuf  = (int16_t *)SpdifPlayCt->SpdifCarry;
	uint16_t cnt,SRADoneaLen;

	spdif_len = DMA_CircularDataLenGet(PERIPHERAL_ID_SPDIF_RX);
	pcm_space = MCUCircular_GetSpaceLen(&SpdifPlayCt->SpdifPcmCircularBuf) - 16;

	if(pcm_space < 16)
	{
		return;
	}
#if 1//def CFG_FUNC_MIXER_SRC_EN
	pcm_space = (pcm_space * SpdifPlayCt->SpdifSampleRate) / CFG_PARA_SAMPLE_RATE - 16;
#endif

	if((spdif_len >> 1) > pcm_space)
	{
		spdif_len = pcm_space * 2;
	}

	spdif_len = spdif_len & 0xFFF8;
	if(!spdif_len)
	{
		return ;
	}
	cnt = (spdif_len / 8) / (MAX_FRAME_SAMPLES);

	while(cnt--)
	{
#ifdef	CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
		DMA_CircularDataGet(PERIPHERAL_ID_SPDIF_RX, pcmBuf, (MAX_FRAME_SAMPLES - SpdifPlayCt->pcmRemainLen) * 8);
		//由于从32bit转换为16bit，buf可以使用同一个，否则要独立申请。
		pcm_len = SPDIF_SPDIFDataToPCMData((int32_t *)pcmBuf, (MAX_FRAME_SAMPLES - SpdifPlayCt->pcmRemainLen) * 8, (int32_t *)pcmBuf, SPDIF_WORDLTH_16BIT);
#endif

		if(pcm_len < 0)
		{
			return;
		}

#ifdef	CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
		if(SpdifPlayCt->pcmRemainLen > 0)
		{
			//DBG("!!pcm_len = %d\n", pcm_len);
			if(pcm_len/4 >  MAX_FRAME_SAMPLES + PCM_REMAIN_SMAPLES)
			{
				pcm_len = MAX_FRAME_SAMPLES * 4 + PCM_REMAIN_SMAPLES * 4;
			}
			memcpy(&pcmBuf[MAX_FRAME_SAMPLES*2 + PCM_REMAIN_SMAPLES * 2], pcmBuf, pcm_len);
			memcpy(pcmBuf, SpdifPlayCt->pcmRemain, SpdifPlayCt->pcmRemainLen * 4);
			memcpy(&pcmBuf[SpdifPlayCt->pcmRemainLen * 2], &pcmBuf[MAX_FRAME_SAMPLES * 2 + PCM_REMAIN_SMAPLES * 2], pcm_len);
			pcm_len += SpdifPlayCt->pcmRemainLen * 4;
			SpdifPlayCt->pcmRemainLen = 0;
		}
		if((pcm_len / 4) > MAX_FRAME_SAMPLES)
		{
			//DBG("&&pcm_len = %d\n", pcm_len);
			SpdifPlayCt->pcmRemainLen = pcm_len / 4 - MAX_FRAME_SAMPLES;
			if(SpdifPlayCt->pcmRemainLen > PCM_REMAIN_SMAPLES)
			{
				SpdifPlayCt->pcmRemainLen = PCM_REMAIN_SMAPLES;
			}
			memcpy(SpdifPlayCt->pcmRemain, &pcmBuf[MAX_FRAME_SAMPLES * 2], SpdifPlayCt->pcmRemainLen * 4);
			pcm_len = MAX_FRAME_SAMPLES * 4;
		}
//		if(SoftFlagGet(SoftFlagBtSra))
//		{
			SRADoneaLen = AudioSpdif_SRAProcess((int16_t*)pcmBuf, MAX_FRAME_SAMPLES);
//		}

		if(SpdifPlayCt->SpdifSampleRate != CFG_PARA_SAMPLE_RATE)
		{
			pcm_len = resampler_polyphase_apply(SpdifPlayCt->ResamplerCt, SpdifPlayCt->SraPcmOutnBuf, (int16_t*)SpdifPlayCt->resampleOutBuf, SRADoneaLen);
			if(pcm_len > 0)
			{
				MCUCircular_PutData(&SpdifPlayCt->SpdifPcmCircularBuf, SpdifPlayCt->resampleOutBuf, pcm_len * 4);
			}
		}
		else
		{
			MCUCircular_PutData(&SpdifPlayCt->SpdifPcmCircularBuf, SpdifPlayCt->SraPcmOutnBuf, SRADoneaLen * 4);
		}
#endif
	}
}
void AudioSpdif_SRAInit(void)
{
#ifdef CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
	resampler_farrow_init(&SpdifPlayCt->SraResFarCt, 2, 1);
	AdjustLevelLow = LowLevel;
	AdjustLevelHigh = HighLevel;
#endif

}

uint16_t AudioSpdif_SRAProcess(int16_t *InBuf, uint16_t InLen)
{
	int16_t    AdjustVal=0;
#ifdef CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
	AdjustVal = SpdifPlayCt->SraDecIncNum;
	SpdifPlayCt->SraDecIncNum     = 0;

	//if(sra_apply(&audioSourceAdjustCt->SraObj, (int16_t *)InBuf, (int16_t *)audioSourceAdjustCt->SraPcmOutnBuf, AdjustVal) != SRA_ERROR_OK)
	if(resampler_farrow_apply(&SpdifPlayCt->SraResFarCt, (int16_t *)InBuf, (int16_t *)SpdifPlayCt->SraPcmOutnBuf, 128, 128 + AdjustVal) != RESAMPLER_FARROW_ERROR_OK)
	{
		return 0;
	}

	return InLen + AdjustVal;
#else
	return AdjustVal;
#endif
}

void AudioSpdif_SRAStepCnt(void)
{
#ifdef CFG_FUNC_SPDIF_MIX_SOFT_ADJUST_IN
	static uint32_t i = 0;
	static int8_t  AdjustFlag = 0;//方向
	static int32_t	Samples = 0;

	if(AudioCore.AudioSource[SPDIF_MIX_SOURCE_NUM].Enable)
	{
		{
			Samples += AudioCore.AudioSource[SPDIF_MIX_SOURCE_NUM].FuncDataGetLen();
		}
		if(i >= SPDIF_MIX_SOURCE_SOFT_ADJUST_STEP)
		{
			i = 0;
			Samples /= SPDIF_MIX_SOURCE_SOFT_ADJUST_STEP;//GetValidSbcDataSize();
			//APP_DBG("Samples = %d\n", (int)Samples);

			if(AdjustFlag == 0)
			{
				if(Samples < AdjustLevelLow)
				{
					AdjustFlag = 1;//正
				}
				else if(Samples > AdjustLevelHigh)
				{
					AdjustFlag = -1;//负
				}
				else
				{
					AdjustFlag = 0;
				}
			}
			else if(AdjustFlag == 1)
			{
				if(Samples < AdjustLevelLow)
				{
					//插点
					SpdifPlayCt->SraDecIncNum = 1;
					//APP_DBG("!>!");
				}
				else
				{
					AdjustFlag = 0;
				}
			}
			else if(AdjustFlag == -1)
			{
				if(Samples > AdjustLevelHigh)
				{
					//丢点
					SpdifPlayCt->SraDecIncNum = -1;
					//APP_DBG("!<!");
				}
				else
				{
					AdjustFlag = 0;
				}
			}
			Samples = 0;
		}
		i++;
	}
#endif
}


/*
****************************************************************
*
*AudioSpdif get  data
*
****************************************************************
*/
uint16_t AudioSpdif_DataGetX(void* Buf, uint16_t Len)
{

	return MCUCircular_GetData(&SpdifPlayCt->SpdifPcmCircularBuf, Buf, Len * 4) / 4;

}
uint16_t AudioSpdif_DataLenGetX(void)
{

	return MCUCircular_GetDataLen(&SpdifPlayCt->SpdifPcmCircularBuf)/4;

}
//-----------------------------------//
#endif
