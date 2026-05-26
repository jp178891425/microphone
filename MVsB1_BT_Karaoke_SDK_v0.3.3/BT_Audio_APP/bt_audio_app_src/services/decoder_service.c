/**
 **************************************************************************************
 * @file    decoder_service.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2018-1-10 20:21:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <stdio.h>
#include <string.h>

#include "type.h"
#include "app_message.h"
#include "rtos_api.h"
#include "mvstdio.h"
#include "audio_decoder_api.h"
#include "audio_core_service.h"
#include "decoder_service.h"
#include "mcu_circular_buf.h"
#include "app_config.h"
#include "core_d1088.h"
#include "debug.h"
#include "main_task.h"

#ifdef CFG_FUNC_TSM_EN
#include "tsm.h"
#include "ctrlvars.h"
#endif
#ifdef CFG_APP_BT_MODE_EN
#include "bt_config.h"
#include "bt_play_api.h"
#include "bt_manager.h"
#if (BT_HFP_SUPPORT == ENABLE)
#include "bt_hf_mode.h"
#include "bt_hf_api.h"
#include "bt_record_api.h"
#endif
#endif
#include "audio_adjust.h"
#include "resampler_polyphase.h"
#include "ff.h"
#include "ffpresearch.h"
#include "audio_common.h"
#include "soft_watch_dog.h"
static void DecoderProcess(void);
static int16_t SaveDecodedPcmData(int16_t * PcmData, uint16_t PcmDataLen);
void DecoderStopProcess(void);
#ifdef LOSSLESS_DECODER_HIGH_RESOLUTION
void AudioCorePcmDataBitWidthConv(int32_t *PcmBuf,uint16_t dataSize);
#endif
extern FileType SongFileType;

#ifdef BT_TWS_SUPPORT
#define DECODER_SERVICE_SIZE				640			//TWS开启后,decoder任务栈需要配置为640以上
#else
#ifdef USE_FLAC_DECODER
#define DECODER_SERVICE_SIZE				512
#else
#define DECODER_SERVICE_SIZE				512//384//640//1024
#endif
#endif

#define DECODER_SERVICE_PRIO				4
#define DECODER_SERVICE_TIMEOUT				1	/*unit ms */
#define DECODER_SERVICE_NUM_MESSAGE_QUEUE	16
#define SBC_DECODER_FIFO_MIN				(119*2)//(119*17)
#define AAC_DECODER_FIFO_MIN				(50)//(600)
#define MSBC_DECODER_FIFO_MIN				57*2//238
#define	SRC_OUT_BUF_LEN						(MAX_FRAME_SAMPLES * 2 * 4 + 48) //旨在控制转采样帧，输出<=256点最经济。

AudioDecoderContext *audio_decoder;

typedef struct _DecoderServiceContext
{
	xTaskHandle 		taskHandle;
	MessageHandle		msgHandle;
	MessageHandle		parentMsgHandle;
	TaskState			serviceState;

	/*for audio decode init*/
	int32_t				ioType;
	int32_t				decoderType;
	uint32_t			DecoderSize;

	/* for decoded out fifo */
	MCU_CIRCULAR_CONTEXT DecoderCircularBuf;
#ifdef CFG_FUNC_TSM_EN
	MCU_CIRCULAR_CONTEXT TsmCircularBuf;
	int8_t              DecoderSpeed;///-1 0, +1
	uint16_t			 TsmFifoSize; //
	int16_t				*TsmFifo;
#endif
	int16_t				*DecoderFifo;
	uint16_t			DecoderFifoSize; //
	osMutexId			DecoderPcmFifoMutex;
	int8_t				*decoder_buf;
	uint8_t				Channel;//Decoderfifo内数据声道参数

	/* for save decoded PCM data, unit: Sample*/
	uint32_t			pcmDataSize;
	uint32_t			savedSize;
	int16_t				*toSavePos;

	DecoderState		decoderState;

	uint32_t			StepTime;				//快进快退

	/*for play timer*/
	uint32_t			DecoderSamples;			//当前尚未计入(指示)时间的采样点
	uint32_t			DecoderCurPlayTime;		//当前播放的时间
	uint32_t			DecoderLastPlayTime;

#ifdef CFG_FUNC_MIXER_SRC_EN
	ResamplerPolyphaseContext	*ResamplerCt;
	int16_t*					SRCOutBuf;
#endif

	uint32_t			CmdErrCnt;
}DecoderServiceContext;

static 	DecoderServiceContext 		DecoderServiceCt;
static uint8_t	DecoderSourecNum;

/**
 * @func        DecoderService_Init
 * @brief       DecoderService初始化
 * @param       MessageHandle parentMsgHandle  
 * @param		BufSize	Decoder buffer Size:4K @wav? 19K @mp3,40K @flac
 * @Output      None
 * @return      bool
 * @Others      解码输出流buf，开启锁机制。
 * Record
 */
static bool DecoderService_Init(MessageHandle parentMsgHandle, uint32_t BufSize, uint16_t FifoSize)
{
	//DecoderServiceCt = (DecoderServiceContext)osPortMalloc(sizeof(DecoderServiceContext));
	memset(&DecoderServiceCt, 0, sizeof(DecoderServiceContext));

	/* register message handle */
	if((DecoderServiceCt.msgHandle = MessageRegister(DECODER_SERVICE_NUM_MESSAGE_QUEUE)) == NULL)
	{
		return FALSE;
	}
	DecoderServiceCt.parentMsgHandle = parentMsgHandle;
	DecoderServiceCt.serviceState = TaskStateCreating;

	DecoderServiceCt.DecoderSize = BufSize;
	DecoderServiceCt.DecoderFifoSize = FifoSize;

#ifdef CFG_FUNC_MIXER_SRC_EN
	DecoderServiceCt.ResamplerCt = (ResamplerPolyphaseContext*)osPortMalloc(sizeof(ResamplerPolyphaseContext));
	if(DecoderServiceCt.ResamplerCt == NULL)
	{
		return FALSE;
	}
//	if((DecoderServiceCt.ResamplerCt = (ResamplerPolyphaseContext*)osPortMalloc(sizeof(ResamplerPolyphaseContext))) == NULL)
//	{
//		return FALSE;
//	}
	memset(DecoderServiceCt.ResamplerCt, 0, sizeof(ResamplerPolyphaseContext));
	DecoderServiceCt.SRCOutBuf = (int16_t*)osPortMalloc(SRC_OUT_BUF_LEN);
	if(DecoderServiceCt.SRCOutBuf == NULL)
	{
		return FALSE;
	}

//	if((DecoderServiceCt.SRCOutBuf = (int16_t*)osPortMalloc(SRC_OUT_BUF_LEN)) == NULL)
//	{
//		return FALSE;
//	}
	memset(DecoderServiceCt.SRCOutBuf, 0, SRC_OUT_BUF_LEN);
#endif

	if(!DecoderServiceCt.DecoderSize || DecoderServiceCt.DecoderFifoSize < DECODER_FIFO_SIZE_MIN)
	{
		return FALSE;
	}

	DecoderServiceCt.decoder_buf = (int8_t*)osPortMalloc(DecoderServiceCt.DecoderSize);
	if(DecoderServiceCt.decoder_buf == NULL)//DECODER_BUF_SIZE);
	{
		return FALSE;
	}
//	if((DecoderServiceCt.decoder_buf = (int8_t*)osPortMalloc(DecoderServiceCt.DecoderSize)) == NULL)//DECODER_BUF_SIZE);
//	{
//		return FALSE;
//	}
	memset(DecoderServiceCt.decoder_buf, 0, DecoderServiceCt.DecoderSize);
	
	if((DecoderServiceCt.DecoderPcmFifoMutex = xSemaphoreCreateMutex()) == NULL)
	{
		return FALSE;
	}

	DecoderServiceCt.DecoderFifo = (int16_t*)osPortMalloc(DecoderServiceCt.DecoderFifoSize);
	if(DecoderServiceCt.DecoderFifo == NULL)
	{
		return FALSE;
	}

//	if((DecoderServiceCt.DecoderFifo = (int16_t*)osPortMalloc(DecoderServiceCt.DecoderFifoSize)) == NULL)
//	{
//		return FALSE;
//	}
	memset(DecoderServiceCt.DecoderFifo, 0, DecoderServiceCt.DecoderFifoSize);
	MCUCircular_Config(&DecoderServiceCt.DecoderCircularBuf, DecoderServiceCt.DecoderFifo, DecoderServiceCt.DecoderFifoSize);

#ifdef CFG_FUNC_TSM_EN
	DecoderServiceCt.DecoderSpeed = 0;
	DecoderServiceCt.TsmFifoSize = FifoSize;

	if((DecoderServiceCt.TsmFifo = (int16_t*)osPortMalloc(DecoderServiceCt.TsmFifoSize)) == NULL)
	{
		return FALSE;
	}
	memset(DecoderServiceCt.TsmFifo, 0, DecoderServiceCt.TsmFifoSize);

	MCUCircular_Config(&DecoderServiceCt.TsmCircularBuf, DecoderServiceCt.TsmFifo, DecoderServiceCt.TsmFifoSize);
#endif
	return TRUE;
}

static void DecoderServiceEntrance(void * param)
{
	MessageContext		msgRecv;
	MessageContext		msgSend;
	uint32_t			WaitingMsg = 300;
	DecoderServiceCt.serviceState = TaskStateReady;

	/* Send message to parent*/
	msgSend.msgId		= MSG_DECODER_SERVICE_CREATED;
	MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
#ifdef SOFT_WACTH_DOG_ENABLE
	SWD_task_register(SWD_DecoderTask_ID);
#endif
	while(1)
	{
		MessageRecv(DecoderServiceCt.msgHandle, &msgRecv, WaitingMsg);
#ifdef SOFT_WACTH_DOG_ENABLE
		SWD_task_reset(SWD_DecoderTask_ID);
#endif
		switch(msgRecv.msgId)
		{
			case MSG_TASK_RESUME:
				APP_DBG("Decoder:Resume\n");
			case MSG_TASK_START:
				if(DecoderServiceCt.serviceState == TaskStateReady)
				{
					msgSend.msgId		= MSG_DECODER_SERVICE_STARTED;
					MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);				
					DecoderServiceCt.serviceState = TaskStateRunning;
					SoftFlagRegister(SoftFlagDecoderRun);
				}
				WaitingMsg = DECODER_SERVICE_TIMEOUT;
				break;

			case MSG_TASK_PAUSE:
				DecoderDeinit();
				DecoderServiceCt.decoderState = DecoderStateNone;
				DecoderServiceCt.serviceState = TaskStateReady;
				msgSend.msgId		= MSG_DECODER_SERVICE_PAUSED;
				MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);	
				break;

			case MSG_TASK_STOP:
				APP_DBG("MSG_TASK_STOP\n");
				SoftFlagDeregister(SoftFlagDecoderRun);
				{
					//Set para
					DecoderDeinit();
					DecoderServiceCt.decoderState = DecoderStateNone;

					//clear msg
					MessageClear(DecoderServiceCt.msgHandle);
					
					//Set state
					DecoderServiceCt.serviceState = TaskStateStopped;

					//reply
					msgSend.msgId		= MSG_DECODER_SERVICE_STOPPED;
					MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
				}
				break;
			case MSG_DECODER_RESET:
				msgSend.msgId		= MSG_DECODER_RESET;
				MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
				DecoderServiceCt.decoderState = DecoderStateNone;
				osMutexLock(DecoderServiceCt.DecoderPcmFifoMutex);
				MCUCircular_Config(&DecoderServiceCt.DecoderCircularBuf, DecoderServiceCt.DecoderFifo, DecoderServiceCt.DecoderFifoSize);
				osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);
				if(!SoftFlagGet(SoftFlagDecoderApp) && !SoftFlagGet(SoftFlagDecoderRemind))
				{//解码器没有复用，并且没有被提示音使用，可以长时间阻塞,节省mcps
					WaitingMsg = 300;
				}
				else
				{
					WaitingMsg = DECODER_SERVICE_TIMEOUT;
				}
				break;
			
			case MSG_DECODER_PAUSE:
				DecoderServiceCt.decoderState = DecoderStatePause;
				break;
			case MSG_DECODER_PLAY:
				if(DecoderServiceCt.decoderState == DecoderStateInitialized)
				{
					//APP_DBG("Decoder:Play @state:%d\n", DecoderServiceCt.serviceState);
					DecoderServiceCt.decoderState = DecoderStatePlay;
					WaitingMsg = DECODER_SERVICE_TIMEOUT;
				}
				break;
			case MSG_DECODER_RESUME:
				if(DecoderServiceCt.decoderState != DecoderStateNone)
				{
					DecoderServiceCt.decoderState = DecoderStatePlay;
					WaitingMsg = DECODER_SERVICE_TIMEOUT;
				}
				break;

			case MSG_DECODER_STOP:
				DecoderServiceCt.decoderState = DecoderStateStop;
				break;

			case MSG_DECODER_FF:
				{
					uint32_t SamplesMs,TotalMs;
					//APP_DBG("Samples %d val ,PlayTime %ds, %dhz\n",DecoderServiceCt.DecoderSamples,DecoderServiceCt.DecoderCurPlayTime,audio_decoder->song_info.sampling_rate);
					SamplesMs = DecoderServiceCt.DecoderSamples * 1000 / audio_decoder->song_info.sampling_rate;
					TotalMs = DecoderServiceCt.DecoderCurPlayTime * 1000 + SamplesMs;
					//APP_DBG("Samples %d ms\n",SamplesMs);
					//APP_DBG("Total ms %d ms\n",TotalMs);
					//APP_DBG("StepTime %d ms\n",DecoderServiceCt.StepTime);
					TotalMs = TotalMs + DecoderServiceCt.StepTime - 200;		//200ms ：软件补偿步进间隔时间中播放器播放的时间。
					if(TotalMs > audio_decoder->song_info.duration)
					{
						DecoderServiceCt.decoderState = DecoderStatePause;
						APP_DBG("SONG FF END; Play next song.\n");
						msgSend.msgId		= MSG_NEXT;
						MessageSend(GetAppMessageHandle(), &msgSend);
						break;
					}
					DecoderServiceCt.DecoderCurPlayTime = TotalMs/1000;
					DecoderServiceCt.DecoderSamples = (TotalMs % 1000)*(audio_decoder->song_info.sampling_rate)/1000;
					//APP_DBG("Samples %d val ,PlayTime %ds, %dhz\n",DecoderServiceCt.DecoderSamples,DecoderServiceCt.DecoderCurPlayTime,audio_decoder->song_info.sampling_rate);
					//APP_DBG("seek %d ms\n",TotalMs);
					audio_decoder_seek(audio_decoder,TotalMs);
				}
				break;

			case MSG_DECODER_FB:
				{
					uint32_t SamplesMs,TotalMs;
					//APP_DBG("Samples %d val ,PlayTime %ds, %dhz\n",DecoderServiceCt.DecoderSamples,DecoderServiceCt.DecoderCurPlayTime,audio_decoder->song_info.sampling_rate);
					SamplesMs = DecoderServiceCt.DecoderSamples * 1000 / audio_decoder->song_info.sampling_rate;
					TotalMs = DecoderServiceCt.DecoderCurPlayTime * 1000 + SamplesMs;
					//APP_DBG("Samples %dms\n",SamplesMs);
					//APP_DBG("Total ms %dms\n",TotalMs);
					//APP_DBG("StepTime %dms\n",DecoderServiceCt.StepTime);
					if(DecoderServiceCt.DecoderCurPlayTime * 1000 == 0)
					{
						//DecoderServiceCt.decoderState = DecoderStatePause;
						APP_DBG("backward to 0, then pause the player\n");
						msgSend.msgId		= MSG_PLAY_PAUSE;
						MessageSend(GetAppMessageHandle(), &msgSend);
						break;
					} //快退到歌曲起点后，暂停在这里，直到松开快退键后重新开始播放(参考PC端酷狗&QQ客户端都是这样做的)
					else if(DecoderServiceCt.DecoderCurPlayTime * 1000 < DecoderServiceCt.StepTime)
					{
						DecoderServiceCt.DecoderCurPlayTime = 0;
					}
					else
					{
						TotalMs = TotalMs - DecoderServiceCt.StepTime - 200;	//200ms ：软件补偿步进间隔时间中播放器播放的时间。
						DecoderServiceCt.DecoderCurPlayTime = TotalMs/1000;
						DecoderServiceCt.DecoderSamples = (TotalMs % 1000)*(audio_decoder->song_info.sampling_rate)/1000;
						//APP_DBG("Samples %d val ,PlayTime %ds, %dhz\n",DecoderServiceCt.DecoderSamples,DecoderServiceCt.DecoderCurPlayTime,audio_decoder->song_info.sampling_rate);
						//APP_DBG("seek %d ms\n",TotalMs);
						audio_decoder_seek(audio_decoder,TotalMs);
					}
				}
				break;

			default:
				break;
		}
		if(DecoderServiceCt.serviceState == TaskStateRunning)
		{
			DecoderProcess();
			AudioCoreServiceMsg();
		}
	}
}


//extern MemHandle SBC_MemHandle;
static void DecoderProcess(void)
{
	int32_t DecoderErrCode;
	static int32_t DecoderCnt = 0;
	uint16_t SampleForRetry = CFG_PARA_MAX_SAMPLES_PER_FRAME;//单次解码的目标解码采样点数量。

#ifdef CFG_FUNC_TSM_EN
	uint16_t TsmLoop = 0;
	if(DecoderServiceCt.DecoderSpeed == -1)//减速解码
	{
		return;
	}
	if(DecoderServiceCt.DecoderSpeed == 1)//加速解码
	{
		TsmLoop = 1;
	}
	_TsmLoop:
#endif ////#ifdef CFG_FUNC_TSM_EN

	do{
		switch(DecoderServiceCt.decoderState)
		{
			case DecoderStateInitialized:
				break;

			case DecoderStatePlay:
				{
					DecoderServiceCt.decoderState	= DecoderStateDecoding;
					MessageContext		msgSend;
					msgSend.msgId		= MSG_DECODER_PLAYING;
					MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
					osMutexLock(DecoderServiceCt.DecoderPcmFifoMutex);
					MCUCircular_Config(&DecoderServiceCt.DecoderCircularBuf, DecoderServiceCt.DecoderFifo, DecoderServiceCt.DecoderFifoSize);
					AudioCoreSourcePcmFormatConfig(DecoderSourecNum, (int)audio_decoder->song_info.num_channels);//配置audiocore
					DecoderServiceCt.Channel = audio_decoder->song_info.num_channels;//配置本地fifo 声道数（init后固定不变）
					//AudioCoreSourceUnmute(DecoderSourecNum, TRUE, TRUE);//编码歌曲大多有淡入，sdk淡入时间短且早，效果不显。
                    #ifdef CFG_FUNC_TSM_EN
	                MCUCircular_Config(&DecoderServiceCt.TsmCircularBuf, DecoderServiceCt.TsmFifo, DecoderServiceCt.TsmFifoSize);
                    #endif
					osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);
				}
				break;

			case DecoderStateDecoding:
				if(RT_SUCCESS != audio_decoder_can_continue(audio_decoder))
				{
					DecoderServiceCt.decoderState = DecoderStatePause;//DecoderStateStop;//数据不够，
					if(DecoderServiceCt.serviceState == TaskStateRunning)
					{
						MessageContext		msgSend;
						//DecoderServiceCt.serviceState = TaskStateStopped;
						msgSend.msgId		= MSG_DECODER_SERVICE_SONG_PLAY_END;
						MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
					}
					return;
				}

#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT == ENABLE)
				if(mainAppCt.appCurrentMode == AppModeBtHfPlay)
				{
					if(!SoftFlagGet(SoftFlagDecoderRemind))
					{
						if(BtHf_MsbcDataLenGet() <= MSBC_DECODER_FIFO_MIN)
						{
							return;
						}
					}
				}
#endif
#if defined(BT_RECORD_FUNC_ENABLE)
				if(mainAppCt.appCurrentMode == AppModeBtRecordPlay)
				{
					if(!SoftFlagGet(SoftFlagDecoderRemind))
					{
						if(BtRecord_MsbcDataLenGet() <= MSBC_DECODER_FIFO_MIN)
						{
							return;
						}
					}
				}
#endif
#endif

				if(audio_decoder_decode(audio_decoder) != RT_SUCCESS)
				{
					DecoderErrCode = audio_decoder_get_error_code(audio_decoder);
					
#ifdef CFG_FUNC_RECORD_FLASH
					if(mainAppCt.appCurrentMode == AppModeFlashPlayBack)
					{
						DecoderServiceCt.decoderState = DecoderStatePause;//DecoderStateStop;//数据不够，
						MessageContext		msgSend;
						msgSend.msgId		= MSG_DECODER_SERVICE_SONG_PLAY_END;
						MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
					}
#endif

#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN) || defined(CFG_FUNC_RECORDER_EN)
					if((audio_decoder->decoder_type != MP3_DECODER)
						&& (audio_decoder->decoder_type != MVB_DECODER)
						&& (audio_decoder->decoder_type != MVC_DECODER)
						&& (audio_decoder->decoder_type != MVD_DECODER)
						&& (audio_decoder->decoder_type != SBC_DECODER)
						&& (audio_decoder->decoder_type != MSBC_DECODER)
						&& (audio_decoder->decoder_type != WAV_DECODER)
						&& (audio_decoder->decoder_type != DTS_DECODER)
						&&((mainAppCt.appCurrentMode == AppModeCardAudioPlay)
								|| (mainAppCt.appCurrentMode == AppModeUDiskAudioPlay)
								|| (mainAppCt.appCurrentMode == AppModeCardPlayBack)
								|| (mainAppCt.appCurrentMode == AppModeUDiskPlayBack)
								|| (mainAppCt.appCurrentMode == AppModeFlashFsPlayBack)))
					{
						if(DecoderServiceCt.serviceState == TaskStateRunning)
						{
							extern uint32_t CmdErrCnt;
							MessageContext		msgSend;

							if(CmdErrCnt >= 3)
							{
								msgSend.msgId = MSG_DECODER_SERVICE_DISK_ERROR;
								MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
								vTaskDelay(100);
							}
							else
							{
								msgSend.msgId = MSG_DECODER_SERVICE_SONG_PLAY_END;
								MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
							}
						}
						if(!SoftFlagGet(SoftFlagDecoderRemind))
						{
							APP_DBG("decoder err, can not continue\n");
							//WMA, MP4, FLAC
							if((audio_decoder->decoder_type==WMA_DECODER) || \
							   (audio_decoder->decoder_type==FILE_TYPE_AAC) \
									)
							{
								uint32_t Timer,TotalMs;
								Timer = DecoderServiceCt.DecoderCurPlayTime * 1000 + 20;
								//APP_DBG("Samples %d val ,PlayTime %ds, %dhz\n",DecoderServiceCt.DecoderSamples,DecoderServiceCt.DecoderCurPlayTime,audio_decoder->song_info.sampling_rate);
								TotalMs = audio_decoder->song_info.duration;///1000;
								if(Timer > TotalMs)
								{
									Timer = TotalMs-DecoderServiceCt.DecoderCurPlayTime * 1000;
								}
								audio_decoder_seek(audio_decoder, Timer);
								extern uint32_t CmdErrCnt;
								MessageContext  msgSendErr;
								if(++CmdErrCnt >= 30)
								{
								  msgSendErr.msgId  = MSG_NEXT;
								  MessageSend(GetAppMessageHandle(), &msgSendErr);
								  CmdErrCnt = 0;
								}
							}
						    return;
						}
					}
#endif					
					if(-127 == DecoderErrCode)
					{
						MessageContext		msgSend;

						DecoderCnt++;
						//异常,连续500次读不到数据
						if(DecoderCnt > 500)
						{
							DecoderCnt = 0;
							DecoderServiceCt.decoderState = DecoderStatePause;
							//msgSend.msgId		= MSG_DECODER_SERVICE_SONG_PLAY_END;
							msgSend.msgId = MSG_DECODER_SERVICE_DISK_ERROR;
							MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
							APP_DBG("END SONG FOR DECODER TIMEOUT\n");
						}
						//该消息在app task中没有处理，为空时，会导致该msg不停的发送，从而冲爆了msg handle的消息队列，可能引起其他msg得不到响应
						//暂时屏蔽
						/*else
						{
							msgSend.msgId		= MSG_DECODER_SERVICE_FIFO_EMPTY;
							MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
						}*/
					}
					/*else
					{
						if(mainAppCt.appCurrentMode == AppModeBtAudioPlay)
						{
							//出现错误时，重新初始化decoder
							BtSbcDecoderRefresh();
						}
					}*/
					return;
				}
				else if(DecoderCnt != 0)
				{
					APP_DBG("FIFO_EMPTY Cnt = %d\n",(int)DecoderCnt);
				}
				// decoder ok
				if((audio_decoder->decoder_type==WMA_DECODER) || (audio_decoder->decoder_type==FILE_TYPE_AAC))
				{
					extern uint32_t CmdErrCnt;
					CmdErrCnt = 0;
				}
				
				DecoderCnt = 0;
				//计算播放时间
				{
					uint32_t Temp;
					DecoderServiceCt.DecoderSamples += audio_decoder->song_info.pcm_data_length;
					Temp = DecoderServiceCt.DecoderSamples / audio_decoder->song_info.sampling_rate;
					DecoderServiceCt.DecoderSamples -= Temp * audio_decoder->song_info.sampling_rate;
					DecoderServiceCt.DecoderCurPlayTime += Temp;
					//--Resolve the inconsistency between WAV file size and playback time size//
					if(audio_decoder->decoder_type == WAV_DECODER)
					{
					    uint32_t play_samples;

					    play_samples = DecoderServiceCt.DecoderCurPlayTime * 1000;

						if(play_samples >= audio_decoder->song_info.duration)
						{
							DecoderServiceCt.decoderState = DecoderStatePause;
							MessageContext  MsgPlayEnd;
							MsgPlayEnd.msgId  = MSG_DECODER_SERVICE_SONG_PLAY_END;
							MessageSend(GetAppMessageHandle(), &MsgPlayEnd);
							break;
						 }
					}
					//--------------------------
					if((DecoderServiceCt.DecoderCurPlayTime ) != (DecoderServiceCt.DecoderLastPlayTime))
					{
						MessageContext		msgSend;

						msgSend.msgId		= MSG_DECODER_SERVICE_UPDATA_PLAY_TIME;
						MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);

						//DecoderServiceCt.DecoderLastPlayTime = DecoderServiceCt.DecoderCurPlayTime;
					}
					DecoderServiceCt.DecoderLastPlayTime = DecoderServiceCt.DecoderCurPlayTime;
				}
				DecoderServiceCt.decoderState = DecoderStateToSavePcmData;
			case DecoderStateToSavePcmData:
				{
					DecoderServiceCt.pcmDataSize	= audio_decoder->song_info.pcm_data_length * DecoderServiceCt.Channel;
					DecoderServiceCt.savedSize		= 0;
					DecoderServiceCt.toSavePos		= (int16_t *)audio_decoder->song_info.pcm_addr;
#ifdef LOSSLESS_DECODER_HIGH_RESOLUTION
					if(audio_decoder->song_info.pcm_bit_width != 16)
					{

						if(audio_decoder->song_info.pcm_bit_width == 32)	//32转成24bit
						{
							int16_t  i;
							int32_t * Pcm32Data = (int32_t *)DecoderServiceCt.toSavePos;
							for(i=0;i<audio_decoder->song_info.pcm_data_length;i++)
							{
								Pcm32Data[i] >>= 8;
							}
						}
						AudioCorePcmDataBitWidthConv((int32_t *)DecoderServiceCt.toSavePos,audio_decoder->song_info.pcm_data_length*DecoderServiceCt.Channel);
					}
#endif
					if(DecoderServiceCt.Channel != audio_decoder->song_info.num_channels)//针对特殊歌曲，声道数可变
					{
						int16_t  i;
						if(audio_decoder->song_info.num_channels == 2)//DecoderServiceCt.Channel == 1 双合并为单
						{
							for(i = 0; i < audio_decoder->song_info.pcm_data_length; i++)
							{
								DecoderServiceCt.toSavePos[i] = (((int32_t)((DecoderServiceCt.toSavePos)[2 * i]) + ((DecoderServiceCt.toSavePos)[2 * i + 1])) / 2);
							}
						}
						else//单扩为双
						{
							for(i = audio_decoder->song_info.pcm_data_length - 1; i >= 0; i--)
							{
								DecoderServiceCt.toSavePos[2 * i + 1] = (DecoderServiceCt.toSavePos)[i];
								DecoderServiceCt.toSavePos[2 * i] = (DecoderServiceCt.toSavePos)[i];
							}
						}
					}
					DecoderServiceCt.decoderState = DecoderStateSavePcmData;

					if(DecoderServiceCt.pcmDataSize == 0)//add by sam, 20180814，增加一个容错处理，有客户歌曲，解码出来，length为0
					{
						DecoderServiceCt.decoderState = DecoderStateDecoding;
						SampleForRetry -= 1;//避免死机,反复解出0个数据时退出。
						break;
					}
				}

			case DecoderStateSavePcmData:
				{
					int32_t		savedSize;  //采样点值

					savedSize = SaveDecodedPcmData(DecoderServiceCt.toSavePos,
								(DecoderServiceCt.pcmDataSize - DecoderServiceCt.savedSize) / DecoderServiceCt.Channel);

					if(savedSize > 0)
					{
						DecoderServiceCt.savedSize += savedSize * DecoderServiceCt.Channel;
						DecoderServiceCt.toSavePos += savedSize * DecoderServiceCt.Channel;
						if(DecoderServiceCt.savedSize == DecoderServiceCt.pcmDataSize) //上次解码输出已全部 放入fifo
						{
							DecoderServiceCt.decoderState = DecoderStateDecoding;
							if(SampleForRetry > savedSize)
							{
								SampleForRetry -= savedSize;
								break;//避免单次解码数据太少，引起OS切换后断音，流和buf可用时，再次解码。
							}
						}
					}
					SampleForRetry = 0;
				}
				break;

			case DecoderStateDeinitializing:
				DecoderServiceCt.decoderState = DecoderStateNone;
				break;

			case DecoderStateStop:
				DecoderCnt = 0;
				DecoderStopProcess();
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_DECODER_STOPPED;
					MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
				}
				break;

			case DecoderStatePause:
				//APP_DBG("Decoder play Pause\n");
				//DecoderServiceCt.decoderState = DecoderStateNone;
				break;

			default:
				break;
		}
	}while(SampleForRetry && (DecoderServiceCt.decoderState == DecoderStateSavePcmData || DecoderServiceCt.decoderState == DecoderStateDecoding));

#ifdef CFG_FUNC_TSM_EN
	if(DecoderServiceCt.DecoderSpeed == 1)//加速解码
	{
		if(TsmLoop)
		{
			TsmLoop--;
			 goto _TsmLoop;
		}
	}
#endif ////#ifdef CFG_FUNC_TSM_EN
}


static int16_t SaveDecodedPcmData(int16_t * PcmData, uint16_t PcmDataLen)
{
	int32_t		TargetSampleRate = CFG_PARA_SAMPLE_RATE;
	uint32_t	ProcessSampleLen, SpaceSampleLen, SavedSampleLen = 0;
#ifdef CFG_FUNC_SOFT_ADJUST_IN
	int32_t		SRADoneaLen;//SRA后有效数据
#endif
	uint32_t	FillSpaceLen = 0;//已填入fifo数据量
#ifdef CFG_FUNC_MIXER_SRC_EN
	uint16_t	SRCOutLimit;
	int16_t SRCDoneLen; 			//SRC之后存放于outbuf的数据长度
#endif

#ifdef CFG_FUNC_TSM_EN
    uint8_t tsm_en ;

    if(gCtrlVars.tsm_unit.tsm_en && (!SoftFlagGet(SoftFlagDecoderRemind)))
    {
		if((GetSystemMode() == AppModeCardPlayBack)	|| (GetSystemMode() == AppModeUDiskPlayBack)
				                                    || (GetSystemMode() == AppModeCardAudioPlay)
				                                    || (GetSystemMode() == AppModeUDiskAudioPlay))
		{
    	   tsm_en = 1;
		}
		else
		{
			tsm_en = 0;
		}
    }
    else
    {
    	tsm_en = 0;
    }
#endif

	if(PcmData == NULL || PcmDataLen == 0 || DecoderServiceCt.DecoderPcmFifoMutex == NULL)
	{
		return -1;
	}
#if (BT_HFP_SUPPORT == ENABLE)
	if(GetSystemMode() == AppModeBtHfPlay)
	{
		TargetSampleRate = CFG_BTHF_PARA_SAMPLE_RATE;//通话模式采样率为16K
	}
#endif
#ifdef CFG_APP_USB_PHONE_MODE_EN
	if(GetSystemMode() == AppModeUsbPhone)
	{
		TargetSampleRate = 16000;//通话模式采样率为16K
	}
#endif
	osMutexLock(DecoderServiceCt.DecoderPcmFifoMutex);

#ifdef CFG_FUNC_TSM_EN
	if(tsm_en)
    {
		SpaceSampleLen = MCUCircular_GetSpaceLen(&DecoderServiceCt.TsmCircularBuf)/(DecoderServiceCt.Channel * 2) - 4;
    }
	else
#endif
	{
		SpaceSampleLen = MCUCircular_GetSpaceLen(&DecoderServiceCt.DecoderCircularBuf)/(DecoderServiceCt.Channel * 2) - 4;
	}

#ifdef CFG_FUNC_MIXER_SRC_EN
	SRCOutLimit =  ((SRC_OUT_BUF_LEN - 48) * audio_decoder->song_info.sampling_rate) / (TargetSampleRate * DecoderServiceCt.Channel * 2);

#ifndef BT_PROFILE_BQB_ENABLE
#if defined(CFG_FUNC_SOFT_ADJUST_IN) && defined(CFG_APP_BT_MODE_EN)
	if((GetSystemMode() == AppModeBtAudioPlay)
	&& !SoftFlagGet(SoftFlagDecoderRemind))
	{
		if(SpaceSampleLen < MAX_FRAME_SAMPLES + 20)
		{
			osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);
			//*RetryLen = 0;
			return 0;///空间不足先退出
		}
		ProcessSampleLen = MAX_FRAME_SAMPLES;
		if(ProcessSampleLen > SRCOutLimit)
		{
			ProcessSampleLen = SRCOutLimit;
		}
		//only debug
		if(PcmDataLen < 128)
		{
			APP_DBG("!!!err,SaveDecodedPcmData: %d\n",PcmDataLen);
			osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);
			return 0; //少于128会导致resampler_farrow_apply死机
		}

		if(SoftFlagGet(SoftFlagBtSra) && !SoftFlagGet(SoftFlagDecoderRemind))
		{
			SRADoneaLen = AudioSourceSRAProcess(PcmData, (uint16_t)ProcessSampleLen);
		}
		else
		{
			SRADoneaLen = ProcessSampleLen;
		}
		if(audio_decoder->song_info.sampling_rate != TargetSampleRate)//采样率和目标采样率不一致，需要转采样
		{
			SRCDoneLen = resampler_polyphase_apply(DecoderServiceCt.ResamplerCt, audioSourceAdjustCt->SraPcmOutnBuf, DecoderServiceCt.SRCOutBuf, SRADoneaLen);
			if(SRCDoneLen > 0)
			{
				MCUCircular_PutData(&DecoderServiceCt.DecoderCircularBuf, DecoderServiceCt.SRCOutBuf, SRCDoneLen * DecoderServiceCt.Channel * 2);
				FillSpaceLen += SRADoneaLen;
			}
		
		}
		else
		{
			MCUCircular_PutData(&DecoderServiceCt.DecoderCircularBuf, audioSourceAdjustCt->SraPcmOutnBuf, SRADoneaLen * DecoderServiceCt.Channel * 2);
			FillSpaceLen += SRADoneaLen;
		}
		SavedSampleLen = ProcessSampleLen;
	}
	else
#endif
#endif
	{
		if(audio_decoder->song_info.sampling_rate == TargetSampleRate)
		{
			ProcessSampleLen = SpaceSampleLen < PcmDataLen ? SpaceSampleLen : PcmDataLen;
#ifdef CFG_FUNC_TSM_EN
			if(tsm_en)
			{
				MCUCircular_PutData(&DecoderServiceCt.TsmCircularBuf, PcmData, ProcessSampleLen * DecoderServiceCt.Channel * 2);
			}
			else
#endif
			{
				MCUCircular_PutData(&DecoderServiceCt.DecoderCircularBuf, PcmData, ProcessSampleLen * DecoderServiceCt.Channel * 2);
			}
			FillSpaceLen = SavedSampleLen = ProcessSampleLen;
		}
		else if(SpaceSampleLen >= MAX_FRAME_SAMPLES)//下面需要转采样，避免转采样碎片化，space过小时等下次调用再存处理
		{
			do
			{
				ProcessSampleLen = ((SpaceSampleLen - FillSpaceLen - 1) * audio_decoder->song_info.sampling_rate) / TargetSampleRate;//预留一个偏差。;
				if(ProcessSampleLen >= PcmDataLen - SavedSampleLen) //buf 比 目标数据多
				{
					if(PcmDataLen - SavedSampleLen <= MAX_FRAME_SAMPLES)
					{
						ProcessSampleLen = PcmDataLen - SavedSampleLen;
					}
					else if(PcmDataLen - SavedSampleLen - MAX_FRAME_SAMPLES < MAX_FRAME_SAMPLES / 2) //避免残留孤点，残余做0.5~0.75帧处理。
					{
						ProcessSampleLen = (PcmDataLen - SavedSampleLen) / 2;
					}
					else
					{
						ProcessSampleLen = MAX_FRAME_SAMPLES;
					}
				}
				else // buf少于目标数据，适配space 处理
				{
					//if(PcmDataLen - SavedSampleLen >= MAX_FRAME_SAMPLES + MAX_FRAME_SAMPLES / 2)
					if(PcmDataLen - SavedSampleLen >= MAX_FRAME_SAMPLES)
					{
						if(ProcessSampleLen >= MAX_FRAME_SAMPLES)
						{
							ProcessSampleLen = MAX_FRAME_SAMPLES;
						}//else ProcessSampleLen保持 适配space 如果小于 space1/4会出循环
					}
					else
					{
						break;//space不足时先等等
					}
				}
				if(ProcessSampleLen > SRCOutLimit)//考虑SRCOut buf，8K转 44K之类，需要降低采样点数，避免溢出。
				{
					ProcessSampleLen = SRCOutLimit;
				}
				SRCDoneLen = resampler_polyphase_apply(DecoderServiceCt.ResamplerCt, PcmData + SavedSampleLen * DecoderServiceCt.Channel, DecoderServiceCt.SRCOutBuf, ProcessSampleLen);
				if(SRCDoneLen > 0)
				{
#ifdef CFG_FUNC_TSM_EN
				if(tsm_en)
				{
					MCUCircular_PutData(&DecoderServiceCt.TsmCircularBuf, DecoderServiceCt.SRCOutBuf, SRCDoneLen * DecoderServiceCt.Channel * 2);
				}
				else
#endif
				{
					MCUCircular_PutData(&DecoderServiceCt.DecoderCircularBuf, DecoderServiceCt.SRCOutBuf, SRCDoneLen * DecoderServiceCt.Channel * 2);
				}
					FillSpaceLen += SRCDoneLen;
				}
				SavedSampleLen += ProcessSampleLen;
			}while(SpaceSampleLen - FillSpaceLen >= MAX_FRAME_SAMPLES / 4 && PcmDataLen - SavedSampleLen >= MAX_FRAME_SAMPLES / 4 && audio_decoder->song_info.sampling_rate > TargetSampleRate);
		}//(FillSpaceLen  <= MAX_FRAME_SAMPLES) &&
	}
#else
	ProcessSampleLen = SpaceSampleLen < PcmDataLen ? SpaceSampleLen : PcmDataLen;
	MCUCircular_PutData(&DecoderServiceCt.DecoderCircularBuf, PcmData, ProcessSampleLen * DecoderServiceCt.Channel * 2);
	FillSpaceLen = SavedSampleLen = ProcessSampleLen;
#endif
	osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);
	return SavedSampleLen;
}

void DecoderStopProcess(void)
{
	APP_DBG("Decoder:play stop\n");
	DecoderServiceCt.DecoderLastPlayTime = 0;
	DecoderServiceCt.DecoderCurPlayTime = 0;
	DecoderServiceCt.DecoderSamples = 0;
	DecoderServiceCt.decoderState = DecoderStateNone;
}


/***************************************************************************************
 *
 * APIs
 *
 */
bool DecoderServiceCreate(MessageHandle parentMsgHandle, uint32_t BufSize, uint16_t FifoSize)
{
	bool		ret = TRUE;
	
	ret = DecoderService_Init(parentMsgHandle, BufSize, FifoSize);
	
	
	if(ret)
	{
		DecoderServiceCt.taskHandle = NULL;
		xTaskCreate(DecoderServiceEntrance,"DecoderService", DECODER_SERVICE_SIZE, NULL, DECODER_SERVICE_PRIO, &DecoderServiceCt.taskHandle);
		if(DecoderServiceCt.taskHandle == NULL)
		{
			ret = FALSE;
		}
	}
	if(!ret)
		APP_DBG("DecoderService create fail!\n");

	
	return ret;
}

void DecoderServiceStart(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}
	msgSend.msgId		= MSG_TASK_START;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

void DecoderServicePause(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}

	msgSend.msgId		= MSG_TASK_PAUSE;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

void DecoderServiceResume(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}
	msgSend.msgId		= MSG_TASK_RESUME;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

void DecoderServiceStop(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}

	msgSend.msgId		= MSG_TASK_STOP;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

void DecoderServiceKill(void)
{
	//AudioCoreSourceUnmute(DecoderSourecNum, TRUE, TRUE);//解码器通道恢复常态。注意:kill时可能是提示音通道而非app音源通道。
	SoftFlagDeregister(SoftFlagDecoderRun);//存在强行杀死 状态恢复。
	//PortFree... 
	if(DecoderServiceCt.DecoderPcmFifoMutex != NULL)
	{
		osMutexLock(DecoderServiceCt.DecoderPcmFifoMutex);
	}
	
	if(DecoderServiceCt.DecoderFifo != NULL)
	{
		osPortFree(DecoderServiceCt.DecoderFifo);
		DecoderServiceCt.DecoderFifo = NULL;
	}

	if(DecoderServiceCt.decoder_buf != NULL)
	{
		osPortFree(DecoderServiceCt.decoder_buf);
		DecoderServiceCt.decoder_buf = NULL;
	}
	if(DecoderServiceCt.DecoderPcmFifoMutex != NULL)
	{
		osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);
	}

#ifdef CFG_FUNC_TSM_EN
	if(DecoderServiceCt.TsmFifo != NULL)
	{
		osPortFree(DecoderServiceCt.TsmFifo);
		DecoderServiceCt.TsmFifo = NULL;
	}
#endif

	//task 先删任务，再删邮箱，收资源
	if(DecoderServiceCt.taskHandle != NULL)
	{
#ifdef SOFT_WACTH_DOG_ENABLE
		SWD_task_destroy(SWD_DecoderTask_ID);
#endif
		vTaskDelete(DecoderServiceCt.taskHandle);
		DecoderServiceCt.taskHandle = NULL;
	}

	//Msgbox
	if(DecoderServiceCt.msgHandle != NULL)
	{
		MessageDeregister(DecoderServiceCt.msgHandle);
		DecoderServiceCt.msgHandle = NULL;
	}

	if(DecoderServiceCt.DecoderPcmFifoMutex != NULL)
	{
		vSemaphoreDelete(DecoderServiceCt.DecoderPcmFifoMutex);
		DecoderServiceCt.DecoderPcmFifoMutex = NULL;
	}

#ifdef CFG_FUNC_MIXER_SRC_EN
	if(DecoderServiceCt.SRCOutBuf != NULL)
	{
		osPortFree(DecoderServiceCt.SRCOutBuf);
		DecoderServiceCt.SRCOutBuf = NULL;
	}

	if(DecoderServiceCt.ResamplerCt != NULL)
	{
		osPortFree(DecoderServiceCt.ResamplerCt);
		DecoderServiceCt.ResamplerCt = NULL;
	}
#endif	
}

//仅用于驱动解码器
void DecoderServiceMsg(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_NONE;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

MessageHandle GetDecoderServiceMsgHandle(void)
{
	return DecoderServiceCt.msgHandle;
}

TaskState GetDecoderServiceState(void)
{
	return DecoderServiceCt.serviceState;
}

int32_t DecoderInit(void *io_handle, int32_t ioType, int32_t decoderType)
{
		int32_t 	ret;//= DECODER_NORMAL;
		int32_t 	errCode;
		if(DecoderServiceCt.decoderState != DecoderStateNone && DecoderServiceCt.decoderState != DecoderStateInitialized)
		{
			APP_DBG("Decoder State Error :%d \n", DecoderServiceCt.decoderState);
			return RT_FAILURE;
		}
		
		audio_decoder = (AudioDecoderContext*)DecoderServiceCt.decoder_buf;
		ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt.decoder_buf, io_handle, ioType, decoderType);
		if(ret != RT_SUCCESS)
		{
			errCode = audio_decoder_get_error_code(audio_decoder);
#ifdef USE_DTS_DECODER
			if(decoderType == WAV_DECODER && errCode == -245)//WAV_ERROR_UNSUPPORTED_DTS_FORMAT,       /**< Unsupported DTS stream format  */
			{
				ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt.decoder_buf, io_handle, ioType, DTS_DECODER);
			}
			else
#endif
			if(ioType == IO_TYPE_FILE)
			{
				uint8_t FileHead[10];
				UINT Len;
				DecoderType Guess;
				f_lseek(io_handle, 0);
				f_read(io_handle, FileHead, 10, &Len);
				Guess = audio_decoder_guess_format(FileHead);
				if(Guess != UNKNOWN_DECODER && Guess != decoderType)
				{
					ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt.decoder_buf, io_handle, ioType, Guess);
				}
			}
		}
		if(audio_decoder_get_memory_size(decoderType) >= DecoderServiceCt.DecoderSize)
		{
			APP_DBG("Decodersize set error!!!!!!!");
			return RT_FAILURE;//安全监测
		}

        if(audio_decoder->song_info.num_channels > 2)
        {
            return RT_FAILURE;
        }

		if(ret == RT_SUCCESS)
		{
			DecoderServiceCt.decoderState = DecoderStateInitialized;
			//APP_DBG("[SONG_INFO]: ChannelCnt : %6d\n",		  (int)audio_decoder->song_info.num_channels);
			APP_DBG("[SONG_INFO]: SampleRate : %6d Hz\n",	  (int)audio_decoder->song_info.sampling_rate);
			//APP_DBG("[SONG_INFO]: BitRate	 : %6d Kbps\n",   (int)audio_decoder->song_info.bitrate / 1000);
			//APP_DBG("[SONG_INFO]: DecoderSize: %6d Bytes \n", (int)audio_decoder.decoder_size);
#ifndef LOSSLESS_DECODER_HIGH_RESOLUTION
			if(audio_decoder->song_info.stream_type != STREAM_TYPE_FLAC && audio_decoder->song_info.sampling_rate > 48000)
			{
				return RT_FAILURE;//读取数据速度受限，屏蔽部分高码率歌曲。
			}
#endif
			
#ifdef CFG_FUNC_MIXER_SRC_EN
			if(GetSystemMode() == AppModeBtHfPlay)
			{
				if(audio_decoder->song_info.sampling_rate != CFG_BTHF_PARA_SAMPLE_RATE)
				{
#if defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT == ENABLE) && (CFG_BT_RING_MODE >= RING_LOCAL_AND_PHONE)
					if(GetHfpState() == BT_HFP_STATE_INCOMING && !GetScoConnectFlag())
					{
						resampler_polyphase_init(DecoderServiceCt.ResamplerCt, audio_decoder->song_info.num_channels, RESAMPLER_POLYPHASE_SRC_RATIO_160_441);
					}
					else
#endif
					{
						resampler_polyphase_init(DecoderServiceCt.ResamplerCt, audio_decoder->song_info.num_channels, RESAMPLER_POLYPHASE_SRC_RATIO_160_441);
					}
				}
			}
			else if(GetSystemMode() == AppModeUsbPhone)
			{
				if(audio_decoder->song_info.sampling_rate != 16000)
				{
					resampler_polyphase_init(DecoderServiceCt.ResamplerCt, audio_decoder->song_info.num_channels, RESAMPLER_POLYPHASE_SRC_RATIO_160_441);
				}
			}
			else
			{
				if (audio_decoder->song_info.sampling_rate != CFG_PARA_SAMPLE_RATE)
				{
					resampler_polyphase_init(DecoderServiceCt.ResamplerCt, audio_decoder->song_info.num_channels, Get_Resampler_Polyphase(audio_decoder->song_info.sampling_rate));
				}
			}
#endif
		}
		else
		{
			//int32_t 	errCode;
			errCode = audio_decoder_get_error_code(audio_decoder);
			APP_DBG("AudioDecoder init err code = %d\n", (int)errCode);
		}

		return ret;
}

void DecoderDeinit(void)
{
	DecoderServiceCt.decoderState = DecoderStateDeinitializing;
}


/**
 * @func        DecodedPcmDataGet
 * @brief       Decoder数据流输出的 拉流API
 * @param       void * pcmData
                uint16_t sampleLen  
 * @Output      None
 * @return      uint16_t 取出的采样点数
 * @Others      数据区需要锁定，指定采样点数取出数据，不足时，按实际长度取数据。
 * Record
 */
uint16_t DecodedPcmDataGet(void * pcmData, uint16_t sampleLen)
{
	int16_t		dataSize;
	uint32_t	getSize;
	uint32_t 	SpacLen = 0;
	uint32_t 	ValLen = 0;

	if(DecoderServiceCt.DecoderPcmFifoMutex == NULL)
	{
		return 0;
	}

	if(DecoderServiceCt.serviceState != TaskStateRunning)
	{
		return 0;
	}

	getSize = sampleLen * 2 * DecoderServiceCt.Channel;//采样点数 * 2byte深度* 通道数
	if(getSize == 0)
	{
		return 0;
	}

	osMutexLock(DecoderServiceCt.DecoderPcmFifoMutex);
	SpacLen = MCUCircular_GetSpaceLen(&DecoderServiceCt.DecoderCircularBuf);

	ValLen = DecoderServiceCt.DecoderFifoSize - SpacLen;

	if(getSize > ValLen)
	{
		getSize = ValLen;
	}

	dataSize = MCUCircular_GetData(&DecoderServiceCt.DecoderCircularBuf, pcmData, getSize);
	osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);

	return dataSize / (2 * DecoderServiceCt.Channel);
}

/**
 * @func        DecodedPcmDataLenGet
 * @brief       Decoder数据输出FIFO内采样点数
 * @Output      None
 * @return      uint16_t FIFO中采样点数
 * Record
 */
uint16_t DecodedPcmDataLenGet(void)
{
	if(DecoderServiceCt.DecoderPcmFifoMutex == NULL || DecoderServiceCt.serviceState != TaskStateRunning)
	{
		return 0;
	}
	return MCUCircular_GetDataLen(&DecoderServiceCt.DecoderCircularBuf) / (2 * DecoderServiceCt.Channel);
}

SongInfo * GetSongInfo(void)
{
	//return &(audio_decoder->song_info);
	return audio_decoder_get_song_info(audio_decoder);
}

//解码器工作
void DecoderPlay(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}

	msgSend.msgId		= MSG_DECODER_PLAY;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

//解码器复位
void DecoderReset(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}

	msgSend.msgId		= MSG_DECODER_RESET;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

//解码器停止
void DecoderStop(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}
	msgSend.msgId		= MSG_DECODER_STOP;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

//解码器停止
void DecoderMuteAndStop(void)
{
	//MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}

	AudioCoreSourceMute(DecoderSourecNum, TRUE, TRUE);
	vTaskDelay(20);
	DecoderStop();
}

void DecoderPause(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}
	if(DecoderServiceCt.decoderState <= DecoderStatePause)
	{
		return;
	}

	msgSend.msgId		= MSG_DECODER_PAUSE;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

void DecoderResume(void)
{
	MessageContext		msgSend;
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}
	APP_DBG("Decoder Resume()\n");

	msgSend.msgId		= MSG_DECODER_RESUME;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

//注意，StepTime 单位为毫秒，实际传递值为整数秒
void DecoderFF(uint32_t StepTime)
{
	MessageContext		msgSend;

	APP_DBG("Decoder FF()\n");
	APP_DBG("Decoder state = %d\n", DecoderServiceCt.decoderState);
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}
	if(DecoderServiceCt.decoderState <= DecoderStatePause)//Playing
	{
		return;
	}

	DecoderServiceCt.StepTime = StepTime;

	msgSend.msgId		= MSG_DECODER_FF;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

//注意，StepTime 单位为毫秒，实际传递值为整数秒
void DecoderFB(uint32_t StepTime)
{
	MessageContext		msgSend;

	APP_DBG("Decoder FB()\n");
	if(DecoderServiceCt.msgHandle==NULL)
	{	
		return;
	}

	if(DecoderServiceCt.decoderState <= DecoderStatePause)//Playing
	{
		return;
	}

	DecoderServiceCt.StepTime = StepTime;

	msgSend.msgId		= MSG_DECODER_FB;
	MessageSend(DecoderServiceCt.msgHandle, &msgSend);
}

//解码器seek 文件播放时间 秒，因不同格式情况，精度不定。
bool DecoderSeek(uint32_t Time)
{
	APP_DBG("DecoderTaskSeek()\n");
	DecoderServiceCt.DecoderSamples = 0;
	DecoderServiceCt.DecoderCurPlayTime = 0;
	if(audio_decoder_seek(audio_decoder, Time * 1000) != RT_SUCCESS)
	{
		return FALSE;
	}
	DecoderServiceCt.DecoderCurPlayTime = Time;
	return TRUE;
}

uint32_t DecoderServicePlayTimeGet(void)
{
 	return DecoderServiceCt.DecoderCurPlayTime;
}

uint32_t GetDecoderState(void)
{
	return DecoderServiceCt.decoderState;
}

//此变量作为全局获取，不要为空。
uint8_t DecoderSourceNumGet(void)
{
	return DecoderSourecNum;
}

void DecoderSourceNumSet(uint8_t Num)
{
	DecoderSourecNum = Num;
}


static struct
{
	uint8_t 	InitFlag;
	uint8_t 	State;
	uint16_t 	ErrCnt;
}BtDecoder = {0};

void BtDecoderDeinit(void)
{
	BtDecoder.InitFlag  = 0;
	BtDecoder.ErrCnt 	= 0;
	BtDecoder.State		= 0;
}

uint16_t PcmDataLenCnt = 0;

static void BtDecodedPcmData(void)
{
	uint16_t SampleForRetry = CFG_PARA_MAX_SAMPLES_PER_FRAME;//单次解码的目标解码采样点数量。

	if(SoftFlagGet(SoftFlagDecoderRemind) || !BtDecoder.InitFlag)
	{
		return;
	}

	do
	{
	switch(BtDecoder.State)
	{
		default:
			BtDecoder.State = 0;
		case 0:
			if(BtDecoder.ErrCnt > 100)
			{
				MessageContext		msgSend;
				msgSend.msgId = MSG_DECODER_SERVICE_DISK_ERROR;
				MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
				BtDecoderDeinit();
				APP_DBG("------------------BtDecoder Reset!!!!!!\n");
				return;
			}
			if(RT_SUCCESS != audio_decoder_can_continue(audio_decoder))
			{
				//APP_DBG("BtDecoder Unable to continue!!!  %ld\n",BtDecoderErrCnt);
				if(BtDecoder.ErrCnt > 0)
					BtDecoder.ErrCnt++;
				return;
			}
			if(audio_decoder->decoder_type == SBC_DECODER && GetValidSbcDataSize() <= SBC_DECODER_FIFO_MIN)
			{
				//蓝牙模式下SBC解码,需要判断decoder_mem水位,避免数据太少引起解码错误。
				//APP_DBG("SBC empty: %lu\n",GetValidSbcDataSize());
				return;
			}
			if(audio_decoder->decoder_type == AAC_DECODER)
			{
#ifdef CFG_APP_BT_MODE_EN
				if(btManager.aacFrameNumber == 0)
				{
					//APP_DBG("AAC empty\n");
					return;
				}
				else
					btManager.aacFrameNumber--;
#endif
			}
			if(audio_decoder_decode(audio_decoder) == RT_SUCCESS)
			{
				DecoderServiceCt.pcmDataSize	= audio_decoder->song_info.pcm_data_length * DecoderServiceCt.Channel;
				DecoderServiceCt.savedSize		= 0;
				DecoderServiceCt.toSavePos		= (int16_t *)audio_decoder->song_info.pcm_addr;	
				BtDecoder.State = 1;
				BtDecoder.ErrCnt = 0;
			}
			else
			{
				int32_t 	errCode;
				errCode = audio_decoder_get_error_code(audio_decoder);
				if(  ((audio_decoder->decoder_type == SBC_DECODER) && errCode != -123)  //stream empty
				   ||((audio_decoder->decoder_type == AAC_DECODER) && errCode != -127))
				{
					APP_DBG("BtDecoder err code = %d\n", (int)errCode);
					if(audio_decoder->decoder_type == SBC_DECODER)
					{
						MessageContext		msgSend;
						msgSend.msgId = MSG_DECODER_SERVICE_DISK_ERROR;
						MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
						BtDecoderDeinit();
						return;
					}
				}
				SampleForRetry = 0;
				BtDecoder.ErrCnt++;
				break;
			}
		case 1:
			while(1)
			{
				int32_t		savedSize;  //采样点值
				savedSize = SaveDecodedPcmData(DecoderServiceCt.toSavePos,
						(DecoderServiceCt.pcmDataSize - DecoderServiceCt.savedSize) / DecoderServiceCt.Channel);
				if(savedSize > 0)
				{
					PcmDataLenCnt = 0;
					DecoderServiceCt.savedSize += savedSize * DecoderServiceCt.Channel;
					DecoderServiceCt.toSavePos += savedSize * DecoderServiceCt.Channel;
					if(SampleForRetry > savedSize)
					{
						SampleForRetry -= savedSize;
					}
					else
						SampleForRetry = 0;
					if(DecoderServiceCt.savedSize == DecoderServiceCt.pcmDataSize) //上次解码输出已全部 放入fifo
					{
						BtDecoder.State = 0;
						break;
					}
				}
				else
				{
					SampleForRetry = 0;
					if (PcmDataLenCnt++ > 100)
					{
						PcmDataLenCnt = 0;
						MessageContext		msgSend;
						msgSend.msgId = MSG_DECODER_SERVICE_DISK_ERROR;
						MessageSend(DecoderServiceCt.parentMsgHandle, &msgSend);
						BtDecoderDeinit();
						APP_DBG("------------------BtDecoder Reset2!!!!!!\n");
					}
					break;
				}
			}
			break;
	}
	}while(SampleForRetry);
}

int32_t BtDecoderInit(void *io_handle,int32_t decoder_type)
{
	int32_t 	ret;//= DECODER_NORMAL;
	int32_t 	errCode;

	audio_decoder = (AudioDecoderContext*)DecoderServiceCt.decoder_buf;
	ret = audio_decoder_initialize((uint8_t*)DecoderServiceCt.decoder_buf, io_handle, (int32_t)IO_TYPE_MEMORY, decoder_type);
	
	if(audio_decoder_get_memory_size(decoder_type) >= DecoderServiceCt.DecoderSize)
	{
		APP_DBG("Decodersize set error!!!!!!!");
		return RT_FAILURE;//安全监测
	}
	if(ret == RT_SUCCESS)
	{
		APP_DBG("[SONG_INFO]: ChannelCnt : %6d\n",		  (int)audio_decoder->song_info.num_channels);
		APP_DBG("[SONG_INFO]: SampleRate : %6d Hz\n",	  (int)audio_decoder->song_info.sampling_rate);

		MCUCircular_Config(&DecoderServiceCt.DecoderCircularBuf, DecoderServiceCt.DecoderFifo, DecoderServiceCt.DecoderFifoSize);
		DecoderServiceCt.Channel = audio_decoder->song_info.num_channels;//配置本地fifo 声道数（init后固定不变）
		if (audio_decoder->song_info.sampling_rate != CFG_PARA_SAMPLE_RATE)
		{
#ifdef CFG_FUNC_MIXER_SRC_EN
			resampler_polyphase_init(DecoderServiceCt.ResamplerCt, audio_decoder->song_info.num_channels, Get_Resampler_Polyphase(audio_decoder->song_info.sampling_rate));
#endif
		}
		BtDecoder.InitFlag = 1;
		BtDecoder.State	   = 0;
	}
	else
	{
		//int32_t 	errCode;
		errCode = audio_decoder_get_error_code(audio_decoder);
		APP_DBG("BtAACDecoderInit init err code = %d\n", (int)errCode);
	}
	return ret;
}

uint16_t BtDecodedPcmDataGet(void * pcmData, uint16_t sampleLen)
{
	int16_t		dataSize;
	uint32_t	getSize;
	uint32_t 	SpacLen = 0;
	uint32_t 	ValLen = 0;

	if(SoftFlagGet(SoftFlagDecoderRemind))
		return 0;
	BtDecodedPcmData();
	getSize = sampleLen * 2 * DecoderServiceCt.Channel;//采样点数 * 2byte深度* 通道数
	if(getSize == 0 || DecoderServiceCt.DecoderPcmFifoMutex == NULL || !BtDecoder.InitFlag)
	{
		return 0;
	}

	osMutexLock(DecoderServiceCt.DecoderPcmFifoMutex);
	SpacLen = MCUCircular_GetSpaceLen(&DecoderServiceCt.DecoderCircularBuf);

	ValLen = DecoderServiceCt.DecoderFifoSize - SpacLen;

	if(getSize > ValLen)
	{
		getSize = ValLen;
	}

	dataSize = MCUCircular_GetData(&DecoderServiceCt.DecoderCircularBuf, pcmData, getSize);
	osMutexUnlock(DecoderServiceCt.DecoderPcmFifoMutex);

	return dataSize / (2 * DecoderServiceCt.Channel);
}

uint16_t BtDecodedPcmDataLenGet(void)
{
	if(SoftFlagGet(SoftFlagDecoderRemind) || !BtDecoder.InitFlag)
		return 0;

	return MCUCircular_GetDataLen(&DecoderServiceCt.DecoderCircularBuf) / (2 * DecoderServiceCt.Channel);
}

#ifdef LOSSLESS_DECODER_HIGH_RESOLUTION
void AudioCorePcmDataBitWidthConv(int32_t *PcmBuf,uint16_t dataSize)
{
	uint32_t n;
	int32_t	*PcmBuf32 = (int32_t *)PcmBuf;
	int16_t	*PcmBuf16 = (int16_t *)PcmBuf;

//	if(BitWidth == 24) //16bit转成24bit
//	{
//		for(n = dataSize; n > 0 ; n--)
//		{
//			PcmBuf32[n - 1] = PcmBuf16[n - 1] << 8;
//		}
//	}
//	else	//24bit转成16bit
	{
		for(n=0; n < dataSize; n++)
		{
			PcmBuf16[n] = (PcmBuf32[n] >> 8)&0xffff;
		}
//
//		for(n=0; n < dataSize/2; n++)
//		{
//			PcmBuf16[n+1] = PcmBuf16[n+2];
//		}
	}
}
#endif
/*****************************************
 *
 *
 *******************************************/
#ifdef CFG_FUNC_TSM_EN
void DecoderTsmProcess(void)
{
	uint16_t DecoderFifoSpace;
	uint16_t DecoderDataLen;
	uint16_t TsmDataLen;

	//-------------------------------//
	if(gCtrlVars.tsm_unit.tsm_en)
	{
		DecoderFifoSpace = MCUCircular_GetSpaceLen(&DecoderServiceCt.DecoderCircularBuf)/4;
		DecoderDataLen = MCUCircular_GetDataLen(&DecoderServiceCt.DecoderCircularBuf)/4;
		TsmDataLen = MCUCircular_GetDataLen(&DecoderServiceCt.TsmCircularBuf)/4;
		//DBG("Tsm:%d\n",TsmDataLen);
		//DBG("Dec:%d\n",DecoderDataLen);

		if((DecoderFifoSpace > gCtrlVars.tsm_unit.out_size)&&(TsmDataLen>= gCtrlVars.tsm_unit.in_size))
		{
			MCUCircular_GetData(&DecoderServiceCt.TsmCircularBuf, gCtrlVars.tsm_unit.in_buf, gCtrlVars.tsm_unit.in_size*4);

			AudioEffectTsmApply(gCtrlVars.tsm_unit.in_buf,gCtrlVars.tsm_unit.out_buf);

			MCUCircular_PutData(&DecoderServiceCt.DecoderCircularBuf, gCtrlVars.tsm_unit.out_buf, gCtrlVars.tsm_unit.out_size*4);

			TsmDataLen = MCUCircular_GetDataLen(&DecoderServiceCt.TsmCircularBuf)/4;

			if(gCtrlVars.tsm_unit.ct->sa < gCtrlVars.tsm_unit.ct->ss)////慢播放
			{
				if(TsmDataLen >= gCtrlVars.tsm_unit.in_size*4)
				{
					DecoderServiceCt.DecoderSpeed = -1;
				}
				else
				{
					DecoderServiceCt.DecoderSpeed = 0;
				}
			}
			else if(gCtrlVars.tsm_unit.ct->sa < gCtrlVars.tsm_unit.ct->ss)////快播放
			{
				if(TsmDataLen > gCtrlVars.tsm_unit.in_size*4)
				{
					DecoderServiceCt.DecoderSpeed = -1;
				}
				else if(TsmDataLen < gCtrlVars.tsm_unit.in_size*2)
				{
					DecoderServiceCt.DecoderSpeed = 1;
				}
				else
				{
					DecoderServiceCt.DecoderSpeed = 0;
				}
			}
			else //正常
			{
				DecoderServiceCt.DecoderSpeed = 0;
			}
		}

		DecoderDataLen = MCUCircular_GetDataLen(&DecoderServiceCt.DecoderCircularBuf)/4;
	}
	else
	{
		DecoderServiceCt.DecoderSpeed = 0;
	}
}
#endif ///CFG_FUNC_TSM_EN
