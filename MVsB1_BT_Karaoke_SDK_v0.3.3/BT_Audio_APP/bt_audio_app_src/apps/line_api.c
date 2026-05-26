/*
 * line_api.c
 *
 *  Created on: Apr 13, 2020
 *      Author: szsj-1
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
#include "line_api.h"

#ifdef CFG_FUNC_LINE_MIX_MODE

void AudioLine_ResMalloc(uint16_t SampleLen)
{
	DBG("%s\n",__func__);
	uint16_t FifoLenStereo = SampleLen * 2 * 2 * 2;//蕾极汒8捷湮苤衾痋酗ㄛ等弇byte
	uint16_t AudioCoreBufLen = SampleLen * 2 * 2;//AudioCore諉諳歙偌蕾极汒懂揭燴

	if(mainAppCt.Source2Buf_Line==NULL)
	{
	  mainAppCt.Source2Buf_Line = (int16_t*)osPortMallocFromEnd(AudioCoreBufLen);// buff
	}

	if(mainAppCt.Source2Buf_Line != NULL)
	{
		memset(mainAppCt.Source2Buf_Line, 0, AudioCoreBufLen);
	}
	else
	{
		DBG("Source2Buf_Line error\n");
	}

	if(mainAppCt.Fifo_Line==NULL)
	{
	  mainAppCt.Fifo_Line = (int16_t*)osPortMallocFromEnd(FifoLenStereo);// fifo
	}

	if(mainAppCt.Fifo_Line != NULL)
	{
		memset(mainAppCt.Fifo_Line, 0, FifoLenStereo);
	}
	else
	{
		DBG("Fifo_Line error\n");
	}
}

void AudioLine_Release(void)
{
	DBG("%s\n",__func__);

	AudioCoreSourceDisable(LINE_SOURCE_NUM);
	AudioADC_Disable(ADC0_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC0_RX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC0_RX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC0_RX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_ADC0_RX);

	if(mainAppCt.Source2Buf_Line != NULL)
	{
		DBG("free Source2Buf_Line\n");
		osPortFree(mainAppCt.Source2Buf_Line);
		mainAppCt.Source2Buf_Line = NULL;
	}
	if(mainAppCt.Fifo_Line != NULL)
	{
		DBG("free Fifo_Line\n");
		osPortFree(mainAppCt.Fifo_Line);
		mainAppCt.Fifo_Line = NULL;
	}

}


void AudioLine_ConfigInit(void)
{
	DBG("%s\n",__func__);
	if(!mainAppCt.Fifo_Line) return;
    mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].Enable = 0;
    mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].FuncDataGet = AudioLine_DataGet;
    mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].FuncDataGetLen = AudioLine_DataLenGet;
    mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].IsSreamData = TRUE;
    mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].PcmFormat = 2;//
    mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source2Buf_Line;

    AudioCoreSourceEnable(LINE_SOURCE_NUM);
    //AudioCoreSourceMute(LINE_SOURCE_NUM,1,1);
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_ADC0_RX, mainAppCt.SamplesPreFrame * 2 * 2, (void*)mainAppCt.Fifo_Line, mainAppCt.SamplesPreFrame * 2 * 2 * 2);
	if(AudioADC_IsOverflow(ADC0_MODULE))
	{
		AudioADC_OverflowClear(ADC0_MODULE);
	}
	AudioADC_Clear(ADC0_MODULE);
    DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_ADC0_RX);
    AudioADC_LREnable(ADC0_MODULE, 1, 1);
    AudioADC_Enable(ADC0_MODULE);
	uint32_t ch;
	ch = DMA_ChannelNumGet(PERIPHERAL_ID_AUDIO_ADC0_RX);
	if(ch > 7 )
	{
		DBG("ADC1 LINE DMA NOT OPEN!\n");
	}
}

/*
****************************************************************
*
*  data
*
****************************************************************
*/
uint16_t AudioLine_DataGet(void* Buf, uint16_t Len)
{
	if(Buf==NULL) return 0;

	Len = AudioADC0DataGet(Buf,Len);

    return Len;

}

uint16_t AudioLine_DataLenGet(void)
{
	uint16_t Len;
	Len = AudioADC0DataLenGet();
	return Len;

}

//-------------------------------------------------//
#endif///end of #ifdef CFG_FUNC_LINE_MIX_MODE



