/**
 **************************************************************************************
 * @file    audio_common.c
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2020-8-6 13:17:21$
 *
 * @Copyright (C) 2020, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "type.h"
#include "app_config.h"
#include "resampler_polyphase.h"
#include "debug.h"
static const int32_t SamplerRatio[][3] =
{
	{8000,  RESAMPLER_POLYPHASE_SRC_RATIO_441_80,	RESAMPLER_POLYPHASE_SRC_RATIO_6_1},
	{11025, RESAMPLER_POLYPHASE_SRC_RATIO_4_1,		RESAMPLER_POLYPHASE_SRC_RATIO_640_147},
	{12000, RESAMPLER_POLYPHASE_SRC_RATIO_147_40,	RESAMPLER_POLYPHASE_SRC_RATIO_4_1},
	{16000, RESAMPLER_POLYPHASE_SRC_RATIO_441_160,	RESAMPLER_POLYPHASE_SRC_RATIO_3_1},
	{22050, RESAMPLER_POLYPHASE_SRC_RATIO_2_1,		RESAMPLER_POLYPHASE_SRC_RATIO_320_147},
	{22500, RESAMPLER_POLYPHASE_SRC_RATIO_2_1,		RESAMPLER_POLYPHASE_SRC_RATIO_320_147},
	{24000, RESAMPLER_POLYPHASE_SRC_RATIO_147_80,	RESAMPLER_POLYPHASE_SRC_RATIO_2_1},
	{32000, RESAMPLER_POLYPHASE_SRC_RATIO_441_320, 	RESAMPLER_POLYPHASE_SRC_RATIO_3_2},
	{44100, 0,										RESAMPLER_POLYPHASE_SRC_RATIO_160_147},
	{48000, RESAMPLER_POLYPHASE_SRC_RATIO_147_160,	0},
	{88200, RESAMPLER_POLYPHASE_SRC_RATIO_1_2,		RESAMPLER_POLYPHASE_SRC_RATIO_80_147},
	{96000, RESAMPLER_POLYPHASE_SRC_RATIO_147_320,	RESAMPLER_POLYPHASE_SRC_RATIO_1_2},
	{176400,RESAMPLER_POLYPHASE_SRC_RATIO_1_4,		RESAMPLER_POLYPHASE_SRC_RATIO_40_147},
	{192000,RESAMPLER_POLYPHASE_SRC_RATIO_147_640,	RESAMPLER_POLYPHASE_SRC_RATIO_1_4},
	{33075, RESAMPLER_POLYPHASE_SRC_RATIO_4_3,		RESAMPLER_POLYPHASE_SRC_RATIO_4_3},
};

int32_t Get_Resampler_Polyphase(uint32_t resampler)
{
	int32_t res = 0;
	uint32_t i;
	for(i=0; i<15; i++)
	{
		if(SamplerRatio[i][0] == resampler)
		{
			if(CFG_PARA_SAMPLE_RATE == 44100)
			{
				res = SamplerRatio[i][1];
				break;
			}
			else if(CFG_PARA_SAMPLE_RATE == 48000)
			{
				res = SamplerRatio[i][2];
				break;
			}
			else
			{
				res = 0;
			}
		}
	}
	//APP_DBG("res = %d\n", res);		
	return res;
}
/*********************************************************
 *
 *
 *
 *********************************************************
 */
RESAMPLER_POLYPHASE_SRC_RATIO GetRatioEnum(uint32_t Scale1000);

int32_t Get_Resampler_Polyphase_new(uint32_t SourceSampleRate,uint32_t TargetSampleRate)
{
	RESAMPLER_POLYPHASE_SRC_RATIO ratio;

	ratio =GetRatioEnum((TargetSampleRate*1000)/SourceSampleRate);

	if(ratio == RESAMPLER_POLYPHASE_SRC_RATIO_UNSUPPORTED)
	{
		DBG("Get_Resampler_Polyphase Err\n");
		return 0;
	}

	return ratio;

}

#define AUDIO_CORE_DEBUG 	//强化配置安全检测和定位

//resampler_polyphase_init(DecoderServiceCt.ResamplerCt, audio_decoder->song_info.num_channels, GetRatioEnum((1000 * CFG_BTHF_PARA_SAMPLE_RATE) / audio_decoder->song_info.sampling_rate));
//Scale1000 = (1000 * CFG_BTHF_PARA_SAMPLE_RATE) / audio_decoder->song_info.sampling_rate)
//Scale1000 = ((1000 * target samplerate) / source samplerate) )
RESAMPLER_POLYPHASE_SRC_RATIO GetRatioEnum(uint32_t Scale1000)
{
	switch(Scale1000)
	{
		case (1000 * 6):
			return RESAMPLER_POLYPHASE_SRC_RATIO_6_1;
		case (1000 * 441 / 80):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_80;
		case (1000 * 640 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_640_147;
		case (1000 * 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_4_1;
		case (1000 * 147 / 40):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_40;
		case (1000 * 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_1;
		case (1000 * 441 / 160):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_160;
		case (1000 * 320 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_320_147;
		case (1000 * 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_2_1;
		case (1000 * 147 / 80):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_80;
		case (1000 * 3 / 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_2;
		case (1000 * 441 / 320):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_320;
		case (1000 * 4 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_4_3;
		case (1000 * 160 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_160_147;
/*****************************************************************/
		case (1000 * 147 / 160):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_160;
		case (1000 * 3 / 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_4;
		case (1000 * 80 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_80_147;
		case (1000 / 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_2;
		case (1000 * 147 / 320):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_320;
		case (1000 * 160 / 441):
			return RESAMPLER_POLYPHASE_SRC_RATIO_160_441;
		case (1000 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_3;
		case (1000 * 40 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_40_147;
		case (1000 / 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_4;
		case (1000 * 147 / 640):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_640;
		case (1000 * 320 / 441):
			return RESAMPLER_POLYPHASE_SRC_RATIO_320_441;
		case (1000 * 2 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_2_3;
		default:
#ifdef AUDIO_CORE_DEBUG
			if(Scale1000 != 1000)
			{
				APP_DBG("SRC Samplerate Error！\n");
			}
#endif
			return RESAMPLER_POLYPHASE_SRC_RATIO_UNSUPPORTED;
	}
}



