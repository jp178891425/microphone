/**
 **************************************************************************************
 * @file    audio_aec.c
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2021-3-12 15:42:47$
 *
 * @Copyright (C) 2021, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "string.h"
#include "type.h"
#include "app_config.h"
#include "audio_aec.h"
#include "ctrlvars.h"
#include "rtos_api.h"
#include "mvstdio.h"
#include "debug.h"
#include "mode_switch_api.h"
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
/*******************************************************************************
 * AEC
 * 用于进行AEC的缓存远端发送数据
 * uint:sample
 ******************************************************************************/
void Audio_AECEffectInit(void)
{
}
#ifdef BT_HFP_MIC_PITCH_SHIFTER_FUNC
/*******************************************************************************
 * pitch shifter
 * 变调
 ******************************************************************************/
const int32_t btHfPitchShifterTable[MAX_PITCH_SHIFTER_STEP] =
{
	0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120,
	-120, -110, -100, -90, -80, -70, -60, -50, -40, -30, -20, -10
};

void BtHf_PitchShifterEffectInit(void)
{
}

void BtHf_PitchShifterEffectConfig(uint8_t step)
{
#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN
	gCtrlVars.pitch_shifter_unit.semitone_steps = btHfPitchShifterTable[step];

	AudioEffectPitchShifterConfig(&gCtrlVars.pitch_shifter_unit);
#endif
}
#endif

bool Audio_AECInit(AudioAECContext* AecCt)
{
	if(!AecCt->AecDelayBuf)
		AecCt->AecDelayBuf = (uint8_t*)osPortMalloc(AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK);  //64*2*32

	if(AecCt->AecDelayBuf == NULL)
	{
		return FALSE;
	}
	memset(AecCt->AecDelayBuf, 0, (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK));

	AecCt->AecDelayRingBuf.addr = AecCt->AecDelayBuf;
	AecCt->AecDelayRingBuf.mem_capacity = (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK);
	AecCt->AecDelayRingBuf.mem_len = (AEC_BLK_LEN*LEN_PER_SAMPLE*DEFAULT_DELAY_BLK);
#ifdef CFG_APP_USB_PHONE_MODE_EN
	extern uint32_t GetSystemMode(void);
	if((GetSystemMode() == AppModeUsbPhone))
	{
		AecCt->AecDelayRingBuf.mem_len = (AEC_BLK_LEN*LEN_PER_SAMPLE*USB_DEFAULT_DELAY_BLK);
	}
#endif
	AecCt->AecDelayRingBuf.p = 0;
	//AecCt->AecDelayRingBuf.cb = NULL;
	AecCt->SourceBuf_Aec = (uint16_t*)osPortMalloc(CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2);
	if(AecCt->SourceBuf_Aec == NULL)
	{
		return FALSE;
	}
	memset(AecCt->SourceBuf_Aec, 0, CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2);
	return TRUE;
}

void Audio_AECReset(AudioAECContext* AecCt, uint32_t DelayBlock)
{
	memset(AecCt->AecDelayBuf, 0, (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK));

	AecCt->AecDelayRingBuf.addr = AecCt->AecDelayBuf;

	AecCt->AecDelayRingBuf.mem_capacity = (AEC_BLK_LEN*LEN_PER_SAMPLE*MAX_DELAY_BLOCK);
	AecCt->AecDelayRingBuf.mem_len = AEC_BLK_LEN*LEN_PER_SAMPLE*DelayBlock;

	AecCt->AecDelayRingBuf.p = 0;
	//AecCt->AecDelayRingBuf.cb = NULL;

	memset(AecCt->SourceBuf_Aec, 0, CFG_BTHF_PARA_SAMPLES_PER_FRAME * 2);
}

void Audio_AECDeinit(AudioAECContext* AecCt)
{
	if(AecCt->SourceBuf_Aec)
	{
		osPortFree(AecCt->SourceBuf_Aec);
		AecCt->SourceBuf_Aec = NULL;
	}

	AecCt->AecDelayRingBuf.addr = NULL;
	AecCt->AecDelayRingBuf.mem_capacity = 0;
	AecCt->AecDelayRingBuf.mem_len = 0;
	AecCt->AecDelayRingBuf.p = 0;
	//AecCt->AecDelayRingBuf.cb = NULL;

	if(AecCt->AecDelayBuf)
	{
		osPortFree(AecCt->AecDelayBuf);
		AecCt->AecDelayBuf = NULL;
	}

}


uint32_t Audio_AECRingDataSet(AudioAECContext* AecCt, void *InBuf, uint16_t InLen)
{
	if(InLen == 0)
		return 0;

	return mv_mwrite(InBuf, 1, InLen*2, &AecCt->AecDelayRingBuf);
}

uint32_t Audio_AECRingDataGet(AudioAECContext* AecCt, void* OutBuf, uint16_t OutLen)
{
	if(OutLen == 0)
		return 0;

	return mv_mread(OutBuf, 1, OutLen*2, &AecCt->AecDelayRingBuf);
}

int32_t Audio_AECRingDataSpaceLenGet(AudioAECContext* AecCt)
{
	return mv_mremain(&AecCt->AecDelayRingBuf)/2;
}

int32_t Audio_AECRingDataLenGet(AudioAECContext* AecCt)
{
	return mv_msize(&AecCt->AecDelayRingBuf)/2;
}

int16_t *Audio_AecInBuf(AudioAECContext* AecCt, uint16_t OutLen)
{
	if(Audio_AECRingDataLenGet(AecCt) >= OutLen)
	{
		Audio_AECRingDataGet(AecCt, AecCt->SourceBuf_Aec , OutLen);
	}
	else
	{
		memset(AecCt->SourceBuf_Aec, 0, OutLen * 2);
	}
	return (int16_t *)AecCt->SourceBuf_Aec;
}
#else
void Audio_AECEffectInit(void)
{
}
bool Audio_AECInit(AudioAECContext* AecCt)
{
}
void Audio_AECDeinit(AudioAECContext* AecCt)
{
}
void Audio_AECReset(AudioAECContext* AecCt, uint32_t DelayBlock)
{
}
#endif
