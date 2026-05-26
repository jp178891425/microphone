#include <string.h>
#include <nds32_intrinsic.h>
#include "debug.h"
#include "app_config.h"
#include "ctrlvars.h"
#include "watchdog.h"
#include "rtos_api.h"
#include "audio_effect.h"
#include "audio_effect_library.h"
#include "communication.h"
#include "audio_adc.h"
#include "main_task.h"
#include "math.h"
#include "bt_config.h"
#include "bt_hf_api.h"
#include "audio_aec.h"
#include "stdlib.h"

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
extern bool GetWhetherRecMusic(void);
extern uint8_t DecoderTaskState;

#if defined(CFG_FUNC_ECHO_DENOISE)||defined(CFG_FUNC_EQMODE_FADIN_FADOUT_EN)||(CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN)||(CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN) \
		|| defined(CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN)
    extern   int16_t*          EchoAudioBuf;
    #define  MusicVolBuf       EchoAudioBuf
    #define  EqModeAudioBuf    EchoAudioBuf
    #define  DynamicEQBuf      EchoAudioBuf
    #define  VirSurroundBuf    EchoAudioBuf
#endif

#if (CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN)
    extern int16_t*    DynamicEQWathcBuf;
#endif

#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
EQContext EqBufferBak;
#endif

#define SCALING_Q20_MAX (0x100000)
#define SCALING_Q20_MAX_HALF (SCALING_Q20_MAX/2)

#define SCALING_Q15_MAX (0x8000)
#define SCALING_Q15_MAX_HALF (SCALING_Q15_MAX/2)

#define SCALING_Q12_MAX (0x1000)
#define SCALING_Q12_MAX_HALF (SCALING_Q12_MAX/2)

#define SCALING_Q8_MAX (0x100)
#define SCALING_Q8_MAX_HALF (SCALING_Q8_MAX/2)

uint32_t roboeffect_db_to_scaling(float db, uint32_t scaling_max)
{
        // printf("-->%0.2f\n", db);
        return (uint32_t)roundf(powf(10.0f,((float)db/20.0f)) * scaling_max);
}

void du_efft_fadein_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch)
{
	int16_t* temp = (int16_t *)pcm_in;
	int32_t n = 0, w = 0, step = 32768/pcm_length;

	if(pcm_in == NULL) return;

	if(ch == 2)
	{
		for(n = 0;	n < pcm_length; n++)
		{
			temp[n * 2] = (int16_t)(((int32_t)temp[n * 2] * w + 16384) >> 15);
			temp[n * 2 + 1] = (int16_t)(((int32_t)temp[n * 2 + 1] * w + 16384) >> 15);
			w += step;
		}
	}
	else if(ch == 1)
	{
		for(n = 0;	n < pcm_length; n++)
		{
			temp[n] = (int16_t)(((int32_t)temp[n] * w + 16384) >> 15);
			w += step;
		}
	}
}

void du_efft_fadeout_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch)
{
	int16_t* temp = (int16_t *)pcm_in;
	int32_t n = 0, w = (32768/pcm_length)*(pcm_length-1), step = 32768/pcm_length;

	if(pcm_in == NULL) return;

	if(ch == 2)
	{
		for(n = 0; n < pcm_length; n++)
		{
			temp[n * 2] = (int16_t)(((int32_t)temp[n * 2] * w + 16384) >> 15);
			temp[n * 2 + 1] = (int16_t)(((int32_t)temp[n * 2 + 1] * w + 16384) >> 15);
			w -= step;
		}
	}
	else if(ch == 1)
	{
		for(n = 0; n < pcm_length; n++)
		{
			temp[n] = (int16_t)(((int32_t)temp[n] * w + 16384) >> 15);
			w -= step;
		}
	}
}

void du_efft_fadein_sw24(int32_t* pcm_in, uint16_t pcm_length, uint16_t ch, uint16_t ch_ex)
{
        int32_t* temp = (int32_t *)pcm_in;
        int32_t n = 0,fade_counter = 0;

        if(ch == 2)
        {
                for(n = 0;        n < pcm_length; n++)
                {
                        int32_t w = (int64_t)(fade_counter < pcm_length ? fade_counter++ : pcm_length) * 32768 / pcm_length;
                        if(ch_ex & 0x01)
                                temp[n * 2] = (((int64_t)temp[n * 2] * w + 16384) >> 15);
                        if(ch_ex & 0x02)
                                temp[n * 2 + 1] = (((int64_t)temp[n * 2 + 1] * w + 16384) >> 15);
                }
        }
        else if(ch == 1)
        {
                for(n = 0;        n < pcm_length; n++)
                {
                        int32_t w = (int64_t)(fade_counter < pcm_length ? fade_counter++ : pcm_length) * 32768 / pcm_length;
                        temp[n] = (((int64_t)temp[n] * w + 16384) >> 15);
                }
        }
}


void du_efft_fadeout_sw24(int32_t* pcm_in, uint16_t pcm_length, uint16_t ch, uint16_t ch_ex)
{
        int32_t* temp = (int32_t *)pcm_in;
        int32_t n = 0, fade_counter = pcm_length;

        if(ch == 2)
        {
                for(n = 0; n < pcm_length; n++)
                {
                        int32_t w = (int64_t)(fade_counter ? --fade_counter : 0) * 32768 / pcm_length;
                        if(ch_ex & 0x01)
                                temp[n * 2] = (((int64_t)temp[n * 2] * w + 16384) >> 15);
                        if(ch_ex & 0x02)
                                temp[n * 2 + 1] = (((int64_t)temp[n * 2 + 1] * w + 16384) >> 15);
                }
        }
        else if(ch == 1)
        {
                for(n = 0; n < pcm_length; n++)
                {
                        int32_t w = (int64_t)(fade_counter ? --fade_counter : 0) * 32768 / pcm_length;
                        temp[n] = (((int64_t)temp[n] * w + 16384) >> 15);
                }
        }
}
/*
****************************************************************
* 关闭所有音效功能
*
*
****************************************************************
*/
void AudioEffectsAllDisable(void)
{
	uint16_t i;
    
	APP_DBG("AudioEffectsAllDisable \n");
	gCtrlVars.auto_tune_unit.enable 	  = 0;
	gCtrlVars.echo_unit.enable			  = 0;
	gCtrlVars.pitch_shifter_unit.enable   = 0;
	#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
	gCtrlVars.pitch_shifter_pro_unit.enable = 0;
	#endif
	gCtrlVars.voice_changer_unit.enable   = 0;
	#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
	gCtrlVars.voice_changer_pro_unit.enable = 0;
	#endif
	gCtrlVars.reverb_unit.enable		  = 0;
	gCtrlVars.plate_reverb_unit.enable	  = 0;
    #if CFG_AUDIO_EFFECT_PINGPONG_EN
    gCtrlVars.ping_pong_unit.enable       = 0;
    #endif
	#if CFG_AUDIO_EFFECT_FLANGER_EN
	gCtrlVars.flanger_uint.enable         = 0;
	gCtrlVars.flanger_uint.bit_width      = 0;
	#endif

	#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
	gCtrlVars.overdrive_unit.enable         =0;
	#endif

    #if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
    gCtrlVars.overdrive_poly_unit.enable   =0;
    #endif
	#if CFG_AUDIO_EFFECT_DISTORTION_EN
	gCtrlVars.distortion_unit.enable         =0;
	#endif

    #if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
    gCtrlVars.distortion_ds1_unit.enable         =0;
    #endif
	#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	gCtrlVars.pitch_detector_unit.enable   = 0;	
	#endif
	#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
	gCtrlVars.reverb_pro_unit.enable      = 0;
	#endif
	gCtrlVars.freq_shifter_unit.enable	  = 0;
	#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
	gCtrlVars.freq_shifter_fine_unit.enable = 0;
	#endif
	gCtrlVars.howling_dector_unit.enable  = 0;
    #if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
	gCtrlVars.howling_dector_fine_unit.enable = 0;
    #endif
    #if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
	gCtrlVars.howling_dector_specified_unit.enable = 0;
    #endif
	gCtrlVars.MicAudioSdct_unit.enable    = 0;
	gCtrlVars.MusicAudioSdct_unit.enable  = 0;
	#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
	gCtrlVars.music_threed_unit.enable    = 0;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
	gCtrlVars.music_threed_plus_unit.enable= 0;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
	gCtrlVars.music_vb_unit.enable        = 0;
	#endif
	#if CFG_AUDIO_EFFECT_REC_VIRTUAL_BASS_EN
	gCtrlVars.rec_vb_unit.enable          = 0;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
	gCtrlVars.music_vb_classic_unit.enable = 0;
	#endif
    #if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
	gCtrlVars.stereo_winden_unit.enable   = 0;
    #endif
    #if CFG_AUDIO_EFFECT_AUTOWAH_EN
    gCtrlVars.auto_wah_unit.enable        = 0;
    #endif
	#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
	gCtrlVars.music_delay_unit.enable     = 0;
	#endif
	gCtrlVars.music_exciter_unit.enable   = 0;	
	gCtrlVars.vocal_cut_unit.enable       = 0;
	gCtrlVars.vocal_remove_unit.enable    = 0;
	gCtrlVars.chorus_unit.enable          = 0;
	gCtrlVars.chorus2_unit.enable          = 0;
	gCtrlVars.phase_control_unit.enable   = 0;

    #if defined (CFG_APP_BT_MODE_EN) || defined(CFG_APP_USB_PHONE_MODE_EN)
    //#if (BT_HFP_SUPPORT == ENABLE)
	gCtrlVars.expander_for_aec_unit.enable   =   0;
	gCtrlVars.drc_for_aec_unit.enable        =   0;
	gCtrlVars.eq_for_aec_unit.enable         =   0;
	gCtrlVars.hfp_ns_unit.enable = 0;
	gCtrlVars.mic_aec_unit.enable = 0;

	gCtrlVars.aec_gain_control_unit.enable     = 0;
	gCtrlVars.aec_mic_in_gain_unit.enable     = 0;
	gCtrlVars.aec_phone_in_gain_unit.enable     = 0;
    //#endif
    #endif
	//expand
	for(i = 0; i < sizeof(expander_unit_aggregate)/sizeof(expander_unit_aggregate[0]); i++)
	{
		expander_unit_aggregate[i]->enable = 0;
	}
    //drc
	for(i = 0; i < sizeof(drc_unit_aggregate)/sizeof(drc_unit_aggregate[0]); i++)
	{
		drc_unit_aggregate[i]->enable		= 0;
	}

	#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
	for(i = 0; i < sizeof(drc_legacy_unit_init)/sizeof(drc_legacy_unit_init[0]); i++)
	{
		drc_legacy_unit_init[i]->enable		= 0;
	}
	#endif
	//eq
	for(i = 0; i < sizeof(eq_unit_aggregate)/sizeof(eq_unit_aggregate[0]); i++)
	{
		eq_unit_aggregate[i]->enable		= 0;
	}
    //gain control
	for(i = 0; i < sizeof(gain_unit_aggregate)/sizeof(gain_unit_aggregate[0]); i++)
	{
		gain_unit_aggregate[i]->enable		= 0;
	}
	//eq
	for(i = 0; i < sizeof(eq_unit_aggregate)/sizeof(eq_unit_aggregate[0]); i++)
	{
		eq_unit_aggregate[i]->enable 	= 0;
	}

    #if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
	for(i = 0; i < sizeof(eq_drc_unit)/sizeof(eq_drc_unit[0]); i++)
	{
		eq_drc_unit[i]->enable 	= 0;
	}
    #endif
	#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN
	gCtrlVars.mic_ns_unit.enable = 0;
	#endif

    #if CFG_AUDIO_EFFECT_COMPANDER_EN
	gCtrlVars.compander_unit.enable = 0;
    #endif

    #if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
    gCtrlVars.mic_low_level_compressor_unit.enable = 0;
    #endif

    #if CFG_AUDIO_EFFECT_BIQUAD_EN
    gCtrlVars.mic_low_level_compressor_unit.enable  = 0;
    #endif

    #if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
    gCtrlVars.mic_howling_guard_unit.enable  = 0;
    #endif

	#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
	gCtrlVars.rec_phase_shifter_unit.enable  = 0;
	#endif
    #if CFG_AUDIO_EFFECT_DRAPOST_EN
	gCtrlVars.dra_post_unit.enable        = 0;
    #endif
    #if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
    gCtrlVars.dc_blocker_unit.enable   = 0;
    #endif
	#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	gCtrlVars.virtual_surround_unit.enable   = 0;
	#endif

    #if CFG_AUDIO_EFFECT_NOISE_GENERATOR_EN
	gCtrlVars.noise_generator_unit.enable   = 0;
    #endif

    #if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
    gCtrlVars.music_butterworth_unit.enable         = 0;
    #endif

    #if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
    void DynmmicEq_Disable(DynamicEqUnit *unit);

    DynmmicEq_Disable(&gCtrlVars.music_dynamic_eq_unit);
    #endif

    #if CFG_AUDIO_EFFECT_LRBALANCER_EN
	gCtrlVars.music_lr_balancer.enable = 0;
	#endif
}

/*
****************************************************************
* 音效模块反初始化
*
*
****************************************************************
*/
void AudioEffectsDeInit(void)
{
	uint8_t i;
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
#endif
    APP_DBG("AudioEffectsDeInit \n");

	if(gCtrlVars.freq_shifter_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.freq_shifter_unit.ct);
	}
	#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
	if(gCtrlVars.freq_shifter_fine_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.freq_shifter_fine_unit.ct);
	}
	#endif
	if(gCtrlVars.howling_dector_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.howling_dector_unit.ct);
	}
   #if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
	if(gCtrlVars.howling_dector_fine_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.howling_dector_fine_unit.ct);
	}
   #endif

   #if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
	if(gCtrlVars.howling_dector_specified_unit.ct != NULL)
	 {
	   osPortFree(gCtrlVars.howling_dector_specified_unit.ct);
	 }
	#endif

	if(gCtrlVars.pitch_shifter_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.pitch_shifter_unit.ct);
	}
	#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
	if(gCtrlVars.pitch_shifter_pro_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.pitch_shifter_pro_unit.ct);
	}
	#endif
	if(gCtrlVars.auto_tune_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.auto_tune_unit.ct);
	}
	if(gCtrlVars.voice_changer_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.voice_changer_unit.ct);
	}
	#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
	if(gCtrlVars.voice_changer_pro_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.voice_changer_pro_unit.ct);
	}
	#endif
	if(gCtrlVars.echo_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.echo_unit.ct);
	}
	if(gCtrlVars.reverb_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.reverb_unit.ct);
	}
	if(gCtrlVars.plate_reverb_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.plate_reverb_unit.ct);
	}
    #if CFG_AUDIO_EFFECT_PINGPONG_EN
    if(gCtrlVars.ping_pong_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.ping_pong_unit.ct);
	}
    #endif

	#if CFG_AUDIO_EFFECT_FLANGER_EN
	if(gCtrlVars.flanger_uint.ct != NULL)
	{
		osPortFree(gCtrlVars.flanger_uint.ct);
	}
	#endif

	#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
	if(gCtrlVars.overdrive_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.overdrive_unit.ct);
	}
	#endif

    #if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
    if(gCtrlVars.overdrive_poly_unit.ct != NULL)
     {
	     osPortFree(gCtrlVars.overdrive_poly_unit.ct);
     }
    #endif

	#if CFG_AUDIO_EFFECT_DISTORTION_EN
	if(gCtrlVars.distortion_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.distortion_unit.ct);
	}
	#endif
    #if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
    if(gCtrlVars.distortion_ds1_unit.ct != NULL)
    {
	   osPortFree(gCtrlVars.distortion_ds1_unit.ct);
    }
    #endif
	#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	if(gCtrlVars.pitch_detector_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.pitch_detector_unit.ct);
	}
	AudioEffectPitchDetectorParameterInit();
	#endif
	#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
	if(gCtrlVars.reverb_pro_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.reverb_pro_unit.ct);
	}
	#endif
	if(gCtrlVars.MicAudioSdct_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.MicAudioSdct_unit.ct);
	}
	if(gCtrlVars.MusicAudioSdct_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.MusicAudioSdct_unit.ct);
	}
	if(gCtrlVars.vocal_cut_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.vocal_cut_unit.ct);
	}
	if(gCtrlVars.vocal_remove_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.vocal_remove_unit.ct);
	}
	#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
	if(gCtrlVars.music_threed_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.music_threed_unit.ct);
	}
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
	if(gCtrlVars.music_threed_plus_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.music_threed_plus_unit.ct);
	}
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
	if(gCtrlVars.music_vb_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.music_vb_unit.ct);
	}
	#endif
	#if CFG_AUDIO_EFFECT_REC_VIRTUAL_BASS_EN
	if(gCtrlVars.rec_vb_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.rec_vb_unit.ct);
	}
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
	if(gCtrlVars.music_vb_classic_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.music_vb_classic_unit.ct);
	}
	#endif
    #if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
    if(gCtrlVars.stereo_winden_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.stereo_winden_unit.ct);
	}
    #endif
    #if CFG_AUDIO_EFFECT_AUTOWAH_EN
    if(gCtrlVars.auto_wah_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.auto_wah_unit.ct);
	}
    #endif
	#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
	if(gCtrlVars.music_delay_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.music_delay_unit.ct);
	}
	#endif
	if(gCtrlVars.music_exciter_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.music_exciter_unit.ct);
	}
	if(gCtrlVars.chorus_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.chorus_unit.ct);
	}
	if(gCtrlVars.chorus2_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.chorus2_unit.ct);
	}
    //expand
	for(i = 0; i < sizeof(expander_unit_aggregate)/sizeof(expander_unit_aggregate[0]); i++)
	{
		if(expander_unit_aggregate[i]->ct != NULL)
		{
			osPortFree(expander_unit_aggregate[i]->ct);
		}
	}
    //drc
    for(i = 0; i < sizeof(drc_unit_aggregate)/sizeof(drc_unit_aggregate[0]); i++)
	{
		if(drc_unit_aggregate[i]->ct != NULL)
		{
    		osPortFree(drc_unit_aggregate[i]->ct);
		}
	}
	 //eq
    for(i = 0; i < sizeof(eq_unit_aggregate)/sizeof(eq_unit_aggregate[0]); i++)
	{
		if(eq_unit_aggregate[i]->ct != NULL)
		{
    		osPortFree(eq_unit_aggregate[i]->ct);
    		eq_unit_aggregate[i]->ct = NULL;
		}
	}

	#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
	for(i = 0; i < sizeof(eq_drc_unit)/sizeof(eq_drc_unit[0]); i++)
	{
		if(eq_drc_unit[i]->ct != NULL)
		{
    		osPortFree(eq_drc_unit[i]->ct);
		}
        eq_drc_unit[i]->ct =NULL;
	}
	#endif

    #if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
    for(i = 0; i < sizeof(drc_legacy_unit_init)/sizeof(drc_legacy_unit_init[0]); i++)
	{
		if(drc_legacy_unit_init[i]->ct != NULL)
		{
    		osPortFree(drc_legacy_unit_init[i]->ct);
    		drc_legacy_unit_init[i]->ct =NULL;
		}
	}
    #endif

	gCtrlVars.freq_shifter_unit.ct   =   NULL;
	#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
	gCtrlVars.freq_shifter_fine_unit.ct = NULL;
	#endif
	gCtrlVars.howling_dector_unit.ct =   NULL;
    #if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
	gCtrlVars.howling_dector_fine_unit.ct  = NULL;
    #endif

    #if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
	gCtrlVars.howling_dector_specified_unit.ct  = NULL;
    #endif

	gCtrlVars.pitch_shifter_unit.ct  =   NULL;
	#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
	gCtrlVars.pitch_shifter_pro_unit.ct  =   NULL;
	#endif
	gCtrlVars.auto_tune_unit.ct		 =   NULL;
	gCtrlVars.voice_changer_unit.ct  =   NULL;
	#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
	gCtrlVars.voice_changer_pro_unit.ct  =   NULL;
	#endif
	gCtrlVars.echo_unit.ct			 =   NULL;
	gCtrlVars.echo_unit.s_buf		 =   NULL;
	gCtrlVars.reverb_unit.ct		 =   NULL;
	gCtrlVars.plate_reverb_unit.ct	 =   NULL;
    #if CFG_AUDIO_EFFECT_PINGPONG_EN
    gCtrlVars.ping_pong_unit.ct      =   NULL;
    gCtrlVars.ping_pong_unit.s       =   NULL;
    #endif
	#if CFG_AUDIO_EFFECT_FLANGER_EN
	gCtrlVars.flanger_uint.ct = 0;
	#endif
	#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
	gCtrlVars.overdrive_unit.ct = NULL;
	#endif

    #if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
    gCtrlVars.overdrive_poly_unit.ct = NULL;
    #endif
	#if CFG_AUDIO_EFFECT_DISTORTION_EN
	gCtrlVars.distortion_unit.ct = NULL;
	#endif
    #if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
    gCtrlVars.distortion_ds1_unit.ct = NULL;
    #endif

	#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	gCtrlVars.pitch_detector_unit.ct  =   NULL;
	#endif
	#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
	gCtrlVars.reverb_pro_unit.ct     =   NULL;
	#endif

	gCtrlVars.MicAudioSdct_unit.ct   =   NULL;
	gCtrlVars.MusicAudioSdct_unit.ct =   NULL;
	gCtrlVars.vocal_cut_unit.ct	     =   NULL;
	gCtrlVars.vocal_remove_unit.ct	 =   NULL;
	#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
	gCtrlVars.music_threed_unit.ct   =   NULL;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
	gCtrlVars.music_threed_plus_unit.ct =NULL;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
	gCtrlVars.music_vb_unit.ct       =   NULL;
	#endif
	#if CFG_AUDIO_EFFECT_REC_VIRTUAL_BASS_EN
	gCtrlVars.rec_vb_unit.ct         =   NULL;
	#endif
	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
	gCtrlVars.music_vb_classic_unit.ct = NULL;
	#endif
    #if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
    gCtrlVars.stereo_winden_unit.ct  =   NULL;
    #endif
    #if CFG_AUDIO_EFFECT_AUTOWAH_EN
    gCtrlVars.auto_wah_unit.ct       =   NULL;
    #endif
	#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
	gCtrlVars.music_delay_unit.ct	 =   NULL;
	#endif
	gCtrlVars.music_exciter_unit.ct	 =   NULL;
	gCtrlVars.chorus_unit.ct         =   NULL;
	gCtrlVars.chorus2_unit.ct         =   NULL;
	
    //expand
	for(i = 0; i < sizeof(expander_unit_aggregate)/sizeof(expander_unit_aggregate[0]); i++)
	{
		expander_unit_aggregate[i]->ct		= NULL;
	}
    //drc
    for(i = 0; i < sizeof(drc_unit_aggregate)/sizeof(drc_unit_aggregate[0]); i++)
	{
		drc_unit_aggregate[i]->ct		    = NULL;
	}
	 //eq
    for(i = 0; i < sizeof(eq_unit_aggregate)/sizeof(eq_unit_aggregate[0]); i++)
	{
		eq_unit_aggregate[i]->ct		    = NULL;
	}
	#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN

	if(gCtrlVars.mic_ns_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.mic_ns_unit.ct);
		gCtrlVars.mic_ns_unit.ct= NULL;
	}
	#endif
    #ifdef CFG_AUDIO_EFFECT_FADER_EN
	AudioEffectFaderDenit(&gCtrlVars.MusicFaderCt);
    #endif

    #if defined (CFG_APP_BT_MODE_EN) || defined(CFG_APP_USB_PHONE_MODE_EN)
	#if CFG_AUDIO_EFFECT_HFP_NS_EN
	if(gCtrlVars.hfp_ns_unit.ct != NULL)
	{
		osPortFree(gCtrlVars.hfp_ns_unit.ct);
		gCtrlVars.hfp_ns_unit.ct = NULL;
	}
	#endif
    if(gCtrlVars.mic_aec_unit.ct)
    {
		osPortFree(gCtrlVars.mic_aec_unit.ct);
		gCtrlVars.mic_aec_unit.ct          = NULL;
    }

	// aec expander init
    if(gCtrlVars.expander_for_aec_unit.ct)
    {
		osPortFree(gCtrlVars.expander_for_aec_unit.ct);
		gCtrlVars.expander_for_aec_unit.ct          = NULL;
    }
	// aec eq init
    if(gCtrlVars.eq_for_aec_unit.ct)
    {
		osPortFree(gCtrlVars.eq_for_aec_unit.ct);
		gCtrlVars.eq_for_aec_unit.ct          = NULL;
    }
	// aec drc init
    if(gCtrlVars.drc_for_aec_unit.ct)
    {
		osPortFree(gCtrlVars.drc_for_aec_unit.ct);
		gCtrlVars.drc_for_aec_unit.ct          = NULL;
    }
    #endif

    #if CFG_AUDIO_EFFECT_COMPANDER_EN
    if(gCtrlVars.compander_unit.ct != NULL)
    {
		osPortFree(gCtrlVars.compander_unit.ct);
		gCtrlVars.compander_unit.ct          = NULL;
    }
    #endif

    #if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
    if(gCtrlVars.mic_low_level_compressor_unit.ct != NULL)
    {
		osPortFree(gCtrlVars.mic_low_level_compressor_unit.ct);
		gCtrlVars.mic_low_level_compressor_unit.ct          = NULL;
    }
    #endif


    #if CFG_AUDIO_EFFECT_BIQUAD_EN
    if(gCtrlVars.mic_low_level_compressor_unit.ct != NULL)
      {
	     osPortFree(gCtrlVars.biquad_unit.ct);
	     gCtrlVars.biquad_unit.ct          = NULL;
      }
    #endif

    #if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
    if(gCtrlVars.mic_howling_guard_unit.ct != NULL)
       {
          osPortFree(gCtrlVars.mic_howling_guard_unit.ct);
          gCtrlVars.mic_howling_guard_unit.ct   = NULL;
      }
    #endif

	#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
    if(gCtrlVars.rec_phase_shifter_unit.ct != NULL)
       {
          osPortFree(gCtrlVars.rec_phase_shifter_unit.ct);
          gCtrlVars.rec_phase_shifter_unit.ct   = NULL;
      }
	#endif

	#if CFG_AUDIO_EFFECT_DRAPOST_EN
    if(gCtrlVars.dra_post_unit.ct != NULL)
       {
          osPortFree(gCtrlVars.dra_post_unit.ct);
          gCtrlVars.dra_post_unit.ct   = NULL;
      }
	#endif

    #if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
    if(gCtrlVars.dc_blocker_unit.ct != NULL)
       {
          osPortFree(gCtrlVars.dc_blocker_unit.ct);
          gCtrlVars.dc_blocker_unit.ct   = NULL;
      }
    #endif

	#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	if(gCtrlVars.virtual_surround_unit.ct != NULL)
	   {
		  osPortFree(gCtrlVars.virtual_surround_unit.ct);
		  gCtrlVars.virtual_surround_unit.ct   = NULL;
	  }
	#endif

    #if CFG_AUDIO_EFFECT_NOISE_GENERATOR_EN
    if(gCtrlVars.noise_generator_unit.ct != NULL)
       {
	       osPortFree(gCtrlVars.noise_generator_unit.ct);
	       gCtrlVars.noise_generator_unit.ct   = NULL;
      }
    #endif

    #if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
    if(gCtrlVars.music_butterworth_unit.ct != NULL)
      {
         osPortFree(gCtrlVars.music_butterworth_unit.ct);
         gCtrlVars.music_butterworth_unit.ct   = NULL;
      }
     #endif

    #if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
    void DynmmicEq_Free(DynamicEqUnit *unit);
    if(gCtrlVars.music_dynamic_eq_unit.ct != NULL)
      {
    	DynmmicEq_Free(&gCtrlVars.music_dynamic_eq_unit);
      }

    #endif

	#if CFG_AUDIO_EFFECT_LRBALANCER_EN
    if(gCtrlVars.music_lr_balancer.ct != NULL)
      {
         osPortFree(gCtrlVars.music_lr_balancer.ct);
         gCtrlVars.music_lr_balancer.ct   = NULL;
      }
	#endif

//-----------------------------------------------------------------------//
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
#endif
}
/*
****************************************************************
* 音效模块初始化
*
*
****************************************************************
*/
void AudioEffectsInit(void)
{
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
#endif
	
	DBG("%s\n",__func__);
	#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
	AudioEffectReverbProInit(&gCtrlVars.reverb_pro_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate,0);
	#endif
	
	#if CFG_AUDIO_EFFECT_MUSIC_NOISE_SUPPRESSOR_EN
	AudioEffectExpanderInit(&gCtrlVars.music_expander_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

    #if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN
	gCtrlVars.MusicAudioSdct_unit.enable = 1;
	AudioEffectSilenceDectorInit(&gCtrlVars.MusicAudioSdct_unit,gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
    #endif

	#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
	AudioEffectPcmDelayInit(&gCtrlVars.music_delay_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate,0);
	#endif

	#if CFG_AUDIO_EFFECT_MUSIC_EXCITER_EN
	AudioEffectExciterInit(&gCtrlVars.music_exciter_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
	AudioEffectPitchShifterProInit(&gCtrlVars.pitch_shifter_pro_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif
	
	#if CFG_AUDIO_EFFECT_MIC_NOISE_SUPPRESSOR_EN
	AudioEffectExpanderInit(&gCtrlVars.mic_expander_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif


    #if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN
	AudioEffectFreqShifterInit(&gCtrlVars.freq_shifter_unit);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
	AudioEffectFreqShifterFineInit(&gCtrlVars.freq_shifter_fine_unit,gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN
	AudioEffectHowlingSuppressorInit(&gCtrlVars.howling_dector_unit);
	#endif
	
    #if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
    AudioEffectHowlingSuppressorFineInit(&gCtrlVars.howling_dector_fine_unit,gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
    AudioEffectHowlingSuppressorSpecifieInit(&gCtrlVars.howling_dector_specified_unit,gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_MIC_SILENCE_DECTOR_EN
	gCtrlVars.MicAudioSdct_unit.enable = 1;
	AudioEffectSilenceDectorInit(&gCtrlVars.MicAudioSdct_unit,gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
    #endif

	#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN
	gCtrlVars.pitch_shifter_unit.channel = 2;
	AudioEffectPitchShifterInit(&gCtrlVars.pitch_shifter_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate,0);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_AUTO_TUNE_EN
	AudioEffectAutoTuneInit(&gCtrlVars.auto_tune_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate,0);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_EN
	AudioEffectVoiceChangerInit(&gCtrlVars.voice_changer_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif

    #if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
	AudioEffectVoiceChangerProInit(&gCtrlVars.voice_changer_pro_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif
	
	#if CFG_AUDIO_EFFECT_MIC_ECHO_EN
	AudioEffectEchoInit(&gCtrlVars.echo_unit,  gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate,0);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_REVERB_EN
	AudioEffectReverbInit(&gCtrlVars.reverb_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate,0);
	#endif

    #if CFG_AUDIO_EFFECT_MIC_PLATE_REVERB_EN
	AudioEffectPlateReverbInit(&gCtrlVars.plate_reverb_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif	

    #if CFG_AUDIO_EFFECT_PINGPONG_EN
    AudioEffectPingPongInit(&gCtrlVars.ping_pong_unit,gCtrlVars.sample_rate,0);
    #endif

    #if CFG_AUDIO_EFFECT_FLANGER_EN
    AudioEffectFlangerInit(&gCtrlVars.flanger_uint,  gCtrlVars.sample_rate,0);
    #endif

	#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
    AudioEffectOverdriveInit(&gCtrlVars.overdrive_unit,  gCtrlVars.sample_rate);
	#endif

    #if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
    AudioEffectOverdrivePolyInit(&gCtrlVars.overdrive_poly_unit,  gCtrlVars.sample_rate);
    #endif
	#if CFG_AUDIO_EFFECT_DISTORTION_EN
    AudioEffectDistortionExpInit(&gCtrlVars.distortion_unit,  gCtrlVars.sample_rate);
	#endif
	
    #if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
    AudioEffectDistortionDS1Init(&gCtrlVars.distortion_ds1_unit,  gCtrlVars.sample_rate);
    #endif

	#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	gCtrlVars.pitch_detector_unit.enable = 1;
    AudioEffectPitchDetectorInit(&gCtrlVars.pitch_detector_unit, 1, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
	AudioEffectVocalCutInit(&gCtrlVars.vocal_cut_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
    AudioEffectVocalRemoveInit(&gCtrlVars.vocal_remove_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate,0);
    #endif

	#if CFG_AUDIO_EFFECT_CHORUS_EN
    AudioEffectChorusInit(&gCtrlVars.chorus_unit, gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_CHORUS2_EN
    AudioEffectChorus2Init(&gCtrlVars.chorus2_unit,  gCtrlVars.sample_rate,0);
    #endif

	#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
	AudioEffectThreeDInit(&gCtrlVars.music_threed_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
	AudioEffectThreeDPlusInit(&gCtrlVars.music_threed_plus_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
	AudioEffectVBInit(&gCtrlVars.music_vb_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_REC_VIRTUAL_BASS_EN
	AudioEffectVBInit(&gCtrlVars.rec_vb_unit, 1, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
	AudioEffectVBClassicInit(&gCtrlVars.music_vb_classic_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif
	
    #if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
	AudioEffectStereoWidenerInit(&gCtrlVars.stereo_winden_unit,gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_AUTOWAH_EN
    AudioEffectAutoWahInit(&gCtrlVars.auto_wah_unit,gCtrlVars.sample_rate);
    #endif

	#if CFG_AUDIO_EFFECT_MUSIC_DRC_EN
	AudioEffectDRCInit(&gCtrlVars.music_drc_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
	AudioEffectDRCLegacyInit(&gCtrlVars.music_drc_legacy_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

    #if CFG_AUDIO_EFFECT_MIC_DRC_EN
	AudioEffectDRCInit(&gCtrlVars.mic_drc_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif
	
	#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
	AudioEffectDRCLegacyInit(&gCtrlVars.mic_drc_legacy_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_REC_DRC_EN
	AudioEffectDRCInit(&gCtrlVars.rec_drc_unit, 1, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MUSIC_PRE_EQ_EN
	AudioEffectEQInit(&gCtrlVars.music_pre_eq_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MUSIC_OUT_EQ_EN
	AudioEffectEQInit(&gCtrlVars.music_out_eq_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_PRE_EQ_EN
	AudioEffectEQInit(&gCtrlVars.mic_pre_eq_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif
	
	#if CFG_AUDIO_EFFECT_MIC_BYPASS_EQ_EN
	AudioEffectEQInit(&gCtrlVars.mic_bypass_eq_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_ECHO_EQ_EN
	AudioEffectEQInit(&gCtrlVars.mic_echo_eq_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_REVERB_EQ_EN
	AudioEffectEQInit(&gCtrlVars.mic_reverb_eq_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_OUT_EQ_EN
	#ifdef CFG_FUNC_MIC_KARAOKE_EN
	AudioEffectEQInit(&gCtrlVars.mic_out_eq_unit, 2, gCtrlVars.sample_rate);
	#else
	AudioEffectEQInit(&gCtrlVars.mic_out_eq_unit, 1, gCtrlVars.sample_rate);
	#endif
	#endif

	#if CFG_AUDIO_EFFECT_REC_EQ_EN
	AudioEffectEQInit(&gCtrlVars.rec_eq_unit, 1, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN
	AudioEffectBlueNSInit(&gCtrlVars.mic_ns_unit,gCtrlVars.SamplesPerFrame,0);
	#endif

    #if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
	AudioEffectEqDrcInit(&gCtrlVars.music_eq_drc_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);

	AudioEffectEqDrcInit(&gCtrlVars.rec_eq_drc_unit, 1, gCtrlVars.sample_rate);
    #endif

    #ifdef CFG_AUDIO_EFFECT_FADER_EN
    AudioEffectFaderInit(&gCtrlVars.MusicFaderCt,gCtrlVars.adc_line_channel_num,1);
    #endif

    #if defined (CFG_APP_BT_MODE_EN) || defined(CFG_APP_USB_PHONE_MODE_EN)

    AudioEffectBlueNSInit(&gCtrlVars.hfp_ns_unit,gCtrlVars.SamplesPerFrame,0);

	AudioEffectAecInit(&gCtrlVars.mic_aec_unit, 16000);//固定为16K采样率

	if((GetSystemMode() == AppModeBtHfPlay)||(GetSystemMode() == AppModeUsbPhone))
	{
		gCtrlVars.pitch_shifter_unit.channel = 1;
		AudioEffectPitchShifterInit(&gCtrlVars.pitch_shifter_unit, gCtrlVars.adc_mic_channel_num, gCtrlVars.sample_rate,0);
	}
	// aec expander init
	gCtrlVars.expander_for_aec_unit.channel = 2;
	AudioEffectExpanderInit(&gCtrlVars.expander_for_aec_unit, 2, 16000);

	// aec eq init
	gCtrlVars.eq_for_aec_unit.channel = 1;
	AudioEffectEQInit(&gCtrlVars.eq_for_aec_unit, 2, 16000);

	// aec drc init
	gCtrlVars.drc_for_aec_unit.channel = 1;
	AudioEffectDRCInit(&gCtrlVars.drc_for_aec_unit, 1, 16000);
    #endif

    #if CFG_AUDIO_EFFECT_COMPANDER_EN
	AudioEffectCompanderInit(&gCtrlVars.compander_unit, gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
    AudioEffectLowLevelCompressorInit(&gCtrlVars.mic_low_level_compressor_unit,gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_BIQUAD_EN
    AudioEffectBiquadInit(&gCtrlVars.biquad_unit,gCtrlVars.adc_line_channel_num, gCtrlVars.sample_rate);
    #endif


	#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
	AudioEffectHowlingGuardInit(&gCtrlVars.mic_howling_guard_unit, gCtrlVars.sample_rate);
	#endif

	#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
	AudioEffectPhaseShifterdInit(&gCtrlVars.rec_phase_shifter_unit,gCtrlVars.adc_mic_channel_num,mainAppCt.SamplesPreFrame,gCtrlVars.sample_rate);
	#endif

    #if CFG_AUDIO_EFFECT_DRAPOST_EN
    AudioEffectDraPostInit(&gCtrlVars.dra_post_unit, gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
    AudioEffectDCBlockerInit(&gCtrlVars.dc_blocker_unit, gCtrlVars.adc_mic_channel_num);
    #endif

	#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	AudioEffectVirtualSurroundInit(&gCtrlVars.virtual_surround_unit,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
	#endif

    #if CFG_AUDIO_EFFECT_NOISE_GENERATOR_EN
    AudioEffectNoiseGeneratorInit(&gCtrlVars.noise_generator_unit,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
    AudioEffectButterWorthInit(&gCtrlVars.music_butterworth_unit,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
    #endif

    #if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
    AudioEffectDynamicEqInit(&gCtrlVars.music_dynamic_eq_unit,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
    #endif
     #ifdef CFG_FUNC_TSM_EN
     AudioEffectTsmInit(2,gCtrlVars.sample_rate);
     #endif

    #if CFG_AUDIO_EFFECT_LRBALANCER_EN
    AudioEffectLRBalancerInit(&gCtrlVars.music_lr_balancer,gCtrlVars.adc_line_channel_num);
   #endif

#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
#endif
}
/*
****************************************************************
* drc legacy 音效初始化
* economize  8M Mcps
*
****************************************************************
*/
void AudioEffectDRCLegacyInit(DRCLegacyUnit *unit, uint32_t num_channels,uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN

	uint16_t q[2];
	int32_t threshold[3];
	int32_t ratio[3];
	int32_t attack_tc[3];
	int32_t release_tc[3];

	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

    if(unit->channel==0)
    {
    	unit->channel = num_channels;
    }

   if(unit->ct == NULL)
	{
		unit->ct = (DRCLegacyContext *)osPortMallocFromEnd(sizeof(DRCLegacyContext));
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("DRCLegacyContext malloc err! %ld\n",sizeof(DRCLegacyContext));
			return;
		}
	}

	if(unit->ct != NULL)
	{

		q[0]                 = unit->q[0];
		q[1]                 = unit->q[1];

		threshold[0]         = (int32_t)unit->threshold[0];
		threshold[1]         = (int32_t)unit->threshold[1];
		threshold[2]         = (int32_t)unit->threshold[2];

		ratio[0]             = (int32_t)unit->ratio[0];
		ratio[1]             = (int32_t)unit->ratio[1];
		ratio[2]             = (int32_t)unit->ratio[2];

		attack_tc[0]         = (int32_t)unit->attack_tc[0];
		attack_tc[1]         = (int32_t)unit->attack_tc[1];
		attack_tc[2]         = (int32_t)unit->attack_tc[2];

		release_tc[0]        = (int32_t)unit->release_tc[0];
		release_tc[1]        = (int32_t)unit->release_tc[1];
		release_tc[2]        = (int32_t)unit->release_tc[2];

		drc_legacy_init(unit->ct,  num_channels, sample_rate,
				        unit->fc,  unit->mode,   q,
						threshold, ratio,        attack_tc, release_tc);
	}
#endif
}
/*
****************************************************************
* drc legacy音效配置函数
*
*
****************************************************************
*/
void AudioEffectDRCLegacyConfig(DRCLegacyUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
	uint16_t q[2];
	int32_t threshold[3];
	int32_t ratio[3];
	int32_t attack_tc[3];
	int32_t release_tc[3];

	if(!unit->enable)
	{
		return;
	}

	if(unit->ct != NULL)
	{
		q[0]                 = unit->q[0];
		q[1]                 = unit->q[1];

		threshold[0]         = (int32_t)unit->threshold[0];
		threshold[1]         = (int32_t)unit->threshold[1];
		threshold[2]         = (int32_t)unit->threshold[2];

		ratio[0]             = (int32_t)unit->ratio[0];
		ratio[1]             = (int32_t)unit->ratio[1];
		ratio[2]             = (int32_t)unit->ratio[2];

		attack_tc[0]         = (int32_t)unit->attack_tc[0];
		attack_tc[1]         = (int32_t)unit->attack_tc[1];
		attack_tc[2]         = (int32_t)unit->attack_tc[2];

		release_tc[0]        = (int32_t)unit->release_tc[0];
		release_tc[1]        = (int32_t)unit->release_tc[1];
		release_tc[2]        = (int32_t)unit->release_tc[2];

		drc_legacy_init(unit->ct,  unit->channel, sample_rate,
				        unit->fc,  unit->mode,   q,
						threshold, ratio,        attack_tc, release_tc);
	}
#endif
}
/*
****************************************************************
* drc legacy 16 主循环处理函数
*
*
****************************************************************
*/
void AudioEffectDRCLegacyApply(DRCLegacyUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN//

	if((unit->enable) && (unit->ct != NULL) &&(pcm_out != NULL)&&(pcm_in != NULL))
	{
		drc_legacy_apply(unit->ct, pcm_in, pcm_out, n, unit->pregain1, unit->pregain2);
	}
#endif
}
/*
****************************************************************
* drc legacy 24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectDRCLegacyApply24(DRCLegacyUnit *unit, int32_t *pcm_in,int32_t *pcm_out, int32_t n)
{
#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN//
	if((unit->enable) && (unit->ct != NULL) &&(pcm_out != NULL)&&(pcm_in != NULL))
	{
		drc_legacy_apply24(unit->ct, pcm_in, pcm_out, n, unit->pregain1, unit->pregain2);
	}
#endif
}
/*
****************************************************************
*vad 音效初始化
*only mono,only 16bit
*for sample_rate = 48000, n = 480 samples.
*for sample_rate = 44100, n = 440 samples. <<< Here n is 440, NOT 441!
*for sample_rate = 32000, n = 320 samples.
*for sample_rate = 22050, n = 220 samples. <<< Here n is 220, NOT 221!
*for sample_rate = 16000, n = 160 samples.
*for sample_rate =  8000, n =  80 samples.
****************************************************************
*/
void AudioEffectVadInit(VADUnit *unit,uint32_t sample_rate,uint16_t SamplesPerFrame)
{
#if CFG_AUDIO_EFFECT_VAD_EN
	 uint32_t size;
	 uint32_t persistent_size, scratch_size;
     if(!gCtrlVars.audio_effect_init_flag)
		{
			return;
		}

	 if(!unit->enable)
		{
			return;
		}

	 if(vad_estimate_memory_usage(sample_rate, &persistent_size, &scratch_size)==VAD_ERROR_OK)
	 {
		 size = persistent_size+scratch_size;
	 }
	 else
	 {
		 APP_DBG("VAD malloc err!\n");
		 return;
	 }

	 extern const uint32_t vad_tab[6][2];

	 for(size=0; size < 6; size++)
	 {
		 if(vad_tab[size][0]==sample_rate)
		 {
			 break;
		 }
	 }


	 if(size==6)
	 {
		 APP_DBG("VAD sample_rate,SamplesPerFrame err\n");
		 return;
	 }
	 if(unit->ct == NULL)
	 {
		unit->ct = (uint8_t *)osPortMallocFromEnd(size);
		if(unit->ct == NULL)
		{
			APP_DBG("VAD malloc err! %ld\n",size);
			unit->enable = 0;
		}
		else
		{
			unit->scratch = unit->ct + persistent_size;
		}
	  }

     if(unit->ct != NULL)
	  {
    	 vad_init(unit->ct, unit->scratch, sample_rate, unit->post_processing);
	  }
#endif
}
/*
****************************************************************
*vad 主循环处理函数
*only mono ,only 16bit
*for sample_rate = 48000, n = 480 samples.
*for sample_rate = 44100, n = 440 samples. <<< Here n is 440, NOT 441!
*for sample_rate = 32000, n = 320 samples.
*for sample_rate = 22050, n = 220 samples. <<< Here n is 220, NOT 221!
*for sample_rate = 16000, n = 160 samples.
*for sample_rate =  8000, n =  80 samples.
****************************************************************
*/
void AudioEffectVadApply(VADUnit *unit, int16_t *pcm_in,  uint32_t n)
{
#if CFG_AUDIO_EFFECT_VAD_EN
	int32_t ret;
	if((unit->enable) && (unit->ct != NULL))
	{
		ret = vad_apply(unit->ct, pcm_in,n);
		if(ret==0)
		{
			unit->voiced_status = 0;
		}
		else if(ret==1)
		{
			unit->voiced_status = 1;
		}
		else
		{
			unit->voiced_status = 0;
		}
	}
#endif
}
/*
****************************************************************
* HowlingDector specified音效初始化
* only mono
*
****************************************************************
*/
void AudioEffectHowlingSuppressorSpecifieInit(HowlingSpecifiedUnit *unit,uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN

	uint8_t   i;
	int32_t   center_freq[6];          //Range: 2 ~ sample_rate/2-2 Hz.
	int32_t   q[6];                    //Range: 1 ~ 32767
	int32_t   depth[6];                //Range: -100 ~ 0 dB.

	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (HowlingSpecifiedContext *)osPortMallocFromEnd(sizeof(HowlingSpecifiedContext));
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("HowlingSpecifiedContext malloc err! %ld\n",sizeof(HowlingSpecifiedContext));
		}
	}

	if(unit->ct != NULL)
	{
        if( (sample_rate<44100) || (sample_rate>48000))
        {
        	DBG("Please modify HolingSpecifiedTab, max freq\n");
        }

        for(i = 0; i < 6; i++)
        {
        	center_freq[i] =(int32_t)unit->center_freq[i];
        }
        for(i = 0; i < 6; i++)
        {
        	q[i] =(int32_t)unit->q[i];
        }
        for(i = 0; i < 6; i++)
        {
        	depth[i] =(int32_t)unit->depth[i];
        }

		howling_suppressor_specified_init(unit->ct, sample_rate,unit->num_specified_filters,&center_freq[0],&q[0],&depth[0]);
	}
#endif
}
/*
****************************************************************
* HowlingDector specified主循环处理函数
*only mono
*
****************************************************************
*/
void AudioEffectHowlingSuppressorSpecifiedApply(HowlingSpecifiedUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in==NULL) return;

		howling_suppressor_specified_apply(unit->ct,  pcm_in, pcm_out, n);
	}
#endif
}
/*
****************************************************************
* lr balancer音效初始化
*
*
****************************************************************
*/
void AudioEffectLRBalancerInit(LRBalancerUnit *unit,uint8_t channel)
{
    #if CFG_AUDIO_EFFECT_LRBALANCER_EN
		if(!gCtrlVars.audio_effect_init_flag)
		{
			return;
		}
	    if(unit->channel == 0)
	    {
			unit->channel = channel;
	    }

		channel = unit->channel;

	    if(!unit->enable)
		{
			return;
		}

	 if(unit->ct == NULL)
	 {
		unit->ct = (LRBalancerContext *)osPortMallocFromEnd(sizeof(LRBalancerContext));
		if(unit->ct == NULL)
		{
			APP_DBG("LRBalancerContext malloc err! %ld\n",sizeof(LRBalancerContext));
			unit->enable = 0;
		}
	  }

     if(unit->ct != NULL)
	  {
	     lr_balancer_init(unit->ct,channel);
	  }
    #endif
}
/*
****************************************************************
* lr balancer 主循环处理函数
*
*
****************************************************************
*/
void AudioEffectLRBalancerApply(LRBalancerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_LRBALANCER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		lr_balancer_apply16(unit->ct, pcm_in,pcm_out,n,unit->balance);
	}
#endif
}

void AudioEffectLRBalancerApply24(LRBalancerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_LRBALANCER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		lr_balancer_apply24(unit->ct, pcm_in,pcm_out,n,unit->balance);
	}
#endif
}

/*
****************************************************************
* sine, noise generator音效初始化
*
*
****************************************************************
*/
void AudioEffectNoiseGeneratorInit(NoiseGeneratorUnit *unit,uint8_t channel,uint16_t SampleRate)
{
#if CFG_AUDIO_EFFECT_NOISE_GENERATOR_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	if(unit->ct == NULL)
	{
		unit->ct = (NoiseGeneratorContext *)osPortMallocFromEnd(sizeof(NoiseGeneratorContext));
		if(unit->ct == NULL)
		{
			APP_DBG("NoiseGeneratorContext malloc err! %ld\n",sizeof(NoiseGeneratorContext));
			unit->enable = 0;
		}
	}
	if(unit->ct != NULL)
	{
		noise_generator_init(unit->ct,unit->noise_type,unit->amplitude);
	}
#endif
}

/*
****************************************************************
*  Noise,Generator 主循环处理函数
*
*
****************************************************************
*/
void AudioEffectNoiseGeneratordApply(NoiseGeneratorUnit *unit, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_NOISE_GENERATOR_EN
	uint16_t s;
	int16_t Pcm;
	int16_t *InPcm  = pcm_out + n - 1;
	int16_t *OutPcm = pcm_out + n * 2 - 1;

	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_out==NULL) return;

		noise_generator_apply16(unit->ct, pcm_out,n,unit->amplitude);

		if(unit->channel==2)
		{
			for(s = 0; s < n; s++)
			{
				Pcm = *InPcm--;
				*OutPcm-- = Pcm;
				*OutPcm-- = Pcm;
			}
		}
	}
#endif
}

void AudioEffectNoiseGeneratorApply24(NoiseGeneratorUnit *unit, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_NOISE_GENERATOR_EN
	uint16_t s;
	int32_t Pcm;
	int32_t *InPcm  = pcm_out + n - 1;
	int32_t *OutPcm = pcm_out + n * 2 - 1;

	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_out==NULL) return;

		noise_generator_apply24(unit->ct, pcm_out,n,unit->amplitude);

		if(unit->channel==2)
		{
			for(s = 0; s < n; s++)
			{
				Pcm = *InPcm--;
				*OutPcm-- = Pcm;
				*OutPcm-- = Pcm;
			}
		}
	}
#endif
}
/*
****************************************************************
* vb surround音效初始化
 * @param num_channels number of input channels. Range: 2,4,6,8. The following channel layouts are assumed according to the number of channels:
 * -----------------------------------------------------
 * #channels    1    2    3    4    5    6    7    8
 * -----------------------------------------------------
 *    2         L    R
 *    4(3.1)    L    R    C    LFE
 *    6(5.1)    L    R    C    LFE  Ls   Rs
 *    8(7.1)    L    R    C    LFE  Ls   Rs   Lsd  Rsd
 * -----------------------------------------------------
 * @param sample_rate sample raten in Hz. Only 48000, 44100 and 32000 are supported.
*
****************************************************************
*/
void AudioEffectVirtualSurroundInit(VirtualSurroundUnit *unit,uint8_t channel,uint16_t SampleRate)
{
#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	unit->SampleRate = SampleRate;

    if(!unit->enable)
	{
		return;
	}
	if(unit->ct == NULL)
	{
		unit->ct = (VirtualSurroundContext *)osPortMallocFromEnd(sizeof(VirtualSurroundContext));
		if(unit->ct == NULL)
		{
			APP_DBG("VirtualSurroundContext malloc err! %ld\n",sizeof(VirtualSurroundContext));
			unit->enable = 0;
		}
	}
	if(unit->ct != NULL)
	{
		virtual_surround_init(unit->ct,channel,SampleRate);
	}
#endif
}

/*
****************************************************************
* vb surround 主循环处理函数
*
*
****************************************************************
*/
void AudioEffectVirtualSurroundApply(VirtualSurroundUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		memcpy(VirSurroundBuf,pcm_in,n*2*unit->channel);

		virtual_surround_apply16(unit->ct, VirSurroundBuf,pcm_out,n);
	}
#endif
}

void AudioEffectVirtualSurroundApply24(VirtualSurroundUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in == pcm_out)
		{
			DBG("pcm_out CANNOT be the same as pcm_in\n");
			return;
		}
		virtual_surround_apply24(unit->ct, pcm_in,pcm_out,n);
	}
#endif
}
/*
****************************************************************
* dc blocker音效初始化
*
*
****************************************************************
*/
void AudioEffectDCBlockerInit(DCBlockerUnit *unit,uint8_t channel)
{
#if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	if(unit->ct == NULL)
	{
		unit->ct = (DCBlocker *)osPortMallocFromEnd(sizeof(DCBlocker));
		if(unit->ct == NULL)
		{
			APP_DBG("DCBlocker malloc err! %ld\n",sizeof(DCBlocker));
			unit->enable = 0;
		}
	}
	if(unit->ct != NULL)
	{
		dc_blocker_init(unit->ct,channel);
	}
#endif
}

/*
****************************************************************
* DCBlocker 主循环处理函数
*
*
****************************************************************
*/
void AudioEffectDCBlockerApply(DCBlockerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		dc_blocker_apply(unit->ct, pcm_in,pcm_out,n);
	}
#endif
}

void AudioEffectDCBlockerApply24(DCBlockerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		dc_blocker_apply24(unit->ct, pcm_in,pcm_out,n);
	}
#endif
}

/*
****************************************************************
* dra post parameter copy初始化
*
*
****************************************************************
*/

void AudioEffectDraPostParameterCopy(DraPostUnit *unit)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN
	const int16_t VB_CUT_FREQ1_TAB[]= {30, 40, 55, 70};
	const int16_t  HARMAN_CUT_FREQ_TAB[]={240, 300, 400, 500, 600, 700, 800};

	if((unit->enable) && (unit->ct != NULL))
	{
			unit->ct->nReserverd1 = (int32_t)unit->nReserverd1;
			unit->ct->nReserverd2 = (int32_t)unit->nReserverd2;

			unit->parameter.nEffectActive            = (int32_t)unit->nEffectActive;
			unit->parameter.nFreq1                   = (int32_t)unit->nFreq1;
			unit->parameter.nFreq2                   = (int32_t)unit->nFreq2;
			unit->parameter.nFreq3                   = (int32_t)unit->nFreq3;
			unit->parameter.nFreq4                   = (int32_t)unit->nFreq4;
			unit->parameter.nWidenCenter             = (int32_t)unit->nWidenCenter;
			unit->parameter.nWidenGain               = (int32_t)unit->nWidenGain;
			unit->parameter.nTotalGain               = (int32_t)unit->nTotalGain;   			// 10x
			unit->parameter.bCTCActive               = (int32_t)unit->bCTCActive;
			unit->parameter.nCTCMode                 = (int32_t)unit->nCTCMode;
			unit->parameter.bUpmixActive             = (int32_t)unit->bUpmixActive;
			unit->parameter.fUpmixLRGainCoef         = (int32_t)unit->fUpmixLRGainCoef;     	// 10x
			unit->parameter.fUpmixTlTrGainCoef       = (int32_t)unit->fUpmixTlTrGainCoef;   	// 10x
			unit->parameter.fUpmixGainCoef           = (int32_t)unit->fUpmixGainCoef;       	// 10x
			unit->parameter.fSpeechEnhancementGain   = (int32_t)unit->fSpeechEnhancementGain;  // 10x
			unit->parameter.bVirtualBassActive       = (int32_t)unit->bVirtualBassActive;
			unit->parameter.nVBCutOffFreq            = (int32_t)unit->nVBCutOffFreq;
			unit->parameter.fVBGain                  = (int32_t)unit->fVBGain;
			unit->parameter.nLowFreq1                = (int32_t)VB_CUT_FREQ1_TAB[unit->nLowFreq1];
			unit->parameter.nLowFreq2                = (int32_t)unit->nLowFreq2;
			unit->parameter.nHarmCutOff              = (int32_t)HARMAN_CUT_FREQ_TAB[unit->nHarmCutOff];
			unit->parameter.fScale                   = (int32_t)unit->fScale;					// 10x
			unit->parameter.fGain_f1                 = (int32_t)unit->fGain_f1;					// 10x
			unit->parameter.nVolume                  = (int32_t)unit->nVolume;
			unit->parameter.nDistance                = (int32_t)unit->nDistance;
			unit->parameter.nRoomMode                = (int32_t)unit->nRoomMode+1;
	}
#endif
}
/*
****************************************************************
* dra post音效初始化
*
*
****************************************************************
*/
void AudioEffectDraPostInit(DraPostUnit *unit,uint16_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	if(unit->ct == NULL)
	{
		unit->ct = (DraPostProcessingContext *)osPortMallocFromEnd(DRA_POST_SIZE);
		if(unit->ct == NULL)
		{
			APP_DBG("CompanderContext malloc err! %ld\n",DRA_POST_SIZE);
			unit->enable = 0;
		}
	}
	if(unit->ct != NULL)
	{

		AudioEffectDraPostParameterCopy(unit);
		if(DRA_APP_init(unit->ct, channel, sample_rate, &unit->parameter) == 0)
		{
			return;
		}

		APP_DBG("DRA_APP_init  err!\n");
		unit->ct     = 0;
		unit->enable = 0;
	}
#endif
}

/*
****************************************************************
* DraPost主循环处理函数
*
*
****************************************************************
*/
void AudioEffectDraPostApply(DraPostUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN

	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in && pcm_out)
		{
			DRA_APP_apply(unit->ct, pcm_in, pcm_out, n, unit->nReserverd1, unit->nReserverd2);
		}
	}
#endif
}
/*
****************************************************************
* DraPost ,config 复位处理函数
*
*
****************************************************************
*/
void AudioEffectDraPostReset(DraPostUnit *unit)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN

	if((unit->enable) && (unit->ct != NULL))
	{
		AudioEffectDraPostParameterCopy(unit);
		DRA_APP_reset(unit->ct,&unit->parameter);
	}
#endif
}

/*
****************************************************************
* compander音效初始化
*
*
****************************************************************
*/
void AudioEffectCompanderInit(CompanderUnit *unit,uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_COMPANDER_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	if(unit->ct == NULL)
	{
		unit->ct = (CompanderContext *)osPortMallocFromEnd(COMPANDER_SIZE);
		if(unit->ct == NULL)
		{
			APP_DBG("CompanderContext malloc err! %ld\n",COMPANDER_SIZE);
		}
	}
	if(unit->ct != NULL)
	{
		compander_init(unit->ct,channel,sample_rate,
		unit->threshold,unit->ratio_below,unit->ratio_above,unit->attack_time,unit->release_time);
	}
#endif
}

/*
****************************************************************
* Compander主循环处理函数
*
*
****************************************************************
*/
void AudioEffectCompanderApply(CompanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_COMPANDER_EN
	uint32_t pregain;

	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in && pcm_out)
		{
			pregain	= roboeffect_db_to_scaling(unit->pregain/100,SCALING_Q12_MAX);
			compander_apply(unit->ct, pcm_in,pcm_out,n,pregain);
		}
	}
#endif
}

void AudioEffectCompanderApply24(CompanderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_COMPANDER_EN
	uint32_t pregain;
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in && pcm_out)
		{
			pregain	= roboeffect_db_to_scaling(unit->pregain/100,SCALING_Q12_MAX);
			compander_apply24(unit->ct, pcm_in,pcm_out,n,pregain);
		}
	}
#endif
}

/*
****************************************************************
* low level compressor音效初始化
*
*
****************************************************************
*/
void AudioEffectLowLevelCompressorInit(LowLevelCompressorUnit *unit,uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	if(unit->ct == NULL)
	{
		unit->ct = (LowLevelCompressorContext *)osPortMallocFromEnd(LOW_LEVEL_COMPRESSOR_SIZE);
		if(unit->ct == NULL)
		{
			APP_DBG("LowLevelCompressorContext malloc err! %ld\n",LOW_LEVEL_COMPRESSOR_SIZE);
		}
	}
	if(unit->ct != NULL)
	{
		low_level_compressor_init(unit->ct,channel,sample_rate,
		unit->threshold,unit->gain,unit->attack_time,unit->release_time);
	}
#endif
}

/*
****************************************************************
* low level compressor主循环处理函数
*
*
****************************************************************
*/
void AudioEffectLowLevelCompressorApply(LowLevelCompressorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in && pcm_out)
		{
			low_level_compressor_apply(unit->ct, pcm_in,pcm_out,n);
		}
	}
#endif
}
/*
****************************************************************
* low level compressor24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectLowLevelCompressorApply24(LowLevelCompressorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in && pcm_out)
		{
			low_level_compressor_apply24(unit->ct, pcm_in,pcm_out,n);
		}
	}
#endif
}
/*
****************************************************************
* fader音效初始化
*
*
****************************************************************
*/
void AudioEffectFaderInit(FaderUnit *unit,uint8_t channel, uint32_t fade_samples)
{
#ifdef CFG_AUDIO_EFFECT_FADER_EN
	//------------------------------//
	if(unit->in_ct == NULL)
	{
		unit->in_ct = (FaderContext *)osPortMallocFromEnd(FADER_SIZE);
		if(unit->in_ct == NULL)
		{
			APP_DBG("FaderContext malloc err! %ld\n",FADER_SIZE);
		}
	}

	if(unit->in_ct != NULL)
	{
		fader_init(unit->in_ct,channel,1,fade_samples);
	}
    //-------------------------------------------------------//

	if(unit->out_ct == NULL)
	{
		unit->out_ct = (FaderContext *)osPortMallocFromEnd(FADER_SIZE);
		if(unit->out_ct == NULL)
		{
			APP_DBG("FaderContext malloc err! %ld\n",FADER_SIZE);
		}
	}

	if(unit->out_ct != NULL)
	{
		fader_init(unit->out_ct,channel,2,fade_samples);
	}
	//-------------------------------------------------------//
	if((unit->out_ct != NULL)&&(unit->in_ct != NULL))
	{
		unit->enable = 1;
	}
	else
	{
		unit->enable = 0;
	}

#endif
}

void AudioEffectFaderDenit(FaderUnit *unit)
{
#ifdef CFG_AUDIO_EFFECT_FADER_EN
	 unit->enable = 0;

     if(unit->in_ct)
     {
    	 osPortFree(unit->in_ct);
    	 unit->in_ct = NULL;
     }

     if(unit->out_ct)
     {
    	 osPortFree(unit->out_ct);
    	 unit->out_ct = NULL;
     }
#endif
}
/*
****************************************************************
* fader主循环处理函数
*
*
****************************************************************
*/
void AudioEffectFaderInApply(FaderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#ifdef CFG_AUDIO_EFFECT_FADER_EN
	if((unit->enable) && (unit->in_ct != NULL))
	{
		fader_apply(unit->in_ct, pcm_in, pcm_out, n);
	}
#endif
}
void AudioEffectFaderOutApply(FaderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#ifdef CFG_AUDIO_EFFECT_FADER_EN
	if((unit->enable) && (unit->out_ct != NULL))
	{
		fader_apply(unit->out_ct, pcm_in, pcm_out, n);
	}
#endif
}
void AudioEffectFaderInApply24(FaderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#ifdef CFG_AUDIO_EFFECT_FADER_EN
	if((unit->enable) && (unit->in_ct != NULL))
	{
		fader_apply24(unit->in_ct, pcm_in, pcm_out, n);
	}
#endif
}
void AudioEffectFaderOutApply24(FaderUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#ifdef CFG_AUDIO_EFFECT_FADER_EN
	if((unit->enable) && (unit->out_ct != NULL))
	{
		fader_apply24(unit->out_ct, pcm_in, pcm_out, n);
	}
#endif
}
#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
/*
****************************************************************
* Pcm Delay音效初始化
*
*bit_width: 0=16bit, 1=24bit
****************************************************************
*/
void AudioEffectPcmDelayInit(PcmDelayUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}

    unit->bit_width = bit_width;
	
	if(unit->ct == NULL)
	{
		uint32_t s_buff_size;
		if(unit->high_quality)
		{
			s_buff_size = unit->max_delay_samples*2*channel;
		}
		else
		{
			s_buff_size = ceil(unit->max_delay_samples/32)*19*channel+64;;
		}
        unit->ct = (PCMDelay *)osPortMallocFromEnd(PCM_DELAY_SIZE + s_buff_size);
		
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("PCMDelay mem malloc err! %ld\n",PCM_DELAY_SIZE);
			return;
		}
		unit->s_buf = (uint8_t *)unit->ct + PCM_DELAY_SIZE;
	}
	
	if(unit->ct != NULL)
	{
		if(unit->bit_width==0)
		{
		  pcm_delay_init16(unit->ct, channel, unit->max_delay_samples, unit->high_quality, unit->s_buf);
		}
		else
		{
			pcm_delay_init24(unit->ct, channel, unit->max_delay_samples, unit->high_quality, unit->s_buf);
		}
	}	
}
#endif

#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
/*
****************************************************************
* Pcm Delay音效配置
*
*
****************************************************************
*/
void AudioEffectPcmDelayConfig(PcmDelayUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
		  pcm_delay_init16(unit->ct, unit->channel, unit->max_delay_samples, unit->high_quality, unit->s_buf);
		}
		else
		{
			pcm_delay_init24(unit->ct, unit->channel, unit->max_delay_samples, unit->high_quality, unit->s_buf);
		}
	}
}
#endif	
/*
****************************************************************
* Exciter音效初始化
*
*
****************************************************************
*/
void AudioEffectExciterInit(ExciterUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_EXCITER_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (ExciterContext *)osPortMallocFromEnd(EXCITER_SIZE);	
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("ExciterContext mem malloc err! %ld\n",EXCITER_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		exciter_init(unit->ct, channel, sample_rate, unit->f_cut);
	}
#endif
}
/*
****************************************************************
* Exciter音效配置
*
*
****************************************************************
*/
void AudioEffectExciterConfig(ExciterUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_EXCITER_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		exciter_init(unit->ct, unit->channel, sample_rate, unit->f_cut);
	}
#endif
}

/*
****************************************************************
* Aec音效初始化
*
*
****************************************************************
*/
const uint8_t es_level_tab[]={
0,1,2,3,4,5, 11,12,13,14,15,		 //es_level Echo suppression level. (0, 1~5, 11~15)
};

void AudioEffectAecInit(AecUnit *unit,  uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_AEC_EN
   //const int32_t es_level_tab[]={0,1,2,3,4,5,11,12,13,14,15,15,};
   int32_t es_level;

	if( (!gCtrlVars.audio_effect_init_flag) || (!unit->enable))
	{
		return;
	}
	
	if(unit->ct == NULL)
	{	
		unit->ct = (BlueAECContext *)osPortMallocFromEnd(AEC_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("BlueAECContext mem malloc err! %ld\n",AEC_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		es_level = es_level_tab[unit->es_level];
		APP_DBG("blue_aec_init Es level = %lu\n",es_level);
		blue_aec_init(unit->ct, es_level);
	}
#endif	
}
/*
****************************************************************
* Expander音效初始化
*
*
****************************************************************
*/
void AudioEffectExpanderInit(ExpanderUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_NOISE_SUPPRESSOR_EN || CFG_AUDIO_EFFECT_MIC_NOISE_SUPPRESSOR_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}

    if(unit->channel == 0)
    {
		unit->channel = channel;
    }
	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (ExpanderContext *)osPortMallocFromEnd(EXPANDER_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("ExpanderContext mem malloc err! %ld\n",AEC_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		expander_init(unit->ct,  channel, sample_rate,
								 unit->threshold,
								 unit->ratio,
								 unit->attack_time,
								 unit->release_time);
	}
#endif
}
/*
****************************************************************
* Expander音效INIT参数配置
*
*
****************************************************************
*/
void AudioEffectExpanderConfig(ExpanderUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_NOISE_SUPPRESSOR_EN || CFG_AUDIO_EFFECT_MIC_NOISE_SUPPRESSOR_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		expander_init(unit->ct,  channel, sample_rate,
								 unit->threshold,
								 unit->ratio,
								 unit->attack_time,
								 unit->release_time);
	}
#endif
}
/*
****************************************************************
* Expander音效配置函数
* 1，适用于实时调节的场合
*
****************************************************************
*/
void AudioEffectExpanderThresholdConfig(ExpanderUnit *unit)
{
#if CFG_AUDIO_EFFECT_MUSIC_NOISE_SUPPRESSOR_EN || CFG_AUDIO_EFFECT_MIC_NOISE_SUPPRESSOR_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}
	
	if(unit->ct != NULL)
	{
		expander_set_threshold(unit->ct,  unit->threshold);
	}
#endif
}
/*
****************************************************************
* FreqShifter音效初始化
*
*
****************************************************************
*/
void AudioEffectFreqShifterInit(FreqShifterUnit *unit)
{
#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (FreqShifterContext *)osPortMallocFromEnd(FREQSHIFTER_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("FreqShifterContext mem malloc err! %ld\n",FREQSHIFTER_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		freqshifter_init(unit->ct,unit->deltaf);  
	}
#endif
}
/*
****************************************************************
* FreqShifter音效配置
*
*
****************************************************************
*/
void AudioEffectFreqShifterConfig(FreqShifterUnit *unit)
{
#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct != NULL)
	{
		freqshifter_init(unit->ct,unit->deltaf);  
	}
#endif
}
/*
****************************************************************
* FreqShifter fine音效初始化
*
*
****************************************************************
*/
void AudioEffectFreqShifterFineInit(FreqShifterFineUnit *unit, int32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{

		unit->ct = (FreqShifterFineContext *)osPortMallocFromEnd(FREQSHIFTER_FINE_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("FreqShifterFine mem malloc err! %ld\n",FREQSHIFTER_FINE_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		freqshifter_fine_init(unit->ct,sample_rate,unit->deltaf);
	}
#endif
}
/*
****************************************************************
* HowlingDector音效初始化
*
*
****************************************************************
*/
void AudioEffectHowlingSuppressorInit(HowlingDectorUnit *unit)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (HowlingContext *)osPortMallocFromEnd(HOWLING_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("HowlingContext mem malloc err! %ld\n",HOWLING_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		howling_suppressor_init(unit->ct, unit->suppression_mode);
	}
#endif
}
/*
****************************************************************
* HowlingDector音效配置
*
*
****************************************************************
*/
void AudioEffectHowlingSuppressorConfig(HowlingDectorUnit *unit)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		howling_suppressor_init(unit->ct, unit->suppression_mode);
	}
#endif
}


/*
****************************************************************
* HowlingDector Fine音效初始化
*
*
****************************************************************
*/
void AudioEffectHowlingSuppressorFineInit(HowlingDectorFineUnit *unit,uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (HowlingFineContext *)osPortMallocFromEnd(HOWLING_FINE_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("HowlingFineContext malloc err! %ld\n",HOWLING_FINE_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		howling_suppressor_fine_init(unit->ct, sample_rate);
	}
#endif
}
/*
****************************************************************
* HowlingDectorFine主循环处理函数
*
*
****************************************************************
*/
void AudioEffectHowlingSuppressorFineApply(HowlingDectorFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		howling_suppressor_fine_apply(unit->ct,  pcm_in, pcm_out, n,unit->q_min,unit->q_max);
	}
#endif
}
/*
****************************************************************
* HowlingDectorFine24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectHowlingSuppressorFineApply24(HowlingDectorFineUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		howling_suppressor_fine_apply24(unit->ct,  pcm_in, pcm_out, n,unit->q_min,unit->q_max);
	}
#endif
}
/*
****************************************************************
* SilenceDector音效初始化
*
*
****************************************************************
*/
void AudioEffectSilenceDectorInit(SilenceDetectorUnit *unit,uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN || CFG_AUDIO_EFFECT_MIC_SILENCE_DECTOR_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

    if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (SilenceDetectorContext *)osPortMallocFromEnd(SILENCE_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("SilenceDetectorContext mem malloc err! %ld\n",SILENCE_SIZE);
		}	
	}
		
	if(unit->ct != NULL)
	{
	   silence_detector_init(unit->ct, channel, sample_rate);
	}
#endif
}
/*
****************************************************************
* PitchShifter音效初始化
*
*bit_width:0=16bit, 1=24it
****************************************************************
*/
void AudioEffectPitchShifterInit(PitchShifterUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width)
{
#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN
	uint32_t size;
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	unit->bit_width = bit_width;
	
	if(unit->bit_width==0)
	{
		size = sizeof(PitchShifterContext16);
	}
	else
	{
		size = sizeof(PitchShifterContext24);
	}
	if(unit->ct == NULL)
	{
		unit->ct = (void *)osPortMallocFromEnd(size);

		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("PSContext mem malloc err! %ld\n",size);
		}
	}
		
	if(unit->ct != NULL)
	{
		//unit->ct->w = 300;//////改善延时!!!!!!!!!!!!!

		//gCtrlVars.SamplesPerFrame = CFG_MIC_PITCH_SHIFTER_FRAME_SIZE / 2;
		if(unit->bit_width==0)
		{
			pitch_shifter_init16((PitchShifterContext16 *)unit->ct, channel, sample_rate, unit->semitone_steps, CFG_MIC_PITCH_SHIFTER_FRAME_SIZE);//512
		}
		else
		{
			pitch_shifter_init24((PitchShifterContext24 *)unit->ct, channel, sample_rate, unit->semitone_steps, CFG_MIC_PITCH_SHIFTER_FRAME_SIZE);//512

		}

	}
#endif
}
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
/*
****************************************************************
* PitchShifterPro音效初始化
*
*
****************************************************************
*/
void AudioEffectPitchShifterProInit(PitchShifterProUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (PitchShifterProContext *)osPortMallocFromEnd(PS_PRO_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("PitchShifterProContext mem malloc err! %ld\n",PS_PRO_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		pitch_shifter_pro_init(unit->ct, channel, sample_rate, unit->semitone_steps, CFG_MUSIC_PITCH_SHIFTER_PRO_FRAME_SIZE);
		gCtrlVars.SamplesPerFrame = CFG_MUSIC_PITCH_SHIFTER_PRO_FRAME_SIZE;
	}
#endif
}
#endif
/*
****************************************************************
* PitchShifter音效参数配置
* 1，适用于实时调节的场合
*
****************************************************************
*/
void AudioEffectPitchShifterConfig(PitchShifterUnit *unit)
{
#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN
	/*if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}*/
	
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
		  pitch_shifter_configure16((PitchShifterContext16 *)unit->ct, unit->semitone_steps);
		}
		else
		{
			pitch_shifter_configure24((PitchShifterContext24 *)unit->ct, unit->semitone_steps);
		}
	}
#endif
}
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
/*
****************************************************************
* PitchShifterPro音效参数配置
* 1，适用于实时调节的场合
*
****************************************************************
*/
void AudioEffectPitchShifterProConfig(PitchShifterProUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
	if((unit->enable)&&(unit->ct != NULL))
	{
		pitch_shifter_pro_init(unit->ct, channel, sample_rate, unit->semitone_steps, CFG_MUSIC_PITCH_SHIFTER_PRO_FRAME_SIZE);
	}
#endif
}
#endif
/*
****************************************************************
* AutoTune音效初始化
*
*bit_width :0=16bit, 1=24bit
****************************************************************
*/
void AudioEffectAutoTuneInit(AutoTuneUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width)
{
#if CFG_AUDIO_EFFECT_MIC_AUTO_TUNE_EN
	uint32_t size;
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
    	 unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	unit->bit_width = bit_width;//0=16bit, 1=24bit

	if(bit_width==0)
	{
		size = sizeof(AutoTuneContext16);
	}
	else
	{
		size = sizeof(AutoTuneContext24);
	}

	if(unit->ct == NULL)
	{
		unit->ct = (void *)osPortMallocFromEnd(size);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("AutoTuneContext mem malloc err! %ld\n",size);
		}
	}
		
	if(unit->ct != NULL)
	{
		if(bit_width==0)
		{
			auto_tune_init16((AutoTuneContext16 *)unit->ct, channel, sample_rate, unit->key, mainAppCt.SamplesPreFrame);
		}
		else
		{
			auto_tune_init24((AutoTuneContext24 *)unit->ct, channel, sample_rate, unit->key, mainAppCt.SamplesPreFrame);
		}
	}
#endif
}

/*
****************************************************************
* AutoTune音效配置
*
*bit_width: 0=16bit, 1=24bit
****************************************************************
*/
void AudioEffectAutoTuneConfig(AutoTuneUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_AUTO_TUNE_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
			auto_tune_init16((AutoTuneContext16 *)unit->ct, unit->channel, sample_rate, unit->key, mainAppCt.SamplesPreFrame);
		}
		else
		{
			auto_tune_init24((AutoTuneContext24 *)unit->ct, unit->channel, sample_rate, unit->key, mainAppCt.SamplesPreFrame);
		}
	}
#endif
}

/*
****************************************************************
* VoiceChanger音效初始化
*
*
****************************************************************
*/
void AudioEffectVoiceChangerInit(VoiceChangerUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (VoiceChangerContext *)osPortMallocFromEnd(VOICECHANGER_SIZE);			
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("VoiceChangerContext mem malloc err! %ld\n",VOICECHANGER_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		voice_changer_init(unit->ct, channel, sample_rate, unit->pitch_ratio, unit->formant_ratio);//512
	}
#endif
}
/*
****************************************************************
* VoiceChanger音效配置
*
*
****************************************************************
*/
void AudioEffectVoiceChangerConfig(VoiceChangerUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		voice_changer_init(unit->ct, channel, sample_rate, unit->pitch_ratio, unit->formant_ratio);//512
	}
#endif
}
#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
/*
****************************************************************
* VoiceChangerPro音效初始化
*
*
****************************************************************
*/
void AudioEffectVoiceChangerProInit(VoiceChangerProUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if 0//CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (VoiceChangerProContext *)osPortMallocFromEnd(VOICECHANGER_PRO_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("VoiceChangerProContext mem malloc err! %ld\n",VOICECHANGER_PRO_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		voice_changer_pro_init(unit->ct, sample_rate, 256, unit->pitch_ratio, unit->formant_ratio);///256				
	}
#endif
}
#endif

#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
/*
****************************************************************
* VoiceChangerPro音效配置
*
*
****************************************************************
*/
void AudioEffectVoiceChangerProConfig(VoiceChangerProUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if 0
	if((unit->enable)&&(unit->ct != NULL))
	{
		voice_changer_pro_init(unit->ct, sample_rate, 256, unit->pitch_ratio, unit->formant_ratio);///256				
	}
#endif
}
#endif
/*
****************************************************************
* Echo音效初始化
*
*bit_width:0=16bit,1=24bit
****************************************************************
*/
void AudioEffectEchoInit(EchoUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width)
{
#if CFG_AUDIO_EFFECT_MIC_ECHO_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}

	unit->bit_width = bit_width;

	if(unit->ct == NULL)
	{
        uint32_t s_buff_size;

        if(unit->high_quality==1)
        {
        	s_buff_size = 	unit->max_delay_samples*2;
        }
        else
        {
        	s_buff_size = ceil(unit->max_delay_samples/32)*19+64;
        }
        unit->ct = (EchoContext *)osPortMallocFromEnd(ECHO_SIZE + s_buff_size);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("EchoContext mem malloc err! %ld\n",ECHO_SIZE);
			return;
		}
        unit->s_buf = (uint8_t *)unit->ct + ECHO_SIZE; //(uint8_t *)osPortMallocFromEnd(size);
	}
	if(unit->ct != NULL)
	{
		if(unit->bit_width==0)
		{
			echo_init16(unit->ct, channel, sample_rate, unit->fc, unit->max_delay_samples, unit->high_quality, unit->s_buf);
		}
		else
		{
			echo_init24(unit->ct, channel, sample_rate, unit->fc, unit->max_delay_samples, unit->high_quality, unit->s_buf);
		}
	}
#endif
}

/*
****************************************************************
* Echo音效配置
*
*
****************************************************************
*/
void AudioEffectEchoConfig(EchoUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_ECHO_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
		  echo_init16(unit->ct, channel, sample_rate, unit->fc, unit->max_delay_samples, unit->high_quality,unit->s_buf);
		}
		else
		{
			echo_init24(unit->ct, channel, sample_rate, unit->fc, unit->max_delay_samples, unit->high_quality,unit->s_buf);
		}
	}
#endif
}
/*
****************************************************************
* Reverb音效初始化
*
*bit_width: 0=16bit, 1=24bit
****************************************************************
*/
void AudioEffectReverbInit(ReverbUnit *unit, uint8_t channel, uint32_t sample_rate,uint8_t bit_width)
{
#if CFG_AUDIO_EFFECT_MIC_REVERB_EN
	uint32_t size;
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	unit->bit_width = bit_width;

	if(unit->bit_width==0)
	{
		size = sizeof(ReverbContext16);
	}
	else
	{
		size = sizeof(ReverbContext24);
	}

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (void *)osPortMallocFromEnd(size);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("ReverbContext mem malloc err! %ld\n",size);
		}
	}
		
	if(unit->ct != NULL)
	{
		if(unit->bit_width==0)
		{
			reverb_init16((ReverbContext16 *)unit->ct, channel, sample_rate);
			reverb_configure16((ReverbContext16 *)unit->ct, unit->dry_scale, unit->wet_scale, unit->width_scale, unit->roomsize_scale, unit->damping_scale);
		}
		else
		{
			reverb_init24((ReverbContext24 *)unit->ct, channel, sample_rate);
			reverb_configure24((ReverbContext24 *)unit->ct, unit->dry_scale, unit->wet_scale, unit->width_scale, unit->roomsize_scale, unit->damping_scale);
		}
	}
#endif
}
/*
****************************************************************
* Reverb音效配置函数
* 1，适用于实时调节的场合
*
****************************************************************
*/
void AudioEffectReverbConfig(ReverbUnit *unit)
{
#if CFG_AUDIO_EFFECT_MIC_REVERB_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
	//	return;
	}
	
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
			reverb_configure16((ReverbContext16 *)unit->ct, unit->dry_scale, unit->wet_scale, unit->width_scale, unit->roomsize_scale, unit->damping_scale);
		}
		else
		{
			reverb_configure24((ReverbContext24 *)unit->ct, unit->dry_scale, unit->wet_scale, unit->width_scale, unit->roomsize_scale, unit->damping_scale);
		}
	}
#endif
}
/*
****************************************************************
* PlateReverb音效初始化
*
*bit_width: 0=16bit, 1=24bit
****************************************************************
*/
void AudioEffectPlateReverbInit(ReverbPlateUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_PLATE_REVERB_EN

	uint32_t PLATE_REVERB_PLATE_SIZE;

	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	//unit->bit_width;//only 16, 24

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		if(unit->bit_width==0)
		{
		   unit->ct = (void *)osPortMallocFromEnd(REVERB_PLATE16_SIZE);
		   PLATE_REVERB_PLATE_SIZE = REVERB_PLATE16_SIZE;
		}
		else if(unit->bit_width==1)
		{
		   unit->ct = (void *)osPortMallocFromEnd(REVERB_PLATE24_SIZE);
		   PLATE_REVERB_PLATE_SIZE = REVERB_PLATE24_SIZE;
		}
		else
		{
			unit->enable = 0;
			APP_DBG("PlateReverbContext bit width err!n");
			return;
		}
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("PlateReverbContext mem malloc err! %ld\n",PLATE_REVERB_PLATE_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		if(unit->bit_width==0)
		{
			reverb_plate_init16((ReverbPlateContext16 *)unit->ct, channel, sample_rate,unit->highcut_freq,unit->modulation_en);
			reverb_plate_configure((ReverbPlateContext16 *)unit->ct, unit->predelay, unit->diffusion, unit->decay, unit->damping, unit->wetdrymix);
		}
		if(unit->bit_width==1)
		{
			reverb_plate_init24((ReverbPlateContext24 *)unit->ct, channel, sample_rate,unit->highcut_freq,unit->modulation_en);
			reverb_plate_configure((ReverbPlateContext24 *)unit->ct, unit->predelay, unit->diffusion, unit->decay, unit->damping, unit->wetdrymix);
		}

	}
#endif
}
/*
****************************************************************
* PlateReverb音效modulatio配置
*
*
****************************************************************
*/
void AudioEffectPlateReverbHighcutModulaConfig(ReverbPlateUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MIC_PLATE_REVERB_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
			reverb_plate_init16((ReverbPlateContext16 *)unit->ct, channel, sample_rate,unit->highcut_freq,unit->modulation_en);
			reverb_plate_configure((ReverbPlateContext16 *)unit->ct, unit->predelay, unit->diffusion, unit->decay, unit->damping, unit->wetdrymix);
		}
		if(unit->bit_width==1)
		{
			reverb_plate_init24((ReverbPlateContext24 *)unit->ct, channel, sample_rate,unit->highcut_freq,unit->modulation_en);
			reverb_plate_configure((ReverbPlateContext24 *)unit->ct, unit->predelay, unit->diffusion, unit->decay, unit->damping, unit->wetdrymix);
		}
	}
#endif
}

void AudioEffectPlateReverbConfig(ReverbPlateUnit *unit)
{
#if CFG_AUDIO_EFFECT_MIC_PLATE_REVERB_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
	//	return;
	}
	
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
			reverb_plate_configure((ReverbPlateContext16 *)unit->ct, unit->predelay, unit->diffusion, unit->decay, unit->damping, unit->wetdrymix);
		}
		if(unit->bit_width==1)
		{
			reverb_plate_configure((ReverbPlateContext24 *)unit->ct, unit->predelay, unit->diffusion, unit->decay, unit->damping, unit->wetdrymix);
		}
	}
#endif
}
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
/*
****************************************************************
* ReverbPro音效初始化
*
*bit_width: 0= 16bit, 1=24bit
****************************************************************
*/
void AudioEffectReverbProInit(ReverbProUnit *unit, uint8_t channel, uint32_t sample_rate,uint32_t bit_width)
{
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN

	uint32_t real_size=0,ret;
	int32_t erfactor;
	int32_t delay;
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel==0)
    {
    	 unit->channel = channel;
    }

	channel = unit->channel;
	
	unit->bit_width = bit_width;

	if(!unit->enable)
	{
		return;
	}
	if((sample_rate>48000) || (sample_rate < 32000))
	{
		APP_DBG("Pro Reverb sample must <= 44100\n");
		return;
	}
	unit->sample_rate = sample_rate;
	// default (erfactor=160, delay=20)
	//maximum (erfactor=250, delay=100)
	 erfactor = 250;
	 delay    = 100;
	
	if(unit->bit_width==0)
	 {
		 bit_width = 16;
	 }
	 else
	 {
		 bit_width = 24;
	 }
	ret = reverb_pro_estimate_memory_usage(bit_width, sample_rate, erfactor, delay, &real_size);

	if( (unit->ct == NULL) && (ret == REVERB_PRO_ERROR_OK) )
	{
			unit->ct = (uint8_t *)osPortMallocFromEnd(real_size);
			if(unit->ct == NULL)
			{
				unit->enable = 0;
				APP_DBG("ReverbPro 44k mem malloc err! %ld\n",real_size);
				return;

			}			
	}
		
	if(unit->ct != NULL)
	{
		reverb_pro_init(unit->ct,bit_width,sample_rate,\
				        unit->dry,unit->wet,unit->erwet,\
						unit->erfactor,unit->erwidth,unit->ertolate,\
						unit->rt60,unit->delay,unit->width,unit->wander,unit->spin,\
			            unit->inputlpf,unit->damplpf,unit->basslpf,unit->bassb,unit->outputlpf);

		real_size  = reverb_pro_context_size(unit->ct);
	}

#endif
}
#endif
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
/*
****************************************************************
* ReverProb音效配置函数
* 1，适用于实时调节的场合
* 2.暂时只调节干声或湿声
****************************************************************
*/
void AudioEffectReverProbConfig(ReverbProUnit *unit,uint32_t sample_rate)
{

	uint32_t bit_width;
	if((!unit->enable)||(unit->ct == NULL))
	{
		return;
	}
	if(unit->bit_width==0)
	{
		bit_width = 16;
	}
	else
	{
		bit_width = 24;
	}


	reverb_pro_init(unit->ct,bit_width,sample_rate,\
			        unit->dry,unit->wet,unit->erwet,\
					unit->erfactor,unit->erwidth,unit->ertolate,\
					unit->rt60,unit->delay,unit->width,unit->wander,unit->spin,\
		            unit->inputlpf,unit->damplpf,unit->basslpf,unit->bassb,unit->outputlpf);

}

void AudioEffectReverProbParameterConfig(ReverbProUnit *unit)
{
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable)||(unit->ct == NULL))
	{
		return;
	}
    reverb_pro_configure_level(unit->ct, unit->dry, unit->wet, unit->erwet);

    reverb_pro_configure_rt60(unit->ct, unit->rt60);

    reverb_pro_configure_width(unit->ct, unit->width);
}
#endif
/*
****************************************************************
* VocalCut音效初始化
*
*
****************************************************************
*/
void AudioEffectVocalCutInit(VocalCutUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;
	
	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (VocalCutContext *)osPortMallocFromEnd(VOCALCUT_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("VocalCutContext mem malloc err! %ld\n",VOCALCUT_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		vocal_cut_init(unit->ct, sample_rate);			
	}
#endif
}
/*
****************************************************************
* VocalRemove音效初始化
*
*
****************************************************************
*/
void AudioEffectVocalRemoveInit(VocalRemoveUnit *unit,  uint8_t channel, uint32_t sample_rate,uint8_t bit_width)
{
#if 0//CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
	uint32_t size;
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
	if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;
	
	if(!unit->enable)
	{
		return;
	}
	unit->bit_width = bit_width;

	if(unit->bit_width==0)
	{
		size = sizeof(VocalRemoverContext16);
	}
	else
	{
		size = sizeof(VocalRemoverContext24);
	}

	if(unit->ct == NULL)
	{
	    unit->ct = (void *)osPortMallocFromEnd(size);
	    if(unit->ct == NULL)
	    {
			unit->enable = 0;
			APP_DBG("VocalRemoverContext mem malloc err! %ld\n",size);
	    }
	}
		
	if(unit->ct != NULL)
	{
		if(unit->bit_width==0)
		{
			vocal_remover_init16((VocalRemoverContext16 *)unit->ct, sample_rate,unit->lower_freq,unit->higher_freq,unit->step_size);
		}
		else
		{
			vocal_remover_init24((VocalRemoverContext24 *)unit->ct, sample_rate,unit->lower_freq,unit->higher_freq,unit->step_size);
		}
	}
#endif
}
/*
****************************************************************
* VocalRemove音效配置
*
*
****************************************************************
*/
void AudioEffectVocalRemoveConfig(VocalRemoveUnit *unit,  uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN	
	if((unit->enable)&&(unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
			vocal_remover_init16((VocalRemoverContext16 *)unit->ct, sample_rate,unit->lower_freq,unit->higher_freq,unit->step_size);
		}
		else
		{
			vocal_remover_init24((VocalRemoverContext24 *)unit->ct, sample_rate,unit->lower_freq,unit->higher_freq,unit->step_size);
		}
	}
#endif
}
/*
****************************************************************
* Chorus音效初始化
*
*
****************************************************************
*/
void AudioEffectChorusInit(ChorusUnit *unit, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_CHORUS_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
	
	if(!unit->enable)
	{
		return;
	}
	if(unit->ct == NULL)
	{
	    unit->ct = (ChorusContext *)osPortMallocFromEnd(CHORUS_SIZE);					
	    if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("ChorusContext mem malloc err! %ld\n",CHORUS_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		chorus_init(unit->ct, sample_rate, unit->delay_length, unit->mod_depth, unit->mod_rate);
	}
#endif
}

/*
****************************************************************
* Chorus2音效初始化
*
*bit_width : 0= 16bit, 1= 24bit
****************************************************************
*/
void AudioEffectChorus2Init(Chorus2Unit *unit,   uint32_t sample_rate,uint16_t bit_width)
{
#if CFG_AUDIO_EFFECT_CHORUS2_EN
	uint32_t size;
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}

	if(!unit->enable)
	{
		return;
	}

	unit->bit_width = bit_width;

	if(bit_width==0)
	{
		size = sizeof(Chorus2Context16);
	}
	else
	{
		size = sizeof(Chorus2Context24);
	}


	if(unit->ct == NULL)
	{
	    unit->ct = (void *)osPortMallocFromEnd(size);
	    if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("Chorus2 mem malloc err! %ld\n",size);
		}
	}

	if(unit->ct != NULL)
	{

		if(unit->bit_width==0)
		{
			chorus2_init16((Chorus2Context16 *)unit->ct, sample_rate, unit->delay_length);
		}
		else
		{
			chorus2_init24((Chorus2Context24 *)unit->ct, sample_rate, unit->delay_length);
		}
	}
#endif
}
#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
/*
****************************************************************
* ThreeD音效初始化
*
*
****************************************************************
*/
void AudioEffectThreeDInit(ThreeDUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (ThreeDContext *)osPortMallocFromEnd(THREED_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("ThreeDContext mem malloc err! %ld\n",THREED_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		three_d_init(unit->ct, channel,	sample_rate);	
	}
}
#endif
#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
/*
****************************************************************
* ThreeD Plus音效初始化
*
*
****************************************************************
*/
void AudioEffectThreeDPlusInit(ThreeDPlusUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (ThreeDPlusContext *)osPortMallocFromEnd(THREED_PLUS_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("ThreeDPlusContext mem malloc err! %ld\n",THREED_PLUS_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		three_d_plus_init(unit->ct, sample_rate);	
	}
}
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
/*
****************************************************************
* VB音效初始化
*
*
****************************************************************
*/
void AudioEffectVBInit(VBUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	uint32_t ram_size;

	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		if(unit->vb_type ==0)//0=normal, 1=td
		{
		   unit->ct = (void *)osPortMallocFromEnd(VB_SIZE);
		   ram_size = VB_SIZE;
		}
		else if(unit->vb_type ==1)//0=normal, 1=td
		{
		   unit->ct = (void *)osPortMallocFromEnd(VB_TD_SIZE);
		   ram_size = VB_TD_SIZE;
		}
		else
		{
			unit->enable = 0;
			APP_DBG("VBContext type  err!\n");
			return;
		}
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("VBContext mem malloc err! %ld\n",ram_size);
		}
	}
		
	if(unit->ct != NULL)
	{
		if(unit->vb_type ==0)//0=normal, 1=td
		{
			vb_init((VBContext *)unit->ct, channel,	sample_rate, unit->f_cut);
		}
		else if(unit->vb_type ==1)//0=normal, 1=td
		{
			vb_td_init((VBTDContext *)unit->ct, channel,	sample_rate, unit->f_cut);
		}
	}
}
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
/*
****************************************************************
* VB音效配置
*
*
****************************************************************
*/
void AudioEffectVBConfig(VBUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if((unit->enable)&&(unit->ct != NULL))
	{
		unit->channel = channel;

		if(unit->vb_type ==0)//0=normal, 1=td
		{
			vb_init((VBContext *)unit->ct, channel,	sample_rate, unit->f_cut);
		}
		else if(unit->vb_type ==1)//0=normal, 1=td
		{
			vb_td_init((VBTDContext *)unit->ct, channel,	sample_rate, unit->f_cut);
		}
	}
}
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
/*
****************************************************************
* VBClassic音效初始化
*
*
****************************************************************
*/
void AudioEffectVBClassicInit(VBClassicUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (VBClassicContext *)osPortMallocFromEnd(VBCLASSIC_SIZE);//(VBContext *)
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("VBContext mem malloc err! %ld\n",VBCLASSIC_SIZE);
		}
	}
		
	if(unit->ct != NULL)
	{
		vb_classic_init(unit->ct, channel,	sample_rate, unit->f_cut);	
	}
}
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
/*
****************************************************************
* VBClassic音效配置
*
*
****************************************************************
*/
void AudioEffectVBClassicConfig(VBClassicUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if((unit->enable)&&(unit->ct != NULL))
	{
		vb_classic_init(unit->ct, channel,	sample_rate, unit->f_cut);	
	}
}
#endif
/*
****************************************************************
* DRC音效初始化
*
*
****************************************************************
*/
void AudioEffectDRCInit(DRCUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_DRC_EN || CFG_AUDIO_EFFECT_MIC_DRC_EN || CFG_AUDIO_EFFECT_REC_DRC_EN
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (DRCContext *)osPortMallocFromEnd(DRC_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("DRCContext mem malloc err! %ldu\n",DRC_SIZE);
		}
	}
	
	if(unit->ct != NULL)
	{
		{
		   drc_init((DRCContext *)unit->ct, channel, sample_rate, unit->mode, unit->cf_type, unit->q_l, unit->q_h,
				    unit->fc, unit->threshold, unit->ratio, unit->attack, unit->release);
		}
	}
#endif
}

/*
****************************************************************
* DRC音效配置函数
*
*
****************************************************************
*/
void AudioEffectDRCConfig(DRCUnit *unit, uint8_t channel, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_MUSIC_DRC_EN || CFG_AUDIO_EFFECT_MIC_DRC_EN || CFG_AUDIO_EFFECT_REC_DRC_EN
	if((unit->enable)&&(unit->ct != NULL))
	{
		drc_init(unit->ct, channel, sample_rate, unit->mode, unit->cf_type, unit->q_l, unit->q_h,
				unit->fc, unit->threshold, unit->ratio, unit->attack, unit->release);
	}
#endif
}

/*
****************************************************************
* EQ音效初始化
*
*
****************************************************************
*/
void AudioEffectEQInit(EQUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}
	
    if(unit->channel == 0)
    {
		unit->channel = channel;
    }

	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}
	
	if(unit->ct == NULL)
	{
		unit->ct = (EQContext *)osPortMallocFromEnd(EQ_SIZE);				
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("EQContext mem malloc err! %ld\n",EQ_SIZE);
		}
	}
	
	if(unit->ct != NULL)
	{
		eq_clear_delay_buffer(unit->ct);
		if(unit->calculation_type)
		{
			eq_init_float(unit->ct, sample_rate, unit->filter_params, unit->filter_count, unit->pregain, channel);
		}
		else
		{
			eq_init(unit->ct, sample_rate, unit->filter_params, unit->filter_count, unit->pregain, channel);
		}
	}
}
/*
****************************************************************
* EQ预增益配置函数
*
*
****************************************************************
*/
void AudioEffectEQPregainConfig(EQUnit *unit)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		//return;
	}
	
	if((unit->enable)&&(unit->ct != NULL))
	{
		eq_configure_pregain(unit->ct, unit->pregain);
	}
}
/*
****************************************************************
* EQ滤波器配置函数
*
*
****************************************************************
*/
void AudioEffectEQFilterConfig(EQUnit *unit, uint32_t sample_rate)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		//return;
	}
	
	if((unit->enable)&&(unit->ct != NULL))
	{
		eq_configure_filters(unit->ct, sample_rate, unit->filter_params, unit->filter_count);
	}
}
void AudioEffectEQFilterClearBufConfig(EQUnit *unit, uint32_t sample_rate)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		//return;
	}
	
	if((unit->enable)&&(unit->ct != NULL))
	{
		eq_clear_delay_buffer(unit->ct);
		eq_configure_filters(unit->ct, sample_rate, unit->filter_params, unit->filter_count);
	}
}
/*
****************************************************************
* EQ_DRC音效初始化
*
*
****************************************************************
*/
#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
void AudioEffectEqDrcInit(EQ_DRCUnit *unit, uint8_t channel, uint32_t sample_rate)
{
	if(!gCtrlVars.audio_effect_init_flag)
	{
		return;
	}

	unit->channel = channel;
	channel = unit->channel;

	if(!unit->enable)
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (EQDRCContext *)osPortMallocFromEnd(EQ_DRC_STRUCT_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("EQDRCContext mem malloc err! %ld\n",EQ_DRC_STRUCT_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		eq_drc_init(unit->ct, channel, sample_rate,
			    unit->filter_params, unit->filter_count,
				unit->drc_mode, unit->cf_type, unit->q_l, unit->q_h,
				(int32_t *)unit->fc, (int32_t *)unit->threshold, (int32_t *)unit->ratio, (int32_t *)unit->attack_tc, (int32_t *)unit->release_tc);
	}
}
/*
****************************************************************
* EQ DRC主循环处理函数
*
*
****************************************************************
*/
void AudioEffectEqDrcApply(EQ_DRCUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
//	uint32_t cnt = n / EQ_BUFFER_NUM_SAMPLES;
//	uint32_t RemainLen = n - cnt * EQ_BUFFER_NUM_SAMPLES;
//	uint32_t i = 0;
//    uint32_t channel;
	if((unit->enable) && (unit->ct != NULL))
	{
//		channel = unit->channel;
		eq_drc_apply(unit->ct, (int16_t *)(pcm_in), (int16_t *)(pcm_out), n, unit->pregain);
	}
}
/*
****************************************************************
* EQ DRC滤波器配置函数
*
*
****************************************************************
*/
void AudioEffectEqDrcFilterConfigForEq(EQ_DRCUnit *unit, uint32_t sample_rate)
{
	if((unit->enable)&&(unit->ct != NULL))
	{
        eq_drc_configure_filters(unit->ct, sample_rate, unit->filter_params, unit->filter_count);
	}
}

void AudioEffectEqDrcFilterConfig(EQ_DRCUnit *unit, uint32_t sample_rate)
{
	if((unit->enable)&&(unit->ct != NULL))
	{
		eq_drc_init(unit->ct, unit->channel, sample_rate,
			    unit->filter_params, unit->filter_count,
				unit->drc_mode, unit->cf_type, unit->q_l, unit->q_h,
				unit->fc, unit->threshold, unit->ratio, unit->attack_tc, unit->release_tc);

		eq_drc_configure_filters(unit->ct, sample_rate, unit->filter_params, unit->filter_count);
	}
}
#endif
/*
****************************************************************
* StereoWidener音效初始化
*
*
****************************************************************
*/
void AudioEffectStereoWidenerInit(StereoWindenUnit *unit, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (StereoWidenerContext *)osPortMallocFromEnd(STEREO_WIDEN_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("stereo_widener_init mem malloc err! %ld\n",STEREO_WIDEN_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		stereo_widener_init(unit->ct, sample_rate, unit->shaping);
	}
#endif
}
/*
****************************************************************
* AutoWah音效初始化
*
*
****************************************************************
*/
void AudioEffectAutoWahInit(AutoWahUnit *unit, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_AUTOWAH_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (AutoWahContext *)osPortMallocFromEnd(AUTOWAH_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("AutoWahContext mem malloc err! %ld\n",AUTOWAH_SIZE);
			return;
		}
	}

	if(unit->ct != NULL)
	{
		auto_wah_init(unit->ct, sample_rate, unit->modulation_rate, unit->min_frequency, unit->max_frequency,unit->depth);
	}
#endif
}
/*
****************************************************************
* PingPong音效初始化
*
* bit_width :0= 16bit, 1=24bit
****************************************************************
*/
void AudioEffectPingPongInit(PingPongUnit *unit, uint32_t sample_rate,uint8_t bit_width)
{
#if CFG_AUDIO_EFFECT_PINGPONG_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}
    unit->bit_width = bit_width;

	if(unit->ct == NULL)
	{
		uint32_t s_buff_size=0;

		if(unit->high_quality)
		{
			s_buff_size = unit->max_delay_samples*4;
		}
		else
		{
			s_buff_size = ceilf(unit->max_delay_samples/32.0f) * 38;
		}
		if(s_buff_size==0)
		{
			unit->enable = 0;
			APP_DBG("PingPongContext S_buf mem malloc err! %ld\n",PINGPONG_SIZE);
			return;
		}

		unit->ct = (PingPongContext *)osPortMallocFromEnd(PINGPONG_SIZE + s_buff_size);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("PingPongContext mem malloc err! %ld\n",PINGPONG_SIZE);
			return;
		}
		unit->s = (uint8_t *)unit->ct + PINGPONG_SIZE; //(uint8_t *)osPortMallocFromEnd(size);
	}

	if(unit->ct != NULL)
	{
		if(unit->bit_width==0)
		{
			pingpong_init16(unit->ct, unit->max_delay_samples, unit->high_quality,unit->s);
		}
		else
		{
			pingpong_init24(unit->ct, unit->max_delay_samples, unit->high_quality,unit->s);
		}
	}
#endif
}
/*
****************************************************************
* Flanger 音效初始化
*
*bit_width : 0= 16bit, 1=24bit
****************************************************************
*/
void AudioEffectFlangerInit(FlangerUnit *unit,  uint32_t sample_rate,uint16_t bit_width)
{
#if CFG_AUDIO_EFFECT_FLANGER_EN
	uint32_t size;
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(bit_width==1)
	{
	  size = sizeof(FlangerContext24);
	}
	else
	{
		size = sizeof(FlangerContext16);
	}
	if(unit->ct == NULL)
	{
		unit->ct = (void *)osPortMallocFromEnd(size);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("FlangerContext mem malloc err! %ldu\n",size);
		}
	}

	if(unit->ct != NULL)
	{
		if(bit_width==0)
		{
			flanger_init16((FlangerContext24 *)unit->ct, sample_rate, unit->delay_length, unit->mod_depth, unit->mod_rate);
		}
		else
		{
			flanger_init24((FlangerContext16 *)unit->ct, sample_rate, unit->delay_length, unit->mod_depth, unit->mod_rate);
		}
	}
#endif
}
/*
****************************************************************
* Flanger主循环处理函数 for mono
*
*
****************************************************************
*/
void AudioEffectFlangerApply(FlangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_FLANGER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		flanger_apply16((FlangerContext16 *)unit->ct, pcm_in, pcm_out, n, unit->feedback, unit->dry, unit->wet, unit->mod_rate);
	}
#endif
}
void AudioEffectFlangerApply24(FlangerUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_FLANGER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		flanger_apply24((FlangerContext24 *)unit->ct, pcm_in, pcm_out, n, unit->feedback, unit->dry, unit->wet, unit->mod_rate);
	}
#endif
}
/*
****************************************************************
* Overdrive 音效初始化
* only mono
*
****************************************************************
*/
void AudioEffectOverdriveInit(OverdriveUnit *unit,  uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (OverdriveContext *)osPortMallocFromEnd(OVERDRIVE_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("OverdriveContext mem malloc err! %ldu\n",OVERDRIVE_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		overdrive_init(unit->ct, sample_rate, unit->threshold_compression);

	}
#endif
}
/*
****************************************************************
* Overdrive主循环处理函数 for mono
*
*
****************************************************************
*/
void AudioEffectOverdriveApply(OverdriveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		overdrive_apply(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}

/*
****************************************************************
* Overdrive Poly 音效初始化
* only mono
*
****************************************************************
*/
#if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
void AudioEffectOverdrivePolyInit(OverdrivePolyUnit *unit,  uint32_t sample_rate)
{
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (OverdrivePolyContext *)osPortMallocFromEnd(OVERDRIVER_POLY_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("OverdrivePolyUnit mem malloc err! %ldu\n",OVERDRIVER_POLY_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		overdrive_poly_init(unit->ct, sample_rate);
	}
}
/*
****************************************************************
* Overdrive poly主循环处理函数 for mono
*
*
****************************************************************
*/
void AudioEffectOverdrivePolyApply(OverdrivePolyUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		overdrive_poly_apply(unit->ct, pcm_in, pcm_out, n,unit->gain,unit->out_level);
	}
}
#endif //#if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
/*
****************************************************************
* DistortionExp 音效初始化
*
*
****************************************************************
*/
void AudioEffectDistortionExpInit(DistortionUnit *unit,  uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_DISTORTION_EN
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (DistortionExpContext *)osPortMallocFromEnd(DISTORTION_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("DistortionExpContext mem malloc err! %ldu\n",DISTORTION_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		distortion_exp_init(unit->ct, sample_rate, unit->gain);
	}
#endif
}
/*
****************************************************************
* DistortionExp主循环处理函数 for mono
*
*
****************************************************************
*/
void AudioEffectDistortionExpApply(DistortionUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_DISTORTION_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		distortion_exp_apply(unit->ct, pcm_in, pcm_out, n, unit->gain, unit->dry, unit->wet);
	}
#endif
}
/*
****************************************************************
* DistortionDS1主循环处理函数 for mono
*
*
****************************************************************
*/
#if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
void AudioEffectDistortionDS1Apply(DistortionDS1Unit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		distortion_ds1_apply(unit->ct, pcm_in, pcm_out, n, unit->out_level);
	}
}
/*
****************************************************************
* DistortionDS1配置处理函数 for mono
*
*
****************************************************************
*/
void AudioEffectDistortionDS1Configer(DistortionDS1Unit *unit)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		distortion_ds1_configure_distortion(unit->ct, unit->distortion_level);
	}
}
/*
****************************************************************
* DistortionDS1 音效初始化
*
*
****************************************************************
*/
void AudioEffectDistortionDS1Init(DistortionDS1Unit *unit,  int32_t sample_rate)
{
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (DistortionDS1Context *)osPortMallocFromEnd(DISTORTION_DS1_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("DistortionDS1Context mem malloc err! %ldu\n",DISTORTION_DS1_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		distortion_ds1_init(unit->ct, sample_rate, unit->distortion_level);
	}
}
#endif //#if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN

/*
****************************************************************
* pitch_detector paramater音效初始化
*
* 调试参数时可以使用按键对下面的参数进行更改，然后再调用
* 初始化应用，并且要求帧变化的消息发送一下
* 或调用一次 AudioEffectModeSel(mainAppCt.EffectMode, 2);//0=init hw,1=effect,2=hw+effect
* 以实现所有RAM的重分配
****************************************************************
*/
void AudioEffectPitchDetectorParameterInit(void)
{
#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	int32_t min_hz;
	APP_DBG("%s\n",__func__);
	gCtrlVars.pitch_detector_unit.pitch_min = 30;//unit: HZ MIN = 30HZ
	gCtrlVars.pitch_detector_unit.pitch_max = 2000;//unit: HZ,MAX= 2000HZ
	gCtrlVars.pitch_detector_unit.confidence_threshold = 8000;//Range: 5000~10000 for 50.00% ~ 100%
	gCtrlVars.pitch_detector_unit.confidence = 0;//result
	gCtrlVars.pitch_detector_unit.window_size = CFG_PARA_PITCH_DECT_SAMPLES_PER_FRAME;//must=2048
	gCtrlVars.pitch_detector_unit.step_size = gCtrlVars.pitch_detector_unit.window_size;// <= Windows_size

	if(gCtrlVars.pitch_detector_unit.step_size>gCtrlVars.pitch_detector_unit.window_size)
	{
		APP_DBG("pitch_detector step size err! Default value = window_size\n");
		gCtrlVars.pitch_detector_unit.step_size=gCtrlVars.pitch_detector_unit.window_size;
	}
#endif
}
/*
****************************************************************
* pitch_detector 音效初始化
*
*
****************************************************************
*/
void AudioEffectPitchDetectorInit(PitchDetectorUnit *unit,  uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
    int32_t ret;
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(sample_rate>48000)
	{
		APP_DBG("PitchDetector SampleRate > 48000, faile!\n");
		return;
	}
	if(unit->ct == NULL)
	{
		unit->ct = (PitchDetectorContext *)osPortMallocFromEnd(PITCH_DECTOR_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("PitchDetectorContext mem malloc err! %ldu\n",PITCH_DECTOR_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		ret = pitch_detector_init(unit->ct, sample_rate, unit->pitch_min,unit->pitch_max,
				            unit->window_size,unit->step_size,unit->confidence_threshold);
		if(ret !=0)
		{
			DBG("pitch_detector_init Fail\n");
			unit->enable = 0;
			osPortFree(unit->ct);
			unit->ct = NULL;
		}
		else
		{
			DBG("pitch_detector_init ok\n");
		}
	}
#endif
}
/*
****************************************************************
* pitch_detector主循环处理函数 for mono
*
*
****************************************************************
*/
void AudioEffectPitchDetectorApply(PitchDetectorUnit *unit, int16_t *pcm_in,uint32_t n)
{
#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	static uint32_t pitch_cnt = 0;
	int32_t pitch_hz;
	if((unit->enable) && (unit->ct != NULL))
	{
		pitch_hz = pitch_detector_apply(unit->ct, pcm_in,&unit->confidence);
		if(++pitch_cnt > 50)//此值越大，判断测试的结果越准确
		{
			pitch_cnt = 0;
		    DBG("conf :%ld hz:%ld\n",unit->confidence,pitch_hz);
		}
	}
#endif
}
/*
****************************************************************
* Pcm Delay主循环处理函数
*
*
****************************************************************
*/
#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
void AudioEffectPcmDelayApply(PcmDelayUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		pcm_delay_apply16(unit->ct, pcm_in, pcm_out, n, unit->delay_samples);
	}
}

void AudioEffectPcmDelayApply24(PcmDelayUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		pcm_delay_apply24(unit->ct, pcm_in, pcm_out, n, unit->delay_samples);
	}
}

#endif
#if CFG_AUDIO_EFFECT_MIC_AEC_EN
/*
****************************************************************
* Aec主循环处理函数
*
*
****************************************************************
*/
void AudioEffectAecApply(AecUnit *unit, int16_t *u_pcm_in, int16_t *d_pcm_in, int16_t *pcm_out, uint32_t n)
{
    uint16_t fram,i/*,samples*/;

	fram = n/AEC_BLK_LEN;
	if(fram == 0)
	{
		fram = 1;
	}
			
	if((unit->enable) && (unit->ct != NULL))
	{
        for(i = 0; i < fram; i++)	
    	{
			blue_aec_run(unit->ct,  (int16_t *)(u_pcm_in + i * AEC_BLK_LEN), (int16_t *)(d_pcm_in + i * AEC_BLK_LEN), (int16_t *)(pcm_out + i * AEC_BLK_LEN));
    	}
	}
}
#endif
/*
****************************************************************
* Exciter主循环处理函数
*
*
****************************************************************
*/
void AudioEffectExciterApply(ExciterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		exciter_apply(unit->ct, pcm_in, pcm_out, n, unit->dry, unit->wet);		
	}
}
void AudioEffectExciterApply24(ExciterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		exciter_apply24(unit->ct, pcm_in, pcm_out, n, unit->dry, unit->wet);
	}
}
/*
****************************************************************
* Expander主循环处理函数
*
*
****************************************************************
*/
void AudioEffectExpanderApply(ExpanderUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		expander_apply(unit->ct, pcm_in, pcm_out, n);
	}
}
/*
****************************************************************
* FreqShifter主循环处理函数
*
*
****************************************************************
*/
void AudioEffectFreqShifterApply(FreqShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{	
	if((unit->enable) && (unit->ct != NULL))
	{
		freqshifter_apply(unit->ct, pcm_in, pcm_out, n);
	}
}
/*
****************************************************************
* FreqShifter24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectFreqShifterApply24(FreqShifterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		freqshifter_apply24(unit->ct, pcm_in, pcm_out, n);
	}
}
/*
****************************************************************
* FreqShifterFine主循环处理函数
*
*
****************************************************************
*/
void AudioEffectFreqShifterFineApply(FreqShifterFineUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		freqshifter_fine_apply(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}
/*
****************************************************************
* FreqShifterFine24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectFreqShifterFineApply24(FreqShifterFineUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		freqshifter_fine_apply24(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}
/*
****************************************************************
* HowlingDector主循环处理函数
*
*
****************************************************************
*/
void AudioEffectHowlingSuppressorApply(HowlingDectorUnit *unit, int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		howling_suppressor_apply(unit->ct,  pcm_in, pcm_out, n);
	}
}
/*
****************************************************************
* HowlingDector24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectHowlingSuppressorApply24(HowlingDectorUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{

	}
}
/*
****************************************************************
* SilenceDector主循环处理函数
*
*
****************************************************************
*/
#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
uint32_t  Silence_Power_Off_Time = SILENCE_POWER_OFF_DELAY_TIME;
#endif
void AudioEffectSilenceDectorApply(SilenceDetectorUnit *unit, int16_t *pcm_in, int32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in == NULL) return;
		unit->level = silence_detector_apply(unit->ct, pcm_in,n);
		#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
		if(unit->level > SILENCE_THRESHOLD)
			Silence_Power_Off_Time = SILENCE_POWER_OFF_DELAY_TIME;
		#endif
	}
}
void AudioEffectSilenceDectorApply24(SilenceDetectorUnit *unit, int32_t *pcm_in, int32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		unit->level = silence_detector_apply24(unit->ct, pcm_in,n)&0x7fff;//max=32768

		#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
		if(unit->level > SILENCE_THRESHOLD)
			Silence_Power_Off_Time = SILENCE_POWER_OFF_DELAY_TIME;
		#endif
	}
}

/*
****************************************************************
* Phase主循环处理函数
*
*
****************************************************************
*/
void AudioEffectPhaseApply(PhaseControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n,  uint8_t channel)
{
	int32_t s;

    if(unit->channel == 0)
    {
    	 unit->channel = channel;
    }

	if(unit->enable && unit->phase_difference)
	{
		channel = unit->channel;
		for(s=0; s < n * channel; s++)		
		{			
			pcm_in[s] = __nds32__clips(((int32_t)pcm_in[s] * (-1)), (16)-1);		
		}	
	}
}

/*
****************************************************************
* Pregain主循环处理函数
*
*
****************************************************************
*/
void AudioEffectPregainApply(GainControlUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n,  uint8_t channel)
{
#if 1
	int32_t s;
	int32_t pregain;

    if(unit->channel == 0)
    {
    	 unit->channel = channel;
    }

	if(unit->enable)
	{
		if ((pcm_in == NULL)||(pcm_out == NULL))
			return;
		
		channel = unit->channel;
		pregain = unit->mute? 0 : unit->gain;

		if(channel == 2)
		{
			for(s = 0; s < n; s++)
			{
				pcm_out[2 * s + 0] = __nds32__clips((((int32_t)pcm_in[2 * s + 0] * pregain + 2048) >> 12), 16-1);
				pcm_out[2 * s + 1] = __nds32__clips((((int32_t)pcm_in[2 * s + 1] * pregain + 2048) >> 12), 16-1);
			}
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				pcm_out[s] = __nds32__clips((((int32_t)pcm_in[s] * pregain + 2048) >> 12), 16-1);
			}
		}
	}
#else
	int32_t s;
	int32_t pregain;
	int32_t FadeStep;

    if(unit->channel == 0)
    {
    	 unit->channel = channel;
    }

	if(unit->enable)
	{
		if(pcm_in == NULL || pcm_out==NULL) return;

		channel = unit->channel;

		pregain = unit->mute? 0 : unit->gain;

		FadeStep = pregain > unit->lastgain ? pregain - unit->lastgain : unit->lastgain - pregain;

		FadeStep = FadeStep / mainAppCt.SamplesPreFrame + FadeStep % mainAppCt.SamplesPreFrame ? 1 : 0;


		for(s = 0; s < n; s++)
		{
			if(channel == 2)
			{
				pcm_out[2 * s + 0] = __nds32__clips((((int32_t)pcm_in[2 * s + 0] * unit->lastgain + 2048) >> 12), 16-1);
				pcm_out[2 * s + 1] = __nds32__clips((((int32_t)pcm_in[2 * s + 1] * unit->lastgain + 2048) >> 12), 16-1);
			}
			else
			{
				pcm_out[s] = __nds32__clips((((int32_t)pcm_in[s] * pregain + 2048) >> 12), 16-1);
			}

			MixerFadeVolume(unit->lastgain, pregain, FadeStep);
		}
	}
#endif
}
/*
****************************************************************
* PitchShifter主循环处理函数
*
*
****************************************************************
*/
void AudioEffectPitchShifterApply(PitchShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n, uint8_t channel)
{
#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		uint32_t PSSample = (CFG_MIC_PITCH_SHIFTER_FRAME_SIZE >> 1);
		uint32_t Cnt = n / PSSample;
		uint16_t iIdx;

		channel = unit->channel;

		for(iIdx = 0; iIdx < Cnt; iIdx++)
		{
			pitch_shifter_apply16((PitchShifterContext16 *)unit->ct, (int16_t *)(pcm_in  + (PSSample * channel) * iIdx),
									  	  (int16_t *)(pcm_out + (PSSample * channel) * iIdx));
		}
	}
#endif
}

void AudioEffectPitchShifterApply24(PitchShifterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n, uint8_t channel)
{
#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		uint32_t PSSample = (CFG_MIC_PITCH_SHIFTER_FRAME_SIZE >> 1);
		uint32_t Cnt = n / PSSample;
		uint16_t iIdx;

		channel = unit->channel;

		for(iIdx = 0; iIdx < Cnt; iIdx++)
		{
			pitch_shifter_apply24((PitchShifterContext24 *)unit->ct, (int32_t *)(pcm_in  + (PSSample * channel) * iIdx),
									  	  (int32_t *)(pcm_out + (PSSample * channel) * iIdx));
		}
	}
#endif
}
#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN
/*
****************************************************************
* PitchShifterPro主循环处理函数
*
*
****************************************************************
*/
void AudioEffectPitchShifterProApply(PitchShifterProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		pitch_shifter_pro_apply(unit->ct, pcm_in, pcm_out);
	}
}

void AudioEffectPitchShifterProApply24(PitchShifterProUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		pitch_shifter_pro_apply24(unit->ct, pcm_in, pcm_out);
	}
}
#endif
/*
****************************************************************
* PitchDetector主循环处理函数
*
*
****************************************************************
*/
#if CFG_AUDIO_PITCH_DET_EN
void AudioEffectPitchDetectorApply(PitchDetectorUnit *unit, int16_t *pcm_in, uint32_t n)
{
	int32_t i, j, high_conf_count;
	int32_t p, conf;
	
	if(GetSystemMode() != AppModeRadioAudioPlay)
		return;	
	high_conf_count = 0;
	if((unit->enable) && (unit->ct != NULL))
	{
		for (i = 0; i*unit->step_size + unit->step_size <= n; i++)
		{
			unit->freq = pitch_detector_apply(unit->ct, pcm_in, &conf);
			if (i % 10 == 0)
				APP_DBG("\n");

			// One example to post process the pitches detected
			if (conf >= unit->confidence_threshold)
				high_conf_count++;
			else
				high_conf_count = 0;
			// Continuous high confident pitch detected for at least 186 ms (1024*8 @ 44100Hz~=186 ms)
			if (high_conf_count >= 8)
				APP_DBG("freq:[%d] ", unit->freq); // a reliable pitch
			else
				APP_DBG("%d ", unit->freq);
			//APP_DBG("%3d(%5.2f%%) ", unit->freq, conf/100.00f);
			
			//if (i == 19) // for simulatioun purpose only
			//{
			//	APP_DBG("\nSimulation stopped at i=%d", i);
			//	break;
			//}
		}
	}
}
#endif

/*
****************************************************************
* AutoTune16主循环处理函数
*
*
****************************************************************
*/
void AudioEffectAutoTuneApply(AutoTuneUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		auto_tune_apply16((AutoTuneContext16 *)unit->ct, pcm_in, pcm_out, unit->snap_mode,	unit->key,unit->pitch_accuracy);
	}
}
/*
****************************************************************
* AutoTune24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectAutoTuneApply24(AutoTuneUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		auto_tune_apply24((AutoTuneContext24 *)unit->ct, pcm_in, pcm_out, unit->snap_mode,	unit->key,unit->pitch_accuracy);
	}
}
/*
****************************************************************
* VoiceChanger主循环处理函数
*
*
****************************************************************
*/
void AudioEffectVoiceChangerApply(VoiceChangerUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		voice_changer_apply(unit->ct, pcm_in, pcm_out);
	}
}
#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
/*
****************************************************************
* VoiceChanger主循环处理函数
*
*
****************************************************************
*/
void AudioEffectVoiceChangerProApply(VoiceChangerProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		//voice_changer_pro_apply(unit->ct, pcm_in, pcm_out);
	}
}
#endif
/*
****************************************************************
* Echo主循环处理函数
*
*
****************************************************************
*/
void AudioEffectEchoApply(EchoUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	uint32_t s;
#if (CFG_AUDIO_EFFECT_MIC_ECHO_24BIT_EN == 0)
	uint16_t channel = 2;
	#ifdef CFG_FUNC_ECHO_DENOISE
	if((unit->enable) && (unit->ct != NULL) && (EchoAudioBuf != NULL))
	#else
	if(unit->enable && (unit->ct != NULL))
	#endif
	{
		//unit->delay_samples = unit->delay * gCtrlVars.sample_rate / 1000;

		#ifdef CFG_FUNC_ECHO_DENOISE
		if( (unit->delay != unit->delay_bakup))// && (EchoAudioBuf != NULL) )
		{
			//channel = unit->channel;
			memcpy(EchoAudioBuf, pcm_in, n * 2 * channel);

			unit->delay_samples = unit->delay_bakup * gCtrlVars.sample_rate / 1000;
			echo_apply16(unit->ct, pcm_in, pcm_out, n, unit->attenuation, unit->delay_samples, unit->dry,unit->wet);
			du_efft_fadeout_sw(pcm_out, n, channel);

			unit->delay_samples = unit->delay * gCtrlVars.sample_rate / 1000;
			echo_apply16(unit->ct, EchoAudioBuf, EchoAudioBuf, n, unit->attenuation, unit->delay_samples, unit->dry, unit->wet);
			du_efft_fadein_sw(EchoAudioBuf, n, channel);

			if(channel == 2)
			{
				for(s = 0; s < n; s++)
				{
					pcm_out[2*s + 0] = __nds32__clips(((int32_t)pcm_out[2*s + 0] + (int32_t)EchoAudioBuf[2*s + 0]), 16-1);
					pcm_out[2*s + 1] = __nds32__clips(((int32_t)pcm_out[2*s + 1] + (int32_t)EchoAudioBuf[2*s + 1]), 16-1);
				}
			}
			else
			{
				for(s = 0; s < n; s++)
				{
					pcm_out[s] = __nds32__clips(((int32_t)pcm_out[s] + (int32_t)EchoAudioBuf[s]), 16-1);
				}
			}

			unit->delay_bakup = unit->delay;
		}
		else
        #endif
		{
			echo_apply16(unit->ct, pcm_in, pcm_out, n, unit->attenuation, unit->delay_samples,unit->dry, unit->wet);
		}
	}
#endif //end of CFG_AUDIO_EFFECT_MIC_ECHO_24BIT_EN
}

void AudioEffectEchoApply24(EchoUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if (CFG_AUDIO_EFFECT_MIC_ECHO_24BIT_EN ==1)
	uint32_t s;

	uint16_t channel = 2;
	#ifdef CFG_FUNC_ECHO_DENOISE
	if((unit->enable) && (unit->ct != NULL) && (EchoAudioBuf != NULL))
	#else
	if(unit->enable && (unit->ct != NULL))
	#endif
	{
		//unit->delay_samples = unit->delay * gCtrlVars.sample_rate / 1000;

		#ifdef CFG_FUNC_ECHO_DENOISE
		if( (unit->delay != unit->delay_bakup))// && (EchoAudioBuf != NULL) )
		{
			//channel = unit->channel;
			memcpy(EchoAudioBuf, pcm_in, n * 2 * channel * 2);

			unit->delay_samples = unit->delay_bakup * gCtrlVars.sample_rate / 1000;
			echo_apply24(unit->ct, pcm_in, pcm_out, n, unit->attenuation, unit->delay_samples, unit->dry,unit->wet);
			du_efft_fadeout_sw24(pcm_out, n, channel,3);

			unit->delay_samples = unit->delay * gCtrlVars.sample_rate / 1000;
			echo_apply24(unit->ct, (int32_t *)EchoAudioBuf, (int32_t *)EchoAudioBuf, n, unit->attenuation, unit->delay_samples, unit->dry, unit->wet);
			du_efft_fadein_sw24((int32_t *)EchoAudioBuf, n, channel,3);

			if(channel == 2)
			{
				for(s = 0; s < n; s++)
				{
					pcm_out[2*s + 0] = __nds32__clips(((int64_t)pcm_out[2*s + 0] + (int64_t)EchoAudioBuf[2*s + 0]), 24-1);
					pcm_out[2*s + 1] = __nds32__clips(((int64_t)pcm_out[2*s + 1] + (int64_t)EchoAudioBuf[2*s + 1]), 24-1);
				}
			}
			else
			{
				for(s = 0; s < n; s++)
				{
					pcm_out[s] = __nds32__clips(((int64_t)pcm_out[s] + (int64_t)EchoAudioBuf[s]), 24-1);
				}
			}

			unit->delay_bakup = unit->delay;
		}
		else
        #endif
		{
			echo_apply24(unit->ct, pcm_in, pcm_out, n, unit->attenuation, unit->delay_samples,unit->dry, unit->wet);
		}
	}
#endif
}
/*
****************************************************************
* Reverb主循环处理函数
*
*
****************************************************************
*/
void AudioEffectReverbApply(ReverbUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		reverb_apply16(unit->ct, pcm_in, pcm_out, n);
	}
}
/*
****************************************************************
* PlateReverb16主循环处理函数
*
*
****************************************************************
*/
void AudioEffectPlateReverbApply(ReverbPlateUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		if(unit->bit_width==0)
		{
		  reverb_plate_apply16((ReverbPlateContext16 *)unit->ct, pcm_in, pcm_out, n);
		}

	}
}
/*
****************************************************************
* PlateReverb24主循环处理函数
*
*bit_width: 0= 16bit, 1=24bit
****************************************************************
*/
void AudioEffectPlateReverbApply24(ReverbPlateUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		if(unit->bit_width==1)
		{
		  reverb_plate_apply24((ReverbPlateContext24 *)unit->ct, pcm_in, pcm_out, n);
		}
	}
}
/*
****************************************************************
* PinPong主循环处理函数
*
*
****************************************************************
*/
void AudioEffectPinPongApply(PingPongUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_PINGPONG_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		pingpong_apply16(unit->ct, pcm_in, pcm_out, n,unit->attenuation,unit->delay_samples,unit->wetdrymix);
	}
#endif
}
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
/*
****************************************************************
* ReverbPro主循环处理函数
*
*
****************************************************************
*/
void AudioEffectReverbProApply(ReverbProUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{

	if((unit->enable) && (unit->ct != NULL))
	{
		if(reverb_pro_apply16(unit->ct, pcm_in, pcm_out, n)!=REVERB_PRO_ERROR_OK)
		{
			DBG("Pro Err\n");
		}
	}

}

void AudioEffectReverbProApply24(ReverbProUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{

	if((unit->enable) && (unit->ct != NULL))
	{
		reverb_pro_apply24(unit->ct, pcm_in, pcm_out, n);
	}

}
#endif
/*
****************************************************************
* VocalCut主循环处理函数
*
*
****************************************************************
*/
void AudioEffectVocalCutApply(VocalCutUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
    if(unit->vocal_cut_en == 0) return;
	
	if((unit->enable) && (unit->ct != NULL))
	{
		vocal_cut_apply(unit->ct, pcm_in, pcm_out, n,	unit->wetdrymix);
	}
}
/*
****************************************************************
* VocalRemovet主循环处理函数
*
*
****************************************************************
*/
void AudioEffectVocalRemoveApply(VocalRemoveUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
    if(unit->vocal_remove_en == 0) return;

	if((unit->enable) && (unit->ct != NULL))
	{
		vocal_remover_apply16((VocalRemoverContext16 *)unit->ct, pcm_in, pcm_out);
	}
#endif
}

void AudioEffectVocalRemoveApply24(VocalRemoveUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
    if(unit->vocal_remove_en == 0) return;

	if((unit->enable) && (unit->ct != NULL))
	{
		vocal_remover_apply24((VocalRemoverContext24 *)unit->ct, pcm_in, pcm_out);
	}
#endif
}
/*
****************************************************************
* Chorus主循环处理函数
*
*
****************************************************************
*/
void AudioEffectChorusApply(ChorusUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_CHORUS_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in == NULL || pcm_out==NULL) return;
		chorus_apply(unit->ct, pcm_in, pcm_out, n, unit->feedback, unit->dry, unit->wet, unit->mod_rate);
	}
#endif
}
/*
****************************************************************
* Chorus2 16主循环处理函数
*
*
****************************************************************
*/
void AudioEffectChorus2Apply(Chorus2Unit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_CHORUS2_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in == NULL || pcm_out==NULL) return;
		chorus2_apply16(unit->ct, pcm_in, pcm_out, n, unit->dry, unit->wet_1,unit->wet_2, unit->mod1_depth,unit->mod1_rate,unit->mod2_depth,unit->mod2_rate);
	}
#endif
}

/*
****************************************************************
* Chorus2 24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectChorus2Apply24(Chorus2Unit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_CHORUS2_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		if(pcm_in == NULL || pcm_out==NULL) return;
		chorus2_apply24(unit->ct, pcm_in, pcm_out, n, unit->dry, unit->wet_1,unit->wet_2, unit->mod1_depth,unit->mod1_rate,unit->mod2_depth,unit->mod2_rate);
	}
#endif
}
#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
/*
****************************************************************
* ThreeD主循环处理函数
*
*
****************************************************************
*/
void AudioEffectThreeDApply(ThreeDUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{	
    if(unit->three_d_en == 0) return;
	if((unit->enable) && (unit->ct != NULL))
	{
		three_d_apply(unit->ct, pcm_in, pcm_out, n, unit->intensity);
	}
}
void AudioEffectThreeDApply24(ThreeDUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
    if(unit->three_d_en == 0) return;
	if((unit->enable) && (unit->ct != NULL))
	{
		three_d_apply24(unit->ct, pcm_in, pcm_out, n, unit->intensity);
	}
}
#endif
#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
/*
****************************************************************
* ThreeD Plus主循环处理函数
*
*
****************************************************************
*/
void AudioEffectThreeDPlusApply(ThreeDPlusUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
    if(unit->three_d_en == 0) return;
	if((unit->enable) && (unit->ct != NULL))
	{
		three_d_plus_apply(unit->ct, pcm_in, pcm_out, n, unit->intensity);
	}
}
#endif
#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
/*
****************************************************************
* VB主循环处理函数
*
*
****************************************************************
*/
void AudioEffectVBApply(VBUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{    
	uint16_t i;
	if(unit->vb_en == 0)
	{
		if(unit->vb_en_bak)
		{
			unit->vb_en_bak = 0;

			if(MusicVolBuf==NULL) return;

			if((unit->enable) && (unit->ct != NULL))
			{
				DBG("vb fadeOut\n");
				memcpy(MusicVolBuf, pcm_in, n * 2 * unit->channel);
                if(unit->vb_type==0)
                {
				   vb_apply((VBContext *)unit->ct, pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
                }
                if(unit->vb_type==1)
                {
                	vb_td_apply((VBTDContext *)unit->ct, pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
                }

				du_efft_fadeout_sw(pcm_out, n, unit->channel);

				du_efft_fadein_sw((int16_t *)MusicVolBuf, n, unit->channel);

				for(i = 0; i < n; i++)
				{
					pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)MusicVolBuf[2*i + 0]), 16-1);
					pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)MusicVolBuf[2*i + 1]), 16-1);
				}
			}
		}
		return;
	}
	if((unit->enable) && (unit->ct != NULL))
	{
		if(unit->vb_en_bak==0)
		{
			DBG("vb fadeIn\n");
			unit->vb_en_bak = 1;
			if(MusicVolBuf==NULL) return;

			memcpy(MusicVolBuf, pcm_in, n * 2 * unit->channel);
			memcpy(pcm_out, pcm_in, n * 2 * unit->channel);

			du_efft_fadeout_sw(pcm_out, n, unit->channel);


            if(unit->vb_type==0)
            {
            	vb_apply((VBContext *)unit->ct, MusicVolBuf, MusicVolBuf, n, unit->intensity, unit->enhanced);
            }
            if(unit->vb_type==1)
            {
            	vb_td_apply((VBTDContext *)unit->ct, MusicVolBuf, MusicVolBuf, n, unit->intensity, unit->enhanced);
            }
			du_efft_fadein_sw((int16_t *)MusicVolBuf, n, unit->channel);

			for(i = 0; i < n; i++)
			{
				pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)MusicVolBuf[2*i + 0]), 16-1);
				pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)MusicVolBuf[2*i + 1]), 16-1);
			}
		}
		else
		{
            if(unit->vb_type==0)
            {
            	vb_apply((VBContext *)unit->ct, pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
            }
            if(unit->vb_type==1)
            {
            	vb_td_apply((VBTDContext *)unit->ct, pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
            }
		}
	}
 }

void AudioEffectVBApply24(VBUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	uint16_t i;
	if(unit->vb_en == 0)
	{
		if(unit->vb_en_bak)
		{
			unit->vb_en_bak = 0;
			if((unit->enable) && (unit->ct != NULL))
			{
				DBG("vb fadeOut\n");
				if(MusicVolBuf==NULL) return;
				memcpy(MusicVolBuf, pcm_in, n * 2 * unit->channel * 2);
	            if(unit->vb_type==0)
	            {
	            	vb_apply24((VBContext *)unit->ct, pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
	            }
	            if(unit->vb_type==1)
	            {
	            	vb_td_apply24((VBTDContext *)unit->ct, pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
	            }

				du_efft_fadeout_sw24(pcm_out, n, unit->channel,3);

				du_efft_fadein_sw24((int32_t *)MusicVolBuf, n, unit->channel,3);

				for(i = 0; i < n; i++)
				{
					pcm_out[2*i + 0] = __nds32__clips(((int64_t)pcm_out[2*i + 0] + (int64_t)MusicVolBuf[2*i + 0]), 24-1);
					pcm_out[2*i + 1] = __nds32__clips(((int64_t)pcm_out[2*i + 1] + (int64_t)MusicVolBuf[2*i + 1]), 24-1);
				}
			}
		}
		return;
	}
	if((unit->enable) && (unit->ct != NULL))
	{
		if(unit->vb_en_bak==0)
		{
			DBG("vb fadeIn\n");
			unit->vb_en_bak = 1;
			if(MusicVolBuf==NULL) return;
			memcpy(MusicVolBuf, pcm_in, n * 2 * unit->channel * 2);
			memcpy(pcm_out, pcm_in, n * 2 * unit->channel * 2);

			du_efft_fadeout_sw24(pcm_out, n, unit->channel,3);

            if(unit->vb_type==0)
            {
            	vb_apply24((VBContext *)unit->ct,(int32_t *)MusicVolBuf, (int32_t *)MusicVolBuf, n, unit->intensity, unit->enhanced);
            }
            if(unit->vb_type==1)
            {
            	vb_td_apply24((VBTDContext *)unit->ct, (int32_t *)MusicVolBuf, (int32_t *)MusicVolBuf, n, unit->intensity, unit->enhanced);
            }

			du_efft_fadein_sw24((int32_t *)MusicVolBuf, n, unit->channel,3);

			for(i = 0; i < n; i++)
			{
				pcm_out[2*i + 0] = __nds32__clips(((int64_t)pcm_out[2*i + 0] + (int64_t)MusicVolBuf[2*i + 0]), 24-1);
				pcm_out[2*i + 1] = __nds32__clips(((int64_t)pcm_out[2*i + 1] + (int64_t)MusicVolBuf[2*i + 1]), 24-1);
			}
		}
		else
		{
            if(unit->vb_type==0)
            {
            	vb_apply24((VBContext *)unit->ct,pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
            }
            if(unit->vb_type==1)
            {
            	vb_td_apply24((VBTDContext *)unit->ct, pcm_in, pcm_out, n, unit->intensity, unit->enhanced);
            }
		}
	}
 }
//------------------------//
#endif

#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
/*
****************************************************************
* VBClassic主循环处理函数
*
*
****************************************************************
*/
void AudioEffectVBClassicApply(VBClassicUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{	
   // if(unit->vb_en == 0) return;
	//if((unit->enable) && (unit->ct != NULL))
	{
		//vb_classic_apply(unit->ct, pcm_in, pcm_out, n, unit->intensity);
		uint16_t i;

		if(unit->vb_en == 0)
		{
			if(unit->vb_en_bak)
			{
				unit->vb_en_bak = 0;
				if((unit->enable) && (unit->ct != NULL))
				{
					DBG("vb fadeOut\n");
					if(MusicVolBuf==NULL) return;
					memcpy(MusicVolBuf, pcm_in, n * 2 * unit->channel);

					vb_classic_apply(unit->ct, pcm_in, pcm_out, n, unit->intensity);
					du_efft_fadeout_sw(pcm_out, n, unit->channel);

					du_efft_fadein_sw((int16_t *)MusicVolBuf, n, unit->channel);

					for(i = 0; i < n; i++)
					{
						pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)MusicVolBuf[2*i + 0]), 16-1);
						pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)MusicVolBuf[2*i + 1]), 16-1);
					}
				}
			}
			return;
		}
		if((unit->enable) && (unit->ct != NULL))
		{
			if(unit->vb_en_bak==0)
			{
				DBG("vb fadeIn\n");
				unit->vb_en_bak = 1;
				if(MusicVolBuf==NULL) return;
				memcpy(MusicVolBuf, pcm_in, n * 2 * unit->channel);
				memcpy(pcm_out, pcm_in, n * 2 * unit->channel);

				du_efft_fadeout_sw(pcm_out, n, unit->channel);

				vb_classic_apply(unit->ct, MusicVolBuf, MusicVolBuf, n, unit->intensity);
				du_efft_fadein_sw((int16_t *)MusicVolBuf, n, unit->channel);

				for(i = 0; i < n; i++)
				{
					pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)MusicVolBuf[2*i + 0]), 16-1);
					pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)MusicVolBuf[2*i + 1]), 16-1);
				}
			}
			else
			{
				vb_classic_apply(unit->ct, pcm_in, pcm_out, n, unit->intensity);
			}
		}
	}
}

void AudioEffectVBClassicApply24(VBClassicUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
   // if(unit->vb_en == 0) return;
	//if((unit->enable) && (unit->ct != NULL))
	{
		//vb_classic_apply(unit->ct, pcm_in, pcm_out, n, unit->intensity);
		uint16_t i;

		int32_t *MusicVolBuf24 = (int32_t *)MusicVolBuf;

		if(unit->vb_en == 0)
		{
			if(unit->vb_en_bak)
			{
				unit->vb_en_bak = 0;
				if((unit->enable) && (unit->ct != NULL))
				{
					DBG("vb fadeOut\n");
					if(MusicVolBuf24==NULL) return;
					memcpy(MusicVolBuf24, pcm_in, n * 4 * unit->channel);

					vb_classic_apply24(unit->ct, pcm_in, pcm_out, n, unit->intensity);
					du_efft_fadeout_sw24(pcm_out, n, unit->channel,3);

					du_efft_fadein_sw24((int32_t *)MusicVolBuf24, n, unit->channel,3);

					for(i = 0; i < n; i++)
					{
						pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)MusicVolBuf24[2*i + 0]), 24-1);
						pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)MusicVolBuf24[2*i + 1]), 24-1);
					}
				}
			}
			return;
		}
		if((unit->enable) && (unit->ct != NULL))
		{
			if(unit->vb_en_bak==0)
			{
				DBG("vb fadeIn\n");
				unit->vb_en_bak = 1;
				if(MusicVolBuf24==NULL) return;
				memcpy(MusicVolBuf24, pcm_in, n * 4 * unit->channel);
				memcpy(pcm_out, pcm_in, n * 4 * unit->channel);

				du_efft_fadeout_sw24(pcm_out, n, unit->channel,3);

				vb_classic_apply24(unit->ct, MusicVolBuf24, MusicVolBuf24, n, unit->intensity);
				du_efft_fadein_sw24((int32_t *)MusicVolBuf24, n, unit->channel,3);

				for(i = 0; i < n; i++)
				{
					pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)MusicVolBuf24[2*i + 0]), 24-1);
					pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)MusicVolBuf24[2*i + 1]), 24-1);
				}
			}
			else
			{
				vb_classic_apply24(unit->ct, pcm_in, pcm_out, n, unit->intensity);
			}
		}
	}
}
#endif
/*
****************************************************************
* DRC主循环处理函数
*
*
****************************************************************
*/
void AudioEffectDRCApply(DRCUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		drc_apply(unit->ct, pcm_in, pcm_out, n, unit->pregain);
	}
}
void AudioEffectDRCApply24(DRCUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
	if((unit->enable) && (unit->ct != NULL))
	{
		drc_apply24(unit->ct, pcm_in, pcm_out, n, unit->pregain);
	}
}
/*
****************************************************************
* StereoWidener主循环处理函数
*
*
****************************************************************
*/
void AudioEffectStereoWidenerApply(StereoWindenUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		stereo_widener_apply(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}
void AudioEffectStereoWidenerApply24(StereoWindenUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		stereo_widener_apply24(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}
/*
****************************************************************
* AutoWahA16主循环处理函数
*
*
****************************************************************
*/
void AudioEffectAutoWahApply(AutoWahUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_AUTOWAH_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		auto_wah_apply(unit->ct, pcm_in, pcm_out, n,unit->dry,unit->wet, unit->modulation_rate);
	}
#endif
}
/*
****************************************************************
* AutoWahA24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectAutoWahApply24(AutoWahUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_AUTOWAH_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		auto_wah_apply24(unit->ct, pcm_in, pcm_out, n,unit->dry,unit->wet, unit->modulation_rate);
	}
#endif
}
/*
****************************************************************
* EQ主循环处理函数
*
*
****************************************************************
*/
void AudioEffectEQApply(EQUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n, uint8_t channel)
{
	uint32_t cnt = n / EQ_BUFFER_NUM_SAMPLES;
	uint32_t RemainLen = n - cnt * EQ_BUFFER_NUM_SAMPLES;
	uint32_t i = 0;

	if((unit->enable) && (unit->ct != NULL))
	{
		channel = unit->channel;

		#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
		if(music_eq_mode_unit == unit)
		{
			if(mainAppCt.EqModeBak != mainAppCt.EqMode)
			{
				if(EqModeAudioBuf==NULL) return;

				memcpy(EqModeAudioBuf, pcm_in, n * 2 * channel);
				memcpy(&EqBufferBak, unit->ct, sizeof(EQContext));
				// AudioEffectEQApply(&gCtrlVars.music_out_eq_unit, music_pcm, music_pcm, n, channel);
				for(i = 0; i < cnt; i++)
				{
					eq_apply(unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES);
				}
				if(RemainLen > 0)
				{
					eq_apply(unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen);
				}

				du_efft_fadeout_sw(pcm_out, n, channel);
				//eq_clear_delay_buffer(unit->ct);//若前后EQ模式参数中filter数目差异大，需要打开这行代码

				memcpy(unit->ct, &EqBufferBak,sizeof(EQContext));
				#ifdef FUNC_OS_EN
				osMutexUnlock(AudioEffectMutex);
				#endif
				EqModeSet(mainAppCt.EqMode);
				#ifdef FUNC_OS_EN
				osMutexLock(AudioEffectMutex);
				#endif

				//AudioEffectEQApply(&gCtrlVars.music_out_eq_unit, (int16_t *)EqModeAudioBuf, (int16_t *)EqModeAudioBuf, n, channel);
				for(i = 0; i < cnt; i++)
				{
					eq_apply(unit->ct, (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES);
				}
				if(RemainLen > 0)
				{
					eq_apply(unit->ct, (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen);
				}

				du_efft_fadein_sw((int16_t *)EqModeAudioBuf, n, channel);

				for(i = 0; i < n; i++)
				{
					pcm_out[2*i + 0] = __nds32__clips(((int32_t)pcm_out[2*i + 0] + (int32_t)EqModeAudioBuf[2*i + 0]), 16-1);
					pcm_out[2*i + 1] = __nds32__clips(((int32_t)pcm_out[2*i + 1] + (int32_t)EqModeAudioBuf[2*i + 1]), 16-1);
				}

				mainAppCt.EqModeBak = mainAppCt.EqMode;

				return;
			}
		}
		#endif

		for(i = 0; i < cnt; i++)
		{
			eq_apply(unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES);
		}
		if(RemainLen > 0)
		{
			eq_apply(unit->ct, (int16_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int16_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen);
		}
	}
}
void AudioEffectEQApply24(EQUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n, uint8_t channel)
{
	uint32_t cnt = n / EQ_BUFFER_NUM_SAMPLES;
	uint32_t RemainLen = n - cnt * EQ_BUFFER_NUM_SAMPLES;
	uint32_t i = 0;

	if((unit->enable) && (unit->ct != NULL))
	{
		channel = unit->channel;

		#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
		if(music_eq_mode_unit == unit)
		{
			if(mainAppCt.EqModeBak != mainAppCt.EqMode)
			{
				if(EqModeAudioBuf==NULL) return;
				memcpy(EqModeAudioBuf, pcm_in, n * 4 * channel);
				memcpy(&EqBufferBak, unit->ct, sizeof(EQContext));
				// AudioEffectEQApply(&gCtrlVars.music_out_eq_unit, music_pcm, music_pcm, n, channel);
				for(i = 0; i < cnt; i++)
				{
					eq_apply24(unit->ct, (int32_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int32_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES,1);
				}
				if(RemainLen > 0)
				{
					eq_apply24(unit->ct, (int32_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int32_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen,1);
				}

				du_efft_fadeout_sw24(pcm_out, n, channel,3);
				//eq_clear_delay_buffer(unit->ct);//若前后EQ模式参数中filter数目差异大，需要打开这行代码

				memcpy(unit->ct, &EqBufferBak,sizeof(EQContext));
				#ifdef FUNC_OS_EN
				osMutexUnlock(AudioEffectMutex);
				#endif
				EqModeSet(mainAppCt.EqMode);
				#ifdef FUNC_OS_EN
				osMutexLock(AudioEffectMutex);
				#endif

				//AudioEffectEQApply(&gCtrlVars.music_out_eq_unit, (int16_t *)EqModeAudioBuf, (int16_t *)EqModeAudioBuf, n, channel);
				for(i = 0; i < cnt; i++)
				{
					eq_apply24(unit->ct, (int32_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int32_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES,1);
				}
				if(RemainLen > 0)
				{
					eq_apply24(unit->ct, (int32_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), (int32_t *)(EqModeAudioBuf + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen,1);
				}

				du_efft_fadein_sw24((int32_t *)EqModeAudioBuf, n, channel,3);

				for(i = 0; i < n; i++)
				{
					pcm_out[2*i + 0] = __nds32__clips(((int64_t)pcm_out[2*i + 0] + (int64_t)EqModeAudioBuf[2*i + 0]), 24-1);
					pcm_out[2*i + 1] = __nds32__clips(((int64_t)pcm_out[2*i + 1] + (int64_t)EqModeAudioBuf[2*i + 1]), 24-1);
				}

				mainAppCt.EqModeBak = mainAppCt.EqMode;

				return;
			}
		}
		#endif

		for(i = 0; i < cnt; i++)
		{
			eq_apply24(unit->ct, (int32_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int32_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), EQ_BUFFER_NUM_SAMPLES,1);
		}
		if(RemainLen > 0)
		{
			eq_apply24(unit->ct, (int32_t *)(pcm_in + i * EQ_BUFFER_NUM_SAMPLES * channel), (int32_t *)(pcm_out + i * EQ_BUFFER_NUM_SAMPLES * channel), RemainLen,1);
		}
	}
}
/*
****************************************************************
* Biquad 音效初始化
*
*
****************************************************************
*/
void AudioEffectBiquadInit(BiquadUnit *unit,  uint16_t num_channels,uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_BIQUAD_EN
	int32_t use_float;
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}
    if(unit->channel==0)
    {
    	unit->channel = num_channels;
    }
	if(unit->ct == NULL)
	{
		unit->ct = (BiquadContext *)osPortMallocFromEnd(BIQUAD_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("BiquadContext malloc err! %ldu\n",BIQUAD_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		use_float = unit->use_float;
		biquad_init(unit->ct ,unit->channel, sample_rate, unit->filter_params, use_float);
	}
#endif
}
/*
****************************************************************
* Biquad主循环处理函数
*
*
****************************************************************
*/
void AudioEffectBiquadApply(BiquadUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_BIQUAD_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		biquad_apply(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}

/*
****************************************************************
* Biquad24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectBiquadApply24(BiquadUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
#if CFG_AUDIO_EFFECT_BIQUAD_EN
	int32_t output_saturation;
	if((unit->enable) && (unit->ct != NULL))
	{
		output_saturation = unit->use_float&0xff;

		biquad_apply24(unit->ct, pcm_in, pcm_out, n,output_saturation);
	}
#endif
}
/*
****************************************************************
* 实时获取reverb相关参数，以此为电位器调节的最大值
* 只是在装载参数时，才可以获取最大值，用于调节参数
* 用户根据需要获取相应的值
****************************************************************
*/
/*
****************************************************************
* blue ns音效初始化
* only mono
* support 128,256,512
* bit_depth 0= 16, 1= 24;
****************************************************************
*/
void AudioEffectBlueNSInit(BlueNsUnit *unit,uint16_t block_size,uint16_t bit_depth)
{
#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN || CFG_AUDIO_EFFECT_HFP_NS_EN
	uint32_t persistent_size,scratch_size,size;
	int32_t ret;
	uint8_t* scratch;

	if( (!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	unit->block_size = block_size;

	if(bit_depth==0)
	{
		bit_depth = 16;
	}
	else
	{
		bit_depth = 24;
	}

	ret = blue_ns_estimate_memory_usage(unit->block_size, bit_depth, &persistent_size, &scratch_size);

	if(BLUENS_ERROR_OK != ret)
	{
		APP_DBG("BlueNSContext memory usage fail!\n");
		unit->enable = 0;
		return;
	}
	if((unit->block_size != 128)&&(unit->block_size != 256)&&(unit->block_size != 512))
	{
		APP_DBG("BlueNSContext  block size fail!\n");
		unit->enable = 0;
		return;
	}

	size = persistent_size + scratch_size;

	if(unit->ct==NULL)
	{
	    unit->ct = (uint8_t *)osPortMallocFromEnd(size);
		if(unit->ct==NULL)
		{
			APP_DBG("BlueNSContext  memory fail!\n");
			unit->enable = 0;
			return;
		}
	}

	if(unit->ct != NULL)
	{
		scratch =  (uint8_t *)unit->ct + persistent_size;

		ret = blue_ns_init((uint8_t *)unit->ct, scratch, unit->block_size, bit_depth);
	}

#endif
}
/*
****************************************************************
* blue ns主循环处理函数
*
*
****************************************************************
*/
void AudioEffectBlueNSApply(BlueNsUnit *unit, int16_t *pcm_in, int16_t *pcm_out)
{
#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN || CFG_AUDIO_EFFECT_HFP_NS_EN
	if( (unit->enable==0)  || (unit->ct == NULL) )return;

	if( blue_ns_run16((uint8_t *)unit->ct, pcm_in, pcm_out, unit->ns_level) != BLUENS_ERROR_OK)
	{
		DBG("blue_ns_run16 Err\n");
	}
#endif
}
/*
****************************************************************
* HowlingGuard 音效初始化
* only mono
*
****************************************************************
*/
void AudioEffectHowlingGuardInit(HowlingGuardUnit *unit, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN

	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

	if(unit->ct == NULL)
	{
		unit->ct = (HowlingGuardContext *)osPortMallocFromEnd(HOWLINGGUARD_SIZE);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("HowlingGuardContext malloc err! %ldu\n",HOWLINGGUARD_SIZE);
		}
	}

	if(unit->ct != NULL)
	{
		howling_guard_init(unit->ct,sample_rate,\
				           unit->saturation_threshold,\
		                   unit->high_freq_threshold,\
		                   unit->high_freq_energy_ratio_threshold,\
		                   unit->max_saturated_high_freq_duration,\
		                   unit->max_saturated_duration,\
		                   unit->mute_period,\
		                   unit->noise_gate_threshold);
	}
#endif
}
/*
****************************************************************
* HowlingGuard 音效configure
* only mono
*
****************************************************************
*/
void AudioEffectHowlingGuardConfigure(HowlingGuardUnit *unit, uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN

	if((unit->enable) && (unit->ct != NULL))
	{
		howling_guard_init(unit->ct,sample_rate,\
				           unit->saturation_threshold,\
		                   unit->high_freq_threshold,\
		                   unit->high_freq_energy_ratio_threshold,\
		                   unit->max_saturated_high_freq_duration,\
		                   unit->max_saturated_duration,\
		                   unit->mute_period,\
		                   unit->noise_gate_threshold);
		APP_DBG("Howling Guard Configure\n");
	}
#endif
}
/*
****************************************************************
* HowlingGuard主循环处理函数
* only mono
*
****************************************************************
*/
void AudioEffectHowlingGuardApply(HowlingGuardUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		howling_guard_apply(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}

/*
****************************************************************
* PhaseShifter 音效初始化
*
*
****************************************************************
*/
void AudioEffectPhaseShifterdInit(PhaseShifterUnit *unit, uint32_t num_channels,uint32_t step_size,uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN

	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

    if(unit->channel==0)
    {
    	unit->channel = num_channels;
    }

    if( (step_size == 512 ) )
    {
    	unit->step_size = 256;
    }
    else if( (step_size == 256 ) )
    {
    	unit->step_size = 256;
    }
    else if( (step_size == 128 ) )
    {
    	unit->step_size = 128;
    }
    else if( (step_size == 64 ))
    {
    	unit->step_size = 64;
    }
    else
    {
    	APP_DBG("Only Support SamplePer Len {64, 128,256,512}\n");
    	unit->enable = 0;
    	return;
    }


	if(unit->ct == NULL)
	{
		unit->ct = (PhaseShifterContext *)osPortMallocFromEnd(sizeof(PhaseShifterContext)+8);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("PhaseShifterContext malloc err! %ldu\n",sizeof(PhaseShifterContext));
		}
	}

	if(unit->ct != NULL)
	{
		phase_shifter_init(unit->ct,unit->channel, unit->step_size,sample_rate);
	}
#endif
}
/*
****************************************************************
* PhaseShifter主循环处理函数
*
*
****************************************************************
*/
void AudioEffectPhaseShifterApply(PhaseShifterUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
	uint16_t i,Len;

	if((unit->enable) && (unit->ct != NULL))
	{
		Len = n/unit->step_size;

		if(unit->channel==1)
		{

		   for(i = 0; i < Len; i++)
		   {
		       phase_shifter_apply(unit->ct, (pcm_in+(unit->step_size*i)), (pcm_out+(unit->step_size*i)), unit->phase_shift);
		   }
		}
		if(unit->channel==2)
		{
		   for(i = 0; i < Len; i++)
		   {
			 phase_shifter_apply(unit->ct, (pcm_in+2*(unit->step_size*i)), (pcm_out+2*(unit->step_size*i)), unit->phase_shift);
		   }
		}
	}
#endif
}
void AudioEffectPhaseShifterApply24(PhaseShifterUnit *unit, int32_t *pcm_in, int32_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
	uint16_t i,Len;

	if((unit->enable) && (unit->ct != NULL))
	{
		Len = n/unit->step_size;

		if(unit->channel==1)
		{

		   for(i = 0; i < Len; i++)
		   {
		       phase_shifter_apply24(unit->ct, (pcm_in+(unit->step_size*i)), (pcm_out+(unit->step_size*i)), unit->phase_shift);
		   }
		}
		if(unit->channel==2)
		{
		   for(i = 0; i < Len; i++)
		   {
			 phase_shifter_apply24(unit->ct, (pcm_in+2*(unit->step_size*i)), (pcm_out+2*(unit->step_size*i)), unit->phase_shift);
		   }
		}
	}
#endif
}
/*
****************************************************************
* ButterWorth 音效初始化
*
*
****************************************************************
*/
void AudioEffectButterWorthInit(ButterWorthUnit *unit, uint32_t num_channels,uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_BUTTERWORTH_EN

	uint32_t persistent_size;
	int32_t ret;
	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

    if(unit->channel==0)
    {
    	unit->channel = num_channels;
    }


    ret = filter_butterworth_estimate_memory_usage(unit->filter_order, &persistent_size);

    if(FILTERBUTTERWORTH_ERROR_OK != ret)
     {
		unit->enable = 0;
		APP_DBG("ButterWorthUnit malloc err! %ldu\n",persistent_size);
    	return;
     }

    persistent_size = 236;//use easy

	if(unit->ct == NULL)
	{
		unit->ct = (uint8_t *)osPortMallocFromEnd(persistent_size);
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("ButterWorthUnit malloc err! %ldu\n",persistent_size);
		}
	}

	if(unit->ct != NULL)
	{
		filter_butterworth_init(unit->ct, unit->channel, sample_rate, unit->filter_type, unit->filter_order, unit->fc);
	}
#endif
}
/*
****************************************************************
* butterworth16主循环处理函数
*
*
****************************************************************
*/
void AudioEffectButterWorthApply(ButterWorthUnit *unit, int16_t *pcm_in, int16_t *pcm_out, uint32_t n)
{
#if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
	if((unit->enable) && (unit->ct != NULL))
	{
		filter_butterworth_apply16(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}

/*
****************************************************************
* butterworth24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectButterWorthApply24(ButterWorthUnit *unit, int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
#if CFG_AUDIO_EFFECT_BUTTERWORTH_EN

	if((unit->enable) && (unit->ct != NULL))
	{
		filter_butterworth_apply24(unit->ct, pcm_in, pcm_out, n);
	}
#endif
}
/*
****************************************************************
* DynamicEq 音效初始化
*
*
****************************************************************
*/
void AudioEffectDynamicEqInit(DynamicEqUnit *unit, uint32_t num_channels,uint32_t sample_rate)
{
#if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN

	uint32_t size,RemainMem;
	unit->eq_high.enable = unit->enable;
	unit->eq_low.enable = unit->enable;

	if((!gCtrlVars.audio_effect_init_flag)||(!unit->enable))
	{
		return;
	}

    if(unit->channel==0)
    {
    	unit->channel = num_channels;
    }

    size  = sizeof(DynamicEQContext) + sizeof(EQContext) + sizeof(EQContext) + sizeof(EQContext);

    RemainMem = osPortRemainMem();

    DBG("%s\n",__func__);

    if(RemainMem  < size)
    {
    	APP_DBG("DynamicEq malloc err! %ldu\n",size);
    	return;
    }

	if(unit->ct == NULL)
	{
		unit->ct = (DynamicEQContext *)osPortMallocFromEnd(sizeof(DynamicEQContext));
		if(unit->ct == NULL)
		{
			unit->enable = 0;
			APP_DBG("DynamicEq malloc err! %ldu\n",sizeof(DynamicEQContext));
			return;
		}
	}

	AudioEffectEQInit(&unit->eq_low,num_channels, sample_rate);

	AudioEffectEQInit(&unit->eq_high,num_channels, sample_rate);

	AudioEffectEQInit(&unit->eq_watch,num_channels, sample_rate);

	if(unit->ct != NULL)
	{
		dynamic_eq_init(unit->ct, unit->channel, sample_rate, unit->low_energy_threshold, unit->normal_energy_threshold, unit->high_energy_threshold,\
				unit->attack_time,unit->release_time,unit->eq_low.ct,unit->eq_high.ct);
	}
#endif
}
/*
****************************************************************
* DynamicEq16主循环处理函数
*
*
****************************************************************
*/
void AudioEffectDynamicEqApply(DynamicEqUnit *unit, int16_t *pcm_in, int16_t *pcm_out,uint32_t n)
{
#if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN//pcm_out CANNOT be the same as pcm_in.

	int16_t *DynamicEQInBuf=NULL;

	if((unit->enable) && (unit->ct != NULL) &&(pcm_out != NULL)&&(pcm_in != NULL))
	{
		//if(pcm_in == pcm_out)
		//{
		//	DBG("pcm_out16 CANNOT be the same as pcm_in.\n");
			//return;
		//}
		if((unit->eq_low.enable==0)||(unit->eq_high.enable==0))
		{
			return;
		}
		if(DynamicEQWathcBuf==NULL) return;

		DynamicEQInBuf= DynamicEQWathcBuf + n*2*unit->channel;

		memcpy(DynamicEQWathcBuf,pcm_in,n*2*unit->channel);
		memcpy(DynamicEQInBuf,pcm_in,n*2*unit->channel);

       //for watch pcm,   if( music_dynamic_eq_watch_unit.enable =0){ watch = original}
        AudioEffectEQApply(&unit->eq_watch, (int16_t *)DynamicEQWathcBuf, (int16_t *)DynamicEQWathcBuf, n, n*2*unit->channel);

		dynamic_eq_apply16(unit->ct, (int16_t *)DynamicEQInBuf,(int16_t *)DynamicEQWathcBuf, pcm_out, n);

	}
#endif
}

/*
****************************************************************
* DynamicEq24主循环处理函数
*
*
****************************************************************
*/
void AudioEffectDynamicEqApply24(DynamicEqUnit *unit, int32_t *pcm_in,int32_t *pcm_out, int32_t n)
{
#if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN//pcm_out CANNOT be the same as pcm_in.

	if((unit->enable) && (unit->ct != NULL) &&(pcm_out != NULL)&&(pcm_in != NULL)&&(DynamicEQWathcBuf !=NULL))
	{
		if(pcm_in == pcm_out)
		{
			DBG("pcm_out24 CANNOT be the same as pcm_in.\n");
			return;
		}

		//dynamic_eq_apply24(unit->ct, pcm_in,pcm_watch, pcm_out, n);
	}
#endif
}
/*
****************************************************************
* 变速度音效初始化
*
*
****************************************************************
*/
#ifdef CFG_FUNC_TSM_EN
uint8_t tsm_ct[TSM_SIZE];

int16_t tsm_in_buf[TSM_MAX_W_SIZE];
int16_t tsm_out_buf[TSM_MAX_W_SIZE];
#endif

void AudioEffectTsmInit(uint32_t channel, uint32_t sample_rate)
{
#ifdef CFG_FUNC_TSM_EN

	int32_t speed_ratio = 65536;

	TSMUnit *unit = &gCtrlVars.tsm_unit;

	int32_t w = TSM_MAX_W_SIZE/2;

	unit->speed_ratio = speed_ratio;////init ,must is 65536;

	unit->ct       =  (TSMContext *)&tsm_ct[0];

	unit->in_buf   =  &tsm_in_buf[0];

	unit->out_buf  =  &tsm_out_buf[0];

	tsm_init(unit->ct, channel, sample_rate, speed_ratio,w);

	unit->in_size    = unit->ct->sa;

	unit->out_size   = unit->ct->ss;

	DBG("Tsm in frame:%d\n",unit->in_size);

	DBG("Tsm out frame:%d\n",unit->out_size);



//	if(unit->ct == NULL)
//	{
//		unit->ct = (TSMContext *)osPortMallocFromEnd(TSM_SIZE);
//
//		if(unit->ct == NULL)
//		{
//			DBG("TSMContext mem malloc err! %ld\n",TSM_SIZE);
//			return;
//		}
//	}
//
//	if(unit->ct != NULL)
//	{
//		if(unit->out_buf == NULL)
//		{
//			unit->out_buf = (int16_t *)osPortMallocFromEnd(unit->out_size*4);
//			if(unit->out_buf == NULL)
//			{
//			  osPortFree(unit->ct);
//			  unit->ct  = NULL;
//			  DBG("TSMContext out buf malloc err!\n");
//			  return;
//			}
//		}
//		if(unit->in_buf == NULL)
//		{
//			unit->in_buf = (int16_t *)osPortMallocFromEnd(unit->in_size*4);
//			if(unit->in_buf == NULL)
//			{
//			  osPortFree(unit->ct);
//			  unit->ct  = NULL;
//			  osPortFree(unit->out_buf);
//			  unit->out_buf  = NULL;
//			  DBG("TSMContext in buf malloc err!\n");
//			  return;
//			}
//		}
//	    tsm_init(unit->ct, channel, sample_rate, speed_ratio,w);
//		unit->in_size    = unit->ct->sa;
//		unit->out_size   = unit->ct->ss;
//		DBG("Tsm in frame:%d\n",unit->in_size);
//		DBG("Tsm out frame:%d\n",unit->out_size);
//	}

#endif
}
/*
****************************************************************
* 变速度主循环处理函数
*
*
****************************************************************
*/
void AudioEffectTsmApply(int16_t *pcm_in, int16_t *pcm_out)
{
#ifdef CFG_FUNC_TSM_EN

	TSMUnit *unit = &gCtrlVars.tsm_unit;

	if(unit->ct != NULL)
	 {
		tsm_apply(unit->ct, pcm_in, pcm_out);
	 }
#endif
}

/*
****************************************************************
* 变速度配置处理函数
*
*
****************************************************************
*/
void AudioEffectTsmConfigure(int32_t speed_ratio)
{
#ifdef CFG_FUNC_TSM_EN

	TSMUnit *unit = &gCtrlVars.tsm_unit;

	if(unit->ct != NULL)
	{
        tsm_configure(unit->ct, speed_ratio);

        unit->speed_ratio = speed_ratio;

        unit->in_size = unit->ct->sa;

        unit->out_size = unit->ct->ss;

    	DBG("Tsm in frame:%d\n",unit->in_size);

    	DBG("Tsm out frame:%d\n",unit->out_size);
	}
#endif
}

/*****************************************
 *
 *
 *******************************************/
void GetAudioEffectMaxValue(void)
{
	//if(StartWriteCmd == LOAD_BUSY)//////
	{	
		gCtrlVars.max_chorus_wet               = gCtrlVars.chorus_unit.wet;
		
	    gCtrlVars.max_plate_reverb_roomsize    = gCtrlVars.plate_reverb_unit.decay;

		gCtrlVars.max_plate_reverb_wetdrymix   = gCtrlVars.plate_reverb_unit.wetdrymix;

		gCtrlVars.max_reverb_wet_scale         = gCtrlVars.reverb_unit.wet_scale;

		gCtrlVars.max_reverb_roomsize    = gCtrlVars.reverb_unit.roomsize_scale;
		
		gCtrlVars.max_echo_delay        = gCtrlVars.echo_unit.delay_samples;
	
        gCtrlVars.max_echo_depth        = gCtrlVars.echo_unit.attenuation;
	
		//gCtrlVars.ReverbRoom            = 0;

		//gCtrlVars.max_dac0_dig_l_vol    = gCtrlVars.dac0_dig_l_vol;

		//gCtrlVars.max_dac0_dig_r_vol    = gCtrlVars.dac0_dig_r_vol;

		//gCtrlVars.max_dac1_dig_vol      = gCtrlVars.dac1_dig_vol;  
        #if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
		gCtrlVars.max_reverb_pro_wet     = gCtrlVars.reverb_pro_unit.wet ;
		gCtrlVars.max_reverb_pro_erwet   = gCtrlVars.reverb_pro_unit.erwet ;
		#endif
    }
}
/*
****************************************************************
* mic treb,bass调节函数
*
*
****************************************************************
*/
#ifdef CFG_FUNC_MIC_TREB_BASS_EN
void MicBassTrebAjust(int16_t 	BassGain,	int16_t TrebGain)
{
	if(mic_treb_bass_eq_unit==NULL) return;

	mic_treb_bass_eq_unit->filter_params[0].gain = mic_treb_bass_eq_unit->eq_params[1].gain = BassTrebGainTable[BassGain];
	mic_treb_bass_eq_unit->filter_params[1].gain = mic_treb_bass_eq_unit->eq_params[0].gain=  BassTrebGainTable[TrebGain];
	AudioEffectEQFilterConfig(mic_treb_bass_eq_unit, gCtrlVars.sample_rate);
	gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
}
#endif
/*
****************************************************************
* music treb,bass调节函数
*
*
****************************************************************
*/
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
void MusicBassTrebAjust(int16_t 	BassGain,	int16_t TrebGain)
{
	if(music_treb_bass_eq_unit==NULL) return;

#ifdef CFG_RES_USE_EQ_DRC_TREB_BASS_EN//use eq_drc
	music_treb_bass_eq_unit->filter_params[0].gain = music_treb_bass_eq_unit->eq_params[0].gain = BassTrebGainTable[BassGain];
	music_treb_bass_eq_unit->filter_params[1].gain = music_treb_bass_eq_unit->eq_params[1].gain=  BassTrebGainTable[TrebGain];
	AudioEffectEqDrcFilterConfig(music_treb_bass_eq_unit, gCtrlVars.sample_rate);
#else//use eq
	music_treb_bass_eq_unit->filter_params[0].gain = music_treb_bass_eq_unit->eq_params[0].gain = BassTrebGainTable[BassGain];
	music_treb_bass_eq_unit->filter_params[1].gain = music_treb_bass_eq_unit->eq_params[1].gain=  BassTrebGainTable[TrebGain];
	AudioEffectEQFilterConfig(music_treb_bass_eq_unit, gCtrlVars.sample_rate);
#endif
	gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
}
#endif
/*************************************************
 *  混响大小调节函数
 *
 *
 ***************************************************/
#ifdef CFG_FUNC_MIC_KARAOKE_EN
void ReverbStepSet(uint8_t ReverbStep)
{
	uint16_t step,r;
	
    step = gCtrlVars.max_chorus_wet / MAX_MIC_REVB_STEP;
	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.chorus_unit.wet = gCtrlVars.max_chorus_wet;
	}
	else
	{
		gCtrlVars.chorus_unit.wet = ReverbStep * step;
	}
	step = gCtrlVars.max_plate_reverb_roomsize / MAX_MIC_REVB_STEP;
	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.plate_reverb_unit.decay = gCtrlVars.max_plate_reverb_roomsize;
	}
	else
	{
		gCtrlVars.plate_reverb_unit.decay = ReverbStep * step;
	}
	//APP_DBG("mic_wetdrymix   = %d\n",gCtrlVars.max_plate_reverb_wetdrymix);
	step = gCtrlVars.max_plate_reverb_wetdrymix / MAX_MIC_REVB_STEP;
	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.plate_reverb_unit.wetdrymix = gCtrlVars.max_plate_reverb_wetdrymix;
	}
	else
	{
		gCtrlVars.plate_reverb_unit.wetdrymix = ReverbStep * step;
	}			
    step = gCtrlVars.max_echo_delay / MAX_MIC_REVB_STEP;
	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.echo_unit.delay_samples = gCtrlVars.max_echo_delay;
	}
	else
	{
		gCtrlVars.echo_unit.delay_samples = ReverbStep * step;
	}
	step = gCtrlVars.max_echo_depth/ MAX_MIC_REVB_STEP;
	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.echo_unit.attenuation= gCtrlVars.max_echo_depth;
	}
	else
	{
		gCtrlVars.echo_unit.attenuation = ReverbStep * step;
	}

    step = gCtrlVars.max_reverb_wet_scale/ MAX_MIC_REVB_STEP;
	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.reverb_unit.wet_scale = gCtrlVars.max_reverb_wet_scale;
	}
	else
	{
		gCtrlVars.reverb_unit.wet_scale = ReverbStep * step;
	}
    step = gCtrlVars.max_reverb_roomsize/ MAX_MIC_REVB_STEP;
	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.reverb_unit.roomsize_scale = gCtrlVars.max_reverb_roomsize;
	}
	else
	{
		gCtrlVars.reverb_unit.roomsize_scale = ReverbStep * step;
	}
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
	//+0  ~~~ -70
    r = abs(gCtrlVars.max_reverb_pro_wet);
    r = 70-r;
    step = r / MAX_MIC_REVB_STEP;

	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.reverb_pro_unit.wet = gCtrlVars.max_reverb_pro_wet;
	}
	else
	{
		r = MAX_MIC_REVB_STEP - 1 - ReverbStep;

		r*= step;

		gCtrlVars.reverb_pro_unit.wet = gCtrlVars.max_reverb_pro_wet - r;

		if(ReverbStep == 0) gCtrlVars.reverb_pro_unit.wet = -70;
	}

    r = abs(gCtrlVars.max_reverb_pro_erwet);
    r = 70-r;
    step = r / MAX_MIC_REVB_STEP;

	if(ReverbStep >= (MAX_MIC_REVB_STEP-1))
	{
		gCtrlVars.reverb_pro_unit.erwet = gCtrlVars.max_reverb_pro_erwet;
	}
	else
	{
		r = MAX_MIC_REVB_STEP - 1 - ReverbStep;

		r*= step;

		gCtrlVars.reverb_pro_unit.erwet = gCtrlVars.max_reverb_pro_erwet - r;

		if(ReverbStep == 0) gCtrlVars.reverb_pro_unit.erwet = -70;
	}
#endif

	AudioEffectReverbConfig(&gCtrlVars.reverb_unit);
	AudioEffectPlateReverbConfig(&gCtrlVars.plate_reverb_unit);
#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
	AudioEffectReverProbConfig(&gCtrlVars.reverb_pro_unit,gCtrlVars.sample_rate);
#endif
}
#endif

#else
/*
****************************************************************
* Aec音效初始化
*
*
****************************************************************
*/
void AudioEffectAecInit(AecUnit *unit, uint32_t sample_rate)
{
}

/*
****************************************************************
* 音效模块反初始化
*
*
****************************************************************
*/
void AudioEffectsDeInit(void)
{
}

/*
****************************************************************
* 音效模块初始化
*
*
****************************************************************
*/
void AudioEffectsInit(void)
{
}
#endif
/*************************************************
 *  调音时自动更新参数到PC
 *  以下函数不影响实际功能,仅起到显示提醒，
 *  可以忽略
 ***************************************************/
uint16_t PcSetEnable;
uint16_t EffStatusUpdata;
extern uint32_t effect_addr[60];
void Communication_Effect_GetPcSta(uint8_t *buf, uint32_t len)
{

     if((gCtrlVars.IsEffectChangedByPcTool==0) || (len ==0))  return;

	 if((buf[0] == 0)||(buf[0] == 0xf0)||(buf[0] == 0xff))
	  {
		memcpy(&PcSetEnable, &buf[1], 2);

	  }
	 else
	 {
		 PcSetEnable = 0;
	 }
}
/*************************************************
 *  调音时自动更新参数到PC
 *
 *
 ***************************************************/
void Communication_Effect_SetPcSta(uint8_t Control, uint8_t *buf)
{

	if(Control >0xf0) return;

	if((Control > 2)&&(Control != 0x80))
	{
		uint32_t addr = effect_addr[Control - 0x81];

		if(gCtrlVars.IsEffectChangedByPcTool==0) return;

		GainControlUnit *p = (GainControlUnit *)addr;

		if(PcSetEnable)
		{
			if(p->enable==0)//表示分配内存失败
			{
			   EffStatusUpdata = 1;

			}
		}
	}
}

void Communication_Effect_IsUpdataToPC(void)
{
	extern bool IsEffectSampleLenChange;
	extern bool IsEffectChange;

	if(IsEffectSampleLenChange||IsEffectChange || SoftFlagEffectChange)
	{
		EffStatusUpdata = 0;
	}

	if(EffStatusUpdata)
	{
        gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
        EffStatusUpdata = 0;
	}
}
