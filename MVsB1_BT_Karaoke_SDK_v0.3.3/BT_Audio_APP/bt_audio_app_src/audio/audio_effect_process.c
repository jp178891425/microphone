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
#include "audio_vol.h"
#include "stdlib.h"
#ifdef BT_TWS_SUPPORT
#include "bt_manager.h"
#include "bt_tws_api.h"
#endif
#ifdef CFG_FUNC_USB_MIX_MODE///
#include "usb_audio_api.h"
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
AUDIO_EFF_PARAMAS Audio_mode ;//调音参数缓存

int16_t* pcm_buf_1 =0;
int16_t* pcm_buf_2 =0;
int16_t* pcm_buf_3 =0;
int16_t* pcm_buf_4 =0;
int16_t* pcm_buf_5 =0;
int16_t* pcm_buf_6 =0;
int16_t* pcm_buf_7 =0;


#ifdef CFG_FUNC_GUITAR_EN
int16_t* guitar_pcm = NULL;
#endif

#if defined(CFG_FUNC_ECHO_DENOISE)||defined(CFG_FUNC_EQMODE_FADIN_FADOUT_EN)||(CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN)||(CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN) \
		|| defined(CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN)
    int16_t*  EchoAudioBuf=NULL;
#endif

#if (CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN)
    int16_t*    DynamicEQWathcBuf = NULL;
#endif

#ifdef BT_TWS_SUPPORT
void TWS_AudioEffectProcess(int16_t  **MusicAddr, int16_t  **RemindAddr);
void TWS_RemindAudioProcess(int16_t  **RemindAddr);
extern uint8_t tws_get_data_source(void);
#endif

void LoadAudioMode(uint16_t len,const uint8_t *buff, uint8_t init_flag);
void AudioInputMix(MixInputUnit *mix_pcm);
uint8_t  MixAudioFlag;
void EffectPcmBufMalloc(uint32_t SampleLen)
{
	pcm_buf_1 = (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(pcm_buf_1 == NULL)
	{
		APP_DBG("pcm_buf_1 malloc err\n");
		return;
	}
	pcm_buf_2 = (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(pcm_buf_2 == NULL)
	{
		APP_DBG("pcm_buf_2 malloc err\n");
		return;
	}
	pcm_buf_3 = (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(pcm_buf_3 == NULL)
	{
		APP_DBG("pcm_buf_3 malloc err\n");
		return;
	}
	pcm_buf_4 = (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(pcm_buf_4 == NULL)
	{
		APP_DBG("pcm_buf_4 malloc err\n");
		return;
	}
	pcm_buf_5 = (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(pcm_buf_5 == NULL)
	{
		APP_DBG("pcm_buf_5 malloc err\n");
		return;
	}
	pcm_buf_6 = (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(pcm_buf_6 == NULL)
	{
		APP_DBG("pcm_buf_6 malloc err\n");
		return;
	}

    #if CFG_FUNC_MIX_AUDIO_SDCT_EN
	pcm_buf_7= (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(pcm_buf_7 == NULL)
	{
		APP_DBG("pcm_buf_7 malloc err\n");
		return;
	}
    #endif
    #ifdef CFG_FUNC_GUITAR_EN
	guitar_pcm = (int16_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(guitar_pcm == NULL)
	{
		APP_DBG("guitar_pcm malloc err\n");
		return;
	}
    #endif

    #if defined(CFG_FUNC_ECHO_DENOISE)||defined(CFG_FUNC_EQMODE_FADIN_FADOUT_EN)||(CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN)||(CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN) \
		|| defined(CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN)

	EchoAudioBuf = (int16_t*)osPortMallocFromEnd(SampleLen * 2 * 2);
    if(EchoAudioBuf == NULL)
	{
		APP_DBG("EchoAudioBuf malloc err\n");
	}
	else
	{
		memset(EchoAudioBuf, 0, SampleLen * 2 * 2);
	}
    #endif

    #if (CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN)
    DynamicEQWathcBuf = (int16_t*)osPortMallocFromEnd(SampleLen * 2 * 2 * 2);
    if(DynamicEQWathcBuf == NULL)
	{
		APP_DBG("DynamicEQWathcBuf malloc err\n");
	}
	else
	{
		memset(DynamicEQWathcBuf, 0, SampleLen * 2 * 2);
	}
    #endif

	APP_DBG("EffectPcmBufMalloc OK\n");
}


void EffectPcmBufRelease(void)
{
	if(pcm_buf_1 != NULL)
	{
		osPortFree(pcm_buf_1);
		pcm_buf_1 = NULL;
	}
	if(pcm_buf_2 != NULL)
	{
		osPortFree(pcm_buf_2);
		pcm_buf_2 = NULL;
	}
	if(pcm_buf_3 != NULL)
	{
		osPortFree(pcm_buf_3);
		pcm_buf_3 = NULL;
	}
	if(pcm_buf_4 != NULL)
	{
		osPortFree(pcm_buf_4);
		pcm_buf_4 = NULL;
	}
	if(pcm_buf_5 != NULL)
	{
		osPortFree(pcm_buf_5);
		pcm_buf_5 = NULL;
	}
	if(pcm_buf_6 != NULL)
	{
		osPortFree(pcm_buf_6);
		pcm_buf_6 = NULL;
	}
	if(pcm_buf_7 != NULL)
	{
		osPortFree(pcm_buf_7);
		pcm_buf_7 = NULL;
	}
    #ifdef CFG_FUNC_GUITAR_EN
    if(guitar_pcm != NULL)
	{
		osPortFree(guitar_pcm);
		guitar_pcm = NULL;
	}
    #endif

   #if defined(CFG_FUNC_ECHO_DENOISE)||defined(CFG_FUNC_EQMODE_FADIN_FADOUT_EN)||(CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN)||(CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN) \
		|| defined(CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN)
    if(EchoAudioBuf != NULL)
	{
		osPortFree(EchoAudioBuf);
		EchoAudioBuf = NULL;
	}
    #endif

    #if (CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN)
    if(DynamicEQWathcBuf != NULL)
      {
	     osPortFree(DynamicEQWathcBuf);
	     DynamicEQWathcBuf = NULL;
      }
     #endif
}

void EffectPcmBufClear(uint32_t SampleLen)
{
	if(pcm_buf_1 != NULL)
	{
		memset(pcm_buf_1, 0, SampleLen * 2 * 2);
	}
	if(pcm_buf_2 != NULL)
	{
		memset(pcm_buf_2, 0, SampleLen * 2 * 2);
	}
	if(pcm_buf_3 != NULL)
	{
		memset(pcm_buf_3, 0, SampleLen * 2 * 2);
	}
	if(pcm_buf_4 != NULL)
	{
		memset(pcm_buf_4, 0, SampleLen * 2 * 2);
	}
	if(pcm_buf_5 != NULL)
	{
		memset(pcm_buf_5, 0, SampleLen * 2 * 2);
	}
	if(pcm_buf_6 != NULL)
	{
		memset(pcm_buf_6, 0, SampleLen * 2 * 2);
	}
	if(pcm_buf_7 != NULL)
	{
		memset(pcm_buf_7, 0, SampleLen * 2 * 2);
	}
}

/*
****************************************************************
* 音效模式选择函数
* 1.共预留10组调音参数，可由调音工具导出或下载；
* 2.每组调音参数对应1个音效模式；
****************************************************************
*/
#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
extern uint8_t flash_effect_total;
extern uint8_t  flash_effect_name[16];
extern uint8_t Audio_mode_buff[4096];////可动态分配
uint16_t ReadEffectParamas(uint32_t mode, uint8_t *buff,uint16_t rLen);
void WriteEffectParamas(uint32_t mode,uint8_t *buff);
/*
****************************************************************
* FLASH音效模式选择函数，HFP, USB PHONE专用
* 返回长名字长度
*
****************************************************************
*/
uint8_t AECGetMode(uint16_t mode)
{
	uint8_t i;

	if(mode == EFFECT_MODE_HFP_AEC)
	{
		i = 0;
		while(1)
		{
		  if(FLASH_EFFECT_TAB[i].len == 0x4000)
		  {
			return i;
		  }
		 i++;
		 if(i > 9) return mode;//error
		}
	}

	if(mode == EFFECT_MODE_USB_AEC)
	{
		i = 0;
		while(1)
		{
		  if(FLASH_EFFECT_TAB[i].len == 0x8000)
		  {
			return i;
		  }
		 i++;
		 if(i > 9) return mode;//error
		}
	}

	return mode;
}
#endif
/*
****************************************************************
* 音效模式名字选择函数
* 返回长名字长度
*
****************************************************************
*/
uint8_t GetAudioEffName (uint16_t mode, char *Name)
{
	uint16_t i = 0;

#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH

	mode = AECGetMode(mode);

	if(mode < 10)//从下载的10组参数对应的flash存储空间中获取
	{
		i = 0;
		while(1)
		{
		 if(*(FLASH_EFFECT_TAB[mode].name+i) ==0)
		 {
			 return i;
		 }
		 *(Name+i) = *(FLASH_EFFECT_TAB[mode].name+i);
		 i++;
		 if(i > 20) return 0;//error

		}

		return 0;
	}
	else//从调音数组中获取（由调音工具导出)
#endif
	{
		i = 0;

		while(1)
		{
			if(EFFECT_TAB[i].eff_mode == 0xffff)
			{
				return 0;
			}

			if(EFFECT_TAB[i].eff_mode == mode)
			{
				mode = i;
				i = 0;
				while(1)
				{
				 if(*(EFFECT_TAB[mode].name+i) ==0)
				 {
					 return i;
				 }
				 *(Name+i) = *(EFFECT_TAB[mode].name+i);
				 i++;
				 if(i > 20) return 0;//error
				}

	            return 0;
			}
			else
			{
			   i++;
			}
		}
	}
}
/*
****************************************************************
* 获取flash中记忆的调音参数
*
*
****************************************************************
*/
void LoadAudioParamas (uint16_t mode)
{
	uint16_t i = 0;
	APP_DBG("FUNC_ID_EFFECT_MODE -> %d\n", mode);

	Audio_mode.EffectParamas = 0;
	Audio_mode.len           = 0;
	Audio_mode.eff_mode      = 0;

#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
	uint16_t flash_effect_len;
	mode = AECGetMode(mode);

	if(mode < 10)//从下载的10组参数对应的flash存储空间中获取
	{
		APP_DBG("Audio Effect Parameter From FLASH!\n");
		flash_effect_len = ReadEffectParamas(mode,&Audio_mode_buff[0],4096);
		Audio_mode.eff_mode      = mode;
		if(flash_effect_len>289)//289=03~0d
		{
			APP_DBG("Parameter ok\n");
			Audio_mode.EffectParamas = (const uint8_t *)&Audio_mode_buff[0];
	        Audio_mode.len           = flash_effect_len;
	        Audio_mode.name          = FLASH_EFFECT_TAB[mode].name;
			APP_DBG("eff_mode:%d , len:%d name:%s\n",Audio_mode.eff_mode,Audio_mode.len,Audio_mode.name);
			return;
		}
		else
		{
			if(GetSystemMode() == AppModeBtHfPlay)//
			{
				APP_DBG("Parameter Fail,Default AecBuf.C\n");
				mode = EFFECT_MODE_HFP_AEC;
			}
			else if(GetSystemMode() == AppModeUsbPhone)//
			{
				APP_DBG("Parameter Fail,Default UsbAecBuf.C\n");
				mode = EFFECT_MODE_USB_AEC;
			}
			else
			{
				APP_DBG("Parameter Fail,Default HunXiang.C\n");
				mode  = EFFECT_MODE_HunXiang;
			}
		}
	}
#endif

	if(mode >( 10-1))//从调音数组中获取（由调音工具导出)
	{
		APP_DBG("Get Audio Effect Parameter From Data.c!!!!!!\n");

		i = 0;
		while(1)
		{
			if(EFFECT_TAB[i].eff_mode == 0xffff)
			{
				APP_DBG("No Audio effect File load!!!!\n");
				return;
			} 	  

			if(EFFECT_TAB[i].eff_mode == mode)   
			{
				Audio_mode.eff_mode      = EFFECT_TAB[i].eff_mode;
				Audio_mode.EffectParamas = EFFECT_TAB[i].EffectParamas;
	            Audio_mode.len           = EFFECT_TAB[i].len;
				APP_DBG("number:%d , eff_mode:%d , len:%d name:%s\n",i,EFFECT_TAB[i].eff_mode,EFFECT_TAB[i].len,EFFECT_TAB[i].name);
	            return;
			}
			else
			{
			   i++;
			}
		}
	}
}


/*
****************************************************************
* 音效模式选择函数
* 1.共预留10组调音参数，可由调音工具导出或下载；
* 2.每组调音参数对应1个音效模式；
****************************************************************
*/
void AudioEffectModeSel(uint16_t mode, uint8_t init_flag)//0=hw,1=effect,2=hw+effect ff= no init
{
	WDG_Feed();
	#ifdef BT_TWS_SUPPORT
	if((tws_get_role() == BT_TWS_SLAVE)&& (GetBtManager()->twsState == BT_TWS_STATE_CONNECTED))//slave
		mode+= EFFECT_MODE_SLAVE_INDEX;
    #endif
	
    #ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
	if( (mode != EFFECT_MODE_HFP_AEC) && (mode != EFFECT_MODE_USB_AEC))
	{
		if(mode > flash_effect_total)
		{
			mode = 0;//default
			mainAppCt.EffectMode = mode;
		}
	}
    #endif
	
	LoadAudioParamas(mode);

	if(init_flag == 0xff)///no init
	{
		return;
	}

	if(Audio_mode.EffectParamas == 0)
	{
		gCtrlVars.AutoRefresh = 1;
		return;
	}

	if(init_flag == 0)///only hardware init
	{
		LoadAudioMode(Audio_mode.len,Audio_mode.EffectParamas , init_flag);//0=hw,1=effect,2=hw+effect
		gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
		return;
	}

	if(!AudioEffectListJudge(Audio_mode.len,Audio_mode.EffectParamas))
	{
		LoadAudioMode(Audio_mode.len,Audio_mode.EffectParamas , 0);//0=hw,1=effect,2=hw+effect
		AudioEffectsAllDisable();//存在非法的音效或列表
		AudioEffectsDeInit();
		gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
		return;
	}

	AudioEffectsAllDisable();

	LoadAudioMode(Audio_mode.len, Audio_mode.EffectParamas , init_flag);//0=hw,1=effect,2=hw+effect
}

#ifdef CFG_FUNC_MIC_KARAOKE_EN
/*
****************************************************************
* Music+Mic音效处理主函数
*
*
****************************************************************
*/
void AudioEffectProcess(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	int16_t  pcm;
	uint16_t n = mainAppCt.SamplesPreFrame;
	int16_t *mic_pcm    	= NULL;//pBuf->mic_in;///mic input	
	int16_t *bypass_tmp 	= NULL;//pBuf->mic_in;
	int16_t *music_pcm    	= NULL;//pBuf->music_in;///music input
#ifdef CFG_FUNC_LINE_MIX_MODE	
	int16_t *line_in        = NULL;//pBuf->line_in;
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
	int16_t *spdif_in        = NULL;//pBuf->spdif_in;
#endif
	#ifdef CFG_FUNC_USB_MIX_MODE
	#ifdef CFG_RES_AUDIO_USB_IN_EN
	int16_t *usb_in         = NULL;//pBuf->usb_in;
	#endif
	#endif
#ifdef  CFG_FUNC_I2S_MIX_MODE	
	int16_t *i2s0_in        = NULL;//pBuf->i2s0_in;
	int16_t *i2s1_in        = NULL;//pBuf->i2s1_in;
#endif

	int16_t *remind_in      = NULL;//pBuf->remind_in;

#ifdef 	CFG_FUNC_REMIND_MIX_MODE
	int16_t *mix_remind_in  = NULL;//pBuf->mix_remind_in;
#endif
	
	int16_t *monitor_out    = NULL;//pBuf->dac0_out; 
	int16_t *record_out     = NULL;//pBuf->dacx_out; 
	int16_t *i2s_out       	= NULL;//pBuf->i2s0_out;
#ifdef  CFG_FUNC_I2S_MIX_MODE
	int16_t *i2s0_out       = NULL;//pBuf->i2s0_out; 
	int16_t *i2s1_out       = NULL;//pBuf->i2s1_out; 
#endif
	int16_t *usb_out        = NULL;//pBuf->usb_out; 
#ifdef CFG_FUNC_RECORDER_EN
	int16_t *local_rec_out  = NULL;//pBuf->rec_out; 
#endif
#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	int16_t *spdif_out		= NULL;
#endif

	int16_t *echo_tmp   	= (int16_t *)pcm_buf_1;
	int16_t *reverb_tmp 	= (int16_t *)pcm_buf_2;
	int16_t *b_e_r_mix_tmp 	= (int16_t *)pcm_buf_2;
	int16_t *rec_bypass_tmp = (int16_t *)pcm_buf_3;
	int16_t *rec_effect_tmp = (int16_t *)pcm_buf_4;
	int16_t *rec_music_tmp  = (int16_t *)pcm_buf_5;
    int16_t *rec_remind_tmp = (int16_t *)pcm_buf_6;

    MixInputUnit mix_in;

    MixAudioFlag = 0;

	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Enable == TRUE)////mic buff
	{
		bypass_tmp = mic_pcm = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//双mic输入
	}

#ifdef CFG_FUNC_RECORDER_EN
	if(GetSystemMode() == AppModeCardPlayBack
		|| GetSystemMode() == AppModeUDiskPlayBack
		|| GetSystemMode() == AppModeFlashFsPlayBack
		|| GetSystemMode() == AppModeFlashPlayBack)
	{
		if(pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].Enable == TRUE)
		{
			music_pcm = pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf;// include usb/sd source 
		}
	}
	else
#endif
	{
		if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff
		{
			music_pcm = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;// include line/bt/usb/sd/spdif/hdmi/i2s/radio source
		}
	}	
	
#if defined(CFG_FUNC_REMIND_SOUND_EN)	
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}	
    if(remind_in)
	{
		music_pcm = NULL;
	}
#endif

    //-----------------------------------//
    memset(&mix_in,0,sizeof(mix_in));
    mix_in.music_in = &music_pcm;
    //------------------------------------//

#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_RES_AUDIO_USB_IN_EN
    mix_in.usb_in = &usb_in;
	if(pAudioCore->AudioSource[USB_MIX_SOURCE_NUM].Enable == TRUE)
	{
		usb_in = pAudioCore->AudioSource[USB_MIX_SOURCE_NUM].PcmInBuf;
		MixAudioFlag++;
		mix_in.usb_mix_en = CFG_FUNC_USB_MIX_EN;
	}
#endif
#endif

#ifdef CFG_FUNC_LINE_MIX_MODE
	mix_in.line_in = &line_in;
	if(pAudioCore->AudioSource[LINE_SOURCE_NUM].Enable == TRUE)
	{
		line_in = pAudioCore->AudioSource[LINE_SOURCE_NUM].PcmInBuf;
		MixAudioFlag++;
		mix_in.line_mix_en = CFG_FUNC_LINE_MIX_EN;
	}
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
#ifdef CFG_RES_AUDIO_I2S0IN_EN
	mix_in.i2s0_in = &i2s0_in;
	if(pAudioCore->AudioSource[I2S0_SOURCE_NUM].Enable == TRUE)	////dacx buff
	{
	#ifdef CFG_RES_MICIN_BY_I2SIN
		bypass_tmp = mic_pcm = pAudioCore->AudioSource[I2S0_SOURCE_NUM].PcmInBuf;
	#else
		i2s0_in = pAudioCore->AudioSource[I2S0_SOURCE_NUM].PcmInBuf;
		MixAudioFlag++;
		mix_in.i2s0_mix_en = CFG_FUNC_I2S0_MIX_EN;
	#endif
	}
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	mix_in.i2s1_in = &i2s1_in;
	if(pAudioCore->AudioSource[I2S1_SOURCE_NUM].Enable == TRUE)	////dacx buff
	{
		i2s1_in = pAudioCore->AudioSource[I2S1_SOURCE_NUM].PcmInBuf;
		MixAudioFlag++;
		mix_in.i2s1_mix_en = CFG_FUNC_I2S1_MIX_EN;
	}
#endif
#endif //end of CFG_FUNC_I2S_MIX_MODE

#ifdef CFG_FUNC_SPDIF_MIX_MODE
	mix_in.spdif_in = &spdif_in;
	if(pAudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].Enable == TRUE)	////dacx buff
	{
		spdif_in = pAudioCore->AudioSource[SPDIF_MIX_SOURCE_NUM].PcmInBuf;
		MixAudioFlag++;
		mix_in.spdif_mix_en = CFG_FUNC_SPDIF_MIX_EN;
	}
#endif //end of CFG_FUNC_SPDIF_MIX_MODE

#ifdef CFG_FUNC_REMIND_MIX_MODE
	mix_in.remind_mix_in = &mix_remind_in;
	if(pAudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].Enable == TRUE)
	{
		mix_remind_in = pAudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].PcmInBuf;
		MixAudioFlag++;
		mix_in.remind_mix_en = CFG_FUNC_REMIND_MIX_EN;
	}
#endif

#ifdef CFG_RES_AUDIO_DAC0_EN
	if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		monitor_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_DACX_EN 	
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
	}	
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN 	
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		i2s_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].PcmOutBuf;
	}
#endif
#endif

#ifdef CFG_FUNC_RECORDER_EN
	if(pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].Enable == TRUE)
	{
		local_rec_out = pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].Enable == TRUE)
	{
		usb_out = pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	if(pAudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].Enable == TRUE)   
	{
		spdif_out = pAudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
    if((music_pcm == NULL)&&(mainAppCt.EqModeBak != mainAppCt.EqMode))
	{		
		mainAppCt.EqModeBak = mainAppCt.EqMode;
		EqModeSet(mainAppCt.EqMode);	
	}
#endif

#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL){
		osMutexLock(AudioEffectMutex);
	}
#endif

    if(monitor_out){
		memset(monitor_out, 0, n * 2 * 2);
    }

    if(record_out){
		memset(record_out, 0, n * 2 * 2);
    }
	
    if(usb_out){
		memset(usb_out, 0, n * 2 * 2);//mono*2 stereo*4
    }
	
    if(i2s_out){
		memset(i2s_out, 0, n * 2 * 2);//mono*2 stereo*4
    }

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN	
	if(spdif_out){
		memset(spdif_out, 0, n * 2 * 2);
	}
#endif
    EffectPcmBufClear(mainAppCt.SamplesPreFrame);

	#ifdef CFG_FUNC_REMIND_SOUND_EN
	//提示音音效处理
	if(remind_in)
	{
		if(gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND)
		{
		    //get background remind data for record 
			memcpy(rec_remind_tmp,remind_in,n*2*2);
			#ifdef CFG_FUNC_SHUNNING_EN			
			uint32_t gain;
			gain = gCtrlVars.remind_effect_gain_control_unit.gain;
			gCtrlVars.remind_effect_gain_control_unit.gain = gCtrlVars.remind_out_dyn_gain/2;//闪避功能打开时用到
			#endif
			AudioEffectPregainApply(&gCtrlVars.remind_effect_gain_control_unit, remind_in, remind_in, n, 2);
			#ifdef CFG_FUNC_SHUNNING_EN
			gCtrlVars.remind_effect_gain_control_unit.gain = gain;
			#endif
		}
	    else
    	{
			AudioEffectPregainApply(&gCtrlVars.remind_key_gain_control_unit, remind_in, remind_in, n, 2);
    	}
		AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
	}
    #endif

	//---------------------------------------------------//
	//#ifndef BT_TWS_SUPPORT
	AudioInputMix(&mix_in);	//
	//#endif
	//-------------------------------------------------//

	//伴奏信号音效处理
	if(music_pcm)//APP_SOURCE_NUM
	{
		if((GetSystemMode() == AppModeCardPlayBack)
				|| (GetSystemMode() == AppModeUDiskPlayBack)
				|| (GetSystemMode() == AppModeFlashFsPlayBack)
				|| (GetSystemMode() == AppModeFlashPlayBack))
		{
			#if (defined(BT_TWS_SUPPORT) && CFG_EFFECT_MUSIC_MASTER == 0)
			if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff//APP_SOURCE_NUM
			{
				//AudioInputMix(&mix_in);	//
			}
			TWS_AudioEffectProcess(&music_pcm,&remind_in);//don't move
			#endif
			
			#if CFG_AUDIO_EFFECT_USB_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.usb_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
			
			#if CFG_AUDIO_EFFECT_AUX_GAIN_CONTROL_EN
			{
				#ifdef CFG_FUNC_SHUNNING_EN
				uint32_t gain;
				gain = gCtrlVars.aux_gain_control_unit.gain;
				gCtrlVars.aux_gain_control_unit.gain = gCtrlVars.aux_out_dyn_gain;//闪避功能打开时用到
				#endif
				AudioEffectPregainApply(&gCtrlVars.aux_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
				#ifdef CFG_FUNC_SHUNNING_EN
				gCtrlVars.aux_gain_control_unit.gain = gain;
				#endif
			}		
			#endif			

			if((rec_music_tmp)
				#ifdef BT_TWS_SUPPORT
				&&(CFG_EFFECT_MUSIC_MASTER == 0)
				#endif
				)
			{
				memcpy(rec_music_tmp,music_pcm,n*2*2);
			}

#ifdef CFG_FUNC_RECORDER_EN			
			AudioCoreAppSourceVolSet(PLAYBACK_SOURCE_NUM, music_pcm, n, 2);
#endif

#if (defined(BT_TWS_SUPPORT) && CFG_EFFECT_MUSIC_MASTER == 1)
			if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff//APP_SOURCE_NUM
			{
				//AudioInputMix(&mix_in);	//
			}
			TWS_AudioEffectProcess(&music_pcm,&remind_in);//don't move
#endif
	        #if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN
			AudioEffectSilenceDectorApply(&gCtrlVars.MusicAudioSdct_unit,  music_pcm,  n);
			#endif
			
			if((rec_music_tmp)
#ifdef BT_TWS_SUPPORT
				&&(CFG_EFFECT_MUSIC_MASTER == 1)
#endif
				)
			{
				memcpy(rec_music_tmp,music_pcm,n*2*2);
			}			
		}
		else
		{
			
			if(gCtrlVars.adc_line_channel_num == 1)
			{
				for(s = 0; s < n; s++)
				{
					music_pcm[s] = __nds32__clips((((int32_t)music_pcm[2 * s + 0] + (int32_t)music_pcm[2 * s + 1]) ), 16-1);
				}
			}

#if (defined(BT_TWS_SUPPORT) && CFG_EFFECT_MUSIC_MASTER == 0)
			//if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff//APP_SOURCE_NUM
			{
				//AudioInputMix(&mix_in);	//
			}
			TWS_AudioEffectProcess(&music_pcm, &remind_in);//don't move
#endif
			#if CFG_AUDIO_EFFECT_MUSIC_NOISE_SUPPRESSOR_EN
			AudioEffectExpanderApply(&gCtrlVars.music_expander_unit, music_pcm, music_pcm, n);
			#endif
			
            #if CFG_AUDIO_EFFECT_LRBALANCER_EN
			AudioEffectLRBalancerApply(&gCtrlVars.music_lr_balancer,music_pcm,music_pcm,n);
            #endif

			//如果想要在音效处理前获取能量值则打开此处，屏蔽掉下面的函数调用
			#if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN
            #if (CFG_FUNC_MIX_AUDIO_SDCT_EN==0)
			AudioEffectSilenceDectorApply(&gCtrlVars.MusicAudioSdct_unit,  music_pcm,  n);
            #endif
			#endif

			if((GetSystemMode() == AppModeOpticalAudioPlay) || (GetSystemMode() == AppModeCoaxialAudioPlay) || (GetSystemMode() == AppModeHdmiAudioPlay))
			{
				#if CFG_AUDIO_EFFECT_SPDIF_IN_GAIN_CONTROL_EN
				AudioEffectPregainApply(&gCtrlVars.spdif_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
				#endif
			}
			else if(GetSystemMode() == AppModeI2SInAudioPlay)
			{
				#if CFG_AUDIO_EFFECT_I2S_IN_GAIN_CONTROL_EN
				AudioEffectPregainApply(&gCtrlVars.i2s_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
				#endif
			}
			else if((GetSystemMode() == AppModeCardAudioPlay) || (GetSystemMode() == AppModeUDiskAudioPlay) || (GetSystemMode() == AppModeUsbDevicePlay))
			{
				#if CFG_AUDIO_EFFECT_USB_IN_GAIN_CONTROL_EN
				AudioEffectPregainApply(&gCtrlVars.usb_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
				#endif
			}
			else if(GetSystemMode() == AppModeBtAudioPlay)
			{
				#if CFG_AUDIO_EFFECT_BT_IN_GAIN_CONTROL_EN
				AudioEffectPregainApply(&gCtrlVars.bt_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
				#endif
			}
			
			#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
	        AudioEffectVocalCutApply(&gCtrlVars.vocal_cut_unit, music_pcm, music_pcm, n);
			#endif
			
			#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
	        AudioEffectVocalRemoveApply(&gCtrlVars.vocal_remove_unit,  music_pcm, music_pcm, n);
			#endif			

			#if CFG_AUDIO_EFFECT_AUX_GAIN_CONTROL_EN
			{
				#ifdef CFG_FUNC_SHUNNING_EN
				uint32_t gain;
				gain = gCtrlVars.aux_gain_control_unit.gain;
				gCtrlVars.aux_gain_control_unit.gain = gCtrlVars.aux_out_dyn_gain;//闪避功能打开时用到
				#endif
				AudioEffectPregainApply(&gCtrlVars.aux_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
				#ifdef CFG_FUNC_SHUNNING_EN
				gCtrlVars.aux_gain_control_unit.gain = gain;
				#endif
			}		
			#endif	

			#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
			AudioEffectPcmDelayApply(&gCtrlVars.music_delay_unit, music_pcm, music_pcm, n);
			#endif

			#if CFG_AUDIO_EFFECT_COMPANDER_EN
			AudioEffectCompanderApply(&gCtrlVars.compander_unit,music_pcm,music_pcm,n);
			#endif

			#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
			AudioEffectLowLevelCompressorApply(&gCtrlVars.mic_low_level_compressor_unit, music_pcm, music_pcm, n);
			#endif
			
			#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN 
			AudioEffectPitchShifterProApply(&gCtrlVars.pitch_shifter_pro_unit, music_pcm, music_pcm, n);
			#endif	
			
			#if CFG_AUDIO_EFFECT_MUSIC_EXCITER_EN
			AudioEffectExciterApply(&gCtrlVars.music_exciter_unit, music_pcm, music_pcm, n);
			#endif
		
			#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
	        AudioEffectVBApply(&gCtrlVars.music_vb_unit, music_pcm, music_pcm, n);
			#endif

            #if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	        AudioEffectVirtualSurroundApply(&gCtrlVars.virtual_surround_unit, music_pcm, music_pcm, n);
            #endif

			#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
	        AudioEffectVBClassicApply(&gCtrlVars.music_vb_classic_unit, music_pcm, music_pcm, n);
			#endif			

			#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
	        AudioEffectThreeDApply(&gCtrlVars.music_threed_unit, music_pcm, music_pcm, n);
	        #endif	

			#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
	        AudioEffectThreeDPlusApply(&gCtrlVars.music_threed_plus_unit, music_pcm, music_pcm, n);
	        #endif
			
	        #if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
	        AudioEffectStereoWidenerApply(&gCtrlVars.stereo_winden_unit, music_pcm, music_pcm, n);
	        #endif

			#if CFG_AUDIO_EFFECT_MUSIC_PRE_EQ_EN
			AudioEffectEQApply(&gCtrlVars.music_pre_eq_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif

            #if CFG_AUDIO_EFFECT_DRAPOST_EN
			AudioEffectDraPostApply(&gCtrlVars.dra_post_unit, music_pcm, music_pcm, n);
            #endif

            #if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
            AudioEffectButterWorthApply(&gCtrlVars.music_butterworth_unit, music_pcm, music_pcm, n);
            #endif

            #if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
            AudioEffectDynamicEqApply(&gCtrlVars.music_dynamic_eq_unit, music_pcm, music_pcm,n);//??
            #endif

			if((rec_music_tmp)
				#ifdef BT_TWS_SUPPORT
				&&(CFG_EFFECT_MUSIC_MASTER == 0)
				#endif
				)
			{
				memcpy(rec_music_tmp,music_pcm,n*2*2);
			}			

			#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
			AudioEffectEqDrcApply(&gCtrlVars.music_eq_drc_unit, music_pcm, music_pcm, n);
			#endif
			
			#if CFG_AUDIO_EFFECT_MUSIC_DRC_EN
			AudioEffectDRCApply(&gCtrlVars.music_drc_unit, music_pcm, music_pcm, n);
			#endif
            #if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
			AudioEffectDRCLegacyApply(&gCtrlVars.music_drc_legacy_unit, music_pcm, music_pcm, n);
            #endif
			#if CFG_AUDIO_EFFECT_MUSIC_OUT_EQ_EN		
			AudioEffectEQApply(&gCtrlVars.music_out_eq_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif

			if(MixAudioFlag == 0)
			{
				AudioCoreAppSourceVolSet(APP_SOURCE_NUM, music_pcm, n, 2);
			}

			if(gCtrlVars.adc_line_channel_num == 1)
			{
				memcpy(&music_pcm[n], music_pcm, n*2);
				for(s = 0; s < n; s++)
				{
					music_pcm[2*s + 0] = music_pcm[2*s + 1] = music_pcm[n+s];
				}
			}

#if (defined(BT_TWS_SUPPORT) && (CFG_EFFECT_MUSIC_MASTER == 1))
			//if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff//APP_SOURCE_NUM
			{
				//AudioInputMix(&mix_in);	//
			}
			TWS_AudioEffectProcess(&music_pcm, &remind_in);//don't move
#endif			
			//如果想要在音效处理前获取能量值则屏蔽此处
			#if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN
            #if (CFG_FUNC_MIX_AUDIO_SDCT_EN==0)
            AudioEffectSilenceDectorApply(&gCtrlVars.MusicAudioSdct_unit,  music_pcm,  n);
            #endif
			#endif

			if((rec_music_tmp)
				#ifdef BT_TWS_SUPPORT
				&&(CFG_EFFECT_MUSIC_MASTER == 1) 
				#endif
				)
			{
				memcpy(rec_music_tmp,music_pcm,n*2*2);
			}
		}		
	}
	else
	{
		#ifdef BT_TWS_SUPPORT
		//AudioInputMix(&mix_in);	//
		TWS_RemindAudioProcess(&remind_in);//don't move
		#endif
	}

    //MIC信号音效处理
	if(mic_pcm)
	{	
		//pre eq
		#if CFG_AUDIO_EFFECT_MIC_PRE_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_pre_eq_unit, mic_pcm, mic_pcm, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
		#if CFG_AUDIO_EFFECT_MIC_NOISE_SUPPRESSOR_EN
		AudioEffectExpanderApply(&gCtrlVars.mic_expander_unit, mic_pcm, mic_pcm, n);
		#endif

		#ifdef CFG_FUNC_GUITAR_EN
		//吉他信号处理
		if(guitar_pcm)
		{
			for(s = 0; s < n; s++)
			{
				guitar_pcm[s]  = mic_pcm[2 * s + 1];//吉他取MIC的右声道
			}
			for(s = 0; s < n; s++)
			{
				mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0]; 
			}
			#if CFG_AUDIO_EFFECT_GUITAR_EQ_EN
	        //AudioEffectEQApply(&gCtrlVars.guitar_eq_unit, guitar_pcm, guitar_pcm, n, 2);
	        #endif

	        #if CFG_AUDIO_EFFECT_CHORUS_EN
	        AudioEffectChorusApply(&gCtrlVars.chorus_unit,  guitar_pcm, guitar_pcm, n);
	        #endif

			#if CFG_AUDIO_EFFECT_CHORUS2_EN
			AudioEffectChorus2Apply(&gCtrlVars.chorus2_unit,  guitar_pcm, guitar_pcm, n);
			#endif

			#if CFG_AUDIO_EFFECT_AUTOWAH_EN
			AudioEffectAutoWahApply(&gCtrlVars.auto_wah_unit, guitar_pcm, guitar_pcm, n);
			#endif

			#if CFG_AUDIO_EFFECT_FLANGER_EN
			AudioEffectFlangerApply(&gCtrlVars.flanger_uint, guitar_pcm, guitar_pcm, n); //only mono
			#endif

			#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
			AudioEffectOverdriveApply(&gCtrlVars.overdrive_unit, guitar_pcm, guitar_pcm, n);
			#endif

            #if CFG_AUDIO_EFFECT_OVERDRIVER_POLY_EN
            AudioEffectOverdrivePolyApply(&gCtrlVars.overdrive_poly_unit, guitar_pcm, guitar_pcm, n);
            #endif

			#if CFG_AUDIO_EFFECT_DISTORTION_EN
			AudioEffectDistortionExpApply(&gCtrlVars.distortion_unit,  guitar_pcm, guitar_pcm, n);
			#endif

            #if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
            AudioEffectDistortionDS1Apply(&gCtrlVars.distortion_ds1_unit,  guitar_pcm, guitar_pcm, n);
            #endif

            #if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
			AudioEffectPitchDetectorApply(&gCtrlVars.pitch_detector_unit,  guitar_pcm, n);
            #endif
            //********************* mono 2 stereo ****************************
			for(s = 0; s < n; s++)
			{
			    rec_bypass_tmp[s] = guitar_pcm[s];
			}
			for(s = 0; s < n; s++)
			{
			    pcm   =rec_bypass_tmp[s];//
				guitar_pcm[2 * s + 0] = pcm; 
				guitar_pcm[2 * s + 1] = pcm; 
			}	
			
	        #if CFG_AUDIO_EFFECT_PINGPONG_EN				
	        AudioEffectPinPongApply(&gCtrlVars.ping_pong_unit, guitar_pcm, guitar_pcm, n);		
	        #endif

	        #if CFG_AUDIO_EFFECT_GUITAR_ECHO_EN
	        //AudioEffectEchoApply(&gCtrlVars.guitar_echo_unit, guitar_pcm, guitar_pcm, n);
	        #endif

	        #if CFG_AUDIO_EFFECT_GUITAR_GAIN_EN
	        AudioEffectPregainApply(&gCtrlVars.guitar_gain_control_unit, guitar_pcm, guitar_pcm, n, 2);
	        #endif
		}
        #else
		
	    #ifdef CFG_FUNC_DETECT_MIC_EN
		if(gCtrlVars.MicOnlin && (!gCtrlVars.Mic2Onlin))
		{
			#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
			if(gCtrlVars.MicSegment == 3)
			{
				for(s = 0; s < n; s++)
				{
					mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0] = 0; 
				}
			}
			else
			#endif
			{
				for(s = 0; s < n; s++)
				{
					mic_pcm[s*2 + 0] = mic_pcm[s*2 + 1]; 
				}
			}
		}
		else if((!gCtrlVars.MicOnlin) && gCtrlVars.Mic2Onlin)
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0]; 
			}
		}
		else if(gCtrlVars.MicOnlin && gCtrlVars.Mic2Onlin)
		{
			#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
			if(gCtrlVars.MicSegment == 3)
			{
				for(s = 0; s < n; s++)
				{
					mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0]; 
				}
			}
			else
			#endif
			{
				for(s = 0; s < n; s++)
				{
					pcm	= __nds32__clips((((int32_t)mic_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 1]) ), 16-1); 
					mic_pcm[2 * s + 0] = pcm; 
					mic_pcm[2 * s + 1] = pcm; 
				}
			}
		}
		#else
		if( (gCtrlVars.line3_l_mic1_en) && (gCtrlVars.line3_r_mic2_en) )
		{
			for(s = 0; s < n; s++)
			{
				pcm	= __nds32__clips((((int32_t)mic_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 1]) ), 16-1); 
				mic_pcm[2 * s + 0] = pcm; 
				mic_pcm[2 * s + 1] = pcm; 
			}
		}
		else if(gCtrlVars.line3_l_mic1_en )
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0]; 
			}
		}
		else if( gCtrlVars.line3_r_mic2_en )
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s*2 + 0] = mic_pcm[s*2 + 1]; 
			}
		}
		#endif
        else
		{
			memset(mic_pcm, 0, n * 2 * 2);
		}
		#endif
		
		if(gCtrlVars.adc_mic_channel_num == 1)
		{
			for(s = 0; s < n; s++)
			{
				pcm	= __nds32__clips((((int32_t)mic_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 1]) ), 16-1);
				mic_pcm[s] = pcm;
			}
		}
		
        #if CFG_AUDIO_EFFECT_MIC_SILENCE_DECTOR_EN
		AudioEffectSilenceDectorApply(&gCtrlVars.MicAudioSdct_unit,  mic_pcm,  n);
		#endif
		
		#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN || CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN || CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN || CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
		//freq shifter，howling只支持单声道数据	
		 //********************* stereo 2 mono ****************************  
		if((gCtrlVars.adc_mic_channel_num == 2) && (gCtrlVars.freq_shifter_unit.enable 
			|| gCtrlVars.howling_dector_unit.enable
			#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
			|| gCtrlVars.howling_dector_fine_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
			|| gCtrlVars.howling_dector_specified_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
			|| gCtrlVars.mic_howling_guard_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
			|| gCtrlVars.voice_changer_pro_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN
			|| gCtrlVars.mic_ns_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
			|| gCtrlVars.overdrive_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_DISTORTION_EN
			|| gCtrlVars.distortion_unit.enable
			#endif
			))
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s] = mic_pcm[2 * s + 0];
			}
		}
		#endif
		#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
		AudioEffectOverdriveApply(&gCtrlVars.overdrive_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_DISTORTION_EN
		AudioEffectDistortionExpApply(&gCtrlVars.distortion_unit,  mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
		AudioEffectPitchDetectorApply(&gCtrlVars.pitch_detector_unit,  mic_pcm,  n);
		#endif
				
		#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN
		AudioEffectBlueNSApply(&gCtrlVars.mic_ns_unit, mic_pcm, mic_pcm);
		#endif

        #if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN
		AudioEffectFreqShifterApply(&gCtrlVars.freq_shifter_unit, mic_pcm, mic_pcm, n);
		#endif
		
		#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
		AudioEffectFreqShifterFineApply(&gCtrlVars.freq_shifter_fine_unit, mic_pcm, mic_pcm, n);
		#endif

        #if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN
		AudioEffectHowlingSuppressorApply(&gCtrlVars.howling_dector_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
		AudioEffectHowlingSuppressorFineApply(&gCtrlVars.howling_dector_fine_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
		AudioEffectHowlingSuppressorSpecifiedApply(&gCtrlVars.howling_dector_specified_unit,mic_pcm,mic_pcm,n);
		#endif

		#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
		AudioEffectHowlingGuardApply(&gCtrlVars.mic_howling_guard_unit,mic_pcm,mic_pcm,n);
		#endif
		#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN 
		AudioEffectVoiceChangerProApply(&gCtrlVars.voice_changer_pro_unit, mic_pcm, mic_pcm, n);
		#endif
		
		#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN || CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN || CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN || CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
		 //********************* mono 2 stereo ****************************
		if((gCtrlVars.adc_mic_channel_num == 2) && (
				gCtrlVars.freq_shifter_unit.enable 
			#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
			|| gCtrlVars.freq_shifter_fine_unit.enable
			#endif	
			|| gCtrlVars.howling_dector_unit.enable
			#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_FINE_EN
			|| gCtrlVars.howling_dector_fine_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
			|| gCtrlVars.howling_dector_specified_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
			|| gCtrlVars.mic_howling_guard_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
			|| gCtrlVars.voice_changer_pro_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_MIC_BLUENS_SUPPRESSOR_EN
			|| gCtrlVars.mic_ns_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
			 || gCtrlVars.pitch_detector_unit.enable
			#endif	
			#if CFG_AUDIO_EFFECT_OVERDRIVER_EN
			|| gCtrlVars.overdrive_unit.enable
			#endif
			#if CFG_AUDIO_EFFECT_DISTORTION_EN
			|| gCtrlVars.distortion_unit.enable
			#endif	
			))
		{	
		    memcpy(rec_bypass_tmp,mic_pcm,n*2);
			for(s = 0; s < n; s++)
			{
			    pcm   =rec_bypass_tmp[s];//
				mic_pcm[2 * s + 0] = pcm; 
				mic_pcm[2 * s + 1] = pcm; 
			}		
		}
		#endif

		#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_EN 
		AudioEffectVoiceChangerApply(&gCtrlVars.voice_changer_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN 
		AudioEffectPitchShifterApply(&gCtrlVars.pitch_shifter_unit, mic_pcm, mic_pcm, n, gCtrlVars.adc_mic_channel_num);
		#endif					
		
		#if CFG_AUDIO_EFFECT_MIC_AUTO_TUNE_EN
		AudioEffectAutoTuneApply(&gCtrlVars.auto_tune_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_ECHO_EN
		AudioEffectEchoApply(&gCtrlVars.echo_unit, mic_pcm, echo_tmp, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_REVERB_EN
		AudioEffectReverbApply(&gCtrlVars.reverb_unit, mic_pcm, reverb_tmp, n);
		#endif

        #if CFG_AUDIO_EFFECT_MIC_PLATE_REVERB_EN
		AudioEffectPlateReverbApply(&gCtrlVars.plate_reverb_unit, mic_pcm, reverb_tmp, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
		AudioEffectReverbProApply(&gCtrlVars.reverb_pro_unit, mic_pcm, reverb_tmp, n);
		#endif
		
		//bypass eq
		#if CFG_AUDIO_EFFECT_MIC_BYPASS_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_bypass_eq_unit, bypass_tmp, bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

        //get bypass data for record
		memcpy(rec_bypass_tmp,bypass_tmp,n*2*2);
		
        //echo eq
		#if CFG_AUDIO_EFFECT_MIC_ECHO_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_echo_eq_unit, echo_tmp, echo_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif		
		
        //reverb eq
		#if CFG_AUDIO_EFFECT_MIC_REVERB_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_reverb_eq_unit, reverb_tmp, reverb_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

        //get echo+reverb data for record 
		for(s = 0; s < n; s++)
		{
			int32_t Data_L = (int32_t)echo_tmp[2*s+0] + (int32_t)reverb_tmp[2*s+0];
			int32_t Data_R = (int32_t)echo_tmp[2*s+1] + (int32_t)reverb_tmp[2*s+1];

			rec_effect_tmp[2 * s + 0] = __nds32__clips((Data_L >> 0), 16-1);
			rec_effect_tmp[2 * s + 1] = __nds32__clips((Data_R >> 0), 16-1);
		}
		
        //bypass pregain
		#if CFG_AUDIO_EFFECT_MIC_BYPASS_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.mic_bypass_gain_control_unit, bypass_tmp, bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
        //echo pregain
		#if CFG_AUDIO_EFFECT_MIC_ECHO_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.mic_echo_control_unit, echo_tmp, echo_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

		//reverb pregain
		#if CFG_AUDIO_EFFECT_MIC_REVERB_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.mic_reverb_gain_control_unit, reverb_tmp, reverb_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
		//mux
		if(gCtrlVars.adc_mic_channel_num == 2)
		{
			for(s = 0; s < n; s++)
			{
				int32_t Data_L = (int32_t)bypass_tmp[2*s+0] + (int32_t)echo_tmp[2*s+0] + (int32_t)reverb_tmp[2*s+0];
				int32_t Data_R = (int32_t)bypass_tmp[2*s+1] + (int32_t)echo_tmp[2*s+1] + (int32_t)reverb_tmp[2*s+1];

				b_e_r_mix_tmp[2 * s + 0] = __nds32__clips((Data_L >> 0), 16-1);
				b_e_r_mix_tmp[2 * s + 1] = __nds32__clips((Data_R >> 0), 16-1);
			}
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				int32_t Data_M = (int32_t)bypass_tmp[s] + (int32_t)echo_tmp[s] + (int32_t)reverb_tmp[s];
				b_e_r_mix_tmp[s] = __nds32__clips((Data_M >> 0), 16-1);
			}
		}

		if(gCtrlVars.adc_mic_channel_num == 1)
		{
			memcpy(&b_e_r_mix_tmp[n], b_e_r_mix_tmp, n*2);
			memcpy(&bypass_tmp[n], bypass_tmp, n*2);
			for(s = 0; s < n; s++)
			{
				b_e_r_mix_tmp[2*s + 0] = b_e_r_mix_tmp[2*s + 1] = b_e_r_mix_tmp[n+s];
				bypass_tmp[2*s + 0]    = bypass_tmp[2*s + 1]    = bypass_tmp[n+s];
			}
		}
		
		#if CFG_AUDIO_EFFECT_MIC_DRC_EN
		AudioEffectDRCApply(&gCtrlVars.mic_drc_unit, b_e_r_mix_tmp, b_e_r_mix_tmp, n);
		#endif

		#if CFG_AUDIO_EFFECT_DRC_LEGACY_EN
		AudioEffectDRCLegacyApply(&gCtrlVars.mic_drc_legacy_unit, b_e_r_mix_tmp, b_e_r_mix_tmp, n);
		#endif

	}	
	//DAC0立体声监听音效处理
	if(monitor_out)
	{			
		#if CFG_AUDIO_EFFECT_MIC_OUT_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_out_eq_unit, b_e_r_mix_tmp, b_e_r_mix_tmp, n, 2);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_OUT_GAIN_CONTROL_EN
		#if 0//CFG_USB_MODE == AUDIO_GAME_HEADSET//电竞耳机上用到
		{
			gCtrlVars.mic_out_gain_control_unit.enable = 1;
	        gCtrlVars.mic_out_gain_control_unit.mute = 0;
	        gCtrlVars.mic_out_gain_control_unit.gain   = DigVolTab_64[gCtrlVars.UsbMicToSpeakerVolume];  		
	        if(gCtrlVars.UsbMicToSpeakerMute) gCtrlVars.mic_out_gain_control_unit.gain = 0;
		}
		#endif
		AudioEffectPregainApply(&gCtrlVars.mic_out_gain_control_unit, b_e_r_mix_tmp, b_e_r_mix_tmp, n, 2);
		#endif		
		
		#ifdef CFG_FUNC_RECORDER_EN
		if(local_rec_out)
		{
			if(GetWhetherRecMusic() && music_pcm)
			{
				for(s = 0; s < n; s++)
				{
					local_rec_out[2*s + 0] = __nds32__clips((((int32_t)music_pcm[2*s + 0] + (int32_t)b_e_r_mix_tmp[2*s + 0])), 16-1);
					local_rec_out[2*s + 1] = __nds32__clips((((int32_t)music_pcm[2*s + 1] + (int32_t)b_e_r_mix_tmp[2*s + 1])), 16-1);
				}
			}
			else
			{
				memcpy(local_rec_out,b_e_r_mix_tmp,n*2*2);
			}
		}
		#endif

		AudioCoreAppSourceVolSet(MIC_SOURCE_NUM, b_e_r_mix_tmp, n, 2);

		#if defined(CFG_FUNC_REMIND_SOUND_EN)
		if(remind_in)
		{

			//提示音音效处理,若需要linein模式下和提示音同时输出，需要屏蔽掉此部分
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)b_e_r_mix_tmp[2*s + 0] + (int32_t)remind_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)b_e_r_mix_tmp[2*s + 1] + (int32_t)remind_in[2*s + 1])), 16-1);
			}
		}
		else
        #endif
    	{
			if(music_pcm)
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2*s + 0] = __nds32__clips((((int32_t)music_pcm[2*s + 0] + (int32_t)b_e_r_mix_tmp[2*s + 0])), 16-1);
					monitor_out[2*s + 1] = __nds32__clips((((int32_t)music_pcm[2*s + 1] + (int32_t)b_e_r_mix_tmp[2*s + 1])), 16-1);
				}
			}
			else
			{
				memcpy(monitor_out,b_e_r_mix_tmp,n*2*2);
			}
    	}
		
	//#if defined(CFG_FUNC_REMIND_SOUND_EN)//提示音音效处理，若需要linein模式下和提示音同时输出，需要恢复此部分
		//if(remind_in)
		//{
		//	for(s = 0; s < n; s++)
		//	{
		//		monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)remind_in[2*s + 0])), 16-1);
		//		monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)remind_in[2*s + 1])), 16-1);
		//	}
		//}
	//#endif

	#ifdef CFG_FUNC_GUITAR_EN
		if(guitar_pcm)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)guitar_pcm[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)guitar_pcm[2*s + 1])), 16-1);
			}
		}
	#endif

	#if defined (CFG_FUNC_LINE_MIX_MODE) && (CFG_FUNC_LINE_MIX_EN == 0)
		if(line_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)line_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)line_in[2*s + 1])), 16-1);
			}
		}
	#endif

	#if defined (CFG_RES_AUDIO_I2S0IN_EN) && (CFG_FUNC_I2S0_MIX_EN == 0)
		#ifndef CFG_RES_MICIN_BY_I2SIN
		if(i2s0_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)i2s0_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)i2s0_in[2*s + 1])), 16-1);
			}
		}
		#endif
	#endif

	#if defined (CFG_RES_I2S1_EN) && (CFG_FUNC_I2S1_MIX_EN == 0)
		if(i2s1_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)i2s1_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)i2s1_in[2*s + 1])), 16-1);
			}
		}
	#endif

	#if defined (CFG_FUNC_SPDIF_MIX_MODE) && (CFG_FUNC_SPDIF_MIX_EN == 0)
		if(spdif_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)spdif_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)spdif_in[2*s + 1])), 16-1);
			}
		}
	#endif

	#if defined (CFG_FUNC_USB_MIX_MODE) && defined (CFG_RES_AUDIO_USB_IN_EN) && (CFG_FUNC_USB_MIX_EN == 0)
		if(usb_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)usb_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)usb_in[2*s + 1])), 16-1);
			}
		}
	#endif

	#if defined (CFG_FUNC_REMIND_MIX_MODE) && (CFG_FUNC_REMIND_MIX_EN == 0) && !defined(BT_TWS_SUPPORT)
		if(mix_remind_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)mix_remind_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)mix_remind_in[2*s + 1])), 16-1);
			}
		}
	#endif


	#ifdef CFG_RES_AUDIO_I2S0OUT_EN
		if(i2s0_out){
			memcpy(i2s0_out,monitor_out,n*2*2);
		}
	#endif

	#ifdef CFG_RES_AUDIO_I2S1OUT_EN
		if(i2s1_out){
			memcpy(i2s1_out,monitor_out,n*2*2);
		}
	#endif

	#ifdef  CFG_RES_AUDIO_I2SOUT_EN
		if(i2s_out){
			memcpy(i2s_out,monitor_out,n*2*2);
		}
	#endif

	#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
		if(spdif_out){
			memcpy(spdif_out,monitor_out,n*2*2);
		}
	#endif
	}

	
	#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if(record_out)
	{
		#if CFG_AUDIO_EFFECT_REC_BYPASS_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_bypass_gain_control_unit, rec_bypass_tmp, rec_bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_EFFECT_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_effect_gain_control_unit, rec_effect_tmp, rec_effect_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

		#if CFG_AUDIO_EFFECT_REC_AUX_GAIN_CONTROL_EN
		if(music_pcm) AudioEffectPregainApply(&gCtrlVars.rec_aux_gain_control_unit, rec_music_tmp, rec_music_tmp, n, gCtrlVars.adc_line_channel_num);
		#endif

		#ifdef CFG_FUNC_REMIND_SOUND_EN
		if(remind_in && (gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND)) AudioEffectPregainApply(&gCtrlVars.rec_remind_gain_control_unit, rec_remind_tmp, rec_remind_tmp, n, gCtrlVars.adc_line_channel_num);
		#endif
		
		if(music_pcm)
		{
			for(s = 0; s < n; s++)
			{
				record_out[s] = __nds32__clips((((int32_t)rec_effect_tmp[2*s+0]/2 + (int32_t)rec_bypass_tmp[2*s+0]/2 + (int32_t)rec_music_tmp[2*s+0]/2           
									    + (int32_t)rec_effect_tmp[2*s+1]/2 + (int32_t)rec_bypass_tmp[2*s+1]/2 + (int32_t)rec_music_tmp[2*s+1]/2)), 16-1);
			}
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				record_out[s] = __nds32__clips((((int32_t)rec_effect_tmp[2*s+0]/2 + (int32_t)rec_bypass_tmp[2*s+0]/2 + 0           
						                + (int32_t)rec_effect_tmp[2*s+1]/2 + (int32_t)rec_bypass_tmp[2*s+1]/2 + 0)), 16-1);
			}
		}

		#ifdef CFG_FUNC_REMIND_SOUND_EN
		if(remind_in && (gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND))
		{
			for(s = 0; s < n; s++)
			{
				pcm = __nds32__clips( ((int32_t)rec_remind_tmp[2*s+0]/2 + (int32_t)rec_remind_tmp[2*s+1]/2),16-1); 
				pcm = __nds32__clips( ((int32_t)pcm + (int32_t)record_out[s]),16-1); 
				record_out[s] = pcm;
			}
		}
		#endif	 
		
        #ifdef CFG_FUNC_GUITAR_EN
        if(guitar_pcm)
		{
			for(s = 0; s < n; s++)
			{
				pcm = __nds32__clips( ((int32_t)guitar_pcm[2*s+0] + (int32_t)guitar_pcm[2*s+1]),16-1);
				pcm = __nds32__clips( ((int32_t)pcm + (int32_t)record_out[s]),16-1);
				record_out[s] = pcm;
			}
		}
        #endif				
		
		#if CFG_AUDIO_EFFECT_REC_EQ_EN
		AudioEffectEQApply(&gCtrlVars.rec_eq_unit, record_out, record_out, n, 1);
		#endif

		#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
		AudioEffectEqDrcApply(&gCtrlVars.rec_eq_drc_unit, record_out, record_out, n);
		#endif

		#if CFG_AUDIO_EFFECT_REC_DRC_EN
		AudioEffectDRCApply(&gCtrlVars.rec_drc_unit, record_out, record_out, n);
		#endif

        #if CFG_AUDIO_EFFECT_PHASE_EN
		AudioEffectPhaseApply(&gCtrlVars.phase_control_unit, record_out, record_out, n, 1);
		#endif

		#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
		AudioEffectPhaseShifterApply(&gCtrlVars.rec_phase_shifter_unit, record_out, record_out, n);
		#endif
	}
	#endif

	#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(usb_out)
	{
	    if(!record_out)
		{
			#if CFG_AUDIO_EFFECT_REC_BYPASS_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.rec_bypass_gain_control_unit, rec_bypass_tmp, rec_bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
			#endif
			
			#if CFG_AUDIO_EFFECT_REC_EFFECT_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.rec_effect_gain_control_unit, rec_effect_tmp, rec_effect_tmp, n, gCtrlVars.adc_mic_channel_num);
			#endif

			#if 0//CFG_AUDIO_EFFECT_REC_AUX_GAIN_CONTROL_EN
			if(music_pcm) AudioEffectPregainApply(&gCtrlVars.rec_aux_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif

			#if CFG_AUDIO_EFFECT_REC_REMIND_GAIN_CONTROL_EN
            #ifdef CFG_FUNC_REMIND_SOUND_EN
			if(remind_in && (gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND)) AudioEffectPregainApply(&gCtrlVars.rec_remind_gain_control_unit, rec_remind_tmp, rec_remind_tmp, n, gCtrlVars.adc_line_channel_num);
            #else
		    #ifdef CFG_FUNC_REMIND_MIX_MODE
			if(mix_remind_in && (gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND)) AudioEffectPregainApply(&gCtrlVars.rec_remind_gain_control_unit, rec_remind_tmp, rec_remind_tmp, n, gCtrlVars.adc_line_channel_num);
            #endif
			#endif
			#endif

    	}
		
		if(music_pcm)
		{
			for(s = 0; s < n; s++)
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)music_pcm[2*s + 0] + (int32_t)rec_effect_tmp[2*s + 0]  + (int32_t)rec_bypass_tmp[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)music_pcm[2*s + 1] + (int32_t)rec_effect_tmp[2*s + 1]  + (int32_t)rec_bypass_tmp[2*s + 1])), 16-1);
			}
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)rec_effect_tmp[2*s + 0]  + (int32_t)rec_bypass_tmp[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)rec_effect_tmp[2*s + 1]  + (int32_t)rec_bypass_tmp[2*s + 1])), 16-1);
			}
		}

		#ifdef CFG_FUNC_REMIND_SOUND_EN
		if(remind_in && (gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND))
		{
			for(s = 0; s < n; s++)
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)usb_out[2*s + 0]  + (int32_t)rec_remind_tmp[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)usb_out[2*s + 1]  + (int32_t)rec_remind_tmp[2*s + 1])), 16-1);
			}
		}
		#else
		#ifdef CFG_FUNC_REMIND_MIX_MODE
		if(mix_remind_in && (gCtrlVars.remind_type == REMIND_TYPE_BACKGROUND))
		{
			for(s = 0; s < n; s++)
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)usb_out[2*s + 0]  + (int32_t)mix_remind_in[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)usb_out[2*s + 1]  + (int32_t)mix_remind_in[2*s + 1])), 16-1);
			}
		}
		#endif	
		#endif
				
		#ifdef CFG_FUNC_USB_MIX_MODE
		#ifdef CFG_RES_AUDIO_USB_IN_EN
		if(usb_in && gCtrlVars.usb_audio_upload_flag)
		{
			for(s = 0; s < n; s++)
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)usb_out[2*s + 0] + (int32_t)usb_in[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)usb_out[2*s + 1] + (int32_t)usb_in[2*s + 1])), 16-1);
			}
		}
		#endif
		#endif///end of defined(CFG_FUNC_USB_MIX_MODE)///
				
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		#if CFG_SUPPORT_USB_VOLUME_SET
        gCtrlVars.rec_usb_out_gain_control_unit.enable = 1;
        gCtrlVars.rec_usb_out_gain_control_unit.mute = 0;
        gCtrlVars.rec_usb_out_gain_control_unit.gain   = DigVolTab_64[gCtrlVars.UsbMicVolume];  		
        if(gCtrlVars.UsbMicMute) gCtrlVars.rec_usb_out_gain_control_unit.gain = 0;
        AudioEffectPregainApply(&gCtrlVars.rec_usb_out_gain_control_unit, usb_out, usb_out, n, 2);			
		#endif
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_out_gain_control_unit, usb_out, usb_out, n, 2);
		#endif
	}
	#endif
	#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
    #endif

}
#else
void AudioMusicProcess(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	uint16_t n = mainAppCt.SamplesPreFrame;
	int16_t *music_pcm		= NULL;//pBuf->music_in;///music input	
#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_RES_AUDIO_USB_IN_EN
	int16_t *usb_in 		= NULL;//pBuf->usb_in;
#endif
#endif

#ifdef  CFG_FUNC_I2S_MIX_MODE	
	int16_t *i2s0_in		= NULL;//pBuf->i2s0_in;
	int16_t *i2s1_in		= NULL;//pBuf->i2s1_in;
#endif

	int16_t *remind_in      = NULL;//pBuf->remind_in;
	int16_t *monitor_out    = NULL;//pBuf->dac0_out; 
	int16_t *record_out     = NULL;//pBuf->dacx_out; 
	int16_t *i2s0_out       = NULL;//pBuf->i2s0_out; 
	int16_t *i2s1_out       = NULL;//pBuf->i2s1_out; 
	int16_t *usb_out        = NULL;//pBuf->usb_out; 
	int16_t *rec_music_tmp  = (int16_t *)pcm_buf_1;
#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	int16_t *spdif_out		= NULL;
#endif	
#ifdef CFG_FUNC_RECORDER_EN
	int16_t *local_rec_out  = NULL;//pBuf->rec_out;

	if(GetSystemMode() == AppModeCardPlayBack
			|| GetSystemMode() == AppModeUDiskPlayBack
			|| GetSystemMode() == AppModeFlashFsPlayBack
			|| GetSystemMode() == AppModeFlashPlayBack)
	{
		if(pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].Enable == TRUE)
		{
			music_pcm = pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf;// include usb/sd source 
		}
	}
	else
#endif
	{
		#if ((CFG_RES_MIC_SELECT == 0) && defined(BT_TWS_SUPPORT))
		if((pAudioCore->AudioSource[MIC_SOURCE_NUM].Enable == TRUE) && (gCtrlVars.line_num == ANA_INPUT_CH_LINEIN3))////mic buff
		{				
			music_pcm = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//linein3 buffer
		}	
		else
		#endif
		{
			if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff
			{
				music_pcm = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;// include line/bt/usb/sd/spdif/hdmi/i2s/radio source
			}
		}
	}	
	
#if defined(CFG_FUNC_REMIND_SOUND_EN)	
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
	if (remind_in)
	{
		music_pcm = NULL;
	}
#endif

    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		monitor_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
	}	
#ifdef CFG_RES_AUDIO_DACX_EN 	
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
	}	
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN 	
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
#if (CFG_RES_I2S_PORT==1)
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
#else
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
#endif
	}
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2S0IN_EN
	if(pAudioCore->AudioSource[I2S0_SOURCE_NUM].Enable == TRUE) ////dacx buff
	{
		i2s0_in = pAudioCore->AudioSource[I2S0_SOURCE_NUM].PcmInBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	if(pAudioCore->AudioSource[I2S1_SOURCE_NUM].Enable == TRUE) ////dacx buff
	{
		i2s1_in = pAudioCore->AudioSource[I2S1_SOURCE_NUM].PcmInBuf;
	}
#endif

#ifdef CFG_FUNC_RECORDER_EN
	if(pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].Enable == TRUE)
	{
		local_rec_out = pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_RES_AUDIO_USB_IN_EN
	if(pAudioCore->AudioSource[USB_MIX_SOURCE_NUM].Enable == TRUE)
	{
		usb_in = pAudioCore->AudioSource[USB_MIX_SOURCE_NUM].PcmInBuf;
	}
#endif
#endif

#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].Enable == TRUE)
	{
		usb_out = pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	if(pAudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].Enable == TRUE)   
	{
		spdif_out = pAudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
    if((music_pcm == NULL)&&(mainAppCt.EqModeBak != mainAppCt.EqMode))
	{		
		mainAppCt.EqModeBak = mainAppCt.EqMode;
		EqModeSet(mainAppCt.EqMode);
	}
#endif

#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
#endif

    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

    if(record_out)
    {
		memset(record_out, 0, n * 2);
    }
	
    if(usb_out)
    {
		memset(usb_out, 0, n * 2 * 2);//mono*2 stereo*4
    }
	
    if(i2s0_out)
    {
		memset(i2s0_out, 0, n * 2 * 2);//mono*2 stereo*4
    }

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN	
	if(spdif_out)
	{
		memset(spdif_out, 0, n * 2 * 2);
	}
#endif
    EffectPcmBufClear(mainAppCt.SamplesPreFrame);

    #if defined(CFG_FUNC_REMIND_SOUND_EN)//提示音音效处理,若需要linein模式下和提示音同时输出，需要屏蔽掉此部分
	if(remind_in)
	{
		AudioEffectPregainApply(&gCtrlVars.remind_key_gain_control_unit, remind_in, remind_in, n, 2);
		AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
	}
	#endif

	//伴奏信号音效处理
	if(music_pcm)
	{
		if(gCtrlVars.adc_line_channel_num == 1)
		{
			for(s = 0; s < n; s++)
			{
				music_pcm[s] = __nds32__clips((((int32_t)music_pcm[2 * s + 0] + (int32_t)music_pcm[2 * s + 1]) ), 16-1);
			}
		}
		
#if (defined(BT_TWS_SUPPORT) && (CFG_EFFECT_MUSIC_MASTER == 0))
		TWS_AudioEffectProcess(&music_pcm,&remind_in);//don't move
#endif

		if(GetSystemMode() == AppModeLineAudioPlay)
		{
			#if CFG_AUDIO_EFFECT_MUSIC_NOISE_SUPPRESSOR_EN
			AudioEffectExpanderApply(&gCtrlVars.music_expander_unit, music_pcm, music_pcm, n);
			#endif
		}
		
		//如果想要在音效处理前获取能量值则打开此处，屏蔽掉下面的函数调用
		#if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN
		//AudioEffectSilenceDectorApply(&gCtrlVars.MusicAudioSdct_unit,  music_pcm,  n);
		#endif

#ifdef BT_TWS_SUPPORT
		if((CFG_EFFECT_MUSIC_MASTER == 0) && (rec_music_tmp))
#endif
		{
			memcpy(rec_music_tmp,music_pcm,n*2*2);
		}
		if((GetSystemMode() == AppModeOpticalAudioPlay) || (GetSystemMode() == AppModeCoaxialAudioPlay) || (GetSystemMode() == AppModeHdmiAudioPlay))
		{
			#if CFG_AUDIO_EFFECT_SPDIF_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.spdif_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		else if(GetSystemMode() == AppModeI2SInAudioPlay)
		{
			#if CFG_AUDIO_EFFECT_I2S_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.i2s_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		else if((GetSystemMode() == AppModeCardAudioPlay) || (GetSystemMode() == AppModeUDiskAudioPlay) || (GetSystemMode() == AppModeUsbDevicePlay))
		{
			#if CFG_AUDIO_EFFECT_USB_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.usb_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		else if(GetSystemMode() == AppModeBtAudioPlay)
		{
			#if CFG_AUDIO_EFFECT_BT_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.bt_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		#if CFG_AUDIO_EFFECT_AUX_GAIN_CONTROL_EN
		{
			#ifdef CFG_FUNC_SHUNNING_EN
			uint32_t gain;
			gain = gCtrlVars.aux_gain_control_unit.gain;
			gCtrlVars.aux_gain_control_unit.gain = gCtrlVars.aux_out_dyn_gain;//闪避功能打开时用到
			#endif
			AudioEffectPregainApply(&gCtrlVars.aux_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#ifdef CFG_FUNC_SHUNNING_EN
			gCtrlVars.aux_gain_control_unit.gain = gain;
			#endif
		}		
		#endif

		#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
		AudioEffectLowLevelCompressorApply(&gCtrlVars.mic_low_level_compressor_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_COMPANDER_EN
		AudioEffectCompanderApply(&gCtrlVars.compander_unit,music_pcm,music_pcm,n);
		#endif
			
		#if CFG_AUDIO_EFFECT_MUSIC_PRE_EQ_EN		
		AudioEffectEQApply(&gCtrlVars.music_pre_eq_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN 
		AudioEffectPitchShifterProApply(&gCtrlVars.pitch_shifter_pro_unit, music_pcm, music_pcm, n);
		#endif	
		
		#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
        AudioEffectVocalCutApply(&gCtrlVars.vocal_cut_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
        AudioEffectVocalRemoveApply(&gCtrlVars.vocal_remove_unit,  music_pcm, music_pcm, n);
        #endif			
	
		#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
        AudioEffectVBApply(&gCtrlVars.music_vb_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
        AudioEffectVBClassicApply(&gCtrlVars.music_vb_classic_unit, music_pcm, music_pcm, n);
		#endif

        #if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
        AudioEffectStereoWidenerApply(&gCtrlVars.stereo_winden_unit, music_pcm, music_pcm, n);
        #endif

		#if CFG_AUDIO_EFFECT_MUSIC_EXCITER_EN
		AudioEffectExciterApply(&gCtrlVars.music_exciter_unit, music_pcm, music_pcm, n);
		#endif

        #if CFG_AUDIO_EFFECT_MUSIC_3D_EN
        AudioEffectThreeDApply(&gCtrlVars.music_threed_unit, music_pcm, music_pcm, n);
        #endif	

		#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
        AudioEffectThreeDPlusApply(&gCtrlVars.music_threed_plus_unit, music_pcm, music_pcm, n);
        #endif

		#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
		AudioEffectPcmDelayApply(&gCtrlVars.music_delay_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
		AudioEffectEqDrcApply(&gCtrlVars.music_eq_drc_unit, music_pcm, music_pcm, n);
		#endif
			
		#if CFG_AUDIO_EFFECT_MUSIC_DRC_EN
		AudioEffectDRCApply(&gCtrlVars.music_drc_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_OUT_EQ_EN		
		AudioEffectEQApply(&gCtrlVars.music_out_eq_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
		#endif

		AudioCoreAppSourceVolSet(APP_SOURCE_NUM, music_pcm, n, gCtrlVars.adc_line_channel_num);

		if(gCtrlVars.adc_line_channel_num == 1)
		{
			memcpy(&music_pcm[n], music_pcm, n*2);
			for(s = 0; s < n; s++)
			{
				music_pcm[2*s + 0] = music_pcm[2*s + 1] = music_pcm[n+s];
			}
		}
		
#if (defined(BT_TWS_SUPPORT) && (CFG_EFFECT_MUSIC_MASTER == 1))
		TWS_AudioEffectProcess(&music_pcm,&remind_in);//don't move
#endif
		//如果想要在音效处理前获取能量值则屏蔽此处
		#if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN
		AudioEffectSilenceDectorApply(&gCtrlVars.MusicAudioSdct_unit,  music_pcm,  n);			
		#endif

		if((rec_music_tmp)
#ifdef BT_TWS_SUPPORT
		   &&(CFG_EFFECT_MUSIC_MASTER == 1)
#endif
		   )
		{
			memcpy(rec_music_tmp,music_pcm,n*2*2);
		}
	}
#ifdef BT_TWS_SUPPORT
	else
	{
	   TWS_RemindAudioProcess(&remind_in);//don't move
	}
#endif

	//DAC0立体声监听音效处理
	if(monitor_out)
	{	
		#ifdef CFG_FUNC_RECORDER_EN
		if(local_rec_out && music_pcm)
		{
			memcpy(local_rec_out,music_pcm,n*2*2);
		}
		#endif

		#if defined(CFG_FUNC_REMIND_SOUND_EN)
		if(remind_in)
		{
			AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
			//提示音音效处理,若需要linein模式下和提示音同时输出，需要屏蔽掉此部分
			memcpy(monitor_out,remind_in,n*2*2);
		}
		else
		#endif
		{
			if(music_pcm)
			{
				memcpy(monitor_out,music_pcm,n*2*2);
			}
			else
			{
				memset(monitor_out, 0, n*2*2);
			}
		}

		//#if defined(CFG_FUNC_REMIND_SOUND_EN)//提示音音效处理,若需要linein模式下和提示音同时输出，需要恢复此部分
		//if(remind_in)
		//{
		//	for(s = 0; s < n; s++)
		//	{
		//		monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)remind_in[2*s + 0])), 16-1);
		//		monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)remind_in[2*s + 1])), 16-1);
		//	}
		//}
        //#endif

#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_RES_AUDIO_USB_IN_EN
		if(usb_in)
		{		
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)usb_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)usb_in[2*s + 1])), 16-1);
			}
		}
#endif
#endif///end of defined(CFG_FUNC_USB_MIX_MODE)///


#ifdef CFG_RES_AUDIO_I2S0IN_EN
		if(i2s0_in)
		{			
			
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)i2s0_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)i2s0_in[2*s + 1])), 16-1);
			}
		}
#endif
#ifdef CFG_RES_I2S1_EN
		if(i2s1_in)
		{		
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)i2s1_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)i2s1_in[2*s + 1])), 16-1);
			}
		}
#endif
	}
		
#ifdef CFG_RES_AUDIO_I2S0OUT_EN
		if(i2s0_out)
		{
		for(s = 0; s < n; s++)
		{
			i2s0_out[2*s + 0] = monitor_out[2*s + 0];
			i2s0_out[2*s + 1] = monitor_out[2*s + 1];
		}
		}
#endif
#ifdef CFG_RES_AUDIO_I2S1OUT_EN
		if(i2s1_out)
		{
		for(s = 0; s < n; s++)
		{
			i2s1_out[2*s + 0] = monitor_out[2*s + 0];
			i2s1_out[2*s + 1] = monitor_out[2*s + 1];
		}
	}	
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	if(spdif_out)
	{
		for(s = 0; s < n; s++)
		{
			spdif_out[2*s + 0] = monitor_out[2*s + 0];
			spdif_out[2*s + 1] = monitor_out[2*s + 1];
		}	
	}
#endif		
	#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if(record_out)
	{		
		if(remind_in)
		{
			for(s = 0; s < n; s++)
			{
				record_out[s] = __nds32__clips((((int32_t)remind_in[2*s+0]/2 + (int32_t)remind_in[2*s+1]/2)), 16-1);
			}
		}
		else if(music_pcm)
		{
			for(s = 0; s < n; s++)
			{
				record_out[s] = __nds32__clips((((int32_t)rec_music_tmp[2*s+0]/2 + (int32_t)rec_music_tmp[2*s+1]/2)), 16-1);
			}
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				record_out[s] = 0;
			}
		}
		
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_out_gain_control_unit, record_out, record_out, n, 1);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_OUT_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_out_eq_unit, record_out, record_out, n, 1);
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_VIRTUAL_BASS_EN
		AudioEffectVBApply(&gCtrlVars.rec_vb_unit, record_out, record_out, n);
		#endif

		#if CFG_AUDIO_EFFECT_EQ_DRC_STRUCT_EN
		AudioEffectEqDrcApply(&gCtrlVars.rec_eq_drc_unit, record_out, record_out, n);
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_DRC_EN
		AudioEffectDRCApply(&gCtrlVars.rec_drc_unit, record_out, record_out, n);
		#endif
		
        #if CFG_AUDIO_EFFECT_REC_EQ_EN	
		AudioEffectEQApply(&gCtrlVars.rec_eq_unit, record_out, record_out, n, 1);
		#endif	
		
		#if CFG_AUDIO_EFFECT_PHASE_EN
		AudioEffectPhaseApply(&gCtrlVars.phase_control_unit, record_out, record_out, n, 1);
		#endif

		#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
		AudioEffectPhaseShifterApply(&gCtrlVars.rec_phase_shifter_unit, record_out, record_out, n);
		#endif
	}
	#endif

	#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(usb_out)
	{
		if(monitor_out)
			memcpy(usb_out,monitor_out,n*2*2);
		else
			memset(usb_out, 0, n*2*2);
#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_RES_AUDIO_USB_IN_EN
		if(usb_in && gCtrlVars.usb_audio_upload_flag)
		{
			for(s = 0; s < n; s++)
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)usb_out[2*s + 0] + (int32_t)usb_in[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)usb_out[2*s + 1] + (int32_t)usb_in[2*s + 1])), 16-1);
			}
		}
#endif
#endif///end of defined(CFG_FUNC_USB_MIX_MODE)///
				
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		#if CFG_SUPPORT_USB_VOLUME_SET
        gCtrlVars.rec_usb_out_gain_control_unit.enable = 1;
        gCtrlVars.rec_usb_out_gain_control_unit.mute = 0;
        gCtrlVars.rec_usb_out_gain_control_unit.gain   = DigVolTab_64[gCtrlVars.UsbMicVolume];  		
        if(gCtrlVars.UsbMicMute) gCtrlVars.rec_usb_out_gain_control_unit.gain = 0;
        AudioEffectPregainApply(&gCtrlVars.rec_usb_out_gain_control_unit, usb_out, usb_out, n, 2);			
		#endif
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_out_gain_control_unit, usb_out, usb_out, n, 2);
		#endif
	}
	#endif
	#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
    #endif
}
#endif

#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT == ENABLE))
/*
****************************************************************
* HFP+Mic音效处理主函数
* 只用于蓝牙通话下K歌录音功能
****************************************************************
*/
#ifdef BT_RECORD_FUNC_ENABLE
void AudioEffectProcessBTRecord(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	int16_t  pcm;
	uint16_t n = mainAppCt.SamplesPreFrame;
	int16_t *mic_pcm		= NULL;//pBuf->mic_in;///mic input	
	int16_t *bypass_tmp 	= NULL;//pBuf->mic_in;
	int16_t *music_pcm		= NULL;//pBuf->music_in;///music input
	int16_t *remind_in		= NULL;//pBuf->remind_in;
	int16_t *monitor_out	= NULL;//pBuf->dac0_out; 
	int16_t *record_out 	= NULL;//pBuf->dacx_out; 
	int16_t *i2s0_out		= NULL;//pBuf->i2s0_out; 
	int16_t *i2s1_out		= NULL;//pBuf->i2s1_out; 
	int16_t *usb_out		= NULL;//pBuf->usb_out; 
	int16_t *local_rec_out	= NULL;//pBuf->rec_out; 
	int16_t *hf_pcm_out     = NULL;//pBuf->hf_pcm_out;//蓝牙通话上传buffer
	
	int16_t *echo_tmp		= (int16_t *)pcm_buf_1;
	int16_t *reverb_tmp 	= (int16_t *)pcm_buf_2;
	int16_t *b_e_r_mix_tmp	= (int16_t *)pcm_buf_2;
	int16_t *rec_bypass_tmp = (int16_t *)pcm_buf_3;
	int16_t *rec_effect_tmp = (int16_t *)pcm_buf_4;
	int16_t *rec_remind_tmp = (int16_t *)pcm_buf_1;

	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Enable == TRUE)////mic buff
	{
		bypass_tmp = mic_pcm = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//双mic输入
	}

	if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff
	{
		music_pcm = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;// include line/bt/usb/sd/spdif/hdmi/i2s/radio source
	}
	
	#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
	#endif

	if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		monitor_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
		
		//hfp send 
		hf_pcm_out = pAudioCore->AudioSink[AUDIO_HF_SCO_SINK_NUM].PcmOutBuf;
	}

	#ifdef CFG_RES_AUDIO_DACX_EN 	
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
	}	
	#endif

	#ifdef CFG_FUNC_RECORDER_EN
	if(pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].Enable == TRUE)
	{
		local_rec_out = pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].PcmOutBuf;
	}
	#endif

	#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
	#endif

	if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
	}

	if(record_out)
	{
		memset(record_out, 0, n * 2);
	}
	
	if(usb_out)
	{
		memset(usb_out, 0, n * 2 * 2);//mono*2 stereo*4
	}
	
	if(i2s0_out)
	{
		memset(i2s0_out, 0, n * 2 * 2);//mono*2 stereo*4
	}

	if(hf_pcm_out) //nomo
	{
		memset(hf_pcm_out, 0, n * 2);
    }
	
	EffectPcmBufClear(mainAppCt.SamplesPreFrame);
	
	//伴奏信号音效处理
	if(music_pcm)
	{
		//nomo -> stereo
		for(s = n; s > 0; s--)
		{
			music_pcm[2*(s-1)] = music_pcm[s-1];
			music_pcm[2*(s-1)+1] = music_pcm[s-1];
		}
		
		#if CFG_AUDIO_EFFECT_MUSIC_NOISE_SUPPRESSOR_EN
		AudioEffectExpanderApply(&gCtrlVars.music_expander_unit, music_pcm, music_pcm, n);
		#endif
		
		#if CFG_AUDIO_EFFECT_MUSIC_SILENCE_DECTOR_EN
		AudioEffectSilenceDectorApply(&gCtrlVars.MusicAudioSdct_unit,  music_pcm,  n);
		#endif

		if((GetSystemMode() == AppModeOpticalAudioPlay) || (GetSystemMode() == AppModeCoaxialAudioPlay) || (GetSystemMode() == AppModeHdmiAudioPlay))
		{
			#if CFG_AUDIO_EFFECT_SPDIF_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.spdif_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		else if(GetSystemMode() == AppModeI2SInAudioPlay)
		{
			#if CFG_AUDIO_EFFECT_I2S_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.i2s_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		else if((GetSystemMode() == AppModeCardAudioPlay) || (GetSystemMode() == AppModeUDiskAudioPlay) || (GetSystemMode() == AppModeUsbDevicePlay))
		{
			#if CFG_AUDIO_EFFECT_USB_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.usb_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		else if(GetSystemMode() == AppModeBtAudioPlay)
		{
			#if CFG_AUDIO_EFFECT_BT_IN_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.bt_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		
		#if CFG_AUDIO_EFFECT_AUX_GAIN_CONTROL_EN
		{
			#ifdef CFG_FUNC_SHUNNING_EN
			uint32_t gain;
			gain = gCtrlVars.aux_gain_control_unit.gain;
			gCtrlVars.aux_gain_control_unit.gain = gCtrlVars.aux_out_dyn_gain;//闪避功能打开时用到
			#endif
			AudioEffectPregainApply(&gCtrlVars.aux_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#ifdef CFG_FUNC_SHUNNING_EN
			gCtrlVars.aux_gain_control_unit.gain = gain;
			#endif
		}		
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_PITCH_SHIFTER_PRO_EN 
		AudioEffectPitchShifterProApply(&gCtrlVars.pitch_shifter_pro_unit, music_pcm, music_pcm, n);
		#endif	
		
		#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
		AudioEffectVocalCutApply(&gCtrlVars.vocal_cut_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
		AudioEffectVocalRemoveApply(&gCtrlVars.vocal_remove_unit,  music_pcm, music_pcm, n);
    	#endif			
	
		#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
		AudioEffectVBApply(&gCtrlVars.music_vb_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_CLASSIC_EN
		AudioEffectVBClassicApply(&gCtrlVars.music_vb_classic_unit, music_pcm, music_pcm, n);
		#endif
		
    	#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
        AudioEffectStereoWidenerApply(&gCtrlVars.stereo_winden_unit, music_pcm, music_pcm, n);
     	#endif

    	#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
		AudioEffectThreeDApply(&gCtrlVars.music_threed_unit, music_pcm, music_pcm, n);
    	#endif	

		#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
		AudioEffectThreeDPlusApply(&gCtrlVars.music_threed_plus_unit, music_pcm, music_pcm, n);
    	#endif			

		#if CFG_AUDIO_EFFECT_MUSIC_DELAY_EN
		AudioEffectPcmDelayApply(&gCtrlVars.music_delay_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_EXCITER_EN
		AudioEffectExciterApply(&gCtrlVars.music_exciter_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_PRE_EQ_EN
		AudioEffectEQApply(&gCtrlVars.music_pre_eq_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_DRC_EN
		AudioEffectDRCApply(&gCtrlVars.music_drc_unit, music_pcm, music_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MUSIC_OUT_EQ_EN
		AudioEffectEQApply(&gCtrlVars.music_out_eq_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
		#endif

		AudioCoreAppSourceVolSet(APP_SOURCE_NUM, music_pcm, n, 2);
		
		if(gCtrlVars.adc_line_channel_num == 1)
		{
			memcpy(&music_pcm[n], music_pcm, n*2);
			for(s = 0; s < n; s++)
			{
				music_pcm[2*s + 0] = music_pcm[2*s + 1] = music_pcm[n+s];
			}
		}
	}
    #ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
    else
	{		
		if(mainAppCt.EqModeBak != mainAppCt.EqMode)
		{
			EqModeSet(mainAppCt.EqMode);
			mainAppCt.EqModeBak = mainAppCt.EqMode;
		}
	}
	#endif
	
	//MIC信号音效处理
	if(mic_pcm)
	{	
    	#ifdef CFG_FUNC_DETECT_MIC_EN
		if(gCtrlVars.MicOnlin && (!gCtrlVars.Mic2Onlin))
		{
			#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
			if(gCtrlVars.MicSegment == 3)
			{
				for(s = 0; s < n; s++)
				{
					mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0] = 0; 
				}
			}
			else
			#endif
			{
				for(s = 0; s < n; s++)
				{
					mic_pcm[s*2 + 0] = mic_pcm[s*2 + 1]; 
				}
			}
		}
		else if((!gCtrlVars.MicOnlin) && gCtrlVars.Mic2Onlin)
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0]; 
			}
		}
		else if(gCtrlVars.MicOnlin && gCtrlVars.Mic2Onlin)
		{
			#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
			if(gCtrlVars.MicSegment == 3)
			{
				for(s = 0; s < n; s++)
				{
					mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0]; 
				}
			}
			else
			#endif
			{
				for(s = 0; s < n; s++)
				{
					pcm = __nds32__clips((((int32_t)mic_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 1]) ), 16-1); 
					mic_pcm[2 * s + 0] = pcm; 
					mic_pcm[2 * s + 1] = pcm; 
				}
			}
		}
		#else
		if( (gCtrlVars.line3_l_mic1_en) && (gCtrlVars.line3_r_mic2_en) )
		{
			for(s = 0; s < n; s++)
			{
				pcm = __nds32__clips((((int32_t)mic_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 1]) ), 16-1); 
				mic_pcm[2 * s + 0] = pcm; 
				mic_pcm[2 * s + 1] = pcm; 
			}
		}
		else if(gCtrlVars.line3_l_mic1_en )
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s*2 + 1] = mic_pcm[s*2 + 0]; 
			}
		}
		else if( gCtrlVars.line3_r_mic2_en )
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s*2 + 0] = mic_pcm[s*2 + 1]; 
			}
		}
		#endif
		else
		{
			memset(mic_pcm, 0, n*2*2);
		}
		if(gCtrlVars.adc_mic_channel_num == 1)
		{
			for(s = 0; s < n; s++)
			{
				pcm = __nds32__clips((((int32_t)mic_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 1]) ), 16-1);
				mic_pcm[s] = pcm;
			}
		}
		
		//pre eq
		#if CFG_AUDIO_EFFECT_MIC_PRE_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_pre_eq_unit, mic_pcm, mic_pcm, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
		#if CFG_AUDIO_EFFECT_MIC_NOISE_SUPPRESSOR_EN
		AudioEffectExpanderApply(&gCtrlVars.mic_expander_unit, mic_pcm, mic_pcm, n);
		#endif
		
    	#if CFG_AUDIO_EFFECT_MIC_SILENCE_DECTOR_EN
		AudioEffectSilenceDectorApply(&gCtrlVars.MicAudioSdct_unit,  mic_pcm,  n);
		#endif
		
		#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN || CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN || CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
		//freq shifter，howling只支持单声道数据	
		 //********************* stereo 2 mono ****************************  
		if((gCtrlVars.adc_mic_channel_num == 2) && (gCtrlVars.freq_shifter_unit.enable 
			|| gCtrlVars.howling_dector_unit.enable
			#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
			|| gCtrlVars.voice_changer_pro_unit.enable
			#endif
			)
			)
		{
			for(s = 0; s < n; s++)
			{
				mic_pcm[s] = mic_pcm[2 * s + 0];
			}
		}
		#endif
				
    	#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN
		AudioEffectFreqShifterApply(&gCtrlVars.freq_shifter_unit, mic_pcm, mic_pcm, n);
		#endif
		
		#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_FINE_EN
		AudioEffectFreqShifterFineApply(&gCtrlVars.freq_shifter_fine_unit, mic_pcm, mic_pcm, n);
		#endif

    	#if CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN
		AudioEffectHowlingSuppressorApply(&gCtrlVars.howling_dector_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN 
		AudioEffectVoiceChangerProApply(&gCtrlVars.voice_changer_pro_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_FREQ_SHIFTER_EN || CFG_AUDIO_EFFECT_MIC_HOWLING_DECTOR_EN || CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
		 //********************* mono 2 stereo ****************************
		if((gCtrlVars.adc_mic_channel_num == 2) && (
				gCtrlVars.freq_shifter_unit.enable 
			|| gCtrlVars.howling_dector_unit.enable
			#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_PRO_EN
			|| gCtrlVars.voice_changer_pro_unit.enable
			#endif
			))
		{	
			for(s = 0; s < n; s++)
			{
				rec_bypass_tmp[s] = mic_pcm[s];
			}
			for(s = 0; s < n; s++)
			{
				pcm   =rec_bypass_tmp[s];//
				mic_pcm[2 * s + 0] = pcm; 
				mic_pcm[2 * s + 1] = pcm; 
			}		
		}
		#endif

		#if CFG_AUDIO_EFFECT_MIC_VOICE_CHANGER_EN 
		AudioEffectVoiceChangerApply(&gCtrlVars.voice_changer_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN 
		AudioEffectPitchShifterApply(&gCtrlVars.pitch_shifter_unit, mic_pcm, mic_pcm, n, gCtrlVars.adc_mic_channel_num);
		#endif					
		
		#if CFG_AUDIO_EFFECT_MIC_AUTO_TUNE_EN
		AudioEffectAutoTuneApply(&gCtrlVars.auto_tune_unit, mic_pcm, mic_pcm, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_ECHO_EN
		AudioEffectEchoApply(&gCtrlVars.echo_unit, mic_pcm, echo_tmp, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_REVERB_EN
		AudioEffectReverbApply(&gCtrlVars.reverb_unit, mic_pcm, reverb_tmp, n);
		#endif

    	#if CFG_AUDIO_EFFECT_MIC_PLATE_REVERB_EN
		AudioEffectPlateReverbApply(&gCtrlVars.plate_reverb_unit, mic_pcm, reverb_tmp, n);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_REVERB_PRO_EN
		AudioEffectReverbProApply(&gCtrlVars.reverb_pro_unit, mic_pcm, reverb_tmp, n);
		#endif
		
		//bypass eq
		#if CFG_AUDIO_EFFECT_MIC_BYPASS_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_bypass_eq_unit, bypass_tmp, bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

		//get bypass data for record 
		memcpy(rec_bypass_tmp, bypass_tmp, n*2*2);
		
		//echo eq
		#if CFG_AUDIO_EFFECT_MIC_ECHO_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_echo_eq_unit, echo_tmp, echo_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif		
		
		//reverb eq
		#if CFG_AUDIO_EFFECT_MIC_REVERB_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_reverb_eq_unit, reverb_tmp, reverb_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

		//get echo+reverb data for record 
		for(s = 0; s < n; s++)
		{
			int32_t Data_L = (int32_t)echo_tmp[2*s+0] + (int32_t)reverb_tmp[2*s+0];
			int32_t Data_R = (int32_t)echo_tmp[2*s+1] + (int32_t)reverb_tmp[2*s+1];

			rec_effect_tmp[2 * s + 0] = __nds32__clips((Data_L >> 0), 16-1);
			rec_effect_tmp[2 * s + 1] = __nds32__clips((Data_R >> 0), 16-1);
		}
		
		//bypass pregain
		#if CFG_AUDIO_EFFECT_MIC_BYPASS_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.mic_bypass_gain_control_unit, bypass_tmp, bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
		//echo pregain
		#if CFG_AUDIO_EFFECT_MIC_ECHO_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.mic_echo_control_unit, echo_tmp, echo_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

		//reverb pregain
		#if CFG_AUDIO_EFFECT_MIC_REVERB_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.mic_reverb_gain_control_unit, reverb_tmp, reverb_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
		//mux
		for(s = 0; s < n; s++)
		{
			if(gCtrlVars.adc_mic_channel_num == 2)
			{
				int32_t Data_L = (int32_t)bypass_tmp[2*s+0] + (int32_t)echo_tmp[2*s+0] + (int32_t)reverb_tmp[2*s+0];
				int32_t Data_R = (int32_t)bypass_tmp[2*s+1] + (int32_t)echo_tmp[2*s+1] + (int32_t)reverb_tmp[2*s+1];

				b_e_r_mix_tmp[2 * s + 0] = __nds32__clips((Data_L >> 0), 16-1);
				b_e_r_mix_tmp[2 * s + 1] = __nds32__clips((Data_R >> 0), 16-1);
			}
			else
			{
				int32_t Data_M = (int32_t)bypass_tmp[s] + (int32_t)echo_tmp[s] + (int32_t)reverb_tmp[s];
				b_e_r_mix_tmp[s] = __nds32__clips((Data_M >> 0), 16-1);
			}
		}

		if(gCtrlVars.adc_mic_channel_num == 1)
		{
			memcpy(&b_e_r_mix_tmp[n], b_e_r_mix_tmp, n*2);
			memcpy(&bypass_tmp[n], bypass_tmp, n*2);
			for(s = 0; s < n; s++)
			{
				b_e_r_mix_tmp[2*s + 0] = b_e_r_mix_tmp[2*s + 1] = b_e_r_mix_tmp[n+s];
				bypass_tmp[2*s + 0]    = bypass_tmp[2*s + 1]	= bypass_tmp[n+s];
			}
		}
		
		#if CFG_AUDIO_EFFECT_MIC_DRC_EN
		AudioEffectDRCApply(&gCtrlVars.mic_drc_unit, b_e_r_mix_tmp, b_e_r_mix_tmp, n);
		#endif
	}	

	if(hf_pcm_out)
	{
		for(s=0;s<n;s++)
		{
			hf_pcm_out[s] = __nds32__clips((((int32_t)b_e_r_mix_tmp[2 * s + 0] + (int32_t)b_e_r_mix_tmp[2 * s + 1])), 16-1);
		}
	}
	
	//DAC0立体声监听音效处理
	if(monitor_out)
	{			
		#if CFG_AUDIO_EFFECT_MIC_OUT_EQ_EN
		AudioEffectEQApply(&gCtrlVars.mic_out_eq_unit, b_e_r_mix_tmp, b_e_r_mix_tmp, n, 2);
		#endif

		#if CFG_AUDIO_EFFECT_MIC_OUT_GAIN_CONTROL_EN
		#if 0//CFG_USB_MODE == AUDIO_GAME_HEADSET//电竞耳机上用到
		{
			gCtrlVars.mic_out_gain_control_unit.enable = 1;
			gCtrlVars.mic_out_gain_control_unit.mute = 0;
			gCtrlVars.mic_out_gain_control_unit.gain   = DigVolTab_64[gCtrlVars.UsbMicToSpeakerVolume]; 		
			if(gCtrlVars.UsbMicToSpeakerMute) gCtrlVars.mic_out_gain_control_unit.gain = 0;
		}
		#endif
		AudioEffectPregainApply(&gCtrlVars.mic_out_gain_control_unit, b_e_r_mix_tmp, b_e_r_mix_tmp, n, 2);
		#endif

		#ifdef CFG_FUNC_RECORDER_EN
		if(local_rec_out)
		{
			if(GetWhetherRecMusic() && music_pcm)
			{
				for(s = 0; s < n; s++)
				{
					local_rec_out[2*s + 0] = __nds32__clips((((int32_t)music_pcm[2*s + 0] + (int32_t)b_e_r_mix_tmp[2*s + 0])), 16-1);
					local_rec_out[2*s + 1] = __nds32__clips((((int32_t)music_pcm[2*s + 1] + (int32_t)b_e_r_mix_tmp[2*s + 1])), 16-1);
				}
			}
			else
			{
				memcpy(local_rec_out, b_e_r_mix_tmp, n*2*2);
			}
		}
		#endif

		AudioCoreAppSourceVolSet(MIC_SOURCE_NUM, b_e_r_mix_tmp, n, 2);

		#if defined(CFG_FUNC_REMIND_SOUND_EN)//提示音音效处理,若需要linein模式下和提示音同时输出，需要屏蔽掉此部分
		if(remind_in)
		{
			AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)b_e_r_mix_tmp[2*s + 0] + (int32_t)remind_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)b_e_r_mix_tmp[2*s + 1] + (int32_t)remind_in[2*s + 1])), 16-1);
			}
		}
		else
    	#endif
		{
			if(music_pcm)
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2*s + 0] = __nds32__clips((((int32_t)music_pcm[2*s + 0] + (int32_t)b_e_r_mix_tmp[2*s + 0])), 16-1);
					monitor_out[2*s + 1] = __nds32__clips((((int32_t)music_pcm[2*s + 1] + (int32_t)b_e_r_mix_tmp[2*s + 1])), 16-1);
				}
			}
			else
			{
				memcpy(monitor_out, b_e_r_mix_tmp, n*2*2);
			}
		}
		
    	//#if defined(CFG_FUNC_REMIND_SOUND_EN)//提示音音效处理,若需要linein模式下和提示音同时输出，需要恢复此部分
		//if(remind_in)
		//{
		//	for(s = 0; s < n; s++)
		//	{
		//		monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)remind_in[2*s + 0])), 16-1);
		//		monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)remind_in[2*s + 1])), 16-1);
		//	}
		//}
    	//#endif

		if(i2s0_out)
		{
			memcpy(i2s0_out, monitor_out, n*2*2);
		}
		
		if(i2s1_out)
		{
			memcpy(i2s1_out, monitor_out, n*2*2);
		}	
	}
		
	#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if(record_out)
	{
		#if CFG_AUDIO_EFFECT_REC_BYPASS_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_bypass_gain_control_unit, rec_bypass_tmp, rec_bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_EFFECT_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_effect_gain_control_unit, rec_effect_tmp, rec_effect_tmp, n, gCtrlVars.adc_mic_channel_num);
		#endif

		#if CFG_AUDIO_EFFECT_REC_AUX_GAIN_CONTROL_EN
		if(music_pcm) AudioEffectPregainApply(&gCtrlVars.rec_aux_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
		#endif
		
		if(music_pcm)
		{
			for(s = 0; s < n; s++)
			{
				record_out[s] = __nds32__clips((((int32_t)rec_effect_tmp[2*s+0] + (int32_t)rec_bypass_tmp[2*s+0] + (int32_t)music_pcm[2*s+0]		   
										+ (int32_t)rec_effect_tmp[2*s+1] + (int32_t)rec_bypass_tmp[2*s+1] + (int32_t)music_pcm[2*s+1])), 16-1);
			}
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				record_out[s] = __nds32__clips((((int32_t)rec_effect_tmp[2*s+0] + (int32_t)rec_bypass_tmp[2*s+0] + 0		   
										+ (int32_t)rec_effect_tmp[2*s+1] + (int32_t)rec_bypass_tmp[2*s+1] + 0)), 16-1);
			}
		}
				
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_out_gain_control_unit, record_out, record_out, n, 1);
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_EQ_EN
		AudioEffectEQApply(&gCtrlVars.rec_eq_unit, record_out, record_out, n, 1);
		#endif

		#if CFG_AUDIO_EFFECT_REC_DRC_EN
		AudioEffectDRCApply(&gCtrlVars.rec_drc_unit, record_out, record_out, n);
		#endif

    	#if CFG_AUDIO_EFFECT_PHASE_EN
		AudioEffectPhaseApply(&gCtrlVars.phase_control_unit, record_out, record_out, n, 1);
		#endif

		#if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
		AudioEffectPhaseShifterApply(&gCtrlVars.rec_phase_shifter_unit, record_out, record_out, n);
		#endif
	}
	#endif

	#if CFG_USB_OUT_EN
	#if CFG_USB_OUT_STEREO_EN
	if(usb_out)
	{
		if(!record_out)
		{
			#if CFG_AUDIO_EFFECT_REC_BYPASS_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.rec_bypass_gain_control_unit, rec_bypass_tmp, rec_bypass_tmp, n, gCtrlVars.adc_mic_channel_num);
			#endif
			
			#if CFG_AUDIO_EFFECT_REC_EFFECT_GAIN_CONTROL_EN
			AudioEffectPregainApply(&gCtrlVars.rec_effect_gain_control_unit, rec_effect_tmp, rec_effect_tmp, n, gCtrlVars.adc_mic_channel_num);
			#endif

			#if CFG_AUDIO_EFFECT_REC_AUX_GAIN_CONTROL_EN
			if(music_pcm) AudioEffectPregainApply(&gCtrlVars.rec_aux_gain_control_unit, music_pcm, music_pcm, n, gCtrlVars.adc_line_channel_num);
			#endif
		}
		for(s = 0; s < n; s++)
		{
			if(music_pcm)
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)music_pcm[2*s + 0] + (int32_t)rec_effect_tmp[2*s + 0]	+ (int32_t)rec_bypass_tmp[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)music_pcm[2*s + 1] + (int32_t)rec_effect_tmp[2*s + 1]	+ (int32_t)rec_bypass_tmp[2*s + 1])), 16-1);
			}
			else
			{
				usb_out[2*s + 0] = __nds32__clips((((int32_t)rec_effect_tmp[2*s + 0]  + (int32_t)rec_bypass_tmp[2*s + 0])), 16-1);
				usb_out[2*s + 1] = __nds32__clips((((int32_t)rec_effect_tmp[2*s + 1]  + (int32_t)rec_bypass_tmp[2*s + 1])), 16-1);
			}
		}
				
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		#if CFG_SUPPORT_USB_VOLUME_SET
		gCtrlVars.rec_usb_out_gain_control_unit.enable = 1;
		gCtrlVars.rec_usb_out_gain_control_unit.mute = 0;
		gCtrlVars.rec_usb_out_gain_control_unit.gain   = DigVolTab_64[gCtrlVars.UsbMicVolume];			
		if(gCtrlVars.UsbMicMute) gCtrlVars.rec_usb_out_gain_control_unit.gain = 0;
		AudioEffectPregainApply(&gCtrlVars.rec_usb_out_gain_control_unit, usb_out, usb_out, n, 2);			
		#endif
		#endif
		
		#if CFG_AUDIO_EFFECT_REC_OUT_GAIN_CONTROL_EN
		AudioEffectPregainApply(&gCtrlVars.rec_out_gain_control_unit, usb_out, usb_out, n, 2);
		#endif
	}
	#endif
	#endif
	#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
	#endif
}
#endif

/*
****************************************************************
* HFP+Mic音效处理主函数
* 用于正常蓝牙通话
****************************************************************
*/
void btHfRingDataSet(void* PcmIn, uint32_t Len);
int16_t * btHfRingDataGet(uint32_t Len);
void AudioEffectProcessBTHF(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	uint16_t n = mainAppCt.SamplesPreFrame;

	int16_t *remind_in      = NULL;//pBuf->remind_in;
	int16_t *i2s0_in        = NULL;//pBuf->i2s0_out;
	int16_t *i2s1_in        = NULL;//pBuf->i2s1_out;
	int16_t *monitor_out    = NULL;//pBuf->dac0_out;
	int16_t *record_out     = NULL;//pBuf->dacx_out;
	int16_t *i2s0_out       = NULL;//pBuf->i2s0_out;
	int16_t *i2s1_out       = NULL;//pBuf->i2s1_out;
	int16_t *usb_out        = NULL;//pBuf->usb_out;
	int16_t  *hf_mic_in     = NULL;//pBuf->hf_mic_in;//蓝牙通话mic采样buffer
	int16_t  *hf_pcm_in     = NULL;//pBuf->hf_pcm_in;//蓝牙通话下传buffer
	int16_t  *hf_aec_in		= NULL;//pBuf->hf_aec_in;;//蓝牙通话下传delay buffer,专供aec算法处理
	int16_t  *hf_pcm_out    = NULL;//pBuf->hf_pcm_out;//蓝牙通话上传buffer
	int16_t  *hf_dac_out    = NULL;//pBuf->hf_dac_out;//蓝牙通话DAC的buffer
	int16_t  *hf_rec_out    = NULL;//pBuf->hf_rec_out;//蓝牙通话送录音的buffer
	int16_t  *u_pcm_tmp     = (int16_t *)pcm_buf_3;
	int16_t  *d_pcm_tmp     = (int16_t *)pcm_buf_4;


	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Enable == TRUE)////mic buff
	{
		hf_mic_in = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;
	}

#ifdef CFG_RES_AUDIO_I2S0IN_EN
	if(pAudioCore->AudioSource[I2S0_SOURCE_NUM].Enable == TRUE)////mic buff
	{
		i2s0_in = pAudioCore->AudioSource[I2S0_SOURCE_NUM].PcmInBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2S1IN_EN
	if(pAudioCore->AudioSource[I2S1_SOURCE_NUM].Enable == TRUE)////mic buff
	{
		i2s1_in = pAudioCore->AudioSource[I2S1_SOURCE_NUM].PcmInBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2S0OUT_SINK_NUM].PcmOutBuf;
	}
#endif
#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2S1OUT_SINK_NUM].PcmOutBuf;
	}
#endif
	if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff
	{
		//hf sco: nomo
		hf_pcm_in = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;

		//aec process:push the new data, pop the old data
		btHfRingDataSet(hf_pcm_in, n);
		hf_aec_in = btHfRingDataGet(n);
	}

#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif

#if	!defined(CFG_FUNC_REMIND_SOUND_EN) && defined(CFG_FUNC_REMIND_MIX_MODE)
	if(pAudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif
    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		//hf mode
		hf_dac_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
		hf_pcm_out = pAudioCore->AudioSink[AUDIO_HF_SCO_SINK_NUM].PcmOutBuf;
	}

#ifdef CFG_RES_AUDIO_DACX_EN
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
	#if (BT_HFP_SUPPORT == ENABLE)
		if(GetSystemMode() == AppModeBtHfPlay)
		{
			//record_out = NULL;
		}
		///else
	#endif
		{
			record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
		}
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
#if (CFG_RES_I2S_PORT==1)
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
#else
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
#endif
	}
#endif

    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

    if(record_out)
    {
		memset(record_out, 0, n * 2);
    }

    if(usb_out)
    {
		memset(usb_out, 0, n * 2 * 2);//mono*2 stereo*4
    }

    if(i2s0_out)
    {
		memset(i2s0_out, 0, n * 2 * 2);//mono*2 stereo*4
	}

	if(hf_pcm_out)
	{
		memset(hf_pcm_out, 0, n * 2);
	}

	if(hf_dac_out)
	{
		memset(hf_dac_out, 0, n * 2 * 2);
	}

	if(hf_rec_out)
	{
		memset(hf_rec_out, 0, n * 2);
	}
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
#endif
	if(hf_mic_in && hf_pcm_in && hf_pcm_out && hf_dac_out)
	{
		AudioCoreAppSourceVolSet(APP_SOURCE_NUM, hf_pcm_in, n, 1);
		//phone gain
		gCtrlVars.aec_phone_in_gain_unit.channel	 = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmFormat;
		AudioEffectPregainApply(&gCtrlVars.aec_phone_in_gain_unit, hf_pcm_in, hf_pcm_in, n, 1);

		#ifdef CFG_FUNC_REMIND_SOUND_EN//提示音音效处理
		if(remind_in)
		{
			AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
			
			for(s = 0; s < n; s++)
			{
				hf_dac_out[2*s + 0] = __nds32__clips((((int32_t)hf_pcm_in[s] + (int32_t)remind_in[2*s + 0])), 16-1);
				hf_dac_out[2*s + 1] = __nds32__clips((((int32_t)hf_pcm_in[s] + (int32_t)remind_in[2*s + 1])), 16-1);
			}
		}
		else
		#endif
		{
			for(s = 0; s < n; s++)
			{
				hf_dac_out[s*2 + 0] = hf_pcm_in[s];
				hf_dac_out[s*2 + 1] = hf_pcm_in[s];
			}
		}

        //本地MIC采样做降噪处理
		AudioEffectExpanderApply(&gCtrlVars.expander_for_aec_unit, hf_mic_in, hf_mic_in, n);

		AudioCoreAppSourceVolSet(MIC_SOURCE_NUM, hf_mic_in, n, 2);
		//mic gain
		AudioEffectPregainApply(&gCtrlVars.aec_mic_in_gain_unit, hf_mic_in, hf_mic_in, n, 2);

		//aec eq
		AudioEffectEQApply(&gCtrlVars.eq_for_aec_unit, hf_mic_in, hf_mic_in, n, 2);

		//本地MIC采样及手机端通话音频做AEC处理
		//only mic input : stereo -> mono
	    for(s = 0; s < n; s++)
		{
			d_pcm_tmp[s] = __nds32__clips((((int32_t)hf_mic_in[2 * s + 0] + (int32_t)hf_mic_in[2 * s + 1])), 16-1);
			u_pcm_tmp[s] = hf_aec_in[s];
		}		
        #if CFG_AUDIO_EFFECT_HFP_NS_EN
	    AudioEffectBlueNSApply(&gCtrlVars.hfp_ns_unit,d_pcm_tmp,d_pcm_tmp);
        #endif
		
	    #if CFG_AUDIO_EFFECT_MIC_AEC_EN
		if(gCtrlVars.mic_aec_unit.ct != NULL)
		{
			AudioEffectAecApply(&gCtrlVars.mic_aec_unit, u_pcm_tmp , d_pcm_tmp, hf_pcm_out, n);
		}
		else
		{
			for(s = 0; s < n; s++)
			{
			    hf_pcm_out[s] = d_pcm_tmp[s];
			}
		}
	    #else
		for(s = 0; s < n; s++)
		{
		    hf_pcm_out[s] = d_pcm_tmp[s];
		}
	    #endif

		//mic out gain
		AudioEffectPregainApply(&gCtrlVars.aec_gain_control_unit, hf_pcm_out, hf_pcm_out, n, gCtrlVars.adc_mic_channel_num);

		// pitch shifter
		#if ((CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN)&&defined(BT_HFP_MIC_PITCH_SHIFTER_FUNC))
		AudioEffectPitchShifterApply(&gCtrlVars.pitch_shifter_unit, hf_pcm_out, hf_pcm_out, n, 1);//nomo
		#endif

		//aec drc
		AudioEffectDRCApply(&gCtrlVars.drc_for_aec_unit, hf_pcm_out, hf_pcm_out, n);
		
	}
	else if(hf_dac_out)
	{
		#ifdef CFG_FUNC_REMIND_SOUND_EN//提示音音效处理
		if(remind_in)
		{
			AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
			memcpy(hf_dac_out, remind_in, n*2*2);
		}
		#endif
	}
	
#if	!defined(CFG_FUNC_REMIND_SOUND_EN) && defined(CFG_FUNC_REMIND_MIX_MODE)
    if(remind_in&&hf_dac_out)
    {
	    AudioCoreAppSourceVolSet(MIX_REMIND_SOURCE_NUM, remind_in, n, 2);

	    for(s = 0; s < n; s++)
	    {
		   hf_dac_out[2*s + 0] = remind_in[2*s + 0];
		   hf_dac_out[2*s + 1] = remind_in[2*s + 1];
	    }
    }
#endif
	//DAC0立体声监听音效处理
	if(hf_dac_out)
	{
		if(i2s0_out)
		{
			memcpy(i2s0_out, hf_dac_out, n*2*2);
		}
		if(i2s1_out)
		{
			memcpy(i2s1_out, hf_dac_out, n*2*2);
		}
	}

	#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if(record_out&&hf_dac_out)
	{
	    for(s = 0; s < n; s++)
		{
	    	record_out[s] = __nds32__clips((((int32_t)hf_dac_out[2 * s + 0]/2 + (int32_t)hf_dac_out[2 * s + 1]/2)), 16-1);
		}
	}
	#endif
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
#endif
}
#endif

#ifdef CFG_APP_USB_PHONE_MODE_EN
void UsbPhoneRingDataSet(void* PcmIn, uint32_t Len);
int16_t * UsbPhoneRingDataGet(uint32_t Len);

void AudioEffectProcessUsbPhone(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	uint16_t n = mainAppCt.SamplesPreFrame;

	int16_t *remind_in      = NULL;//pBuf->remind_in;
	int16_t *monitor_out    = NULL;//pBuf->dac0_out;
	int16_t *record_out     = NULL;//pBuf->dacx_out;
	int16_t *i2s0_out       = NULL;//pBuf->i2s0_out;
	int16_t *i2s1_out       = NULL;//pBuf->i2s1_out;
	int16_t *usb_out        = NULL;//pBuf->usb_out;
	int16_t  *mic_in     	= NULL;//pBuf->hf_mic_in;
	int16_t  *pcm_in     	= NULL;//pBuf->hf_pcm_in;
	int16_t  *aec_in		= NULL;//pBuf->hf_aec_in;;
	int16_t  *pcm_out    	= NULL;//pBuf->hf_pcm_out;
	int16_t  *dac_out    	= NULL;//pBuf->hf_dac_out;
	int16_t  *rec_out    	= NULL;//pBuf->hf_rec_out;
	int16_t  *pcm_tmp     	= (int16_t *)pcm_buf_1;//缓存USBIN
	int16_t  *u_pcm_tmp     = (int16_t *)pcm_buf_3;
	int16_t  *d_pcm_tmp     = (int16_t *)pcm_buf_4;

	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Enable == TRUE)////mic buff
	{
		mic_in = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;
	}

	if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff
	{
		pcm_in = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;

		for(s = 0; s < n; s++)
		{
			pcm_tmp[s] = __nds32__clips((((int32_t)pcm_in[2 * s + 0] + (int32_t)pcm_in[2 * s + 1])), 16-1);
		}
		//aec process:push the new data, pop the old data
		UsbPhoneRingDataSet(pcm_tmp, n);
		aec_in = UsbPhoneRingDataGet(n);
	}

#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_USB_OUT_EN
	if(pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].Enable == TRUE)
	{
		//usb_out = pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmOutBuf;
	}
#endif

    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)
	{
		dac_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
		pcm_out = pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmOutBuf;
	}

#ifdef CFG_RES_AUDIO_DACX_EN
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		if(GetSystemMode() == AppModeUsbPhone)
		{
			record_out = NULL;
		}
		else
		{
			record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
		}
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
		#if (CFG_RES_I2S_PORT==1)
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
		#else
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
		#endif
	}
#endif

    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

    if(record_out)
    {
		memset(record_out, 0, n * 2);
    }

    if(usb_out)
    {
		memset(usb_out, 0, n * 2 * 2);//mono*2 stereo*4
    }

    if(i2s0_out)
    {
		memset(i2s0_out, 0, n * 2 * 2);//mono*2 stereo*4
	}

	if(pcm_out)
	{
		memset(pcm_out, 0, n * 2);
	}

	if(dac_out)
	{
		memset(dac_out, 0, n * 2 * 2);
	}

	if(rec_out)
	{
		memset(rec_out, 0, n * 2);
	}
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
#endif
	AudioCoreAppSourceVolSet(APP_SOURCE_NUM, pcm_in, n, pAudioCore->AudioSource[APP_SOURCE_NUM].PcmFormat);
	//phone gain
	gCtrlVars.aec_phone_in_gain_unit.channel	 = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmFormat;
	AudioEffectPregainApply(&gCtrlVars.aec_phone_in_gain_unit, pcm_in, pcm_in, n, 2);

	if(mic_in && pcm_in && pcm_out && dac_out)
	{
		//AudioCoreAppSourceVolSet(APP_SOURCE_NUM, pcm_in, n, pAudioCore->AudioSource[APP_SOURCE_NUM].PcmFormat);

#ifdef CFG_FUNC_REMIND_SOUND_EN//提示音音效处理
		if(remind_in)
		{
			AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
			for(s = 0; s < n; s++)
			{
				dac_out[2*s + 0] = __nds32__clips((((int32_t)pcm_in[s] + (int32_t)remind_in[2*s + 0])), 16-1);
				dac_out[2*s + 1] = __nds32__clips((((int32_t)pcm_in[s] + (int32_t)remind_in[2*s + 1])), 16-1);
			}
		}
		else
#endif
		{
			for(s = 0; s < n; s++)
			{
				dac_out[s*2 + 0] = pcm_in[s*2];
				dac_out[s*2 + 1] = pcm_in[s*2 + 1];
			}
		}

        //本地MIC采样做降噪处理
		AudioEffectExpanderApply(&gCtrlVars.expander_for_aec_unit, mic_in, mic_in, n);
	
		AudioCoreAppSourceVolSet(MIC_SOURCE_NUM, mic_in, n, 2);
		
		//mic gain
		AudioEffectPregainApply(&gCtrlVars.aec_mic_in_gain_unit, mic_in, mic_in, n, 2);

		//aec eq
		AudioEffectEQApply(&gCtrlVars.eq_for_aec_unit, mic_in, mic_in, n, 2);

		//本地MIC采样及手机端通话音频做AEC处理
		//only mic input : stereo -> mono
	    for(s = 0; s < n; s++)
		{
			d_pcm_tmp[s] = __nds32__clips((((int32_t)mic_in[2 * s + 0] + (int32_t)mic_in[2 * s + 1])), 16-1);
			u_pcm_tmp[s] = aec_in[s];
		}		
		
		#if CFG_AUDIO_EFFECT_HFP_NS_EN
		AudioEffectBlueNSApply(&gCtrlVars.hfp_ns_unit,d_pcm_tmp,d_pcm_tmp);
		#endif

		if(gCtrlVars.mic_aec_unit.ct)
		{
			AudioEffectAecApply(&gCtrlVars.mic_aec_unit, u_pcm_tmp , d_pcm_tmp, pcm_out, n);
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				pcm_out[s] = d_pcm_tmp[s];
			}
		}

		//mic out gain
		AudioEffectPregainApply(&gCtrlVars.aec_gain_control_unit, pcm_out, pcm_out, n, gCtrlVars.adc_mic_channel_num);

		//AEC处理之后的数据做变调处理
		#if ((CFG_AUDIO_EFFECT_MIC_PITCH_SHIFTER_EN)&&defined(BT_HFP_MIC_PITCH_SHIFTER_FUNC))
		AudioEffectPitchShifterApply(&gCtrlVars.pitch_shifter_unit, pcm_out, pcm_out, n, 1);//nomo
		#endif

		//aec drc
		AudioEffectDRCApply(&gCtrlVars.drc_for_aec_unit, pcm_out, pcm_out, n);

		for(s = 0; s < n; s++)
		{
			pcm_out[n+s] = pcm_out[s];
		}
		for(s = 0; s < n; s++)
		{
			//最终输出
			pcm_out[2*s] = pcm_out[n+s];
			pcm_out[2*s + 1] = pcm_out[n+s];

		}
	}
	else if((dac_out)&&(remind_in))
	{
		#ifdef CFG_FUNC_REMIND_SOUND_EN//提示音音效处理
		if(remind_in)
		{
			AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
			memcpy(dac_out, remind_in, n*2*2);
		}
		#endif
	}
	else
	{
		if(pcm_in && dac_out)
		{
			memcpy(dac_out, pcm_in, n*2*2);
		}
		if(mic_in && pcm_out)
		{
			for(s = 0; s < n; s++)
			{
				pcm_out[s*2 + 0] = __nds32__clips((((int32_t)mic_in[2 * s + 0] + (int32_t)mic_in[2 * s + 1])), 16-1);
				pcm_out[s*2 + 1] = __nds32__clips((((int32_t)mic_in[2 * s + 0] + (int32_t)mic_in[2 * s + 1])), 16-1);
			}
		}
	}
	
	//DAC0立体声监听音效处理
	if(dac_out)
	{
		if(i2s0_out)
		{
			memcpy(i2s0_out, dac_out, n*2*2);
		}
		if(i2s1_out)
		{
			memcpy(i2s1_out, dac_out, n*2*2);
		}
	}

	#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if(record_out)
	{
		for(s = 0; s < n; s++)
		{
			record_out[s] = dac_out[s];
		}
	}
	#endif
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
#endif
}
#endif

#else
int32_t pcm_buf_3[512];
int32_t pcm_buf_4[512];

/*
****************************************************************
* 无音效处理主函数
*
*
****************************************************************
*/
void AudioBypassProcess(AudioCoreContext *pAudioCore)
{
	int16_t  s;
	uint16_t n = mainAppCt.SamplesPreFrame;
	int16_t *mic_pcm    	= NULL;//pBuf->mic_in;///mic input	
	int16_t *music_pcm    	= NULL;//pBuf->music_in;///music input
	int16_t *remind_in      = NULL;//pBuf->remind_in;
	int16_t *monitor_out    = NULL;//pBuf->dac0_out; 
	int16_t *record_out     = NULL;//pBuf->dacx_out; 
	int16_t *i2s0_out       = NULL;//pBuf->i2s0_out; 
	int16_t *i2s1_out       = NULL;//pBuf->i2s1_out; 
	int16_t *usb_out        = NULL;//pBuf->usb_out; 
	int16_t *local_rec_out  = NULL;//pBuf->rec_out; 
	#if (BT_HFP_SUPPORT == ENABLE)
	int16_t  *hf_mic_in     = NULL;//pBuf->hf_mic_in;//蓝牙通话mic采样buffer
	int16_t  *hf_pcm_in     = NULL;//pBuf->hf_pcm_in;//蓝牙通话下传buffer
	int16_t  *hf_aec_in		= NULL;//pBuf->hf_aec_in;;//蓝牙通话下传delay buffer,专供aec算法处理
	int16_t  *hf_pcm_out    = NULL;//pBuf->hf_pcm_out;//蓝牙通话上传buffer
	int16_t  *hf_dac_out    = NULL;//pBuf->hf_dac_out;//蓝牙通话DAC的buffer	
	int16_t  *hf_rec_out    = NULL;//pBuf->hf_rec_out;//蓝牙通话送录音的buffer	
	int16_t  *u_pcm_tmp     = (int16_t *)pcm_buf_3;
	int16_t  *d_pcm_tmp     = (int16_t *)pcm_buf_4;
	#endif

	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Enable == TRUE)////mic buff
	{
#if (BT_HFP_SUPPORT == ENABLE)
		if(GetSystemMode() == AppModeBtHfPlay)
		{
			//hf mode
			hf_mic_in = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;
		}
		else
#endif
		{
			mic_pcm = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//双mic输入
			AudioCoreAppSourceVolSet(MIC_SOURCE_NUM, mic_pcm, n, 2);
		}
	}

#ifdef CFG_FUNC_RECORDER_EN
	if(GetSystemMode() == AppModeCardPlayBack
			|| GetSystemMode() == AppModeUDiskPlayBack
			|| GetSystemMode() == AppModeFlashFsPlayBack
			|| GetSystemMode() == AppModeFlashPlayBack)
	{
		if(pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].Enable == TRUE)
		{
			music_pcm = pAudioCore->AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf;// include usb/sd source
			AudioCoreAppSourceVolSet(PLAYBACK_SOURCE_NUM, music_pcm, n, 2);
		}
	}
	else
#endif
	{
		if(pAudioCore->AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff
		{
#if (BT_HFP_SUPPORT == ENABLE) && defined(CFG_APP_BT_MODE_EN)
			if(GetSystemMode() == AppModeBtHfPlay)
			{
				//hf sco: nomo
				hf_pcm_in = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;

				//aec process:push the new data, pop the old data
				/*if(BtHf_AECRingDataSpaceLenGet() > CFG_BTHF_PARA_SAMPLES_PER_FRAME)
					BtHf_AECRingDataSet(hf_pcm_in, CFG_BTHF_PARA_SAMPLES_PER_FRAME);
				hf_aec_in = BtHf_AecInBuf();*/
			}
			else
#endif
			{
				music_pcm = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;// include line/bt/usb/sd/spdif/hdmi/i2s/radio source
				AudioCoreAppSourceVolSet(APP_SOURCE_NUM, music_pcm, n, 2);
			}
		}
	}	
	
#if defined(CFG_FUNC_REMIND_SOUND_EN)	
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
		AudioCoreAppSourceVolSet(REMIND_SOURCE_NUM, remind_in, n, 2);
	}	
#endif

    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
	#if defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT == ENABLE)
		if(GetSystemMode() == AppModeBtHfPlay)
		{
			//hf mode
			hf_dac_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
			hf_pcm_out = pAudioCore->AudioSink[AUDIO_HF_SCO_SINK_NUM].PcmOutBuf;
		}
		else
	#endif
		{
			monitor_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
		}
	}
	
#ifdef CFG_RES_AUDIO_DACX_EN 	
	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
	}	
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN 	
	if(pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Enable == TRUE)	////dacx buff
	{
#if (CFG_RES_I2S_PORT==1)
		i2s1_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
#else
		i2s0_out = pAudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf;
#endif
	}
#endif

#ifdef CFG_FUNC_RECORDER_EN
	if(pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].Enable == TRUE)
	{
		local_rec_out = pAudioCore->AudioSink[AUDIO_RECORDER_SINK_NUM].PcmOutBuf;
	}
#endif

    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

    if(record_out)
    {
		memset(record_out, 0, n * 2);
    }
	
    if(usb_out)
    {
		memset(usb_out, 0, n * 2 * 2);//mono*2 stereo*4
    }
	
    if(i2s0_out)
    {
		memset(i2s0_out, 0, n * 2 * 2);//mono*2 stereo*4
	}
	
#if (BT_HFP_SUPPORT == ENABLE)
	if(hf_pcm_out)
	{
		memset(hf_pcm_out, 0, n * 2);
	}
	
	if(hf_dac_out)
	{
		memset(hf_dac_out, 0, n * 2 * 2);
	}
	
	if(hf_rec_out)
	{
		memset(hf_rec_out, 0, n * 2);
	}

	if(hf_mic_in && hf_pcm_in && hf_pcm_out && hf_dac_out)
	{
		//hf_pcm_in: 16K nomo //256*2
		//hf_mic_in: 16K stereo (需要转成nomo)  //256*2*2
		//hf_pcm_out: 16K nomo //256*2
		//hf_dac_out: 16K stereo //256*2*2
		
		//手机端通话音频送DAC 16K nomo -> stereo
		for(s = 0; s < n; s++)
		{
			hf_dac_out[s*2 + 0] = hf_pcm_in[s];
			hf_dac_out[s*2 + 1] = hf_pcm_in[s]; 
		}

		//如需AEC,则需要开启CFG_APP_USB_AUDIO_MODE_EN
		for(s = 0; s < n; s++)
		{
			hf_pcm_out[s] = __nds32__clips((((int32_t)hf_mic_in[2*s + 0] + (int32_t)hf_mic_in[2*s + 1])), 16-1);
		}
	}
    //本地MIC采样及手机端通话音频做录音处理
	if(hf_rec_out)
	{
		for(s = 0; s < n; s++)
		{
			hf_rec_out[2*s + 0] = __nds32__clips((((int32_t)hf_mic_in[2*s + 0] + (int32_t)hf_pcm_in[2*s + 0])), 16-1);
			hf_rec_out[2*s + 1] = __nds32__clips((((int32_t)hf_mic_in[2*s + 1] + (int32_t)hf_pcm_in[2*s + 1])), 16-1);
		}
	}
#endif

	//DAC0立体声监听音效处理
	if(monitor_out)
	{		
		if(mic_pcm && music_pcm)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)mic_pcm[2*s + 0] + (int32_t)music_pcm[2*s + 0] )), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)mic_pcm[2*s + 1] + (int32_t)music_pcm[2*s + 1] )), 16-1);
			}
		}
		else if(mic_pcm)
		{				
			memcpy(monitor_out, mic_pcm, n*2*2);
		}
		else if(music_pcm)
		{				
			memcpy(monitor_out, music_pcm, n*2*2);
		}
		else
		{
			memset(monitor_out, 0, n*2*2);
		}
		
        #if defined(CFG_FUNC_REMIND_SOUND_EN)//提示音音效处理
		if(remind_in)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2*s + 0] = __nds32__clips((((int32_t)monitor_out[2*s + 0] + (int32_t)remind_in[2*s + 0])), 16-1);
				monitor_out[2*s + 1] = __nds32__clips((((int32_t)monitor_out[2*s + 1] + (int32_t)remind_in[2*s + 1])), 16-1);
			}
		}
        #endif
#ifdef BT_TWS_SUPPORT
		int16_t *tws_pcm = pAudioCore->AudioSource[TWS_SOURCE_NUM].PcmInBuf;
		memcpy(monitor_out, tws_pcm, n*2*2);
#endif
		if(i2s0_out)
		{
			memcpy(i2s0_out, monitor_out, n*2*2);
		}
		if(i2s1_out)
		{
			memcpy(i2s1_out, monitor_out, n*2*2);
		}	
	}


    #ifdef CFG_FUNC_RECORDER_EN
	if(local_rec_out)
	{
		memcpy(local_rec_out, monitor_out, n*2*2);
	}
	#endif
	
	#ifdef CFG_RES_AUDIO_DACX_EN
	//DAC X单声道录音音效处理
	if((record_out)&&(music_pcm))
	{		
		for(s = 0; s < n; s++)
		{
			record_out[s] = __nds32__clips((( (int32_t)music_pcm[2*s+0] + (int32_t)music_pcm[2*s+1])), 16-1);				    
		}
	}
	#endif
}
#endif

/*
****************************************************************
* 无App通路音频处理主函数
*
*
****************************************************************
*/
void AudioNoAppProcess(AudioCoreContext *pAudioCore)
{
	uint16_t n = mainAppCt.SamplesPreFrame;
	int16_t *mic_pcm    	= NULL;//pBuf->mic_in;///mic input

	int16_t *monitor_out    = NULL;//pBuf->dac0_out;
#ifdef BT_TWS_SUPPORT
	int16_t *remind_in = NULL;
#endif
	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Enable == TRUE)////mic buff
	{
		mic_pcm = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//双mic输入
	}

    if(pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)   ////dacx buff
	{

    	monitor_out = pAudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf;
	}

#ifdef CFG_RES_AUDIO_DACX_EN
//	if(pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable == TRUE)   ////dacx buff
//	{
//		record_out = pAudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf;
//	}
#endif

#ifdef BT_TWS_SUPPORT
	 TWS_RemindAudioProcess(&remind_in);//don't move
#endif

    if(monitor_out)
	{
		memset(monitor_out, 0, n * 2 * 2);
    }

	//DAC0立体声监听音效处理
	if(monitor_out)
	{
		if(mic_pcm)
		{
			memcpy(monitor_out, mic_pcm, n*2*2);
		}
		else
		{
			memset(monitor_out, 0, n*2*2);
		}
	}
}
