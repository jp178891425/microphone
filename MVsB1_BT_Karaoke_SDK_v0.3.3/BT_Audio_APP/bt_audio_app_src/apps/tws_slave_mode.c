/**
 **************************************************************************************
 * @file    tws_slave_mode.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2020-3-6 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include <nds32_intrinsic.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "rtos_api.h"
#include "app_message.h"
#include "app_config.h"
#include "debug.h"
#include "delay.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "decoder_service.h"
#include "remind_sound_service.h"
#include "recorder_service.h"
#include "main_task.h"
#include "audio_effect.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "backup_interface.h"
#include "breakpoint.h"
#include "otg_device_hcd.h"
#include "otg_device_standard_request.h"
#include "audio_vol.h"
#include "ctrlvars.h"
#include "mode_switch_api.h"
#include "reset.h"
#include "bb_api.h"
#include "bt_manager.h"
#include "bt_tws_api.h"
#include "tws_slave_mode.h"
#include "bt_stack_service.h"

extern uint32_t tws_play;
extern uint8_t tws_get_data_source(void);
void AudioEffectModeSel(uint16_t mode, uint8_t init_flag);

#ifdef BT_TWS_SUPPORT
#define TWS_SLAVE_PLAY_TASK_STACK_SIZE		512
#define TWS_SLAVE_PLAY_TASK_PRIO			3
#define TWS_SLAVE_NUM_MESSAGE_QUEUE			10

#define TWS_SLAVE_SOURCE_NUM				APP_SOURCE_NUM

typedef struct _TwsSlavePlayContext
{
	xTaskHandle 		taskHandle;
	MessageHandle		msgHandle;
	MessageHandle		parentMsgHandle;

	TaskState			state;
	uint8_t				runflag;
	uint8_t				umuteflag;
	TIMER				umuteTime;
	uint32_t			TwsBufMuteTimeout;//主机发起dac mute时，从机需要mute 直至fifo播放完毕， ==0 时未开启
	uint32_t			*ADCFIFO;			//ADC的DMA循环fifo
	uint16_t 			*Source1Buf_TwsSlave;	//ADC 取LineIn数据
	AudioCoreContext 	*AudioCoreTwsSlave;
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	uint16_t*			SourceDecoder;
	TaskState			DecoderSync;
#endif
	//play
	uint32_t 			SampleRate; //带提示音时，如果不重采样，要避免采样率配置冲突

#ifdef CFG_FUNC_RECORDER_EN
	TaskState			RecorderSync;
	TaskState			EncoderSync;
#endif

	uint32_t			twsSlaveLinkTimeout;
	uint32_t			twsSlaveDiscTimeout;//断开后等待时间确认是主动断开还是linkloss
}TwsSlavePlayContext;

/**根据appconfig缺省配置:DMA 8个通道配置**/
/*1、cec需PERIPHERAL_ID_TIMER3*/
/*2、SD卡录音需PERIPHERAL_ID_SDIO RX/TX*/
/*3、在线串口调音需PERIPHERAL_ID_UART1 RX/TX,建议使用USB HID，节省DMA资源*/
/*4、线路输入需PERIPHERAL_ID_AUDIO_ADC0_RX*/
/*5、Mic开启需PERIPHERAL_ID_AUDIO_ADC1_RX，mode之间通道必须一致*/
/*6、Dac0开启需PERIPHERAL_ID_AUDIO_DAC0_TX mode之间通道必须一致*/
/*7、DacX需开启PERIPHERAL_ID_AUDIO_DAC1_TX mode之间通道必须一致*/
/*注意DMA 8个通道配置冲突:*/
/*a、UART在线调音和DAC-X有冲突,默认在线调音使用USB HID*/
/*b、UART在线调音与HDMI/SPDIF模式冲突*/
static const uint8_t DmaChannelMap[29] = {
	255,//PERIPHERAL_ID_SPIS_RX = 0,	//0
	255,//PERIPHERAL_ID_SPIS_TX,		//1
	255,//PERIPHERAL_ID_TIMER3,			//2
	4,//PERIPHERAL_ID_SDIO_RX,			//3
	4,//PERIPHERAL_ID_SDIO_TX,			//4
	255,//PERIPHERAL_ID_UART0_RX,		//5
	255,//PERIPHERAL_ID_TIMER1,			//6
	255,//PERIPHERAL_ID_TIMER2,			//7
	255,//PERIPHERAL_ID_SDPIF_RX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SDPIF_TX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SPIM_RX,		//9
	255,//PERIPHERAL_ID_SPIM_TX,		//10
	255,//PERIPHERAL_ID_UART0_TX,		//11
	
#ifdef CFG_COMMUNICATION_BY_UART	
	7,//PERIPHERAL_ID_UART1_RX,			//12
	6,//PERIPHERAL_ID_UART1_TX,			//13
#else
	255,//PERIPHERAL_ID_UART1_RX,		//12
	255,//PERIPHERAL_ID_UART1_TX,		//13
#endif

	255,//PERIPHERAL_ID_TIMER4,			//14
	255,//PERIPHERAL_ID_TIMER5,			//15
	255,//PERIPHERAL_ID_TIMER6,			//16
	0,//PERIPHERAL_ID_AUDIO_ADC0_RX,	//17
	1,//PERIPHERAL_ID_AUDIO_ADC1_RX,	//18
	2,//PERIPHERAL_ID_AUDIO_DAC0_TX,	//19
	3,//PERIPHERAL_ID_AUDIO_DAC1_TX,	//20
	255,//PERIPHERAL_ID_I2S0_RX,		//21
#if	(defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S_PORT==0))
	7,//PERIPHERAL_ID_I2S0_TX,			//22
#else	
	255,//PERIPHERAL_ID_I2S0_TX,		//22
#endif	
	255,//PERIPHERAL_ID_I2S1_RX,		//23
#if	(defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S_PORT==1))
	7,	//PERIPHERAL_ID_I2S1_TX,		//24
#else
	255,//PERIPHERAL_ID_I2S1_TX,		//24
#endif
	255,//PERIPHERAL_ID_PPWM,			//25
	255,//PERIPHERAL_ID_ADC,     		//26
	255,//PERIPHERAL_ID_SOFTWARE,		//27
};

static  TwsSlavePlayContext*		TwsSlavePlayCt;
#ifdef DEBUG_TWS_PACKET
uint8_t 	DebugTwsState = DEBUG_TWS_PACKET;
uint32_t	DebugTwsDataCount = 0;
#if (DEBUG_TWS_PACKET == DEBUG_TWS_DATARATE) || (DEBUG_TWS_PACKET == DEBUG_TWS_DATA_BANDWIDTH)
uint32_t	DebugTwsDataMax = 0;
uint32_t	DebugTwsDataMin = 0xffffffff;
uint32_t	DebugTwsDataTotal = 0;
uint16_t	DebugTwsTime	= DEBUG_SPEED_WIN;
bool		DebugTwsLog		= FALSE;
#endif
#endif

static void TwsSlavePlayModeCreating(uint16_t msgId);
static void TwsSlavePlayModeStarting(uint16_t msgId);
static void TwsSlavePlayModeStopping(uint16_t msgId);
static void TwsSlavePlayModeStopped(void);
static void TwsSlavePlayRunning(uint16_t msgId);

uint16_t TwsSlavePlayDataGet(void* Buf, uint16_t Len)
{
	memset(Buf, 0, Len);
	return Len;
}

void TwsSlavePlayResRelease(void)
{
	if(TwsSlavePlayCt->Source1Buf_TwsSlave != NULL)
	{
		APP_DBG("Source1Buf_TwsSlave\n");
		osPortFree(TwsSlavePlayCt->Source1Buf_TwsSlave);
		TwsSlavePlayCt->Source1Buf_TwsSlave = NULL;
	}
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(TwsSlavePlayCt->SourceDecoder != NULL)
	{
		APP_DBG("SourceDecoder\n");
		osPortFree(TwsSlavePlayCt->SourceDecoder);
		TwsSlavePlayCt->SourceDecoder = NULL;
	}
#endif
}

bool TwsSlavePlayResMalloc(uint16_t SampleLen)
{
	//InCore1 buf
	TwsSlavePlayCt->Source1Buf_TwsSlave = (uint16_t*)osPortMallocFromEnd(SampleLen * 2 * 2);
	if(TwsSlavePlayCt->Source1Buf_TwsSlave == NULL)
	{
		return FALSE;
	}

	//InCore buf
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	TwsSlavePlayCt->SourceDecoder = (uint16_t*)osPortMallocFromEnd(SampleLen * 2 * 2);//One Frame
	if(TwsSlavePlayCt->SourceDecoder == NULL)
	{
		return FALSE;
	}
	memset(TwsSlavePlayCt->SourceDecoder, 0, SampleLen * 2 * 2);//2K
#endif

	return TRUE;
}

void TwsSlavePlayResInit(void)
{
	if(TwsSlavePlayCt->Source1Buf_TwsSlave != NULL)
	{
		TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].PcmInBuf = (int16_t *)TwsSlavePlayCt->Source1Buf_TwsSlave;
	}
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(TwsSlavePlayCt->SourceDecoder != NULL)
	{
		TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[REMIND_SOURCE_NUM].PcmInBuf = (int16_t *)TwsSlavePlayCt->SourceDecoder;
	}
#endif
}

/**
 * @func        TwsSlavePlay_Init
 * @brief       TwsSlave模式参数配置，资源初始化
 * @param       MessageHandle parentMsgHandle
 * @Output      None
 * @return      bool
 * @Others      任务块、Adc、Dac、AudioCore配置
 * @Others      数据流从Adc到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 */
static bool TwsSlavePlay_Init(MessageHandle parentMsgHandle)
{
//System config
	DMA_ChannelAllocTableSet((uint8_t *)DmaChannelMap);//TwsSlave
//Task & App Config
	TwsSlavePlayCt = (TwsSlavePlayContext*)osPortMalloc(sizeof(TwsSlavePlayContext));
	if(TwsSlavePlayCt == NULL)
	{
		return FALSE;
	}
	memset(TwsSlavePlayCt, 0, sizeof(TwsSlavePlayContext));
	TwsSlavePlayCt->msgHandle = MessageRegister(TWS_SLAVE_NUM_MESSAGE_QUEUE);
	if(TwsSlavePlayCt->msgHandle == NULL)
	{
		return FALSE;
	}
	TwsSlavePlayCt->parentMsgHandle = parentMsgHandle;
	TwsSlavePlayCt->state = TaskStateCreating;
	TwsSlavePlayCt->SampleRate = CFG_PARA_SAMPLE_RATE;

	if(!TwsSlavePlayResMalloc(mainAppCt.SamplesPreFrame))
	{
		APP_DBG("TwsSlavePlay Res Error!\n");
		return FALSE;
	}

	
	//Core Source1 para
	TwsSlavePlayCt->AudioCoreTwsSlave = (AudioCoreContext*)&AudioCore;
	//Audio init
//	//note Soure0.和sink0已经在main app中配置，不要随意配置
	//Core Soure1.Para
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].Enable = 0;

#if (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN3)
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].FuncDataGet = AudioADC1DataGet;
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].FuncDataGetLen = AudioADC1DataLenGet;
#else
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].FuncDataGet = TwsSlavePlayDataGet;
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].FuncDataGetLen = NULL;//AudioADC0DataLenGet;
#endif

	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].IsSreamData = FALSE;
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].PcmFormat = 2;//TwsSlave stereo
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[TWS_SLAVE_SOURCE_NUM].PcmInBuf = (int16_t *)TwsSlavePlayCt->Source1Buf_TwsSlave;

	//Core Soure Para
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	DecoderSourceNumSet(REMIND_SOURCE_NUM);
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[REMIND_SOURCE_NUM].Enable = 0;
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[REMIND_SOURCE_NUM].FuncDataGet = DecodedPcmDataGet;
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[REMIND_SOURCE_NUM].FuncDataGetLen = NULL;
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[REMIND_SOURCE_NUM].IsSreamData = FALSE;//Decoder
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[REMIND_SOURCE_NUM].PcmFormat = 2; //stereo
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioSource[REMIND_SOURCE_NUM].PcmInBuf = (int16_t *)TwsSlavePlayCt->SourceDecoder;
#endif

	//Core Process	
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
    #ifdef CFG_FUNC_MIC_KARAOKE_EN
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioEffectProcess = (AudioCoreProcessFunc)AudioEffectProcess;
	#else
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioEffectProcess = (AudioCoreProcessFunc)AudioMusicProcess;
	#endif
#else
	TwsSlavePlayCt->AudioCoreTwsSlave->AudioEffectProcess = (AudioCoreProcessFunc)AudioBypassProcess;	
#endif

#ifdef CFG_FUNC_RECORDER_EN
	TwsSlavePlayCt->RecorderSync = TaskStateNone;
	TwsSlavePlayCt->EncoderSync = TaskStateNone;
#endif

	TwsSlavePlayCt->runflag = 0;
	TwsSlavePlayCt->umuteflag = 0;
	
	AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);

#ifdef BT_TWS_SUPPORT
	AudioCoreSourceMute(TWS_SOURCE_NUM, TRUE, TRUE);
#endif

	if(IsAudioPlayerMute() == FALSE)
	{
		AudioPlayerMute();
	}
	TwsSlavePlayCt->TwsBufMuteTimeout = 0;
	return TRUE;
}

//创建从属services
static void TwsSlavePlayModeCreate(void)
{
	bool NoService = TRUE;
	
	// Create service task
#if defined(CFG_FUNC_REMIND_SBC)
	DecoderServiceCreate(TwsSlavePlayCt->msgHandle, DECODER_BUF_SIZE_SBC, DECODER_FIFO_SIZE_FOR_SBC);//提示音格式决定解码器内存消耗
	NoService = FALSE;
#elif defined(CFG_FUNC_REMIND_SOUND_EN)
	DecoderServiceCreate(TwsSlavePlayCt->msgHandle, DECODER_BUF_SIZE_MP3, DECODER_FIFO_SIZE_FOR_MP3);
	NoService = FALSE;
#endif
	if(NoService)
	{
		TwsSlavePlayModeCreating(MSG_NONE);
	}
}

//All of services is created
//Send CREATED message to parent
static void TwsSlavePlayModeCreating(uint16_t msgId)
{
	MessageContext		msgSend;
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(msgId == MSG_DECODER_SERVICE_CREATED)
	{
		TwsSlavePlayCt->DecoderSync = TaskStateReady;
	}
	if(TwsSlavePlayCt->DecoderSync == TaskStateReady)
#endif
	{
		msgSend.msgId		= MSG_LINE_AUDIO_MODE_CREATED;
		MessageSend(TwsSlavePlayCt->parentMsgHandle, &msgSend);
		TwsSlavePlayCt->state = TaskStateReady;
	}
}

static void TwsSlavePlayModeStart(void)
{
	bool NoService = TRUE;
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	DecoderServiceStart();
	TwsSlavePlayCt->DecoderSync = TaskStateStarting;
	NoService = FALSE;
#endif
	if(NoService)
	{
		TwsSlavePlayModeStarting(MSG_NONE);
	}
	else
	{
		TwsSlavePlayCt->state = TaskStateStarting;
	}
}

static void TwsSlavePlayModeStarting(uint16_t msgId)
{
	MessageContext		msgSend;
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(msgId == MSG_DECODER_SERVICE_STARTED)
	{
		TwsSlavePlayCt->DecoderSync = TaskStateRunning;
	}
	if(TwsSlavePlayCt->DecoderSync == TaskStateRunning)
#endif
	{
		msgSend.msgId		= MSG_LINE_AUDIO_MODE_STARTED;
		MessageSend(TwsSlavePlayCt->parentMsgHandle, &msgSend);

		TwsSlavePlayCt->state = TaskStateRunning;

#ifndef CFG_FUNC_REMIND_SOUND_EN			
		{
			TwsSlavePlayCt->runflag = 1;
			TwsSlavePlayCt->umuteflag = 0;
			TimeOutSet(&TwsSlavePlayCt->umuteTime, 200);
		}
#else
		if(!SoftFlagGet(SoftFlagRemindMask))
		{
			if(!RemindSoundServiceItemRequest(NO_REMIND,REMIND_ATTR_NORMAL)) //插播提示音
			{
				APP_DBG("Tws Slave No Sound\n");
				TwsSlavePlayCt->runflag = 1;
				TwsSlavePlayCt->umuteflag = 0;
				TimeOutSet(&TwsSlavePlayCt->umuteTime, 200);
			}
		}
		else		
		{			
			SoftFlagDeregister(SoftFlagRemindMask);
			if(SoftFlagGet(SoftFlagDiscDelayMask))
			{
				SoftFlagDeregister(SoftFlagDiscDelayMask);
				RemindSoundServiceItemRequest(SOUND_REMIND_DISCONNE, REMIND_ATTR_NORMAL);
			}
			else
			{
				TwsSlavePlayCt->runflag = 1;
				TwsSlavePlayCt->umuteflag = 0;
				TimeOutSet(&TwsSlavePlayCt->umuteTime, 200);
			}
		}
#endif
		AudioCoreSourceEnable(TWS_SLAVE_SOURCE_NUM);

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
		tws_sync_reinit();
#endif
	}
}

static void TwsSlavePlayModeStop(void)
{
	bool NoService = TRUE;
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(TwsSlavePlayCt->DecoderSync != TaskStateNone && TwsSlavePlayCt->DecoderSync != TaskStateStopped)
	{//解码器是 随app kill
		DecoderServiceStop();
		TwsSlavePlayCt->DecoderSync = TaskStateStopping;
		NoService = FALSE;
	}
#endif
#ifdef CFG_FUNC_RECORDER_EN
	if(TwsSlavePlayCt->RecorderSync != TaskStateNone)
	{//此service 随用随Kill
		MediaRecorderServiceStop();
		TwsSlavePlayCt->RecorderSync = TaskStateStopping;
		NoService = FALSE;
	}
#endif

	TwsSlavePlayCt->state = TaskStateStopping;
	if(NoService)
	{
		TwsSlavePlayModeStopped();
	}
}

static void TwsSlavePlayModeStopping(uint16_t msgId)//部分子service 随用随kill
{
#ifdef CFG_FUNC_RECORDER_EN
	if(msgId == MSG_MEDIA_RECORDER_SERVICE_STOPPED)
	{
		TwsSlavePlayCt->RecorderSync = TaskStateNone;
		APP_DBG("Line:Encoder/RecorderKill");
		MediaRecorderServiceKill();
	}
#endif	
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(msgId == MSG_DECODER_SERVICE_STOPPED)
	{
		TwsSlavePlayCt->DecoderSync = TaskStateNone;
	}
#endif
	if((TwsSlavePlayCt->state == TaskStateStopping)
		#ifdef CFG_FUNC_RECORDER_EN
		&& (TwsSlavePlayCt->RecorderSync == TaskStateNone)
		&& (TwsSlavePlayCt->EncoderSync == TaskStateNone)
		#endif
		#if	defined(CFG_FUNC_REMIND_SOUND_EN)
		&& (TwsSlavePlayCt->DecoderSync == TaskStateNone)
		#endif
		)
	{
		TwsSlavePlayModeStopped();
	}
}

static void TwsSlavePlayModeStopped(void)
{
	MessageContext		msgSend;
	
	//clear msg
	MessageClear(TwsSlavePlayCt->msgHandle);
	
	//Set state
	TwsSlavePlayCt->state = TaskStateStopped;

	//reply
	msgSend.msgId		= MSG_LINE_AUDIO_MODE_STOPPED;
	MessageSend(TwsSlavePlayCt->parentMsgHandle, &msgSend);
}

void TwsSlaveFifoMuteTimeSet(void)
{
	if (TwsSlavePlayCt != NULL)
	{
		TwsSlavePlayCt->TwsBufMuteTimeout = GetSysTick1MsCnt() + (TWS_STATRT_PLAY_FRAM * 128 * 1000 ) / CFG_PARA_SAMPLE_RATE;
		TwsSlavePlayCt->runflag = 1;
		TwsSlavePlayCt->umuteflag = 0;
		TimeOutSet(&TwsSlavePlayCt->umuteTime, 0xfffffff);//设一个近似无限值。
	}
}

//收到unmute 命令后 如果已超过fifo深度就快速解mute，反之根据参数延时解mute。
void TwsSlaveFifoUnmuteSet(void)
{
	if (TwsSlavePlayCt != NULL)
	{
		if(TwsSlavePlayCt->runflag == 1 && TwsSlavePlayCt->umuteflag == 0)
		{
			if(TwsSlavePlayCt->TwsBufMuteTimeout)
			{
				if(GetSysTick1MsCnt() >= TwsSlavePlayCt->TwsBufMuteTimeout + 1)
				{
					TimeOutSet(&TwsSlavePlayCt->umuteTime, 1);//旨在触发解mute.
				}
				else
				{
					TimeOutSet(&TwsSlavePlayCt->umuteTime, TwsSlavePlayCt->TwsBufMuteTimeout - GetSysTick1MsCnt());
				}
				TwsSlavePlayCt->TwsBufMuteTimeout = 0;//清理 fifo mute延时机制。
			}
		}
	}
}

static void TwsSlavePlayTimeCheck(void)
{
	if(btManager.twsState == BT_TWS_STATE_NONE)
	{
		TwsSlavePlayCt->twsSlaveLinkTimeout++;
		if(TwsSlavePlayCt->twsSlaveLinkTimeout >= (100*60*3))
		{
			//3分钟无连接超时退出
			TwsSlavePlayCt->twsSlaveLinkTimeout=0;
			TwsSlaveModeExit();
		}

		if(TwsSlavePlayCt->twsSlaveDiscTimeout)
		{
			TwsSlavePlayCt->twsSlaveDiscTimeout--;
			if(TwsSlavePlayCt->twsSlaveDiscTimeout == 0)
			{
				TwsSlaveModeExit();
			}
		}
	}
	else
	{
		TwsSlavePlayCt->twsSlaveLinkTimeout = 0;
	}
}

/**
 * @func        TwsSlavePlayEntrance
 * @brief       模式执行主体
 * @param       void * param  
 * @Output      None
 * @return      None
 * @Others      模式建立和结束过程
 * Record
 */
static void TwsSlavePlayEntrance(void * param)
{
	MessageContext		msgRecv;
	
	APP_DBG("TwsSlave:App\n");
	// Create services
	TwsSlavePlayModeCreate();

#ifdef CFG_FUNC_AUDIO_EFFECT_EN	
	AudioEffectModeSel(mainAppCt.EffectMode, 2);//0=init hw,1=effect,2=hw+effect
#ifdef CFG_COMMUNICATION_BY_UART
	UART1_Communication_Init((void *)(&UartRxBuf[0]), 1024, (void *)(&UartTxBuf[0]), 1024);
#endif
#endif
#ifdef CFG_APP_LINEIN_MODE_EN
	AudioAnaChannelSet(LINEIN_INPUT_CHANNEL);
#endif

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_AUDIO_EFFECT_EN)
	AudioCoreSourceUnmute(MIC_SOURCE_NUM, TRUE, TRUE);
#endif

	if(IsAudioPlayerMute() == TRUE)
	{
		AudioPlayerMute();
	}
		
#if (TWS_PAIRING_MODE != CFG_TWS_PEER_SLAVE)
	if(btManager.twsState != BT_TWS_STATE_CONNECTED)
	{
		TwsSlaveModeExit();
	}
#endif

	while(1)
	{
		MessageRecv(TwsSlavePlayCt->msgHandle, &msgRecv, 10);
		
		switch(msgRecv.msgId)//警告：在此段代码，禁止新增提示音插播位置。
		{	
			case MSG_DECODER_SERVICE_CREATED:
				TwsSlavePlayModeCreating(msgRecv.msgId);
				break;

			case MSG_TASK_START:
				TwsSlavePlayModeStart();
				break;
			case MSG_DECODER_SERVICE_STARTED:
				AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
				#ifdef BT_TWS_SUPPORT
				AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);
				#endif
				//RemindSound request		
				TwsSlavePlayModeStarting(msgRecv.msgId);
				break;

			case MSG_TASK_RESUME:
				if(TwsSlavePlayCt->state == TaskStatePaused)
				{
					TwsSlavePlayCt->state = TaskStateRunning;
				}
				break;

			case MSG_TASK_STOP:
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				RemindSoundServiceReset();
				#endif
				TwsSlavePlayModeStop();
				break;

			case MSG_DECODER_SERVICE_STOPPED:
			case MSG_MEDIA_RECORDER_SERVICE_STOPPED:
			case MSG_ENCODER_SERVICE_STOPPED:
				TwsSlavePlayModeStopping(msgRecv.msgId);
				break;

			case MSG_APP_RES_RELEASE:
				TwsSlavePlayResRelease();
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_APP_RES_RELEASE_SUC;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				break;
			case MSG_APP_RES_MALLOC:
				TwsSlavePlayResMalloc(mainAppCt.SamplesPreFrame);
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_APP_RES_MALLOC_SUC;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				break;
			case MSG_APP_RES_INIT:
				TwsSlavePlayResInit();
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_APP_RES_INIT_SUC;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				break;
				
			case MSG_REMIND_SOUND_PLAY_START:
				break;
			case MSG_REMIND_SOUND_PLAY_DONE://提示音播放结束
			case MSG_REMIND_SOUND_PLAY_REQUEST_FAIL:					
				{
					//AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
					TwsSlavePlayCt->runflag = 1;
					TwsSlavePlayCt->umuteflag = 0;
					TimeOutSet(&TwsSlavePlayCt->umuteTime, 200);
				}
				break;
			default:
				TwsSlavePlayRunning(msgRecv.msgId);
				break;
		}

		TwsSlavePlayTimeCheck();
	}
}

static void TwsSlavePlayRunning(uint16_t msgId)
{
	if(TwsSlavePlayCt->state == TaskStateRunning)
	{
		if(TwsSlavePlayCt->umuteflag == 0)
		{
			if(TwsSlavePlayCt->runflag == 1)
			{
				if(IsTimeOut(&TwsSlavePlayCt->umuteTime))
				{
					#ifdef BT_TWS_SUPPORT
					AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);
					#endif
					TwsSlavePlayCt->umuteflag = 1;
					if(IsAudioPlayerMute() == TRUE)//主机发起mute阶段不解mute
					{
						AudioPlayerMute();
					}
				}
			}
		}
		switch(msgId)
		{
#ifdef	CFG_FUNC_POWERKEY_EN
			case MSG_TASK_POWERDOWN:
				APP_DBG("MSG receive PowerDown, Please breakpoint\n");
				SystemPowerDown();
				break;
#endif

			case MSG_DECODER_STOPPED:
#if defined(CFG_FUNC_REMIND_SOUND_EN)
				if(SoftFlagGet(SoftFlagDecoderRemind))
				{
					MessageContext		msgSend;
					msgSend.msgId = msgId;
					MessageSend(GetRemindSoundServiceMessageHandle(), &msgSend);//提示音期间转发解码器消息。
				}
#endif
				break;			

			case MSG_BT_TWS_DISCONNECT:
				TwsSlavePlayCt->twsSlaveDiscTimeout = 5;//50ms自动退出
				break;

			case MSG_BT_TWS_LINKLOSS:
				TwsSlavePlayCt->twsSlaveDiscTimeout = 1000;//10s回连不成功自动退出
				BtStartReconnectTws(1, 1);
				break;
			default:
				CommonMsgProccess(msgId);
				break;
		}
	}
}

/***************************************************************************************
 *
 * APIs
 *
 */
bool TwsSlavePlayCreate(MessageHandle parentMsgHandle)
{
	bool		ret = TRUE;

	ret = TwsSlavePlay_Init(parentMsgHandle);
	if(ret)
	{
		TwsSlavePlayCt->taskHandle = NULL;
		xTaskCreate(TwsSlavePlayEntrance,
					"TwsSlavePlay",
					TWS_SLAVE_PLAY_TASK_STACK_SIZE,
					NULL, TWS_SLAVE_PLAY_TASK_PRIO,
					&TwsSlavePlayCt->taskHandle);
		if(TwsSlavePlayCt->taskHandle == NULL)
		{
			ret = FALSE;
		}
	}
	else
	{
		APP_DBG("TwsSlavePlay app create fail!\n");
	}
	return ret;
}

bool TwsSlavePlayStart(void)
{
	MessageContext		msgSend;

	if(TwsSlavePlayCt == NULL)
	{
		return FALSE;
	}
	msgSend.msgId		= MSG_TASK_START;
	return MessageSend(TwsSlavePlayCt->msgHandle, &msgSend);
}

bool TwsSlavePlayPause(void)
{
	MessageContext		msgSend;
	if(TwsSlavePlayCt == NULL)
	{
		return FALSE;
	}
	msgSend.msgId		= MSG_TASK_PAUSE;
	return MessageSend(TwsSlavePlayCt->msgHandle, &msgSend);
}

bool TwsSlavePlayResume(void)
{
	MessageContext		msgSend;
	if(TwsSlavePlayCt == NULL)
	{
		return FALSE;
	}
	msgSend.msgId		= MSG_TASK_RESUME;
	return MessageSend(TwsSlavePlayCt->msgHandle, &msgSend);
}

bool TwsSlavePlayStop(void)
{
	MessageContext		msgSend;
	if(TwsSlavePlayCt == NULL)
	{
		return FALSE;
	}
	AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_AUDIO_EFFECT_EN)
	AudioCoreSourceMute(MIC_SOURCE_NUM, TRUE, TRUE);
#endif	
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	AudioCoreSourceMute(REMIND_SOURCE_NUM, TRUE, TRUE);
#endif
#ifdef BT_TWS_SUPPORT
	AudioCoreSourceMute(TWS_SOURCE_NUM, TRUE, TRUE);
#endif

	vTaskDelay(120);
	msgSend.msgId		= MSG_TASK_STOP;
	return MessageSend(TwsSlavePlayCt->msgHandle, &msgSend);
}

bool TwsSlavePlayKill(void)
{
	if(TwsSlavePlayCt == NULL)
	{
		return FALSE;
	}	
	//Kill used services
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
	DecoderServiceKill();
#endif
#ifdef CFG_FUNC_RECORDER_EN
	if(TwsSlavePlayCt->RecorderSync != TaskStateNone)//当录音创建失败时，需要强行回收
	{
		MediaRecorderServiceKill();
		TwsSlavePlayCt->RecorderSync = TaskStateNone;
	}
#endif
	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
	AudioCoreSourceDisable(TWS_SLAVE_SOURCE_NUM);

	//task
	if(TwsSlavePlayCt->taskHandle != NULL)
	{
		vTaskDelete(TwsSlavePlayCt->taskHandle);
		TwsSlavePlayCt->taskHandle = NULL;
	}

	//Msgbox
	if(TwsSlavePlayCt->msgHandle != NULL)
	{
		MessageDeregister(TwsSlavePlayCt->msgHandle);
		TwsSlavePlayCt->msgHandle = NULL;
	}

	//PortFree
	TwsSlavePlayCt->AudioCoreTwsSlave = NULL;
	if(TwsSlavePlayCt->Source1Buf_TwsSlave != NULL)
	{
		osPortFree(TwsSlavePlayCt->Source1Buf_TwsSlave);
		TwsSlavePlayCt->Source1Buf_TwsSlave = NULL;
	}
#if	defined(CFG_FUNC_REMIND_SOUND_EN)
	if(TwsSlavePlayCt->SourceDecoder != NULL)
	{
		osPortFree(TwsSlavePlayCt->SourceDecoder);
		TwsSlavePlayCt->SourceDecoder = NULL;
	}
#endif

	osPortFree(TwsSlavePlayCt);
	TwsSlavePlayCt = NULL;
	APP_DBG("Tws Slave:Kill Ct\n");

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	AudioEffectsDeInit();
#endif

	return TRUE;
}

MessageHandle GetTwsSlaveMessageHandle(void)
{
	if(TwsSlavePlayCt != NULL)
	{
		return TwsSlavePlayCt->msgHandle;
	}
	return NULL;
}

/*****************************************************************************************
 * 进入/退出Tws Slave模式
 ****************************************************************************************/
void TwsSlaveModeEnter(void)
{
	MessageContext		msgSend;
	if(GetSystemMode() != AppModeTwsSlavePlay)
	{
		ResourceRegister(AppResourceTwsSlave);
		msgSend.msgId = MSG_DEVICE_SERVICE_TWS_SLAVE_CONNECTED;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
}
extern uint8_t tws_slave_cap;
void TwsSlaveModeExit(void)
{
	MessageContext		msgSend;
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		//tws_link_disconnect();
		BtTwsDisconnectApi();
	}
	
	if(GetSystemMode() == AppModeTwsSlavePlay)
	{
		APP_DBG("Exit Tws Slave Mode\n");
		ResourceDeregister(AppResourceTwsSlave);
		msgSend.msgId = MSG_DEVICE_SERVICE_TWS_SLAVE_DISCONNECT;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	BtSetFreqTrim(tws_slave_cap);
}

const int16_t jingyin[512]={0,0,0};
TWS_CONFIG	TwsCfg;
/*****************************************************************************************
 * TWS参数初始化设置
 ****************************************************************************************/
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
uint8_t* TWS_Param_Init(void)
#else
void TWS_Param_Init(void)
#endif

{
	uint32_t tws_mem;
	uint8_t*p;
	//TWS_MONO模式下，设备的声道取决于变量：TwsCfg.MasterSound
	#ifdef CFG_TWS_SOUNDBAR_APP
	TwsCfg.AudioMode    = TWS_M_LR__S_MONO;
	#else
	TwsCfg.AudioMode    = TWS_M_L__S_R;
	#endif
	
	tws_mem = tws_mem_size(TWS_STATRT_PLAY_FRAM,TwsCfg.AudioMode,TWS_AUDIO_HIGH);
	p = (uint8_t*)osPortMalloc(tws_mem);
	if(p == 0)
	{
		APP_DBG("tws mem malloc error\n");
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
		return p;
#endif			
		while(1);
	}
	memset(p,0,tws_mem);
	APP_DBG("tws_mem_size:%lu\n",tws_mem);
	tws_mem_set(p);
	
	TwsCfg.IsRemindSyncEn = 1;//0:提示音不发送给slave；1:提示音发送给slave同步播放提示音
	TwsCfg.OutMode     = 0;//默认对箱立体声:master是左声道，slave是右声道
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
	return p;
#endif	
}

/*****************************************************************************************
 * TWS模式下master 发送的是原声，还是带音效的声音
 ****************************************************************************************/
void TWS_AudioEffectProcess(int16_t  **MusicAddr, int16_t  **RemindAddr)
{
  //音效处理前 将数据发送出去
	uint8_t tws_pcm_path = 0xff;
	int16_t *music_pcm;
	int16_t *remind_in;
	
	if(GetBtManager()->twsState != BT_TWS_STATE_CONNECTED) return;
	music_pcm = *MusicAddr;
	remind_in = *RemindAddr;

	if(tws_play == 0x55)
	{
		if(tws_get_role() == BT_TWS_MASTER)
		{
			int16_t *p = NULL;

			if(TwsCfg.OutMode == 1)
			{
				int i;
				int16_t a,b;
				for(i=0; i<mainAppCt.SamplesPreFrame; i++)
				{
					a = music_pcm[2*i + 0];
					b = music_pcm[2*i + 1];
					music_pcm[2*i + 0] = b;
					music_pcm[2*i + 1] = a;
				}					
			}
			
			if(music_pcm)
			{
				if(remind_in)
				{
					if(TwsCfg.IsRemindSyncEn)
					{
						tws_pcm_path = REMIND_SOURCE_NUM;
						p = remind_in;
					}
					else
					{
						tws_pcm_path = 0xFF;
						p = (int16_t *)jingyin;
					}
				}
				else
				{
					tws_pcm_path = APP_SOURCE_NUM;
					p = music_pcm;
				}
			}
			else if(remind_in && TwsCfg.IsRemindSyncEn)
			{
				tws_pcm_path = REMIND_SOURCE_NUM;
				p = remind_in;
			}
			else
			{
				tws_pcm_path = 0xFF;
				p = (int16_t *)jingyin;
			}
			tws_music_pcm_process(p,p,mainAppCt.SamplesPreFrame,tws_pcm_path);
			if(p != jingyin)
			{
				memset(p,0,mainAppCt.SamplesPreFrame*4);
			}
		}
		//接收到的TWS数据 替换music
		if(tws_get_data_source() == APP_SOURCE_NUM)
		{
			*MusicAddr = AudioCore.AudioSource[TWS_SOURCE_NUM].PcmInBuf;
		}

		if(tws_get_data_source() == REMIND_SOURCE_NUM)
		{
			if(TwsCfg.IsRemindSyncEn)
			{
				*RemindAddr = AudioCore.AudioSource[TWS_SOURCE_NUM].PcmInBuf;
			}
			if(tws_get_role() == BT_TWS_SLAVE)
			{
				AudioPlayerUnMute();
			}
		}
	}
}

/*****************************************************************************************
 * TWS模式下的提示音处理
 ****************************************************************************************/
void TWS_RemindAudioProcess(int16_t  **RemindAddr)
{
	uint8_t tws_pcm_path = 0xff;
	int16_t *remind_in;

	if(GetBtManager()->twsState != BT_TWS_STATE_CONNECTED) return;

	remind_in = *RemindAddr;

	if((tws_play == 0x55) && (tws_get_role() == BT_TWS_MASTER))
	{
		int16_t *p = NULL;
		if(remind_in && TwsCfg.IsRemindSyncEn)
		{
			tws_pcm_path = REMIND_SOURCE_NUM;
			p = remind_in;
		}
		else
		{
			tws_pcm_path = 0xFF;
			p = (int16_t *)jingyin;
		}
		tws_music_pcm_process(p,p,mainAppCt.SamplesPreFrame,tws_pcm_path);
		if(p != jingyin)
		{
			memset(p,0,mainAppCt.SamplesPreFrame*4);
		}
	}
	if(tws_get_data_source() == REMIND_SOURCE_NUM)
	{
		if(TwsCfg.IsRemindSyncEn)
		{
			*RemindAddr = AudioCore.AudioSource[TWS_SOURCE_NUM].PcmInBuf;
		}
		if(tws_get_role() == BT_TWS_SLAVE)
		{
			AudioPlayerUnMute();
		}
	}
}

void TwsOutModeSel(void)
{
	if(tws_get_role() != BT_TWS_MASTER)
		return;
	
	if(TwsCfg.OutMode == 1)
	{
		TwsCfg.OutMode = 0;  
		APP_DBG("tws master L, slave R\n");
	}
	else
	{
		TwsCfg.OutMode = 1;  
		APP_DBG("tws master R, slave L\n");
	}
}

void tws_device_close(void)
{
#ifdef CFG_FUNC_MIC_KARAOKE_EN
	uint16_t fifo_len = mainAppCt.SamplesPreFrame * 2 * 2;
#else
	uint16_t fifo_len = mainAppCt.SamplesPreFrame * 2 * 2 * 4;
#endif

#if  (TWS_AUDIO_OUT_PATH	== TWS_IIS0_OUT)
	*(volatile unsigned long *)0x40029000 &= ~0x80;//enable iis1
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S0_TX);
	DMA_CircularConfig(PERIPHERAL_ID_I2S0_TX, fifo_len, mainAppCt.I2SFIFO, fifo_len * 2);
	DMA_ChannelEnable(PERIPHERAL_ID_I2S0_TX);
#elif(TWS_AUDIO_OUT_PATH	== TWS_IIS1_OUT)
	*(volatile unsigned long *)0x4002A000 &= ~0x80;//enable iis1
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S1_TX);
	DMA_CircularConfig(PERIPHERAL_ID_I2S1_TX, fifo_len, mainAppCt.I2SFIFO, fifo_len * 2);
	DMA_ChannelEnable(PERIPHERAL_ID_I2S1_TX);
#elif(TWS_AUDIO_OUT_PATH	== TWS_DAC0_OUT)

#ifdef CFG_RES_AUDIO_DAC0_EN
	*(volatile unsigned long *)0x4002E058 |= 0x0C;
	AudioDAC_Disable(DAC0);
	AudioDAC_Reset(DAC0);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC0_TX);
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC0_TX, fifo_len, mainAppCt.DACFIFO, fifo_len * 2);
	DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_DAC0_TX);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	AudioDAC_Disable(DAC1);
	AudioDAC_Reset(DAC1);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC1_TX);
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC1_TX, fifo_len/2, mainAppCt.DACXFIFO, fifo_len);
	DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_DAC1_TX);	
#endif
#endif
}

__attribute__((section(".tws_sync_code")))
void tws_device_enable(void)
{
#if  (TWS_AUDIO_OUT_PATH	== TWS_IIS0_OUT)
	*(volatile unsigned long *)0x40029000 |= 0x80;//enable iis0
#elif(TWS_AUDIO_OUT_PATH	== TWS_IIS1_OUT)
	*(volatile unsigned long *)0x4002A000 |= 0x80;//enable iis1
#elif(TWS_AUDIO_OUT_PATH	== TWS_DAC0_OUT)
#ifdef CFG_RES_AUDIO_DAC0_EN
	*(volatile unsigned long *)0x4002E058 &= ~0x0C;//模拟的MUTE
	*(volatile unsigned long *)0x4002E000 |= 1;//enable dac
#endif
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	*(volatile unsigned long *)0x4002E024 |= 1;//enable dacx	
#endif
}

bool tws_device_check(void)
{
#ifdef CFG_RES_AUDIO_DACX_EN
	*(volatile unsigned long *)0x4002E024 |= 1;//enable dacx
#endif

#if  (TWS_AUDIO_OUT_PATH	== TWS_IIS0_OUT)
	if ((*(volatile unsigned long *)0x40029000 & 0x80) == 0)
	{
		*(volatile unsigned long *)0x40029000 |= 0x80;
		return 0;
	}
#elif(TWS_AUDIO_OUT_PATH	== TWS_IIS1_OUT)
	if ((*(volatile unsigned long *)0x4002A000 & 0x80) == 0)
	{
		*(volatile unsigned long *)0x4002A000 |= 0x80;
		return 0;
	}
#elif(TWS_AUDIO_OUT_PATH	== TWS_DAC0_OUT)
#ifdef CFG_RES_AUDIO_DAC0_EN
	if(((*(volatile unsigned long *)0x4002E058)&0x0C) == 0x0C)
	{
		*(volatile unsigned long *)0x4002E058 &= ~0x0C;
		*(volatile unsigned long *)0x4002E000 |= 1;
		return 0;
	}
#endif
#endif
	return 1;
}

extern uint16_t AudioDAC0DataLenGet(void);
__attribute__((section(".tws_sync_code")))
uint16_t tws_device_data_len(void)
{
#if  (TWS_AUDIO_OUT_PATH	== TWS_IIS0_OUT)
	return AudioI2S0_TX_DataLenGet();
#elif(TWS_AUDIO_OUT_PATH	== TWS_IIS1_OUT)
	return AudioI2S1_TX_DataLenGet();
#elif(TWS_AUDIO_OUT_PATH	== TWS_DAC0_OUT)
	return AudioDAC0DataLenGet();
#endif
}

#ifdef BT_TWS_FUNCTION_KEY_SWITCH

uint16_t TWSDataGet(void* Buf, uint16_t Samples);
uint16_t TWSDataLenGet(void);

void tws_slave_fifo_clear(void)
{
	if(btManager.TwsAudioCoreBuf)
		memset(btManager.TwsAudioCoreBuf,0,sizeof(512*2*2));
}

uint8_t tws_function_switch(void)
{
	if(btManager.TwsFunctionEnable)
	{
		btManager.TwsFunctionEnable = 0;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].Enable = 0;
		btManager.TwsAudioCoreExitFlag = TRUE;
		if(btManager.btTwsReconnectTimer.timerFlag)
		{
			BtStopReconnectTwsReg();
			BtStopReconnectTws();
		}

		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			uint8_t cnt;

			BtTwsDeviceDisconnect();
			for(cnt = 0;cnt < 100;cnt++)
			{
				if(btManager.twsState != BT_TWS_STATE_CONNECTED)
					break;
				vTaskDelay(10);
			}
		}
		if(btManager.TwsMemP)
		{
			osPortFree(btManager.TwsMemP);
			btManager.TwsMemP = NULL;
		}

		while(btManager.TwsAudioCoreExitFlag)
		{
			vTaskDelay(10);
		}
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].FuncDataGet = NULL;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].FuncDataGetLen = NULL;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].IsSreamData = 0;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].PcmFormat = 0;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].PcmInBuf = NULL;
		if(btManager.TwsAudioCoreBuf)
			osPortFree(btManager.TwsAudioCoreBuf);
		btManager.TwsAudioCoreBuf = NULL;
		return 2;
	}
	else
	{
		btManager.TwsMemP = TWS_Param_Init();
		if(btManager.TwsMemP)
		{
			btManager.TwsAudioCoreBuf = osPortMalloc(512*2*2);
			if(!btManager.TwsAudioCoreBuf)
			{
				osPortFree(btManager.TwsMemP);
				btManager.TwsMemP = NULL;
				return 0;
			}
		}
		else
			return 0;

		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].FuncDataGet = TWSDataGet;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].FuncDataGetLen = TWSDataLenGet;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].IsSreamData = 0;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].PcmFormat = 2;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].PcmInBuf = btManager.TwsAudioCoreBuf;
		mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].Enable = 1;
		btManager.TwsFunctionEnable = 1;

		if(btManager.twsFlag)
		{
			btManager.TwsPowerOnFlag = 0;
			if(btManager.twsRole == BT_TWS_SLAVE)
			{
				BtReconnectTws_Slave();
			}
			else
			{
				BtReconnectTws();
			}
		}
		return 1;
	}	
}

#endif


#endif//#ifdef BT_TWS_SUPPORT

