/**
 **************************************************************************************
 * @file    i2s_api.c
 * @brief    
 *
 * @author  
 * @version  
 *
 * $Created:  
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include "type.h"
#include "app_config.h"
#include "timeout.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "dma.h"
#include "clk.h"
#include "main_task.h"
#include "timer.h"
#include "irqn.h"
#include "watchdog.h"
#include "dac.h"
#include "dac_interface.h"
#include "audio_adc.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "ctrlvars.h"
#include "delay.h"
#include "i2s.h"
#include "i2s_interface.h"
#include "i2s_api.h"
#include "sra.h"

#ifdef CFG_FUNC_I2S_MIX_MODE


uint32_t    RESAMPLE_FIFO_LEN = CFG_PARA_MAX_SAMPLES_PER_FRAME*4*2;

extern const uint32_t SupportSampleRateList[13];
#ifdef CFG_RES_I2S0_EN
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)
MODULE_SRC_SRA SRC_SRA_I2S0_IN;
#endif

#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)
MODULE_SRC_SRA SRC_SRA_I2S0_OUT;
#endif

#endif //

#ifdef CFG_RES_I2S1_EN

#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)
MODULE_SRC_SRA SRC_SRA_I2S1_IN;
#endif

#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)
MODULE_SRC_SRA SRC_SRA_I2S1_OUT;
#endif

#endif //

void AudioI2S_ResMalloc(uint16_t SampleLen)
{
	DBG("%s\n",__func__);
	uint16_t FifoLenStereo = SampleLen * 2 * 2 * 2;//立体声8倍大小于帧长，单位byte
	uint16_t AudioCoreBufLen = SampleLen * 2 * 2;//AudioCore接口均按立体声来处理

	//RESAMPLE_FIFO_LEN        = FifoLenStereo;
//-----------------------------//
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)
	MODULE_SRC_SRA *i2s0_in;
	i2s0_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_IN;
#endif
#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)
	MODULE_SRC_SRA *i2s0_out;
	i2s0_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_OUT;
#endif
#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)
	MODULE_SRC_SRA *i2s1_in;
	i2s1_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_IN;
#endif

#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)
	MODULE_SRC_SRA *i2s1_out;
	i2s1_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_OUT;
#endif
//------------------------------------//
#ifdef CFG_RES_I2S0_EN
	I2S_ModuleDisable(I2S0_MODULE);
#endif

#ifdef CFG_RES_I2S1_EN
	I2S_ModuleDisable(I2S1_MODULE);
#endif

#ifdef CFG_RES_AUDIO_I2S0IN_EN
	if(mainAppCt.Source2Buf_I2S0==NULL)
	{
	  mainAppCt.Source2Buf_I2S0 = (int16_t*)osPortMallocFromEnd(AudioCoreBufLen);// buff
	}

	if(mainAppCt.Source2Buf_I2S0 != NULL)
	{
		memset(mainAppCt.Source2Buf_I2S0, 0, AudioCoreBufLen);
	}
	else
	{
		DBG("Source2Buf_I2S0 error\n");
		return;
	}
	if(mainAppCt.I2S0_IN_FIFO==NULL)
	{
	  mainAppCt.I2S0_IN_FIFO = (int16_t*)osPortMallocFromEnd(FifoLenStereo);// fifo
	}

	if(mainAppCt.I2S0_IN_FIFO != NULL)
	{
		memset(mainAppCt.I2S0_IN_FIFO, 0, FifoLenStereo);
	}
	else
	{
		DBG("I2S0_IN_FIFO error\n");
		return ;
	}
//----------------------------------------//
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)

	if(i2s0_in->OutSrcFifo==NULL)
	{
		i2s0_in->OutSrcFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
	}

	if(i2s0_in->OutSrcFifo != NULL)
	{
		memset(i2s0_in->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
	}
	else
	{
		DBG("i2s0_in->OutSrcFifo error\n");
		return ;
	}
	if(i2s0_in->OutSrcBuff==NULL)
	{
		i2s0_in->OutSrcBuff = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
	}

	if(i2s0_in->OutSrcBuff != NULL)
	{
		memset(i2s0_in->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
	}
	else
	{
		DBG("i2s0_in->OutSrcBuff error\n");
		return ;
	}
#endif

#if defined(CFG_FUNC_I2S0IN_SRA_EN)
	if(i2s0_in->OutSraFifo==NULL)
	{
		i2s0_in->OutSraFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
	}

	if(i2s0_in->OutSraFifo != NULL)
	{
		memset(i2s0_in->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
	}
	else
	{
		DBG("i2s0_in->OutSraFifo error\n");
		return ;
	}
#endif
	//-------------------------------//
	#ifdef CFG_FUNC_I2S0IN_SRC_EN
    #ifdef I2S_SRC_1_EN
	SRC_SRA_I2S0_IN.ResampleObj = (ResamplerContext *)osPortMallocFromEnd(sizeof(ResamplerContext));// SRC
	if(SRC_SRA_I2S0_IN.ResampleObj != NULL)
	{
		memset(SRC_SRA_I2S0_IN.ResampleObj, 0, sizeof(ResamplerContext));
	}
	else
	{
		DBG("I2S0IN SRC error\n");
		return ;
	}
    #else
	SRC_SRA_I2S0_IN.ResampleObj = (ResamplerPolyphaseContext *)osPortMallocFromEnd(sizeof(ResamplerPolyphaseContext));// SRC
	if(SRC_SRA_I2S0_IN.ResampleObj != NULL)
	{
		memset(SRC_SRA_I2S0_IN.ResampleObj, 0, sizeof(ResamplerPolyphaseContext));
	}
	else
	{
		DBG("I2S0IN SRC error\n");
		return ;
	}
    #endif

	#endif
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	if(mainAppCt.Source2Buf_I2S1==NULL)
	{
	   mainAppCt.Source2Buf_I2S1 = (int16_t *)osPortMallocFromEnd(AudioCoreBufLen);//buff
	}

	if(mainAppCt.Source2Buf_I2S1 != NULL)
	{
		memset(mainAppCt.Source2Buf_I2S1, 0, AudioCoreBufLen);
	}
	else
	{
		DBG("Source2Buf_I2S1 error\n");
		return;
	}

	if(mainAppCt.I2S1_IN_FIFO==NULL)
	{
		mainAppCt.I2S1_IN_FIFO = (int16_t *)osPortMallocFromEnd(FifoLenStereo);// fifo
	}

	if(mainAppCt.I2S1_IN_FIFO != NULL)
	{
		memset(mainAppCt.I2S1_IN_FIFO, 0, FifoLenStereo);
	}
	else
	{
		DBG("I2S1_IN_FIFO error\n");
		return ;
	}
	//----------------------------------------//
	#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)

		if(i2s1_in->OutSrcFifo==NULL)
		{
			i2s1_in->OutSrcFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
		}

		if(i2s1_in->OutSrcFifo != NULL)
		{
			memset(i2s1_in->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
		}
		else
		{
			DBG("i2s1_in->OutSrcFifo error\n");
			return ;
		}
		if(i2s1_in->OutSrcBuff==NULL)
		{
			i2s1_in->OutSrcBuff = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
		}

		if(i2s1_in->OutSrcBuff != NULL)
		{
			memset(i2s1_in->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
		}
		else
		{
			DBG("i2s1_in->OutSrcBuff error\n");
			return ;
		}
	#endif

	#if defined(CFG_FUNC_I2S1IN_SRA_EN)
		if(i2s1_in->OutSraFifo==NULL)
		{
			i2s1_in->OutSraFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
		}

		if(i2s1_in->OutSraFifo != NULL)
		{
			memset(i2s1_in->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
		}
		else
		{
			DBG("i2s1_in->OutSraFifo error\n");
			return ;
		}
	#endif
		//-------------------------------//
	#ifdef CFG_FUNC_I2S1IN_SRC_EN
    #ifdef I2S_SRC_1_EN
	SRC_SRA_I2S1_IN.ResampleObj = (ResamplerContext *)osPortMallocFromEnd(sizeof(ResamplerContext));// SRC
	if(SRC_SRA_I2S1_IN.ResampleObj != NULL)
	{
		memset(SRC_SRA_I2S1_IN.ResampleObj, 0, sizeof(ResamplerContext));
	}
	else
	{
		DBG("I2S1IN SRC error\n");
		return ;
	}
    #else
	SRC_SRA_I2S1_IN.ResampleObj = (ResamplerPolyphaseContext *)osPortMallocFromEnd(sizeof(ResamplerPolyphaseContext));// SRC
	if(SRC_SRA_I2S1_IN.ResampleObj != NULL)
	{
		memset(SRC_SRA_I2S1_IN.ResampleObj, 0, sizeof(ResamplerPolyphaseContext));
	}
	else
	{
		DBG("I2S1IN SRC error\n");
		return ;
	}
    #endif
	#endif
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	if(mainAppCt.Sink2Buf_I2S0==NULL)
	{
		mainAppCt.Sink2Buf_I2S0 = (int16_t*)osPortMallocFromEnd(AudioCoreBufLen);// buff
	}

	if(mainAppCt.Sink2Buf_I2S0 != NULL)
	{
		memset(mainAppCt.Sink2Buf_I2S0, 0, AudioCoreBufLen);
	}
	else
	{
		DBG("Sink2Buf_I2S0 error\n");
		return ;
	}
	if(mainAppCt.I2S0_OUT_FIFO==NULL)
	{
		mainAppCt.I2S0_OUT_FIFO = (int16_t*)osPortMallocFromEnd(FifoLenStereo);// fifo
	}

	if(mainAppCt.I2S0_OUT_FIFO != NULL)
	{
		memset(mainAppCt.I2S0_OUT_FIFO, 0, FifoLenStereo);
	}
	else
	{
		DBG("I2S0_OUT_FIFO error\n");
		return ;
	}
//------------------------------------------//
#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)

	if(i2s0_out->OutSrcFifo==NULL)
	{
		i2s0_out->OutSrcFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
	}

	if(i2s0_out->OutSrcFifo != NULL)
	{
		memset(i2s0_out->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
	}
	else
	{
		DBG("i2s0_out->OutSrcFifo error\n");
		return ;
	}
	if(i2s0_out->OutSrcBuff==NULL)
	{
		i2s0_out->OutSrcBuff = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
	}

	if(i2s0_out->OutSrcBuff != NULL)
	{
		memset(i2s0_out->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
	}
	else
	{
		DBG("i2s0_out->OutSrcBuff error\n");
		return ;
	}
#endif

#if defined(CFG_FUNC_I2S0OUT_SRA_EN)
	if(i2s0_out->OutSraFifo==NULL)
	{
		i2s0_out->OutSraFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
	}

	if(i2s0_out->OutSraFifo != NULL)
	{
		memset(i2s0_out->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
	}
	else
	{
		DBG("i2s0_out->OutSraFifo error\n");
		return ;
	}
#endif
//------------------------//
	#ifdef CFG_FUNC_I2S0OUT_SRC_EN
    #ifdef I2S_SRC_1_EN
	i2s0_out->ResampleObj = (ResamplerContext *)osPortMallocFromEnd(sizeof(ResamplerContext));// SRC
	if(i2s0_out->ResampleObj != NULL)
	{
		memset(i2s0_out->ResampleObj, 0, sizeof(ResamplerContext));
	}
	else
	{
		DBG("I2S0OUT SRC error\n");
		return ;
	}
    #else
	i2s0_out->ResampleObj = (ResamplerPolyphaseContext *)osPortMallocFromEnd(sizeof(ResamplerPolyphaseContext));// SRC
	if(i2s0_out->ResampleObj != NULL)
	{
		memset(i2s0_out->ResampleObj, 0, sizeof(ResamplerPolyphaseContext));
	}
	else
	{
		DBG("I2S0OUT SRC error\n");
		return ;
	}
    #endif
	#endif
#endif

#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	mainAppCt.Sink2Buf_I2S1 = (int16_t *)osPortMallocFromEnd(AudioCoreBufLen);// buff

	if(mainAppCt.Sink2Buf_I2S1 != NULL)
	{
		memset(mainAppCt.Sink2Buf_I2S1, 0, AudioCoreBufLen);
	}
	else
	{
		DBG("Sink2Buf_I2S1 error\n");
		return;
	}
	mainAppCt.I2S1_OUT_FIFO = (int16_t *)osPortMallocFromEnd(FifoLenStereo);// fifo

	if(mainAppCt.I2S1_OUT_FIFO != NULL)
	{
		memset(mainAppCt.I2S1_OUT_FIFO, 0, FifoLenStereo);
	}
	else
	{
		DBG("I2S1_OUT_FIFO error\n");
		return;
	}
	//------------------------------------------//
	#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)

		if(i2s1_out->OutSrcFifo==NULL)
		{
			i2s1_out->OutSrcFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
		}

		if(i2s1_out->OutSrcFifo != NULL)
		{
			memset(i2s1_out->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
		}
		else
		{
			DBG("i2s1_out->OutSrcFifo error\n");
			return ;
		}
		if(i2s1_out->OutSrcBuff==NULL)
		{
			i2s1_out->OutSrcBuff = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
		}

		if(i2s1_out->OutSrcBuff != NULL)
		{
			memset(i2s1_out->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
		}
		else
		{
			DBG("i2s1_out->OutSrcBuff error\n");
			return ;
		}
	#endif

	#if defined(CFG_FUNC_I2S1OUT_SRA_EN)
		if(i2s1_out->OutSraFifo==NULL)
		{
			i2s1_out->OutSraFifo = (uint8_t *)osPortMallocFromEnd(RESAMPLE_FIFO_LEN);// fifo
		}

		if(i2s1_out->OutSraFifo != NULL)
		{
			memset(i2s1_out->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
		}
		else
		{
			DBG("i2s1_out->OutSraFifo error\n");
			return ;
		}
	#endif
	//------------------------//
	#ifdef CFG_FUNC_I2S1OUT_SRC_EN
    #ifdef I2S_SRC_1_EN
	SRC_SRA_I2S1_OUT.ResampleObj = (ResamplerContext *)osPortMallocFromEnd(sizeof(ResamplerContext));// SRC
	if(SRC_SRA_I2S1_OUT.ResampleObj != NULL)
	{
		memset(SRC_SRA_I2S1_OUT.ResampleObj, 0, sizeof(ResamplerContext));
	}
	else
	{
		DBG("I2S1OUT SRC error\n");
		return ;
	}
    #else
	SRC_SRA_I2S1_OUT.ResampleObj = (ResamplerPolyphaseContext *)osPortMallocFromEnd(sizeof(ResamplerPolyphaseContext));// SRC
	if(SRC_SRA_I2S1_OUT.ResampleObj != NULL)
	{
		memset(SRC_SRA_I2S1_OUT.ResampleObj, 0, sizeof(ResamplerPolyphaseContext));
	}
	else
	{
		DBG("I2S1OUT SRC error\n");
		return ;
	}
    #endif
	#endif
#endif
}

void AudioI2S_Release(void)
{
	DBG("%s\n",__func__);

	//-----------------------------//
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)
	MODULE_SRC_SRA *i2s0_in;
	i2s0_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_IN;
#endif
#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)
	MODULE_SRC_SRA *i2s0_out;
	i2s0_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_OUT;
#endif
#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)
	MODULE_SRC_SRA *i2s1_in;
	i2s1_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_IN;
#endif

#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)
	MODULE_SRC_SRA *i2s1_out;
	i2s1_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_OUT;
#endif
	//------------------------------------//

#ifdef CFG_RES_AUDIO_I2S0IN_EN

	AudioCoreSourceDisable(I2S0_SOURCE_NUM);

	if(mainAppCt.Source2Buf_I2S0 != NULL)
	{
		DBG("free Source2Buf_I2S0\n");
		osPortFree(mainAppCt.Source2Buf_I2S0);
		mainAppCt.Source2Buf_I2S0 = NULL;
	}


	if(mainAppCt.I2S0_IN_FIFO != NULL)
	{
		DBG("free I2S0_IN_FIFO\n");
		osPortFree(mainAppCt.I2S0_IN_FIFO);
		mainAppCt.I2S0_IN_FIFO = NULL;
	}
//------------------------------//
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)

	if(i2s0_in->OutSrcFifo!=NULL)
	{
		DBG("i2s0_in->OutSrcFifo\n");
		osPortFree(i2s0_in->OutSrcFifo);
		i2s0_in->OutSrcFifo = NULL;
	}

	if(i2s0_in->OutSrcBuff != NULL)
	{
		DBG("i2s0_in->OutSrcBuff\n");
		osPortFree(i2s0_in->OutSrcBuff);
		i2s0_in->OutSrcBuff = NULL;
	}

#endif

#if defined(CFG_FUNC_I2S0IN_SRA_EN)

	if(i2s0_in->OutSraFifo!=NULL)
	{
		DBG("i2s0_in->OutSraFifo\n");
		osPortFree(i2s0_in->OutSraFifo);
		i2s0_in->OutSraFifo = NULL;
	}
#endif
//--------------------------------------//
	#ifdef CFG_FUNC_I2S0IN_SRC_EN
	if(SRC_SRA_I2S0_IN.ResampleObj != NULL)
	{
		DBG("free I2S0_IN_SRC\n");
		osPortFree(SRC_SRA_I2S0_IN.ResampleObj);
		SRC_SRA_I2S0_IN.ResampleObj = NULL;
	}
	#endif
#endif
//-------------------------------------------//
#ifdef CFG_RES_AUDIO_I2S1IN_EN

	AudioCoreSourceDisable(I2S1_SOURCE_NUM);

	if(mainAppCt.Source2Buf_I2S1 != NULL)
	{
		DBG("free Source2Buf_I2S1\n");
		osPortFree(mainAppCt.Source2Buf_I2S1);
		mainAppCt.Source2Buf_I2S1 = NULL;
	}


	if(mainAppCt.I2S1_IN_FIFO != NULL)
	{
		DBG("free I2S1_IN_FIFO\n");
		osPortFree(mainAppCt.I2S1_IN_FIFO);
		mainAppCt.I2S1_IN_FIFO = NULL;
	}
	//------------------------------//
	#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)

		if(i2s1_in->OutSrcFifo!=NULL)
		{
			DBG("i2s1_in->OutSrcFifo\n");
			osPortFree(i2s1_in->OutSrcFifo);
			i2s1_in->OutSrcFifo = NULL;
		}

		if(i2s1_in->OutSrcBuff != NULL)
		{
			DBG("i2s1_in->OutSrcBuff\n");
			osPortFree(i2s1_in->OutSrcBuff);
			i2s1_in->OutSrcBuff = NULL;
		}

	#endif

	#if defined(CFG_FUNC_I2S0IN_SRA_EN)

		if(i2s0_in->OutSraFifo!=NULL)
		{
			DBG("i2s1_in->OutSraFifo\n");
			osPortFree(i2s0_in->OutSraFifo);
			i2s0_in->OutSraFifo = NULL;
		}
	#endif
	#ifdef CFG_FUNC_I2S1IN_SRC_EN
	if(SRC_SRA_I2S1_IN.ResampleObj != NULL)
	{
		DBG("free I2S1_IN_SRC\n");
		osPortFree(SRC_SRA_I2S1_IN.ResampleObj);
		SRC_SRA_I2S1_IN.ResampleObj = NULL;
	}
	#endif
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN

	AudioCoreSinkDisable(AUDIO_I2S0OUT_SINK_NUM);

	if(mainAppCt.Sink2Buf_I2S0 != NULL)
	{
		DBG("free Sink2Buf_I2S0\n");
		osPortFree(mainAppCt.Sink2Buf_I2S0);
		mainAppCt.Sink2Buf_I2S0 = NULL;
	}
	if(mainAppCt.I2S0_OUT_FIFO != NULL)
	{
		DBG("free I2S0_OUT_FIFO\n");
		osPortFree(mainAppCt.I2S0_OUT_FIFO);
		mainAppCt.I2S0_OUT_FIFO = NULL;
	}
//------------------------------------------//
	#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)

		if(i2s0_out->OutSrcFifo != NULL)
		{
			DBG("i2s0_out->OutSrcFifo\n");
			osPortFree(i2s0_out->OutSrcFifo);
			i2s0_out->OutSrcFifo = NULL;
		}

		if(i2s0_out->OutSrcBuff != NULL)
		{
			DBG("i2s0_out->OutSrcBuff\n");
			osPortFree(i2s0_out->OutSrcBuff);
			i2s0_out->OutSrcBuff = NULL;
		}

	#endif

	#if defined(CFG_FUNC_I2S0OUT_SRA_EN)
		if(i2s0_out->OutSraFifo != NULL)
		{
			DBG("i2s0_out->OutSraFifo\n");
			osPortFree(i2s0_out->OutSraFifo);
			i2s0_out->OutSraFifo = NULL;
		}

	#endif
//------------------------//
	#ifdef CFG_FUNC_I2S0OUT_SRC_EN
	if(i2s0_out->ResampleObj != NULL)
	{
		DBG("free I2S0_OUT_SRC\n");
		osPortFree(i2s0_out->ResampleObj);
		i2s0_out->ResampleObj = NULL;
	}
	#endif
#endif

#ifdef CFG_RES_AUDIO_I2S1OUT_EN

	AudioCoreSinkDisable(AUDIO_I2S1OUT_SINK_NUM);

	if(mainAppCt.Sink2Buf_I2S1 != NULL)
	{
		DBG("free Sink2Buf_I2S1\n");
		osPortFree(mainAppCt.Sink2Buf_I2S1);
		mainAppCt.Sink2Buf_I2S1 = NULL;
	}
	if(mainAppCt.I2S1_OUT_FIFO != NULL)
	{
		DBG("free I2S1_OUT_FIFO\n");
		osPortFree(mainAppCt.I2S1_OUT_FIFO);
		mainAppCt.I2S1_OUT_FIFO = NULL;
	}
	//------------------------------------------//
		#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)

			if(i2s1_out->OutSrcFifo != NULL)
			{
				DBG("i2s1_out->OutSrcFifo\n");
				osPortFree(i2s1_out->OutSrcFifo);
				i2s1_out->OutSrcFifo = NULL;
			}

			if(i2s1_out->OutSrcBuff != NULL)
			{
				DBG("i2s1_out->OutSrcBuff\n");
				osPortFree(i2s1_out->OutSrcBuff);
				i2s1_out->OutSrcBuff = NULL;
			}

		#endif

		#if defined(CFG_FUNC_I2S1OUT_SRA_EN)
			if(i2s1_out->OutSraFifo != NULL)
			{
				DBG("i2s1_out->OutSraFifo\n");
				osPortFree(i2s1_out->OutSraFifo);
				i2s1_out->OutSraFifo = NULL;
			}

		#endif
	//------------------------//
	#ifdef CFG_FUNC_I2S1OUT_SRC_EN
	if(SRC_SRA_I2S1_OUT.ResampleObj != NULL)
	{
		DBG("free I2S1_OUT_SRC\n");
		osPortFree(SRC_SRA_I2S1_OUT.ResampleObj);
		SRC_SRA_I2S1_OUT.ResampleObj = NULL;
	}
	#endif
#endif
}

void AudioI2S_HWInit(void)
{
#ifdef CFG_RES_I2S0_I2S1_Binding_GPIO

	GPIO_PortAModeSet(GPIOA24, 9);// mclk out //1001:i2s0_mclk_out_1(o)

    #if (CFG_RES_BINDING_I2S0_IS_MASTER == 0)
	GPIO_PortAModeSet(GPIOA20, 4);// lrclk  //100:i2s0_lrck_1(o)/i2s1_lrck_3(i)
	GPIO_PortAModeSet(GPIOA21, 3);// bclk   //011:i2s0_bclk_1(o)/i2s1_bclk_3(i)
    #endif

	#if (CFG_RES_BINDING_I2S0_IS_MASTER == 1)
	GPIO_PortAModeSet(GPIOA20, 4);// lrclk  //011:i2s0_lrck_1(i)/i2s1_lrck_3(0)
	GPIO_PortAModeSet(GPIOA21, 3);// bclk   //010:i2s0_bclk_1(i)/i2s1_bclk_3(0)
	#endif

	#if (CFG_RES_BINDING_I2S0_IS_MASTER == 2)
	GPIO_PortAModeSet(GPIOA24, 3);// mclk in //11:i2s0_mclk_in_1(i)
	GPIO_PortAModeSet(GPIOA20, 5);// lrclk  //101:i2s0_lrck_1(i)/i2s1_lrck_3(i)
	GPIO_PortAModeSet(GPIOA21, 4);// bclk   //100:i2s0_bclk_1(i)/i2s1_bclk_3(i)
	#endif

	GPIO_PortAModeSet(GPIOA23, 8);// dout//1000:i2s0_dout_3(o)
	GPIO_PortAModeSet(GPIOA22, 3);//  din//0011:i2s0_din_3(i)

	GPIO_PortAModeSet(GPIOA31, 2);// din//0010:i2s1_din_0(i)

#endif

    #ifdef CFG_RES_I2S0_EN
	AudioI2S0_HWInit();
    #endif

    #ifdef CFG_RES_I2S1_EN
	AudioI2S1_HWInit();
    #endif
}

void AudioI2S_ConfigInit(void)
{
	DBG("%s\n",__func__);
	DBG("MODULE_SRC_SRA Size: %ld\n",sizeof(MODULE_SRC_SRA));
#ifdef CFG_RES_I2S0_EN
	#ifdef CFG_RES_AUDIO_I2S0IN_EN
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].FuncDataGet = AudioI2S0_DataGet;
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].FuncDataGetLen = AudioI2S0_DataLenGet;
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].IsSreamData = TRUE;
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].PcmFormat = 2;//test mic audio effect
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source2Buf_I2S0;
	AudioCoreSourceEnable(I2S0_SOURCE_NUM);
	#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)///only I2S1 OK
	AudioI2S0_InResampleInit();
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].FuncDataGet = AudioI2S0_DataGetX;
	mainAppCt.AudioCore->AudioSource[I2S0_SOURCE_NUM].FuncDataGetLen = AudioI2S0_DataLenGetX;
	#endif
	#endif
	#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	mainAppCt.AudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].FuncDataSet = AudioI2S0_DataSet;
	mainAppCt.AudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].FuncDataSpaceLenGet = AudioI2S0_DataSpaceLenGet;
	mainAppCt.AudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_I2S0; //sink audiocore 内buf 1 frame。
	mainAppCt.AudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].PcmFormat = 2;
	AudioCoreSinkEnable(AUDIO_I2S0OUT_SINK_NUM);
	#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)///only I2S1 OK
	AudioI2S0_OutResampleInit();
	mainAppCt.AudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].SreamDataState = 0;
	mainAppCt.AudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].FuncDataSet = AudioI2S0_DataSetX;
	mainAppCt.AudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].FuncDataSpaceLenGet = AudioI2S0_DataSpaceLenGetX;
	#endif
	#endif
#endif

#ifdef CFG_RES_I2S1_EN
   #ifdef CFG_RES_AUDIO_I2S1IN_EN
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].Enable = 0;
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].FuncDataGet = AudioI2S1_DataGet;
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].FuncDataGetLen = AudioI2S1_DataLenGet;
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].IsSreamData = TRUE;
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].PcmFormat = 2;//test mic audio effect
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source2Buf_I2S1;
   AudioCoreSourceEnable(I2S1_SOURCE_NUM);
   #if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)///only I2S1 OK
   AudioI2S1_InResampleInit();
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].FuncDataGet = AudioI2S1_DataGetX;
   mainAppCt.AudioCore->AudioSource[I2S1_SOURCE_NUM].FuncDataGetLen = AudioI2S1_DataLenGetX;
   #endif
   #endif
   #ifdef CFG_RES_AUDIO_I2S1OUT_EN
   mainAppCt.AudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].FuncDataSet = AudioI2S1_DataSet;
   mainAppCt.AudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].FuncDataSpaceLenGet = AudioI2S1_DataSpaceLenGet;
   mainAppCt.AudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_I2S1; //sink audiocore 内buf 1 frame。
   mainAppCt.AudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].PcmFormat = 2;
   AudioCoreSinkEnable(AUDIO_I2S1OUT_SINK_NUM);
   #if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)///only I2S1 OK
   AudioI2S1_OutResampleInit();
   mainAppCt.AudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].FuncDataSet = AudioI2S1_DataSetX;
   mainAppCt.AudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].FuncDataSpaceLenGet = AudioI2S1_DataSpaceLenGetX;
   #endif
   #endif
#endif //

   AudioI2S_HWInit();
}

void AudioI2S_PlayDeInit(void)
{

}
/*
****************************************************************
* I2S1参数初始化 (in and out)
****************************************************************
*/
void AudioI2S0_HWInit(void)
{
#ifdef CFG_RES_I2S0_EN
	uint32_t ch;
	I2SParamCt i2s_set;
	DBG("%s\n",__func__);
	i2s_set.I2sTxRxEnable = 0;
	//i2s_set.IsMasterMode=CFG_RES_I2S0_MODE;// 0:master 1:slave
	i2s_set.IsMasterMode=gCtrlVars.i2s0_work_mode;
	//----如果 SRC 允许，需要注意设置：resampler_init------------/
	//i2s_set.SampleRate= mainAppCt.SampleRate;//跟随系统设置//
	//i2s_set.SampleRate= CFG_PARA_I2S0_SAMPLE;//跟随I2S 宏设置//
	i2s_set.SampleRate= SupportSampleRateList[gCtrlVars.i2s0_sample_rate];//跟随调音文件设置////
	i2s_set.I2sFormat=gCtrlVars.i2s0_format;//I2S_FORMAT_I2S;
	i2s_set.I2sBits=gCtrlVars.i2s0_word_len;//I2S_LENGTH_16BITS;
#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	i2s_set.I2sTxRxEnable += gCtrlVars.i2s0_tx_en;
	i2s_set.TxPeripheralID=PERIPHERAL_ID_I2S0_TX;
	i2s_set.TxBuf=(void*)mainAppCt.I2S0_OUT_FIFO;
	i2s_set.TxLen=mainAppCt.SamplesPreFrame * 2 * 2 * 2;
	ch = DMA_ChannelNumGet(PERIPHERAL_ID_I2S0_TX);
	if(ch > 7 )
	{
		DBG("I2S0 TX DMA NOT OPEN!\n");
	}
	else
	{
		DBG("I2S0 TX DMA SET OK!\n");
	}
#endif
#ifdef CFG_RES_AUDIO_I2S0IN_EN
	i2s_set.I2sTxRxEnable += (gCtrlVars.i2s0_rx_en<<1);
	i2s_set.RxPeripheralID=PERIPHERAL_ID_I2S0_RX;
	i2s_set.RxBuf=(void*)mainAppCt.I2S0_IN_FIFO;
	i2s_set.RxLen=mainAppCt.SamplesPreFrame * 2 * 2 * 2;
	ch = DMA_ChannelNumGet(PERIPHERAL_ID_I2S0_RX);
	if(ch > 7 )
	{
		DBG("I2S0 RX DMA NOT OPEN!\n");
	}
	else
	{
		DBG("I2S0 RX DMA SET OK!\n");
	}
#endif

#ifndef CFG_RES_I2S0_I2S1_Binding_GPIO
	if(i2s_set.IsMasterMode==0)
	{
		GPIO_PortAModeSet(GPIOA0, 9);// mclk out
	}
	else
	{
		GPIO_PortAModeSet(GPIOA0, 3);// mclk in
	}	
	GPIO_PortAModeSet(GPIOA1, 6);// lrclk
	GPIO_PortAModeSet(GPIOA2, 5);// bclk
	GPIO_PortAModeSet(GPIOA3, 7);// dout
	GPIO_PortAModeSet(GPIOA4, 1);// din
#endif

	AudioI2S_Init(I2S0_MODULE, &i2s_set);
    #if CFG_RES_I2S0_MODE
	I2S_SampleRateCheckInterruptClr(I2S0_MODULE);
	I2S_SampleRateCheckInterruptEnable(I2S0_MODULE);
    #endif

	DBG("I2S0->Mode:%d, SampleRate:%ld,TxRx:%d\n",i2s_set.IsMasterMode,i2s_set.SampleRate,i2s_set.I2sTxRxEnable);
#endif
}
/*
****************************************************************
* I2S1参数初始化 (in and out)
****************************************************************
*/
void AudioI2S1_HWInit(void)
{
#ifdef CFG_RES_I2S1_EN
	uint32_t ch;
	I2SParamCt i2s_set;
	i2s_set.I2sTxRxEnable = 0;
	//i2s_set.IsMasterMode=CFG_RES_I2S1_MODE;// 0:master 1:slave
	i2s_set.IsMasterMode=gCtrlVars.i2s1_work_mode;
	//----如果 SRC 允许，需要注意设置：resampler_init------------/
	//i2s_set.SampleRate= mainAppCt.SampleRate;//跟随系统设置//
	//i2s_set.SampleRate= CFG_PARA_I2S1_SAMPLE;//跟随I2S 宏设置//
	i2s_set.SampleRate= SupportSampleRateList[gCtrlVars.i2s1_sample_rate];//跟随调音文件设置////
	i2s_set.I2sFormat=gCtrlVars.i2s1_format;//I2S_FORMAT_I2S;
	i2s_set.I2sBits=gCtrlVars.i2s1_word_len;//I2S_LENGTH_16BITS;
#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	i2s_set.I2sTxRxEnable += gCtrlVars.i2s1_tx_en;
	i2s_set.TxPeripheralID=PERIPHERAL_ID_I2S1_TX;
	i2s_set.TxBuf=(void*)mainAppCt.I2S1_OUT_FIFO;
	i2s_set.TxLen=mainAppCt.SamplesPreFrame * 2 * 2 * 2;
	ch = DMA_ChannelNumGet(PERIPHERAL_ID_I2S1_TX);
	if(ch > 7 )
	{
		DBG("I2S1 TX DMA NOT OPEN!\n");
	}
	else
	{
		DBG("I2S1 TX DMA SET OK!\n");
	}
#endif
#ifdef CFG_RES_AUDIO_I2S1IN_EN
	i2s_set.I2sTxRxEnable += (gCtrlVars.i2s1_rx_en<<1);
	i2s_set.RxPeripheralID=PERIPHERAL_ID_I2S1_RX;
	i2s_set.RxBuf=(void*)mainAppCt.I2S1_IN_FIFO;
	i2s_set.RxLen=mainAppCt.SamplesPreFrame * 2 * 2 * 2;
	ch = DMA_ChannelNumGet(PERIPHERAL_ID_I2S1_RX);
	if(ch > 7 )
	{
		DBG("I2S1 RX DMA NOT OPEN!\n");
	}
	else
	{
		DBG("I2S1 RX DMA SET OK!\n");
	}
#endif

#ifndef CFG_RES_I2S0_I2S1_Binding_GPIO
	if(i2s_set.IsMasterMode==0)
	{
		GPIO_PortAModeSet(GPIOA7, 5);// mclk out
	}
	else
	{
		GPIO_PortAModeSet(GPIOA7, 3);// mclk in
	}
	GPIO_PortAModeSet(GPIOA8, 1);//lrclk
	GPIO_PortAModeSet(GPIOA9, 2);//bclk
	GPIO_PortAModeSet(GPIOA10, 4);//do
	GPIO_PortAModeSet(GPIOA11, 2);//di
#endif

	AudioI2S_Init(I2S1_MODULE, &i2s_set);

    #if CFG_RES_I2S1_MODE
	I2S_SampleRateCheckInterruptClr(I2S1_MODULE);
	I2S_SampleRateCheckInterruptEnable(I2S1_MODULE);
    #endif
	DBG("I2S1->Mode:%d, SampleRate:%ld,TxRx:%d\n",i2s_set.IsMasterMode,i2s_set.SampleRate,i2s_set.I2sTxRxEnable);
#endif
}
/*
****************************************************************
*
*采样率转换   + 采样率微调
*
****************************************************************
*/
void  AudioI2S_DataProcess(void)
{
	uint32_t Rate;
#ifdef CFG_RES_I2S0_EN

	AudioI2S0_DataInProcess();//I2S0
	AudioI2S0_DataOutProcess();//I2S0

	#if CFG_RES_I2S0_MODE
	if(I2S_SampleRateCheckInterruptGet(I2S0_MODULE))
	{
		Rate = I2S_SampleRateGet(I2S0_MODULE);

		I2S_SampleRateCheckInterruptClr(I2S0_MODULE);

		//DBG("I2S0:%d\n",Rate);
	}
	else
	{

	}
	#endif
#endif

#ifdef CFG_RES_I2S1_EN
	AudioI2S1_DataInProcess();//I2S1
	AudioI2S1_DataOutProcess();//I2S1
	#if CFG_RES_I2S1_MODE
	if(I2S_SampleRateCheckInterruptGet(I2S1_MODULE))
	{
		Rate = I2S_SampleRateGet(I2S1_MODULE);

		I2S_SampleRateCheckInterruptClr(I2S1_MODULE);

		//DBG("I2S1:%d\n",Rate);
	}
	else
	{

	}
	#endif
#endif


}

void AudioI2S0_OutResampleInit(void)
{
#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)
	MODULE_SRC_SRA *i2s_out;

	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_OUT;

	memset(i2s_out->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_out->OutFifo, i2s_out->OutSrcFifo, RESAMPLE_FIFO_LEN);

	memset(i2s_out->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_out->InFifo, i2s_out->OutSrcBuff, RESAMPLE_FIFO_LEN);

#endif

#ifdef  CFG_FUNC_I2S0OUT_SRC_EN
    #ifdef I2S_SRC_1_EN
	resampler_init(i2s_out->ResampleObj, 2, CFG_PARA_SAMPLE_RATE, SupportSampleRateList[gCtrlVars.i2s0_sample_rate], 0, 0);
    #else
	resampler_polyphase_init(i2s_out->ResampleObj, 2, Get_Resampler_Polyphase(SupportSampleRateList[gCtrlVars.i2s0_sample_rate]));
    #endif
#endif

#ifdef  CFG_FUNC_I2S0OUT_SRA_EN
	sra_init(&i2s_out->SraObj,2);//默认双声
	memset(i2s_out->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_out->SraFifo, i2s_out->OutSraFifo, RESAMPLE_FIFO_LEN);
#endif


}

void AudioI2S1_OutResampleInit(void)
{
#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)
	MODULE_SRC_SRA *i2s_out;

	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_OUT;

	memset(i2s_out->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_out->OutFifo, i2s_out->OutSrcFifo, RESAMPLE_FIFO_LEN);

	memset(i2s_out->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_out->InFifo, i2s_out->OutSrcBuff, RESAMPLE_FIFO_LEN);

#ifdef  CFG_FUNC_I2S1OUT_SRC_EN
	#ifdef I2S_SRC_1_EN
	resampler_init(i2s_out->ResampleObj, 2, CFG_PARA_SAMPLE_RATE, SupportSampleRateList[gCtrlVars.i2s1_sample_rate], 0, 0);
	#else
	resampler_polyphase_init(i2s_out->ResampleObj, 2, Get_Resampler_Polyphase(SupportSampleRateList[gCtrlVars.i2s1_sample_rate]));
	#endif
#endif

#ifdef  CFG_FUNC_I2S1OUT_SRA_EN
	sra_init(&i2s_out->SraObj,2);//默认双声
	memset(i2s_out->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_out->SraFifo, i2s_out->OutSraFifo, RESAMPLE_FIFO_LEN);
#endif

#endif

}
/*
****************************************************************
*
*I2S0 out 采样率转换   + 采样率微调
*
****************************************************************
*/
void AudioI2S0_DataOutProcess(void)
{
#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)
	static uint8_t timer = 0;
	uint16_t Len;
	int32_t data_len;
	uint16_t samples;
	uint32_t i;
	uint16_t space_len;
	int32_t n_inc_dec_o;
	int32_t SRCDoneLen = 0;
	MODULE_SRC_SRA *i2s_out;

	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_OUT;
#ifdef  CFG_FUNC_I2S0OUT_SRC_EN
	if(i2s_out->ResampleObj==NULL) return;
#endif
    Len = AudioI2S0_DataSpaceLenGet();//I2S1

    samples = Len/MAX_FRAME_SAMPLES;
	for(i = 0; i < samples; i++)
	{
		data_len = AudioI2S0_DataSpaceLenGet();//I2S1
		if(data_len >= MAX_FRAME_SAMPLES)
		{
			data_len = MCUCircular_GetDataLen(&i2s_out->OutFifo)/4;//audio剩余数据
			if(data_len >=MAX_FRAME_SAMPLES)
			{
				data_len = MAX_FRAME_SAMPLES;
				MCUCircular_GetData(&i2s_out->OutFifo, i2s_out->OutTempBuf, data_len*4);//I2sOutBuff
				AudioI2S0_DataSet(i2s_out->OutTempBuf, data_len);
			}
		}
	}
//---------------------------------------------------------------------------//
	data_len = MCUCircular_GetDataLen(&i2s_out->InFifo)/4;//audio剩余数据

	if(data_len < MAX_FRAME_SAMPLES) return;

//-----------SRC--------------------------------------------------------------//
	samples = data_len/MAX_FRAME_SAMPLES;
	for(i = 0; i < samples; i++)
	{

		data_len = MCUCircular_GetData(&i2s_out->InFifo,(uint8_t *)i2s_out->InTempBuf,MAX_FRAME_SAMPLES*4)/4; //

#ifdef  CFG_FUNC_I2S0OUT_SRC_EN
        #ifdef I2S_SRC_1_EN
		data_len = resampler_apply(i2s_out->ResampleObj, (int16_t*)i2s_out->InTempBuf, (int16_t*)i2s_out->OutTempBuf, MAX_FRAME_SAMPLES);
        #else
        data_len = resampler_polyphase_apply(i2s_out->ResampleObj, (int16_t *)i2s_out->InTempBuf, (int16_t*)i2s_out->OutTempBuf, MAX_FRAME_SAMPLES);
        #endif
#else
        memcpy(&i2s_out->OutTempBuf,(uint8_t *)&i2s_out->InTempBuf,data_len*4);
#endif //end if CFG_FUNC_I2S_OUT_SRA_EN

#ifdef CFG_FUNC_I2S0OUT_SRA_EN
		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_out->SraFifo, &i2s_out->OutTempBuf, data_len*4);
		}
#else
		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_out->OutFifo, &i2s_out->OutTempBuf, data_len*4);
		}
#endif///end of CFG_FUNC_I2S_OUT_SRA_EN
	}
//-------SRA----------------------------------------------------------------------//
#ifdef CFG_FUNC_I2S0OUT_SRA_EN

	uint32_t one_fifo = RESAMPLE_FIFO_LEN/8;

	samples = MCUCircular_GetDataLen(&i2s_out->SraFifo)/4;//audio剩余数据

	if(samples < SRA_BLOCK)
	{
		return;
	}
	samples /= SRA_BLOCK;
	//space_len = MCUCircular_GetSpaceLen(&i2s_out->I2sOutFifo)/8;//audio剩余空间
	//DBG("fifo:%d    space:%d\n",one_fifo,space_len);
    for(i = 0; i < samples; i++)
    {
    	space_len = MCUCircular_GetSpaceLen(&i2s_out->OutFifo);//audio剩余空间

		n_inc_dec_o = 0;

		if((space_len >= one_fifo*6)&& (space_len < one_fifo*8))
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
        /////此处可增加定时处理，可以调整微调的速度
		if(timer==0)
		{
			timer = 60;///60*6ms= 360ms
			n_inc_dec_o = 0;
		}
		else
		{
			timer--;
		}
		MCUCircular_GetData(&i2s_out->SraFifo,(uint8_t *)&i2s_out->InTempBuf,SRA_BLOCK*4); //

		if(sra_apply(&i2s_out->SraObj, (int16_t *)&i2s_out->InTempBuf, (int16_t *)i2s_out->OutTempBuf, n_inc_dec_o)==SRA_ERROR_OK)//
		{
			SRCDoneLen = SRA_BLOCK + n_inc_dec_o;
		}
		else
		{
			SRCDoneLen = SRA_BLOCK;
			memcpy(&i2s_out->OutTempBuf,&i2s_out->InTempBuf,SRCDoneLen*4);
		}
//------------Put Data---------------------------------------------------------------------------------//
		MCUCircular_PutData(&i2s_out->OutFifo, &i2s_out->OutTempBuf, SRCDoneLen*4);
     }
#endif
#endif //
}
/*
****************************************************************
*
*I2S1 out 采样率转换   + 采样率微调
*
****************************************************************
*/
void  AudioI2S1_DataOutProcess(void)
{
#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)
	static uint8_t timer = 0;
	uint16_t Len;
	int32_t data_len;
	uint16_t samples;
	uint32_t i;
	uint16_t space_len;
	int32_t n_inc_dec_o;
	int32_t SRCDoneLen = 0;
	MODULE_SRC_SRA *i2s_out;
	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_OUT;
#ifdef  CFG_FUNC_I2S1OUT_SRC_EN
	if(i2s_out->ResampleObj==NULL) return;
#endif
    Len = AudioI2S1_DataSpaceLenGet();//I2S1

    samples = Len/MAX_FRAME_SAMPLES;
	for(i = 0; i < samples; i++)
	{
		data_len = AudioI2S1_DataSpaceLenGet();//I2S1
		if(data_len >= MAX_FRAME_SAMPLES)
		{
			data_len = MCUCircular_GetDataLen(&i2s_out->OutFifo)/4;//audio剩余数据
			if(data_len >=MAX_FRAME_SAMPLES)
			{
				data_len = MAX_FRAME_SAMPLES;
				MCUCircular_GetData(&i2s_out->OutFifo, i2s_out->OutTempBuf, data_len*4);//I2sOutBuff
				AudioI2S1_DataSet(i2s_out->OutTempBuf, data_len);
			}
		}
	}
//---------------------------------------------------------------------------//
	data_len = MCUCircular_GetDataLen(&i2s_out->InFifo)/4;//audio剩余数据

	if(data_len < MAX_FRAME_SAMPLES) return;

//-----------SRC--------------------------------------------------------------//
	samples = data_len/MAX_FRAME_SAMPLES;
	for(i = 0; i < samples; i++)
	{
		data_len = MCUCircular_GetData(&i2s_out->InFifo,(uint8_t *)i2s_out->InTempBuf,MAX_FRAME_SAMPLES*4)/4; //

#ifdef  CFG_FUNC_I2S1OUT_SRC_EN
		#ifdef I2S_SRC_1_EN
		data_len = resampler_apply(i2s_out->ResampleObj, (int16_t*)i2s_out->InTempBuf, (int16_t*)i2s_out->OutTempBuf, MAX_FRAME_SAMPLES);
		#else
		data_len = resampler_polyphase_apply(i2s_out->ResampleObj, (int16_t *)i2s_out->InTempBuf, (int16_t*)i2s_out->OutTempBuf, MAX_FRAME_SAMPLES);
		#endif
#else
		memcpy(&i2s_out->OutTempBuf,(uint8_t *)&i2s_out->InTempBuf,data_len*4);
#endif //end if CFG_FUNC_I2S_OUT_SRA_EN

#ifdef CFG_FUNC_I2S1OUT_SRA_EN
		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_out->SraFifo, &i2s_out->OutTempBuf, data_len*4);
		}
		else
		{
			MCUCircular_PutData(&i2s_out->SraFifo, &i2s_out->OutTempBuf, MAX_FRAME_SAMPLES*4);
		}
#else
		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_out->OutFifo, &i2s_out->OutTempBuf, data_len*4);
		}
		else
		{
			MCUCircular_PutData(&i2s_out->OutFifo, &i2s_out->InTempBuf, MAX_FRAME_SAMPLES*4);
		}
#endif///end of CFG_FUNC_I2S_OUT_SRA_EN
	}
//-------SRA----------------------------------------------------------------------//
#ifdef CFG_FUNC_I2S1OUT_SRA_EN

	uint32_t one_fifo = RESAMPLE_FIFO_LEN/8;

	samples = MCUCircular_GetDataLen(&i2s_out->SraFifo)/4;//audio剩余数据

	if(samples < SRA_BLOCK)
	{
		return;
	}
	samples /= SRA_BLOCK;
	//space_len = MCUCircular_GetSpaceLen(&i2s_out->I2sOutFifo)/8;//audio剩余空间
	//DBG("fifo:%d    space:%d\n",one_fifo,space_len);
    for(i = 0; i < samples; i++)
    {
    	space_len = MCUCircular_GetSpaceLen(&i2s_out->OutFifo);//audio剩余空间

		n_inc_dec_o = 0;

		if((space_len >= one_fifo*6)&& (space_len < one_fifo*8))
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
        /////此处可增加定时处理，可以调整微调的速度
		if(timer==0)
		{
			timer = 60;///60*6ms= 360ms
			n_inc_dec_o = 0;
		}
		else
		{
			timer--;
		}
		MCUCircular_GetData(&i2s_out->SraFifo,(uint8_t *)&i2s_out->InTempBuf,SRA_BLOCK*4); //

		if(sra_apply(&i2s_out->SraObj, (int16_t *)&i2s_out->InTempBuf, (int16_t *)i2s_out->OutTempBuf, n_inc_dec_o)==SRA_ERROR_OK)//
		{
			SRCDoneLen = SRA_BLOCK + n_inc_dec_o;
		}
		else
		{
			SRCDoneLen = SRA_BLOCK;
			memcpy(&i2s_out->OutTempBuf,&i2s_out->InTempBuf,SRCDoneLen*4);
		}
//------------Put Data---------------------------------------------------------------------------------//
		MCUCircular_PutData(&i2s_out->OutFifo, &i2s_out->OutTempBuf, SRCDoneLen*4);
     }
#endif
#endif //
}



void AudioI2S0_InResampleInit(void)
{
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)
	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_IN;

	memset(i2s_in->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_in->OutFifo, i2s_in->OutSrcFifo, RESAMPLE_FIFO_LEN);

	memset(i2s_in->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_in->InFifo, &i2s_in->OutSrcBuff, RESAMPLE_FIFO_LEN);
#endif

#ifdef  CFG_FUNC_I2S0IN_SRC_EN
    #ifdef I2S_SRC_1_EN
	resampler_init(i2s_in->ResampleObj, 2, SupportSampleRateList[gCtrlVars.i2s0_sample_rate],CFG_PARA_SAMPLE_RATE, 0, 0);
    #else
	resampler_polyphase_init(i2s_in->ResampleObj, 2, Get_Resampler_Polyphase(SupportSampleRateList[gCtrlVars.i2s0_sample_rate]));
    #endif
#endif

#ifdef  CFG_FUNC_I2S0IN_SRA_EN
	sra_init(&i2s_in->SraObj,2);//默认双声
	memset(i2s_in->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_in->SraFifo, i2s_in->OutSraFifo, RESAMPLE_FIFO_LEN);
#endif


}

void AudioI2S1_InResampleInit(void)
{
#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)
	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_IN;

	memset(i2s_in->OutSrcFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_in->OutFifo, i2s_in->OutSrcFifo, RESAMPLE_FIFO_LEN);

	memset(i2s_in->OutSrcBuff, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_in->InFifo, i2s_in->OutSrcBuff, RESAMPLE_FIFO_LEN);

#ifdef  CFG_FUNC_I2S1IN_SRC_EN
    #ifdef I2S_SRC_1_EN
	resampler_init(i2s_in->ResampleObj, 2, SupportSampleRateList[gCtrlVars.i2s1_sample_rate], CFG_PARA_SAMPLE_RATE, 0, 0);
    #else
    resampler_polyphase_init(i2s_in->ResampleObj, 2, Get_Resampler_Polyphase(SupportSampleRateList[gCtrlVars.i2s1_sample_rate]));
    #endif
#endif

#ifdef  CFG_FUNC_I2S1IN_SRA_EN
	sra_init(&i2s_in->SraObj,2);//默认双声
	memset(i2s_in->OutSraFifo, 0, RESAMPLE_FIFO_LEN);
	MCUCircular_Config(&i2s_in->SraFifo, i2s_in->OutSraFifo, RESAMPLE_FIFO_LEN);
#endif

#endif
}
/*
****************************************************************
*
*采样率转换   + 采样率微调
*
****************************************************************
*/
void AudioI2S0_DataInProcess(void)
{
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)
	static uint8_t timer = 0;
	int32_t data_len;
	uint16_t samples;
	uint32_t i;
	uint16_t space_len;
	int32_t n_inc_dec_o;
	int32_t SRCDoneLen = 0;
	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_IN;

#ifdef  CFG_FUNC_I2S0IN_SRC_EN
		if(i2s_in->ResampleObj==NULL) return;
#endif

	data_len = AudioI2S0_DataLenGet();

	if(data_len < MAX_FRAME_SAMPLES) return;

//-----------SRC--------------------------------------------------------------//
	samples = data_len/MAX_FRAME_SAMPLES;
	for(i = 0; i < samples; i++)
	{
		data_len = AudioI2S0_DataGet(&i2s_in->InTempBuf,MAX_FRAME_SAMPLES);
#ifdef  CFG_FUNC_I2S0IN_SRC_EN
        #ifdef I2S_SRC_1_EN
		data_len = resampler_apply(i2s_in->ResampleObj, (int16_t *)i2s_in->InTempBuf, (int16_t*)i2s_in->OutTempBuf, MAX_FRAME_SAMPLES);
        #else
		data_len = resampler_polyphase_apply(i2s_in->ResampleObj, (int16_t *)i2s_in->InTempBuf, (int16_t*)i2s_in->OutTempBuf, MAX_FRAME_SAMPLES);
        #endif
#else
		memcpy(&i2s_in->OutTempBuf,(uint8_t *)&i2s_in->InTempBuf,data_len*4);
#endif //end if CFG_FUNC_I2S_IN_SRA_EN


#ifdef CFG_FUNC_I2S0IN_SRA_EN

		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_in->SraFifo, &i2s_in->OutTempBuf, data_len*4);
		}

#else
		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_in->OutFifo, &i2s_in->OutTempBuf, data_len*4);
		}
		else
		{
			MCUCircular_PutData(&i2s_in->OutFifo, &i2s_in->InTempBuf, MAX_FRAME_SAMPLES*4);
		}
#endif///end of CFG_FUNC_I2S_IN_SRA_EN
	}
//-------SRA----------------------------------------------------------------------//
#ifdef CFG_FUNC_I2S0IN_SRA_EN

	uint32_t one_fifo = RESAMPLE_FIFO_LEN/8;

	samples = MCUCircular_GetDataLen(&i2s_in->SraFifo)/4;//audio剩余数据

	if(samples < SRA_BLOCK)
	{
		return;
	}
	samples /= SRA_BLOCK;

	//space_len = MCUCircular_GetSpaceLen(&i2s_in->InFifo)/8;//audio剩余空间
	//DBG("fifo:%d    space:%d\n",one_fifo,space_len);
    for(i = 0; i < samples; i++)
    {
    	space_len = MCUCircular_GetSpaceLen(&i2s_in->OutFifo);//audio剩余空间

		n_inc_dec_o = 0;

		if((space_len >= one_fifo*6)&& (space_len < one_fifo*8))
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
        /////此处可增加定时处理，可以调整微调的速度
		 if(timer==0)
		 {
			 timer = 60;///60*6ms= 360ms
			 n_inc_dec_o = 0;
		 }
		 else
		 {
			 timer--;
		 }

		MCUCircular_GetData(&i2s_in->SraFifo,(uint8_t *)&i2s_in->InTempBuf,SRA_BLOCK*4); //

		if(sra_apply(&i2s_in->SraObj, (int16_t *)i2s_in->InTempBuf, (int16_t *)i2s_in->OutTempBuf, n_inc_dec_o)==SRA_ERROR_OK)//
		{
			SRCDoneLen = SRA_BLOCK + n_inc_dec_o;
		}
		else
		{
			SRCDoneLen = SRA_BLOCK;
			memcpy(&i2s_in->OutTempBuf,(uint8_t *)&i2s_in->InTempBuf,SRCDoneLen*4);
		}
//------------Put Data---------------------------------------------------------------------------------//
		MCUCircular_PutData(&i2s_in->OutFifo, &i2s_in->OutTempBuf, SRCDoneLen*4);
     }
#endif
//------------------------//
#endif//
}

void AudioI2S1_DataInProcess(void)
{
#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)
	static uint8_t timer = 0;
	int32_t data_len;
	uint16_t samples;
	uint32_t i;
	uint16_t space_len;
	int32_t n_inc_dec_o;
	int32_t SRCDoneLen = 0;
	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_IN;
#ifdef  CFG_FUNC_I2S1IN_SRC_EN
	if(i2s_in->ResampleObj==NULL) return;
#endif
	data_len = AudioI2S1_DataLenGet();

	if(data_len < MAX_FRAME_SAMPLES) return;

//-----------SRC--------------------------------------------------------------//
	samples = data_len/MAX_FRAME_SAMPLES;
	for(i = 0; i < samples; i++)
	{
		data_len = AudioI2S1_DataGet(&i2s_in->InTempBuf,MAX_FRAME_SAMPLES);
#ifdef  CFG_FUNC_I2S1IN_SRC_EN
        #ifdef I2S_SRC_1_EN
		data_len = resampler_apply(i2s_in->ResampleObj, (int16_t*)i2s_in->InTempBuf, (int16_t*)i2s_in->OutTempBuf, MAX_FRAME_SAMPLES);
        #else
        data_len = resampler_polyphase_apply(i2s_in->ResampleObj, (int16_t *)i2s_in->InTempBuf, (int16_t*)i2s_in->OutTempBuf, MAX_FRAME_SAMPLES);
        #endif
#else
        memcpy(&i2s_in->OutTempBuf,(uint8_t *)&i2s_in->InTempBuf,data_len*4);
#endif //end if CFG_FUNC_I2S_IN_SRA_EN

#ifdef CFG_FUNC_I2S1IN_SRA_EN
		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_in->SraFifo, &i2s_in->OutTempBuf, data_len*4);
		}
		else
		{
			MCUCircular_PutData(&i2s_in->SraFifo, &i2s_in->OutTempBuf, MAX_FRAME_SAMPLES*4);
		}
#else
		if(data_len > 0)
		{
			MCUCircular_PutData(&i2s_in->OutFifo, &i2s_in->OutTempBuf, data_len*4);
		}
		else
		{
			MCUCircular_PutData(&i2s_in->OutFifo, &i2s_in->InTempBuf, MAX_FRAME_SAMPLES*4);
		}
#endif///end of CFG_FUNC_I2S_IN_SRA_EN
	}
//-------SRA----------------------------------------------------------------------//
#ifdef CFG_FUNC_I2S1IN_SRA_EN

	uint32_t one_fifo = RESAMPLE_FIFO_LEN/8;

	samples = MCUCircular_GetDataLen(&i2s_in->SraFifo)/4;//audio剩余数据

	if(samples < SRA_BLOCK)
	{
		return;
	}
	samples /= 128;

	//space_len = MCUCircular_GetSpaceLen(&i2s_in->InFifo)/8;//audio剩余空间
	//DBG("fifo:%d    space:%d\n",one_fifo,space_len);
    for(i = 0; i < samples; i++)
    {
    	space_len = MCUCircular_GetSpaceLen(&i2s_in->OutFifo);//audio剩余空间

		n_inc_dec_o = 0;

		if((space_len >= one_fifo*6)&& (space_len < one_fifo*8))
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
        /////此处可增加定时处理，可以调整微调的速度
		if(timer==0)
		{
			timer = 60;///60*6ms= 360ms
			n_inc_dec_o = 0;
		}
		else
		{
			timer--;
		}

		MCUCircular_GetData(&i2s_in->SraFifo,(uint8_t *)&i2s_in->InTempBuf,SRA_BLOCK*4); //

		if(sra_apply(&i2s_in->SraObj, (int16_t *)i2s_in->InTempBuf, (int16_t *)i2s_in->OutTempBuf, n_inc_dec_o)==SRA_ERROR_OK)//
		{
			SRCDoneLen = SRA_BLOCK + n_inc_dec_o;
		}
		else
		{
			SRCDoneLen = SRA_BLOCK;
			memcpy(&i2s_in->OutTempBuf,(uint8_t *)&i2s_in->InTempBuf,SRCDoneLen*4);
		}
//------------Put Data---------------------------------------------------------------------------------//
		MCUCircular_PutData(&i2s_in->OutFifo, &i2s_in->OutTempBuf, SRCDoneLen*4);
     }
#endif
//------------------------//
#endif//
}
/*
****************************************************************
*
*i2s0  data
*
****************************************************************
*/
uint16_t AudioI2S0_DataGetX(void* Buf, uint16_t Len)
{
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)

	uint16_t Length;

	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_IN;

	if(Buf==NULL)
	{
		return 0;
	}

	Length = MCUCircular_GetDataLen(&i2s_in->OutFifo);////only i2s0,audio剩余数据

	if(Length > Len * 4)
	{
		Length = Len * 4;
	}

    Len = MCUCircular_GetData(&i2s_in->OutFifo, Buf, Length);
#endif
    return Len/4;

}

uint16_t AudioI2S0_DataLenGetX(void)
{
#if defined(CFG_FUNC_I2S0IN_SRC_EN) || defined(CFG_FUNC_I2S0IN_SRA_EN)
	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_IN;

	return MCUCircular_GetDataLen(&i2s_in->OutFifo)/4;
#endif
	return 0;

}

uint16_t AudioI2S0_DataSetX(void *Buf, uint16_t Len)
{
#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)
	MODULE_SRC_SRA *i2s_out;

	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_OUT;

	MCUCircular_PutData(&i2s_out->InFifo, Buf, Len*4);
#endif
	return Len;
}

uint16_t AudioI2S0_DataSpaceLenGetX(void)
{
	uint16_t Len=0;
#if defined(CFG_FUNC_I2S0OUT_SRC_EN) || defined(CFG_FUNC_I2S0OUT_SRA_EN)
	MODULE_SRC_SRA *i2s_out;

	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S0_OUT;

	Len = MCUCircular_GetSpaceLen(&i2s_out->InFifo);
#endif
    return  Len/4;

}
/*
****************************************************************
*
*i2s1  data
*
****************************************************************
*/
uint16_t AudioI2S1_DataGetX(void* Buf, uint16_t Len)
{
#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)

	uint16_t Length;

	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_IN;

	if(Buf==NULL)
	{
		return 0;
	}

	Length = MCUCircular_GetDataLen(&i2s_in->OutFifo);////only i2s0,audio剩余数据

	if(Length > Len * 4)
	{
		Length = Len * 4;
	}

    Len = MCUCircular_GetData(&i2s_in->OutFifo, Buf, Length);
#endif
    return Len/4;

}

uint16_t AudioI2S1_DataLenGetX(void)
{
#if defined(CFG_FUNC_I2S1IN_SRC_EN) || defined(CFG_FUNC_I2S1IN_SRA_EN)
	MODULE_SRC_SRA *i2s_in;

	i2s_in = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_IN;

	return MCUCircular_GetDataLen(&i2s_in->OutFifo)/4;
#endif
	return 0;

}
uint16_t AudioI2S1_DataSetX(void *Buf, uint16_t Len)
{
#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)
	MODULE_SRC_SRA *i2s_out;

	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_OUT;

	MCUCircular_PutData(&i2s_out->InFifo, Buf, Len*4);
#endif
	return Len;
}

uint16_t AudioI2S1_DataSpaceLenGetX(void)
{
	uint16_t Len = 0;
#if defined(CFG_FUNC_I2S1OUT_SRC_EN) || defined(CFG_FUNC_I2S1OUT_SRA_EN)
	MODULE_SRC_SRA *i2s_out;

	i2s_out = (MODULE_SRC_SRA *)&SRC_SRA_I2S1_OUT;

	Len = MCUCircular_GetSpaceLen(&i2s_out->InFifo);
#endif
    return  Len/4;

}
//-------------------------------------------------//
#endif///end of #ifdef CFG_FUNC_I2S_MIX_MODE
