/**
 **************************************************************************************
 * @file    audio_core.c
 * @brief   audio core 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include <nds32_intrinsic.h>
#include "type.h"
#include "freertos.h"
#include "audio_core_api.h"
#include "app_config.h"
#include "debug.h"
#include "dma.h"
#include "ctrlvars.h"
#include "audio_effect.h"
#include "mode_switch_api.h"
#include "main_task.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "mcu_circular_buf.h"
#include "audio_adjust.h"
#include "audio_vol.h"
#ifdef CFG_APP_BT_MODE_EN
#include "bt_config.h"
#include "bt_play_api.h"
#include "bt_manager.h"
#ifdef BT_TWS_SUPPORT
#include "bt_tws_api.h"
#include "bt_tws_app_func.h"
extern uint8_t tws_link_status_get(void);//bt lib
#endif
#if (BT_HFP_SUPPORT == ENABLE)
#include "bt_hf_api.h"
#endif
#endif

typedef enum
{
	AC_RUN_CHECK,//用于检测是否需要暂停任务，如果需要暂停任务，则停留再该状态
	AC_RUN_GET,
	AC_RUN_PROC,
	AC_RUN_PUT,
}AudioCoreRunState;

static AudioCoreRunState AudioState = AC_RUN_CHECK;
AudioCoreContext		AudioCore;
uint32_t SinkMuteDelay = 0;

#ifdef BT_TWS_SUPPORT
#include "dac.h"
uint32_t adc_data_count = 0xFFFFFFFF;
uint32_t tws_play = 0xFFFFFFFF;
uint32_t tws_delay = 0;
TaskHandle_t audio_core_handle;
extern uint32_t tws_init_done;
uint32_t data_ready_tick;
uint32_t data_ready = 0;
uint32_t dac_mute_tick;
uint32_t dac_mute = 0;
extern uint32_t gSysTick;
extern uint32_t dac_sample_count;
extern uint32_t g_tws_need_init;
extern uint32_t tws_get_delay(void);
#endif
extern bool GIE_STATE_GET(void);

bool AudioCoreDataCheck(void)
{
	uint32_t Cnt = 0;
	uint32_t i;

	for(i = 0; i< AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(AudioCore.AudioSource[i].Enable == FALSE)
		{
			Cnt++;//增加一个计数器，用于统计是否所有source都被禁止
			continue;
		}
		if((AudioCore.AudioSource[i].IsSreamData == FALSE)/**/
		|| (AudioCore.AudioSource[i].FuncDataGetLen == NULL))
		{
			continue;
		}
		if(AudioCore.AudioSource[i].FuncDataGetLen() < mainAppCt.SamplesPreFrame)
		{
			return FALSE;
		}
	}

	if(Cnt == AUDIO_CORE_SOURCE_MAX_NUM)//没有一个通道是能数据，退出
	{
		return FALSE;
	}

	return TRUE;
}
/**
 * @brief       AudioCore 数据源 块获取
 * @note		只有所有使能的音频流buf数据满足1帧，才会DMA搬运数据帧
*/
static bool AudioCoreSourceDateGet(void)
{
	uint32_t i;
	uint32_t Cnt = 0;
#ifdef BT_TWS_SUPPORT
#ifdef CFG_FUNC_MIC_KARAOKE_EN
	uint32_t ret = 0;
#endif
#endif
	bool IsSourceDataEnough = TRUE; 

	//解决mic通道关闭情况一下，非流数据通道会有数据取不全导致杂音问题
	//sam,20200304
#if( CFG_RES_MIC_SELECT != 0)
	if(!AudioCore.AudioSource[MIC_SOURCE_NUM].Enable)
#endif
	{
		for(i = 0; i< AUDIO_CORE_SINK_MAX_NUM; i++)
		{
			if((AudioCore.AudioSink[i].Enable == TRUE)
			&& (AudioCore.AudioSink[i].FuncDataSpaceLenGet() < mainAppCt.SamplesPreFrame))
			{
				return FALSE;
			}
		}
	}


	for(i = 0; i< AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(AudioCore.AudioSource[i].Enable == FALSE)
		{
			Cnt++;//增加一个计数器，用于统计是否所有source都被禁止
			continue;
		}
		if((AudioCore.AudioSource[i].IsSreamData == FALSE)/**/
		|| (AudioCore.AudioSource[i].FuncDataGetLen == NULL))
		{
			continue;
		}
		if(AudioCore.AudioSource[i].FuncDataGetLen() < mainAppCt.SamplesPreFrame)
		{
			IsSourceDataEnough = FALSE; 
			break;
		}
	}
	if(Cnt == AUDIO_CORE_SOURCE_MAX_NUM)//没有一个通道是能数据，退出
	{
		return FALSE;
	}
	if(!IsSourceDataEnough)
	{
		IsSourceDataEnough = TRUE;
		return FALSE;
	}
#ifdef BT_TWS_SUPPORT
	if(AudioCore.AudioSource[MIC_SOURCE_NUM].FuncDataGetLen() >= mainAppCt.SamplesPreFrame*2-1)
	{
		APP_DBG("MIC PCM OVERFLW\n");
	}

	//TWS不在这里取数据
#endif
	for(i = 0; i< AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
#ifdef BT_TWS_SUPPORT
		if(i == TWS_SOURCE_NUM)
		{
			continue;
		}
#endif
		if(AudioCore.AudioSource[i].Enable == FALSE)
		{
			continue;
		}
		if(AudioCore.AudioSource[i].FuncDataGet == NULL)
		{
//			APP_DBG("NULL %ld\n",i);
			continue;
		}
		//打印用于监控
//		if((i == 1)
//		//&& (GetSystemMode() == AppModeBtAudioPlay)
//		&& (AudioCore.AudioSource[i].FuncDataGetLen() < mainAppCt.SamplesPreFrame))//Test
//		{
//			APP_DBG("E, %d, %d\n", DecodedPcmDataLenGet(), GetValidSbcDataSize());
//		}
		//长度必须是FRAME，不足填0
		memset(AudioCore.AudioSource[i].PcmInBuf, 0, mainAppCt.SamplesPreFrame * AudioCore.AudioSource[i].PcmFormat * 2);
#ifdef BT_TWS_SUPPORT
#ifdef CFG_FUNC_MIC_KARAOKE_EN
		if(i == MIC_SOURCE_NUM)
		{
			ret = GIE_STATE_GET();
			GIE_DISABLE();
		}
#endif
#endif
		AudioCore.AudioSource[i].FuncDataGet(AudioCore.AudioSource[i].PcmInBuf, mainAppCt.SamplesPreFrame);
#ifdef BT_TWS_SUPPORT
#ifdef CFG_FUNC_MIC_KARAOKE_EN
		if(i == MIC_SOURCE_NUM)
		{
			if(tws_play == 0x55)
			{
				dac_sample_count += mainAppCt.SamplesPreFrame;
			}
			if(ret)
				GIE_ENABLE();
		}
#endif
#endif
	}

#ifdef BT_TWS_SUPPORT
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
	if(AudioCore.AudioSource[TWS_SOURCE_NUM].Enable)
	{
#endif
	if(tws_play == 0x55)
	{
		uint32_t delay= tws_get_delay();
		uint32_t temp = (delay)/(mainAppCt.SamplesPreFrame/128);//FIFO是100帧中间水位为50  slave比master要慢6-8帧 因此设置为56
		adc_data_count++;
		if(adc_data_count >= temp)
		{
			AudioCore.AudioSource[TWS_SOURCE_NUM].FuncDataGet(AudioCore.AudioSource[TWS_SOURCE_NUM].PcmInBuf, mainAppCt.SamplesPreFrame);
#ifdef CFG_FUNC_MIC_KARAOKE_EN
			if(adc_data_count == (temp+5))
#else
			if(adc_data_count == (temp+15))
#endif
			{
				BT_DBG("need open dac\n");
				AudioPlayerSinkMuteRemind(0);
				//if(!((*(volatile unsigned long *)0x4002E018) & 0x3))	//如果在做softmute的过程中去unmute会使系统卡死
				{
					AudioDAC_DigitalMute(DAC0, FALSE, FALSE);
					BT_DBG("open dac done\n");
				}
			}
		}
		else
		{
			memset(AudioCore.AudioSource[TWS_SOURCE_NUM].PcmInBuf, 0, mainAppCt.SamplesPreFrame * AudioCore.AudioSource[TWS_SOURCE_NUM].PcmFormat * 2);
		}
	}
	else
	{
		memset(AudioCore.AudioSource[TWS_SOURCE_NUM].PcmInBuf, 0, mainAppCt.SamplesPreFrame * AudioCore.AudioSource[TWS_SOURCE_NUM].PcmFormat * 2);
	}
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
	}
#endif
#endif

#ifdef CFG_FUNC_FREQ_ADJUST
#ifdef BT_TWS_SUPPORT
#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER) || (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER) || (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM) || (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE))
#ifdef CFG_FUNC_SOFT_ADJUST_IN
	if(SoftFlagGet(SoftFlagBtSra))
	{
		AudioCoreSourceSRAStepCnt();
#ifdef CFG_FUNC_SOFT_ADJUST_OUT_USBAUDIO
		AudioCoreSinkSRAStepCnt();
#endif
	}
	else
#endif
#endif
	{
		AudioCoreSourceFreqAdjust();
	}
#else
#ifdef CFG_FUNC_SOFT_ADJUST_IN
	if(SoftFlagGet(SoftFlagBtSra))
	{
		AudioCoreSourceSRAStepCnt();
#ifdef CFG_FUNC_SOFT_ADJUST_OUT_USBAUDIO
		AudioCoreSinkSRAStepCnt();
#endif
	}
	else
#endif
	{
		AudioCoreSourceFreqAdjust();
	}
#endif
#endif
	return TRUE;
}

#ifdef BT_TWS_SUPPORT
extern int32_t tws_get_pcm_delay(void);
#endif
/**
 * @func        AudioCoreSinkDataSet
 * @brief       AudioCore 音效输出 推数据帧到 音频输出系统的buf
 * @param       None
 * @Output      DMA搬运数据帧到音频输出buf
 * @return      bool
 * @note		音效输出数据buf满足1帧时，dma搬运1帧数据
 * Record
*/
static bool AudioCoreSinkDataSet(void)
{
	uint32_t i;
#ifdef BT_TWS_SUPPORT
	uint32_t delay;
	uint32_t temp = 0;

	if(tws_play == 0x55)
	{
		delay= tws_get_delay();
		//printf("delay:%u\n",delay);
		temp = (delay)/(mainAppCt.SamplesPreFrame/128);//FIFO是100帧中间水位为50  slave比master要慢6-8帧 因此设置为56
		if(adc_data_count < temp)
		{
			return TRUE;
		}
	}
#endif
	for(i = 0; i< AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		if((AudioCore.AudioSink[i].Enable == TRUE)
		&& (AudioCore.AudioSink[i].FuncDataSpaceLenGet() < mainAppCt.SamplesPreFrame))
		{
			if(AudioCore.AudioSink[i].SreamDataState == 1)
			{
				AudioCore.AudioSink[i].SreamDataState = 2;
				continue;
			}

			if((GetSystemMode() == AppModeUsbDevicePlay)
			|| (GetSystemMode() == AppModeUsbPhone)
			)
			{
				continue;
			}
			return FALSE;
		}
	}

	for(i = 0; i< AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		if(AudioCore.AudioSink[i].SreamDataState == 2)
		{
			AudioCore.AudioSink[i].SreamDataState = 1;
			continue;
		}
		if((AudioCore.AudioSink[i].Enable == TRUE) && (AudioCore.AudioSink[i].FuncDataSet != NULL))
		{
#ifdef CFG_FUNC_MIC_KARAOKE_EN
			AudioCore.AudioSink[i].FuncDataSet(AudioCore.AudioSink[i].PcmOutBuf, mainAppCt.SamplesPreFrame);
#else
#ifdef BT_TWS_SUPPORT
			bool ret = FALSE;
#if  (TWS_AUDIO_OUT_PATH	== TWS_IIS0_OUT)
			if(i == AUDIO_I2SOUT_SINK_NUM)
#elif(TWS_AUDIO_OUT_PATH	== TWS_IIS1_OUT)
			if(i == AUDIO_I2SOUT_SINK_NUM)
#elif(TWS_AUDIO_OUT_PATH	== TWS_DAC0_OUT)
			if(i == AUDIO_DAC0_SINK_NUM)
#endif
			{
				if(tws_play == 0x55)
				{
					ret = GIE_STATE_GET();
					GIE_DISABLE();
				}
				else
				{
					tws_play = 0;
				}
			}
#endif
			AudioCore.AudioSink[i].FuncDataSet(AudioCore.AudioSink[i].PcmOutBuf, mainAppCt.SamplesPreFrame);
#ifdef BT_TWS_SUPPORT
#if  (TWS_AUDIO_OUT_PATH	== TWS_IIS0_OUT)
			if(i == AUDIO_I2SOUT_SINK_NUM)
#elif(TWS_AUDIO_OUT_PATH	== TWS_IIS1_OUT)
			if(i == AUDIO_I2SOUT_SINK_NUM)
#elif(TWS_AUDIO_OUT_PATH	== TWS_DAC0_OUT)
			if(i == AUDIO_DAC0_SINK_NUM)
#endif
			{
				if(tws_play == 0x55)
				{
					dac_sample_count += mainAppCt.SamplesPreFrame;
					if(ret)
					{
						GIE_ENABLE();
					}
				}
			}
#endif
#endif
		}
	}
#ifndef CFG_FUNC_MIC_KARAOKE_EN
#ifdef BT_TWS_SUPPORT
	if(tws_play == 0x55)
	{
		extern uint16_t AudioDAC0DataLenGet(void);
		extern bool tws_device_check(void);
		if(adc_data_count == (temp+1))
		{
			void tws_data_ready(void);
			tws_data_ready();
			printf("tws_data_ready %u\n",AudioDAC0DataLenGet());
			data_ready = 1;
			data_ready_tick = gSysTick;
			g_tws_need_init = 0;
		}
		if( adc_data_count == (temp+100) )
		{
			if(tws_device_check() == FALSE)
			{
				g_tws_need_init = 1;
			}
		}
	}
#endif
#endif
	return TRUE;
}

void AudioCoreSourcePcmFormatConfig(uint8_t Index, uint16_t Format)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].PcmFormat = Format;
	}
}

void AudioCoreSourceEnable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = TRUE;
	}
}

void AudioCoreSourceDisable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = FALSE;
	}
}

void AudioCoreSourceMute(uint8_t Index, bool IsLeftMute, bool IsRightMute)
{
	if(IsLeftMute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = TRUE;
	}
	if(IsRightMute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = TRUE;
	}
}

void AudioCoreSourceUnmute(uint8_t Index, bool IsLeftUnmute, bool IsRightUnmute)
{
	if(IsLeftUnmute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = FALSE;
	}
	if(IsRightUnmute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = FALSE;
	}
}

void AudioCoreSourceVolSet(uint8_t Index, uint16_t LeftVol, uint16_t RightVol)
{
	AudioCore.AudioSource[Index].LeftVol = LeftVol;
	AudioCore.AudioSource[Index].RightVol = RightVol;
}

void AudioCoreSourceVolGet(uint8_t Index, uint16_t* LeftVol, uint16_t* RightVol)
{
	*LeftVol = AudioCore.AudioSource[Index].LeftVol;
	*RightVol = AudioCore.AudioSource[Index].RightVol;
}

void AudioCoreSourceConfig(uint8_t Index, AudioCoreSource* Source)
{
	memcpy(&AudioCore.AudioSource[Index], Source, sizeof(AudioCoreSource));
}

void AudioCoreSinkEnable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = TRUE;
}

void AudioCoreSinkDisable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = FALSE;
}

void AudioCoreSinkMute(uint8_t Index, bool IsLeftMute, bool IsRightMute)
{
	if(IsLeftMute)
	{
		AudioCore.AudioSink[Index].LeftMuteFlag = TRUE;
	}
	if(IsRightMute)
	{
		AudioCore.AudioSink[Index].RightMuteFlag = TRUE;
	}
}

void AudioCoreSinkUnmute(uint8_t Index, bool IsLeftUnmute, bool IsRightUnmute)
{
	if(IsLeftUnmute)
	{
		AudioCore.AudioSink[Index].LeftMuteFlag = FALSE;
	}
	if(IsRightUnmute)
	{
		AudioCore.AudioSink[Index].RightMuteFlag = FALSE;
	}
}

void AudioCoreSinkVolSet(uint8_t Index, uint16_t LeftVol, uint16_t RightVol)
{
	AudioCore.AudioSink[Index].LeftVol = LeftVol;
	AudioCore.AudioSink[Index].RightVol = RightVol;
}

void AudioCoreSinkVolGet(uint8_t Index, uint16_t* LeftVol, uint16_t* RightVol)
{
	*LeftVol = AudioCore.AudioSink[Index].LeftCurVol;
	*RightVol = AudioCore.AudioSink[Index].RightCurVol;
}

void AudioCoreSinkConfig(uint8_t Index, AudioCoreSink* Sink)
{
	memcpy(&AudioCore.AudioSink[Index], Sink, sizeof(AudioCoreSink));
}


void AudioCoreProcessConfig(AudioCoreProcessFunc AudioEffectProcess)
{
	AudioCore.AudioEffectProcess = AudioEffectProcess;
}

///**
// * @func        AudioCoreConfig
// * @brief       AudioCore参数块，本地化API
// * @param       AudioCoreContext *AudioCoreCt
// * @Output      None
// * @return      None
// * @Others      外部配置的参数块，复制一份到本地
// */
//void AudioCoreConfig(AudioCoreContext *AudioCoreCt)
//{
//	memcpy(&AudioCore, AudioCoreCt, sizeof(AudioCoreContext));
//}

bool AudioCoreInit(void)
{
	return TRUE;
}

void AudioCoreDeinit(void)
{
	AudioState = AC_RUN_CHECK;
}

/**
 * @func        AudioCoreRun
 * @brief       音源拉流->音效处理+混音->推流
 * @param       None
 * @Output      None
 * @return      None
 * @Others      当前由audioCoreservice任务保障此功能有效持续。
 * Record
 */
extern uint32_t 	IsAudioCorePause;
extern uint32_t 	IsAudioCorePauseMsgSend;
void AudioProcessMain(void);
#ifdef BT_TWS_SUPPORT
__attribute__((optimize("Og")))
#endif
void AudioCoreRun(void)
{
#ifndef CFG_FUNC_MIC_KARAOKE_EN
#ifdef BT_TWS_SUPPORT
	extern bool tws_device_check(void);
	if(dac_mute == 1)//已经ready了
	{
		if(gSysTick - dac_mute_tick > 2000)
		{
			dac_mute = 0;
			tws_device_check();
		}
	}
#endif
#endif
#ifdef BT_TWS_SUPPORT
	if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
	{
		uint32_t temp_gSysTick;
#ifndef CFG_FUNC_MIC_KARAOKE_EN
		if(tws_play == 0x55)
		{
			if(data_ready == 1)//已经ready了
			{
				if(gSysTick - data_ready_tick > 2000)
				{
					data_ready = 0;
					if(tws_device_check() == FALSE)
					{
						g_tws_need_init = 1;
					}
				}
			}
		}
#endif
//		audio_core_handle = xTaskGetCurrentTaskHandle();
		if(tws_link_status_get() < 4)
		{
			tws_play = 0;
			adc_data_count = 0;
		}
		if(tws_link_status_get()== 2)
		{
			data_ready = 0;
			tws_init_done = 0;
			vTaskDelay(50);//3*4*3
			temp_gSysTick = gSysTick;
			tws_start(tws_delay);
			adc_data_count = 0;
			while(1)
			{
				if(tws_init_done)
				{
#ifndef CFG_FUNC_MIC_KARAOKE_EN
					extern void tws_device_close(void);
					dac_mute = 1;
					dac_mute_tick = gSysTick;
					tws_device_close();
#else
					g_tws_need_init = 0;
#endif
					printf("audio core sync done\n");
					tws_play = 0x55;
					adc_data_count = 0;
					data_ready = 0;
					{
						MessageContext		msgSend;
						msgSend.msgId		= MSG_BT_TWS_AUDIO_START;
						MessageSend(GetMainMessageHandle(), &msgSend);
					}
					break;
				}
				if((gSysTick - temp_gSysTick) > 2000)
				{
					if(GetBtManager()->twsRole == BT_TWS_MASTER)
					{
						g_tws_need_init = 1;
					}
					vTaskPrioritySet(audio_core_handle, 4);
					return;
				}
				vTaskDelay(1);
			}
		}
		vTaskPrioritySet(audio_core_handle, 4);
	}
#endif

	bool ret;
	switch(AudioState)
	{
		case AC_RUN_CHECK:
			if(IsAudioCorePause == TRUE)
			{
				if(IsAudioCorePauseMsgSend == TRUE)
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_AUDIO_CORE_HOLD;
					MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);

					IsAudioCorePauseMsgSend = FALSE;
				}
				return;
			}
		case AC_RUN_GET:
			ret = AudioCoreSourceDateGet();
			if(ret == FALSE)
			{
				return;
			}
		case AC_RUN_PROC:
			//AudioCore.AudioProcess();
			AudioProcessMain();
			AudioState = AC_RUN_PUT;

		case AC_RUN_PUT:
			ret = AudioCoreSinkDataSet();
			if(ret == FALSE)
			{
				return;
			}
			//AudioState = AC_RUN_GET;
			AudioState = AC_RUN_CHECK;
			break;
		default:
			break;
	}

	if (SinkMuteDelay)
	{
		SinkMuteDelay--;
		if (SinkMuteDelay == 0)
		{
			DBG("AudioPlayerSinUnkMute\n");
			AudioPlayerSinkMuteRemind(0);
		}
	}
}

//音效处理函数，主入口
//将mic通路数据剥离出来统一处理
//mic通路数据和具体模式无关
//提示音通路无音效，剥离后在sink端混音。
void AudioProcessMain(void)
{	
	AduioCoreSourceVolSet();

#ifdef CFG_FUNC_RECORDER_EN
	if(AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].Enable == TRUE)
	{
		if(AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].PcmFormat == 1)
		{
			uint16_t i;
			for(i = mainAppCt.SamplesPreFrame * 2 - 1; i > 0; i--)
			{
				AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[PLAYBACK_SOURCE_NUM].PcmInBuf[i / 2];
			}
		}
	}
#endif

	if(AudioCore.AudioSource[APP_SOURCE_NUM].Enable == TRUE)////music buff
	{
		#if (BT_HFP_SUPPORT == ENABLE) && defined(CFG_APP_BT_MODE_EN)
		if((GetSystemMode() != AppModeBtHfPlay) && (GetSystemMode() != AppModeBtRecordPlay))
		#endif
		{
			if(AudioCore.AudioSource[APP_SOURCE_NUM].PcmFormat == 1)
			{
				uint16_t i;
				for(i = mainAppCt.SamplesPreFrame * 2 - 1; i > 0; i--)
				{
					AudioCore.AudioSource[APP_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[APP_SOURCE_NUM].PcmInBuf[i / 2];
				}
			}
		}
	}	
		
#if defined(CFG_FUNC_REMIND_SOUND_EN)	
	if(AudioCore.AudioSource[REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		if(AudioCore.AudioSource[REMIND_SOURCE_NUM].PcmFormat == 1)
		{
			uint16_t i;
			for(i = mainAppCt.SamplesPreFrame * 2 - 1; i > 0; i--)
			{
				AudioCore.AudioSource[REMIND_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[REMIND_SOURCE_NUM].PcmInBuf[i / 2];
			}
		}
	}	
#endif
#ifdef CFG_FUNC_REMIND_MIX_MODE
	if(AudioCore.AudioSource[MIX_REMIND_SOURCE_NUM].Enable == TRUE)////remind buff
	{
		if(AudioCore.AudioSource[MIX_REMIND_SOURCE_NUM].PcmFormat == 1)
		{
			uint16_t i;
			for(i = mainAppCt.SamplesPreFrame * 2 - 1; i > 0; i--)
			{
				AudioCore.AudioSource[MIX_REMIND_SOURCE_NUM].PcmInBuf[i] = AudioCore.AudioSource[MIX_REMIND_SOURCE_NUM].PcmInBuf[i / 2];
			}
		}
	}
#endif
	if(AudioCore.AudioEffectProcess != NULL)
	{
		AudioCore.AudioEffectProcess((AudioCoreContext*)&AudioCore);
	}
	
    #ifdef CFG_FUNC_BEEP_EN
    if(AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].Enable == TRUE)   ////dacx buff
	{
		Beep(AudioCore.AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf, mainAppCt.SamplesPreFrame);
	}
    #endif

	AduioCoreSinkVolSet();//调音量
}

void AduioCoreSourceVolSet(void)
{
#if 0
	uint32_t i, j;
    
	uint16_t LeftVol, RightVol, LeftVolStep, RightVolStep;

#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT == ENABLE)
	if(GetSystemMode() == AppModeBtHfPlay)
	{
		AudioCore.AudioSource[0].PreGain = BT_HFP_MIC_DIGIT_GAIN;
		AudioCore.AudioSource[1].PreGain = BT_HFP_INPUT_DIGIT_GAIN;
	}
	else
	{
		AudioCore.AudioSource[0].PreGain = 4095;//0db
		AudioCore.AudioSource[1].PreGain = 4095;
	}
#endif
#endif

#ifdef BT_TWS_SUPPORT
//使用APP的音量作为TWS的音量
	AudioCore.AudioSource[TWS_SOURCE_NUM].LeftVol 		= AudioCore.AudioSource[APP_SOURCE_NUM].LeftVol;
	AudioCore.AudioSource[TWS_SOURCE_NUM].RightVol 		= AudioCore.AudioSource[APP_SOURCE_NUM].RightVol;
#endif

	for(j=0; j<AUDIO_CORE_SOURCE_MAX_NUM; j++)
	{
		if(!AudioCore.AudioSource[j].Enable)
		{
			continue;
		}
		if(AudioCore.AudioSource[j].LeftMuteFlag == TRUE)
		{
			LeftVol = 0;
		}
		else
		{
			LeftVol = AudioCore.AudioSource[j].LeftVol;
		}
		if(AudioCore.AudioSource[j].RightMuteFlag == TRUE)
		{
			RightVol = 0;
		}
		else
		{
			RightVol = AudioCore.AudioSource[j].RightVol;
		}

		LeftVolStep = LeftVol > AudioCore.AudioSource[j].LeftCurVol ? (LeftVol - AudioCore.AudioSource[j].LeftCurVol) : (AudioCore.AudioSource[j].LeftCurVol - LeftVol);
		LeftVolStep = LeftVolStep / mainAppCt.SamplesPreFrame + (LeftVolStep % mainAppCt.SamplesPreFrame ? 1 : 0);
		RightVolStep = RightVol > AudioCore.AudioSource[j].RightCurVol ? (RightVol - AudioCore.AudioSource[j].RightCurVol) : (AudioCore.AudioSource[j].RightCurVol - RightVol);
		RightVolStep = RightVolStep / mainAppCt.SamplesPreFrame + (RightVolStep % mainAppCt.SamplesPreFrame ? 1 : 0);

		if(AudioCore.AudioSource[j].PcmFormat == 2)//立体声
		{
			for(i=0; i<mainAppCt.SamplesPreFrame; i++)
			{
				AudioCore.AudioSource[j].PcmInBuf[2 * i + 0] = __nds32__clips((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i + 0]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i] * AudioCore.AudioSource[j].LeftCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
				AudioCore.AudioSource[j].PcmInBuf[2 * i + 1] = __nds32__clips((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i + 1]) * AudioCore.AudioSource[j].RightCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i + 1] * AudioCore.AudioSource[j].RightCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
				
				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
				MixerFadeVolume(AudioCore.AudioSource[j].RightCurVol, RightVol, RightVolStep);
			}
		}
		else
		{
			for(i=0; i<mainAppCt.SamplesPreFrame; i++)
			{
				AudioCore.AudioSource[j].PcmInBuf[i] = __nds32__clips((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[i]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[i] * AudioCore.AudioSource[j].LeftCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
				MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
			}
		}
	}
#endif
}


void AudioCoreAppSourceVolSet(uint16_t Source,int16_t *pcm_in,uint16_t n,uint16_t Channel)
{
	uint32_t i, j;
    
	uint16_t LeftVol, RightVol,	LeftVolStep, RightVolStep;

#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT == ENABLE)
	if(GetSystemMode() == AppModeBtHfPlay)
	{
		AudioCore.AudioSource[MIC_SOURCE_NUM].PreGain = 4095;
		if (btManager.appleDeviceFlag)
		{
			AudioCore.AudioSource[APP_SOURCE_NUM].PreGain = 4095;
		}
		else
		{
			AudioCore.AudioSource[APP_SOURCE_NUM].PreGain = 4095+BT_HFP_ANDROID_GAIN;
		}
	}
	else
	{
		AudioCore.AudioSource[MIC_SOURCE_NUM].PreGain = 4095;
		AudioCore.AudioSource[MIC_SOURCE_NUM].PreGain = 4095;
	}
#endif
#endif

    if(pcm_in == NULL) return;
	
	j = Source;

	if(!AudioCore.AudioSource[j].Enable)
	{
		#ifdef BT_TWS_SUPPORT
		if ((j == REMIND_SOURCE_NUM)&&(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)&&(GetBtManager()->twsRole == BT_TWS_SLAVE))
		{
			;
		}
		else
		#endif
		{
			return;
		}
	}
	if(AudioCore.AudioSource[j].LeftMuteFlag == TRUE)
	{
		#ifdef BT_TWS_SUPPORT
		if ((j == REMIND_SOURCE_NUM)&&(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)&&(GetBtManager()->twsRole == BT_TWS_SLAVE))
		{
			LeftVol = AudioCore.AudioSource[j].LeftVol;
		}
		else
		#endif
		{
			LeftVol = 0;
		}
	}
	else
	{
		LeftVol = AudioCore.AudioSource[j].LeftVol;
	}
	if(AudioCore.AudioSource[j].RightMuteFlag == TRUE)
	{
		#ifdef BT_TWS_SUPPORT
		if ((j == REMIND_SOURCE_NUM)&&(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)&&(GetBtManager()->twsRole == BT_TWS_SLAVE))
		{
			RightVol = AudioCore.AudioSource[j].RightVol;
		}
		else
		#endif
		{
			RightVol = 0;
		}
	}
	else
	{
		RightVol = AudioCore.AudioSource[j].RightVol;
	}

	LeftVolStep = LeftVol > AudioCore.AudioSource[j].LeftCurVol ? (LeftVol - AudioCore.AudioSource[j].LeftCurVol) : (AudioCore.AudioSource[j].LeftCurVol - LeftVol);
	LeftVolStep = LeftVolStep / mainAppCt.SamplesPreFrame + (LeftVolStep % mainAppCt.SamplesPreFrame ? 1 : 0);
	RightVolStep = RightVol > AudioCore.AudioSource[j].RightCurVol ? (RightVol - AudioCore.AudioSource[j].RightCurVol) : (AudioCore.AudioSource[j].RightCurVol - RightVol);
	RightVolStep = RightVolStep / mainAppCt.SamplesPreFrame + (RightVolStep % mainAppCt.SamplesPreFrame ? 1 : 0);

	if(Channel == 2)//立体声
	{
		for(i=0; i<mainAppCt.SamplesPreFrame; i++)
		{
			pcm_in[2 * i + 0] = __nds32__clips((((((int32_t)pcm_in[2 * i + 0]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i] * AudioCore.AudioSource[j].LeftCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
			pcm_in[2 * i + 1] = __nds32__clips((((((int32_t)pcm_in[2 * i + 1]) * AudioCore.AudioSource[j].RightCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[2 * i + 1] * AudioCore.AudioSource[j].RightCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);

			MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
			MixerFadeVolume(AudioCore.AudioSource[j].RightCurVol, RightVol, RightVolStep);
		}
	}
	else
	{
		for(i=0; i<mainAppCt.SamplesPreFrame; i++)
		{
			pcm_in[i] = __nds32__clips((((((int32_t)pcm_in[i]) * AudioCore.AudioSource[j].LeftCurVol + 2048) >> 12) * AudioCore.AudioSource[j].PreGain + 2048) >> 12, (16)-1);//__SSAT((((((int32_t)AudioCore.AudioSource[j].PcmInBuf[i] * AudioCore.AudioSource[j].LeftCurVol) >> 12) * AudioCore.AudioSource[j].PreGain) >> 12), 16);
			MixerFadeVolume(AudioCore.AudioSource[j].LeftCurVol, LeftVol, LeftVolStep);
		}
	}
}

void AduioCoreSinkVolSet(void)
{
	uint32_t i;
	uint8_t j;

	uint16_t LeftVol, RightVol,	LeftVolStep, RightVolStep;

	for(j=0; j<AUDIO_CORE_SINK_MAX_NUM; j++)
	{
		if(AudioCore.AudioSink[j].Enable == TRUE)
		{
			if(AudioCore.AudioSink[j].LeftMuteFlag == TRUE)
			{
				LeftVol = 0;
			}
			else
			{
				LeftVol = AudioCore.AudioSink[j].LeftVol;
			}
			if(AudioCore.AudioSink[j].RightMuteFlag == TRUE)
			{
				RightVol = 0;
			}
			else
			{
				RightVol = AudioCore.AudioSink[j].RightVol;
			}

			LeftVolStep = LeftVol > AudioCore.AudioSink[j].LeftCurVol ? (LeftVol - AudioCore.AudioSink[j].LeftCurVol) : (AudioCore.AudioSink[j].LeftCurVol - LeftVol);
			LeftVolStep = LeftVolStep / mainAppCt.SamplesPreFrame + (LeftVolStep % mainAppCt.SamplesPreFrame ? 1 : 0);
			RightVolStep = RightVol > AudioCore.AudioSink[j].RightCurVol ? (RightVol - AudioCore.AudioSink[j].RightCurVol) : (AudioCore.AudioSink[j].RightCurVol - RightVol);
			RightVolStep = RightVolStep / mainAppCt.SamplesPreFrame + (RightVolStep % mainAppCt.SamplesPreFrame ? 1 : 0);

			if(AudioCore.AudioSink[j].PcmFormat == 2)
			{
				for(i=0; i<mainAppCt.SamplesPreFrame; i++)
				{
					AudioCore.AudioSink[j].PcmOutBuf[2 * i + 0] = __nds32__clips((((int32_t)AudioCore.AudioSink[j].PcmOutBuf[2 * i + 0]) * AudioCore.AudioSink[j].LeftCurVol + 2048) >> 12, (16)-1);
					AudioCore.AudioSink[j].PcmOutBuf[2 * i + 1] = __nds32__clips((((int32_t)AudioCore.AudioSink[j].PcmOutBuf[2 * i + 1]) * AudioCore.AudioSink[j].RightCurVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(AudioCore.AudioSink[j].LeftCurVol, LeftVol, LeftVolStep);
					MixerFadeVolume(AudioCore.AudioSink[j].RightCurVol, RightVol, RightVolStep);
				}
			}
			else if(AudioCore.AudioSink[j].PcmFormat == 1)
			{
				for(i=0; i<mainAppCt.SamplesPreFrame; i++)
				{
					AudioCore.AudioSink[j].PcmOutBuf[i] = __nds32__clips((((int32_t)AudioCore.AudioSink[j].PcmOutBuf[i]) * AudioCore.AudioSink[j].LeftCurVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(AudioCore.AudioSink[j].LeftCurVol, LeftVol, LeftVolStep);
				}
			}
		}
	}
}

