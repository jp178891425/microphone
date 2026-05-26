/**
 **************************************************************************************
 * @file    bt_play_api.c
 * @brief   
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2017-3-17 13:06:47$
 * 
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

//有关蓝牙A2DP播放的相关处理函数放置此文件

#include "string.h"
#include "type.h"
#include "app_config.h"
#include "app_message.h"
#include "gpio.h"
#include "irqn.h"
#include "gpio.h"
#include "clk.h"
#include "dac.h"
#include "rtos_api.h"
#include "freertos.h"
#include "debug.h"
#include "bt_play_api.h"
#include "bt_play_mode.h"
#include "audio_core_api.h"
#include "decoder_service.h"
#include "device_detect.h"
#include "typedefine.h"
#include "audio_decoder_api.h"
#include "main_task.h"
#include "mode_switch_api.h"
#include "bt_app_interface.h"
#include "bt_avrcp_api.h"
#include "bt_manager.h"
#include "resampler.h"
#include "resampler_polyphase.h"
#include "bt_tws_app_func.h"
#include "sra.h"
#include "audio_adjust.h"
#include "main_task.h"
#include "bt_tws_api.h"
#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
#include "string_convert.h"
#endif

#ifdef CFG_APP_BT_MODE_EN
extern uint32_t gBtPlayModeExitProcessStart;
osMutexId SbcDecoderMutex = NULL;

#define OS_SBC_LOCK	 	if(SbcDecoderMutex != NULL){osMutexLock(SbcDecoderMutex);}
#define OS_SBC_UNLOCK	if(SbcDecoderMutex != NULL){osMutexUnlock(SbcDecoderMutex);}

extern BT_A2DP_PLAYER *a2dp_player;
extern void BtDecoderDeinit(void);
extern int32_t BtDecoderInit(void *io_handle,int32_t decoder_type);
extern uint16_t BtDecodedPcmDataGet(void * pcmData, uint16_t sampleLen);
extern uint16_t BtDecodedPcmDataLenGet(void);


uint32_t a2dp_unmute_delay_cnt = 0;
uint8_t  a2dp_supend_flag = 0;
uint16_t A2DPDataLenGet(void);
void set_a2dp_stream_suspend(void)
{
	if(!IsBtAudioMode())
		return;
	if(A2DPDataLenGet() > 0)
	{
		AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
		a2dp_supend_flag = 1;
		BtDecoderDeinit();
	}
}

void a2dp_stream_suspend_play_end(void)
{
	if(!a2dp_supend_flag)
		return;
#ifdef BT_TWS_SUPPORT
	if(IsBtAudioMode())
	{
		AudioCoreSourceDisable(APP_SOURCE_NUM);
	}
#endif
	if(GetBtManager()->a2dpState == BT_A2DP_STATE_STREAMING)
		GetBtManager()->a2dpState = BT_A2DP_STATE_CONNECTED;
	
	BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, 2);
	AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
	BtDecoderDeinit();

	a2dp_supend_flag = 0;
}

void a2dp_sbc_decoer_init(void)
{
	if(a2dp_player == NULL)
	{
		return;
	}
	OS_SBC_LOCK;
	int gie_ret = GIE_STATE_GET();
	GIE_DISABLE();
	AudioCoreSourceDisable(APP_SOURCE_NUM);
	AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
	a2dp_unmute_delay_cnt = 0;
	memset(a2dp_player,0,sizeof(BT_A2DP_PLAYER));
	a2dp_player->sbc_init_flag = 1;

	a2dp_player->MemHandle.addr = a2dp_player->sbc_fifo;
	a2dp_player->MemHandle.mem_capacity = sizeof(a2dp_player->sbc_fifo);
	a2dp_player->MemHandle.mem_len = 0;
	a2dp_player->MemHandle.p = 0;
	btManager.aacFrameNumber = 0;

	if(gie_ret)
	{
		GIE_ENABLE();
	}
	OS_SBC_UNLOCK;
}


void BtAudioCoreSourceFreqAdjustEnable(void)
{
	#ifdef CFG_FUNC_FREQ_ADJUST
	if(GetBtManager()->a2dpStreamType == BT_A2DP_STREAM_TYPE_AAC)
		AudioCoreSourceFreqAdjustEnable(1, BT_AAC_LEVEL_LOW, BT_AAC_LEVEL_HIGH);
	else
		AudioCoreSourceFreqAdjustEnable(1, BT_SBC_LEVEL_LOW, BT_SBC_LEVEL_HIGH);
	#endif
}

void SoftFlagRegister(uint32_t SoftEvent);
void a2dp_sbc_save(uint8_t *p,uint32_t len)
{
	if(GetSystemMode() != AppModeBtAudioPlay)
		return;

	if(gBtPlayModeExitProcessStart)
		return;
		
	if(a2dp_player == NULL)
		return;
	
	if(a2dp_player->sbc_init_flag == 0)
		return ;	
	
	if(a2dp_unmute_delay_cnt < 10 && AudioCore.AudioSource[1].Enable)
	{
		a2dp_unmute_delay_cnt++;
		if(a2dp_unmute_delay_cnt == 10)
		{
			AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
		}
	}
	
	OS_SBC_LOCK;
	if(mv_mremain(&a2dp_player->MemHandle) > len)
	{
		mv_mwrite(p, len, 1,&a2dp_player->MemHandle);
		if(GetBtManager()->a2dpStreamType == BT_A2DP_STREAM_TYPE_AAC
		  && btManager.aacFrameNumber < 0xffffffff)
		{
			btManager.aacFrameNumber++;
		}
	}

	if(AudioCore.AudioSource[1].Enable == FALSE)
	{
		if(SoftFlagGet(SoftFlagDecoderRemind))
		{
			AudioCoreSourceDisable(1);
		}
		else
		{
			#ifdef BT_AUDIO_AAC_ENABLE
			if(GetBtManager()->a2dpStreamType == BT_A2DP_STREAM_TYPE_AAC)
			{
				if(btManager.aacFrameNumber >= BT_AAC_START_FRAME)
				{
					BtDecoderInit(&a2dp_player->MemHandle,AAC_DECODER);
					AudioCoreSourceEnable(1);
					BtAudioCoreSourceFreqAdjustEnable();					
				}
			}
			else
			#endif
			{
				if(GetValidSbcDataSize() >= SBC_FIFO_LEVEL_HIGH)
				{
					BtDecoderInit(&a2dp_player->MemHandle,SBC_DECODER);
					AudioCoreSourceEnable(1);
					BtAudioCoreSourceFreqAdjustEnable();
				}
			}
		}
	}

	OS_SBC_UNLOCK;
}

uint32_t GetValidSbcDataSize(void)
{
	uint32_t	dataSize = 0;
	dataSize =  mv_msize(&a2dp_player->MemHandle);
	return dataSize;
}

uint32_t GetValidFrameDataSize(void)
{
	uint32_t	dataSize = 0;

	dataSize = btManager.aacFrameNumber;

	return dataSize;
}

uint16_t A2DPDataLenGet(void)
{
	uint16_t Len = 0;
	if(gBtPlayModeExitProcessStart)
		return 0;
	
	if(a2dp_player == NULL)
		return 0;

	if(a2dp_player->sbc_init_flag == 0)
		return 0;
	
	OS_SBC_LOCK;
	Len = BtDecodedPcmDataLenGet();
	OS_SBC_UNLOCK;
	if(Len == 0)
		a2dp_stream_suspend_play_end();
	return Len;
}

uint16_t A2DPDataGet(void* Buf, uint16_t Samples)
{
	uint16_t Len = 0;

	if(gBtPlayModeExitProcessStart)
		return 0;
	
	if(a2dp_player == NULL)
		return 0;
	
	if(a2dp_player->sbc_init_flag == 0)
		return 0;

	if(AudioCore.AudioSource[1].Enable == FALSE)
	{
		memset(Buf,0,Samples*4);
		return 0;
	}

	OS_SBC_LOCK;
	Len = BtDecodedPcmDataGet(Buf,Samples);
	OS_SBC_UNLOCK;
	return Len;
}

#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
void GetBtMediaInfo(void *params)
{
	#define StringMaxLen 60
	AvrcpAdvMediaInfo	*CurMediaInfo;
	uint8_t i;
	uint8_t StringData[StringMaxLen];
	static uint8_t StringCmp[StringMaxLen];
	uint8_t ConvertStringData[StringMaxLen];
	CurMediaInfo = (AvrcpAdvMediaInfo*)params;

	if((CurMediaInfo)&&(CurMediaInfo->numIds))
	{
		for(i=0;i<CurMediaInfo->numIds;i++)
		{
			memset(StringData, 0, StringMaxLen);
			memset(ConvertStringData, 0, StringMaxLen);
			
			if(CurMediaInfo->property[i].length)
			{
				if(CurMediaInfo->property[i].charSet == 0x006a)
				{
					//APP_DBG("Character Set Id: UTF-8\n");
					if(CurMediaInfo->property[i].length > StringMaxLen)
					{
						memcpy(StringData, CurMediaInfo->property[i].string, StringMaxLen);
						#ifdef CFG_FUNC_STRING_CONVERT_EN
						StringConvert(ConvertStringData, 60, StringData, StringMaxLen ,UTF8_TO_GBK);
						#endif
					}
					else
					{
						memcpy(StringData, CurMediaInfo->property[i].string, CurMediaInfo->property[i].length);
						#ifdef CFG_FUNC_STRING_CONVERT_EN
						StringConvert(ConvertStringData, 60, StringData, CurMediaInfo->property[i].length ,UTF8_TO_GBK);
						#endif
					}

					//Attribute ID
					if(ConvertStringData[0])// no character ,not dispaly ID3
					{
						switch(CurMediaInfo->property[i].attrId)
						{
							case 1:
								//此处为去掉重复歌词打印，若需要打印重复歌词，屏蔽此处
                                if(memcmp(ConvertStringData,StringCmp,StringMaxLen) == 0)
                                    return;
                                memcpy(StringCmp, ConvertStringData, StringMaxLen);
								APP_DBG("Title of the media\n");
								break;
			
							case 2:
								APP_DBG("Name of the artist\n");
								break;
			
							case 3:
								APP_DBG("Name of the Album\n");
								break;
			
							//当前曲目数:只有在自带播放器才能获取到
							case 4:
								APP_DBG("Number of the media\n");
								break;
			
							//总共曲目数:只有在自带播放器才能获取到
							case 5:
								APP_DBG("Totle number of the media\n");
								break;
			
							case 6:
								APP_DBG("Genre\n");
								break;
			
							case 7:
								APP_DBG("Playing time in millisecond\n");
								break;
							
							case 8:
								APP_DBG("Default cover art\n");
								break;
			
							default:
								break;
						}
					}

					#ifdef CFG_FUNC_STRING_CONVERT_EN
					APP_DBG("%s\n", ConvertStringData);
					#endif
					
					}
				else
				{
					;//APP_DBG("Other Character Set Id: 0x%x\n", CurMediaInfo->property[i].charSet);
				}
			}
		}
	}
}
#endif

#else

void BtSbcDecoderRefresh(void)
{
}


#endif//#ifdef CFG_APP_BT_MODE_EN
