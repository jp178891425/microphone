#include <string.h>
#include <nds32_intrinsic.h>
#include "debug.h"
#include "app_config.h"
#include "ctrlvars.h"
#include "audio_effect.h"
#include "audio_effect_library.h"
#include "main_task.h"
#include "math.h"
#include "stdlib.h"

#if defined(CFG_FUNC_ECHO_DENOISE)||defined(CFG_FUNC_EQMODE_FADIN_FADOUT_EN)||(CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN)||(CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN) \
	|| defined(CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN)
    extern int16_t*  EchoAudioBuf;
#endif

#if (CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN)
    extern int16_t*    DynamicEQWathcBuf;
#endif

extern uint8_t MixAudioFlag;
//int16_t *echo_tmp   	= (int16_t *)pcm_buf_1;
extern int16_t *pcm_buf_7;//可优化的变量数组??
/*
****************************************************************
* 卡拉OK 功能时，多路混合音源的音效处理主函数
*
*
****************************************************************
*/
void AudioInputMix(MixInputUnit *mix_pcm)
{
	uint16_t  s;
	uint16_t n = mainAppCt.SamplesPreFrame;
	int16_t *Silence_pcm  =(int16_t *)pcm_buf_7;
    int16_t *music_in =  *mix_pcm->music_in;
#ifdef CFG_FUNC_LINE_MIX_MODE
	int16_t *line_in = *mix_pcm->line_in;
#endif

#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_RES_AUDIO_USB_IN_EN
	 int16_t *usb_in = *mix_pcm->usb_in;
#endif
#endif

#ifdef CFG_RES_AUDIO_I2S0IN_EN
#ifndef CFG_RES_MICIN_BY_I2SIN
	int16_t *i2s0_in = *mix_pcm->i2s0_in;
#endif
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	int16_t *i2s1_in = *mix_pcm->i2s1_in;
#endif

#ifdef CFG_FUNC_REMIND_MIX_MODE
	int16_t *remind_mix_in = *mix_pcm->remind_mix_in;
#endif

#ifdef CFG_FUNC_SPDIF_MIX_MODE
	int16_t *spdif_in = *mix_pcm->spdif_in;
#endif

//-----------silenc detector--------------------------------------//
#if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN &&	CFG_FUNC_MIX_AUDIO_SDCT_EN
	if(Silence_pcm)
	{
		if(music_in)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = music_in[2*s + 0];
				Silence_pcm[2*s + 1] = music_in[2*s + 1];
			}
		}
#ifdef CFG_FUNC_LINE_MIX_MODE
		if(line_in)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)line_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)line_in[2*s + 1])), 16-1);
			}
		}
#endif
#ifdef CFG_FUNC_USB_MIX_MODE
		if(usb_in)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)usb_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)usb_in[2*s + 1])), 16-1);
			}
		}
#endif
#ifdef CFG_RES_AUDIO_I2S0IN_EN
		if(i2s0_in)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)i2s0_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)i2s0_in[2*s + 1])), 16-1);
			}
		}
#endif
#ifdef CFG_RES_AUDIO_I2S1IN_EN
		if(i2s1_in)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)i2s1_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)i2s1_in[2*s + 1])), 16-1);
			}
		}
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
		if(spdif_in)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)spdif_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)spdif_in[2*s + 1])), 16-1);
			}
		}
#endif
      AudioEffectSilenceDectorApply(&gCtrlVars.MusicAudioSdct_unit,  Silence_pcm,  n);
  }
#endif
//--------audio process-------------------------------------------------//
	if(music_in && MixAudioFlag)
	{
		AudioCoreAppSourceVolSet(APP_SOURCE_NUM, music_in, n, 2);
	}
//-------------------------------------------------//

#ifdef CFG_FUNC_LINE_MIX_MODE
	if(line_in)
	{
		//----- effect preprocess-----------//
		AudioCoreAppSourceVolSet(LINE_SOURCE_NUM, line_in, n, 2);
		//----------------------------------//
		#if (CFG_FUNC_LINE_MIX_EN == 1)
		if(music_in)
		{
			for(s = 0; s < n; s++)
			{
				music_in[2*s + 0] = __nds32__clips((((int32_t)music_in[2*s + 0] + (int32_t)line_in[2*s + 0])), 16-1);
				music_in[2*s + 1] = __nds32__clips((((int32_t)music_in[2*s + 1] + (int32_t)line_in[2*s + 1])), 16-1);
			}
		}
		else
		{
			music_in = line_in;
			*mix_pcm->music_in = *mix_pcm->line_in;
		}
		*mix_pcm->line_in = 0;
		#endif
	}
#endif

#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_RES_AUDIO_USB_IN_EN
	if(usb_in)
	{
		//-----effect preprocess-----------//
#if CFG_AUDIO_EFFECT_USB_VOCAL_CUT_EN
		if(gCtrlVars.vocal_cut_unit.vocal_cut_en)
			AudioEffectVocalCutApply(&gCtrlVars.usb_vocal_cut_unit, p->usb_in, p->usb_in, n);
#endif

#if CFG_AUDIO_EFFECT_USB_IN_GAIN_CONTROL_EN
#ifdef CFG_FUNC_SHUNNING_EN
		uint32_t gain;
		gain = gCtrlVars.usb_gain_control_unit.gain;
		gCtrlVars.usb_gain_control_unit.gain = gCtrlVars.usb_in_dyn_gain;//闪避功能打开时用到
#endif
		AudioEffectPregainApply(&gCtrlVars.usb_gain_control_unit, usb_in, usb_in, n, 2);
#ifdef CFG_FUNC_SHUNNING_EN
		gCtrlVars.usb_gain_control_unit.gain = gain;
#endif
#endif
		AudioCoreAppSourceVolSet(USB_MIX_SOURCE_NUM, usb_in, n, 2);
		//----------------------------------//
		#if (CFG_FUNC_USB_MIX_EN == 1)
		if(music_in)
		{
			for(s = 0; s < n; s++)
			{
				music_in[2*s + 0] = __nds32__clips((((int32_t)music_in[2*s + 0] + (int32_t)usb_in[2*s + 0])), 16-1);
				music_in[2*s + 1] = __nds32__clips((((int32_t)music_in[2*s + 1] + (int32_t)usb_in[2*s + 1])), 16-1);
			}
		}
		else
		{
			music_in = usb_in;
			*mix_pcm->music_in = *mix_pcm->usb_in;
		}
		*mix_pcm->usb_in =0;
		#endif
	}
#endif
#endif///end of defined(CFG_FUNC_USB_MIX_MODE)///

#ifdef CFG_RES_AUDIO_I2S0IN_EN
#ifndef CFG_RES_MICIN_BY_I2SIN
	if(i2s0_in)
	{
		//----- effect preprocess-----------//

		AudioCoreAppSourceVolSet(I2S0_SOURCE_NUM, i2s0_in, n, 2);
		//----------------------------------//
		if(mix_pcm->i2s0_mix_en)
		{
			if(music_in)
			{
			  for(s = 0; s < n; s++)
			   {
				   music_in[2*s + 0] = __nds32__clips((((int32_t)music_in[2*s + 0] + (int32_t)i2s0_in[2*s + 0])), 16-1);
				   music_in[2*s + 1] = __nds32__clips((((int32_t)music_in[2*s + 1] + (int32_t)i2s0_in[2*s + 1])), 16-1);
			   }
			}
			else
			{
				music_in = i2s0_in;
				*mix_pcm->music_in = *mix_pcm->i2s0_in;
			}
		  *mix_pcm->i2s0_in =0;
		}
		else
		{
			//-----add effect process-----------//

			//----------------------------------//
		}
	}
#endif
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	if(i2s1_in)
	{
		if(Silence_pcm && CFG_FUNC_MIX_AUDIO_SDCT_EN)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)i2s1_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)i2s1_in[2*s + 1])), 16-1);
			}
		}

		//----- effect preprocess-----------//
		AudioCoreAppSourceVolSet(I2S1_SOURCE_NUM, i2s1_in, n, 2);
		//----------------------------------//

		if(mix_pcm->i2s1_mix_en)
		{
			if(music_in)
			{
				for(s = 0; s < n; s++)
				{
					music_in[2*s + 0] = __nds32__clips((((int32_t)music_in[2*s + 0] + (int32_t)i2s1_in[2*s + 0])), 16-1);
					music_in[2*s + 1] = __nds32__clips((((int32_t)music_in[2*s + 1] + (int32_t)i2s1_in[2*s + 1])), 16-1);
				}
			}
			else
			{
				music_in = i2s1_in;
				*mix_pcm->music_in = *mix_pcm->i2s1_in;
			}
			*mix_pcm->i2s1_in =0;
		}
		else
		{
			//-----add effect process-----------//

			//----------------------------------//
		}
	}
#endif

#ifdef CFG_FUNC_REMIND_MIX_MODE
	if(remind_mix_in)
	{
		if(Silence_pcm && CFG_FUNC_MIX_AUDIO_SDCT_EN)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)remind_mix_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)remind_mix_in[2*s + 1])), 16-1);
			}
		}
		//----- effect preprocess-----------//
		if(gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND)
		{
		    //get background remind data for record
			#ifdef CFG_FUNC_SHUNNING_EN
			uint32_t gain;
			gain = gCtrlVars.remind_effect_gain_control_unit.gain;
			gCtrlVars.remind_effect_gain_control_unit.gain = gCtrlVars.remind_out_dyn_gain/2;//闪避功能打开时用到
			#endif
			AudioEffectPregainApply(&gCtrlVars.remind_effect_gain_control_unit, remind_mix_in, remind_mix_in, n, 2);
			#ifdef CFG_FUNC_SHUNNING_EN
			gCtrlVars.remind_effect_gain_control_unit.gain = gain;
			#endif
		}
	    else
    	{
			AudioEffectPregainApply(&gCtrlVars.remind_key_gain_control_unit, remind_mix_in, remind_mix_in, n, 2);
    	}

		AudioCoreAppSourceVolSet(MIX_REMIND_SOURCE_NUM, remind_mix_in, n, 2);

		#if defined(BT_TWS_SUPPORT) || (CFG_FUNC_REMIND_MIX_EN == 1)
		if(music_in)
		{
			for(s = 0; s < n; s++)
			{
				music_in[2*s + 0] = __nds32__clips((((int32_t)music_in[2*s + 0] + (int32_t)remind_mix_in[2*s + 0])), 16-1);
				music_in[2*s + 1] = __nds32__clips((((int32_t)music_in[2*s + 1] + (int32_t)remind_mix_in[2*s + 1])), 16-1);
			}
		}
		else
		{
			music_in = remind_mix_in;
			*mix_pcm->music_in = *mix_pcm->remind_mix_in;
		}
		*mix_pcm->remind_mix_in =0;
		#endif
	}
#endif


#ifdef CFG_FUNC_SPDIF_MIX_MODE
	if(spdif_in)
	{
		if(Silence_pcm && CFG_FUNC_MIX_AUDIO_SDCT_EN)
		{
			for(s = 0; s < n; s++)
			{
				Silence_pcm[2*s + 0] = __nds32__clips((((int32_t)Silence_pcm[2*s + 0] + (int32_t)spdif_in[2*s + 0])), 16-1);
				Silence_pcm[2*s + 1] = __nds32__clips((((int32_t)Silence_pcm[2*s + 1] + (int32_t)spdif_in[2*s + 1])), 16-1);
			}
		}

		//----- effect preprocess-----------//
		AudioCoreAppSourceVolSet(SPDIF_MIX_SOURCE_NUM, spdif_in, n, 2);
		//----------------------------------//

		if(mix_pcm->spdif_mix_en)
		{
			if(music_in)
			{
				for(s = 0; s < n; s++)
				{
					music_in[2*s + 0] = __nds32__clips((((int32_t)music_in[2*s + 0] + (int32_t)spdif_in[2*s + 0])), 16-1);
					music_in[2*s + 1] = __nds32__clips((((int32_t)music_in[2*s + 1] + (int32_t)spdif_in[2*s + 1])), 16-1);
				}
			}
			else
			{
				music_in = spdif_in;
				*mix_pcm->music_in = *mix_pcm->spdif_in;
			}
			*mix_pcm->spdif_in =0;
		}
		else
		{
			//-----add effect process-----------//

			//----------------------------------//
		}
	}
#endif
}

