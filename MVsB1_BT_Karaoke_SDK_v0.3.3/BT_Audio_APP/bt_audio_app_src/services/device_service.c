/**
 **************************************************************************************
 * @file    device_service.c
 * @brief   
 *
 * @author  halley
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "gpio.h"
#include "debug.h"
#include "rtos_api.h"
#include "clk.h"
#include "main_task.h"
#include "app_message.h"
#include "device_detect.h"
#include "device_service.h"
#include "mode_switch_api.h"
#include "key.h" 
#include "backup_interface.h"
#include "breakpoint.h"
#include "timeout.h"
#include "media_play_api.h"
#include "radio_api.h"
#include "rtc_ctrl.h"
#include "bt_manager.h"
#ifdef CFG_FUNC_RECORDER_EN		
#include "recorder_service.h"
#endif
#ifdef CFG_FUNC_POWER_MONITOR_EN
#include "power_monitor.h"
#endif
#include "hdmi_in_api.h"
#ifdef BT_TWS_SUPPORT
#include "tws_slave_mode.h"
#endif
#define DEVICE_SERVICE_SIZE				384//1024
#define DEVICE_SERVICE_PRIO				3
#define DEVICE_SERVICE_TIMEOUT			1	/* 1ms */
#define DEVICE_SERVICE_TIMEOUT_1MS		1
#define NUM_MESSAGE_QUEUE				4

#ifdef CFG_FUNC_BREAKPOINT_EN
void BreakPointSave(MessageContext msgRec);
extern MainAppContext	mainAppCt;
TIMER TimerBreakPoint;
#endif
extern uint8_t deviceService_run;
extern void Clear_Error_Code(void);
typedef struct _DeviceServiceContext
{
	xTaskHandle		taskHandle;
	MessageHandle	msgHandle;
	MessageHandle	parentMsgHandle;
	TaskState		serviceState;
}DeviceServiceContext;

static DeviceServiceContext			deviceServiceCt;

#ifdef CFG_FUNC_RECORDER_EN
#define RECORDE_GO 0
#define RECORDE_PAUSE 1
static uint32_t sRecState = RECORDE_PAUSE;
#endif
/***************************************************************************************
 *
 * Internal functions
 *
 */




/**
 * @brief	Device servcie init
 * @param	MessageHandle parentMsgHandle
 * @return	0 for success
 */
#ifdef DEVICE_SERVICE_DELETE_EN 
TIMER	ScanTimer;
#endif
static int32_t DeviceServiceInit(MessageHandle parentMsgHandle)
{
	memset(&deviceServiceCt, 0, sizeof(DeviceServiceContext));
	/* register message handle */
#ifdef DEVICE_SERVICE_DELETE_EN	
	deviceServiceCt.msgHandle  = parentMsgHandle;
#else
	deviceServiceCt.msgHandle = MessageRegister(NUM_MESSAGE_QUEUE);
	if(deviceServiceCt.msgHandle == NULL)
	{
		return -1;
	}
#endif
	deviceServiceCt.parentMsgHandle = parentMsgHandle;
	deviceServiceCt.serviceState = TaskStateNone;
	ResourceDeregister(AppResourceMask);

#ifdef CFG_FUNC_POWER_MONITOR_EN
	PowerMonitorInit();
#endif
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_RES_CODE_KEY_USE)|| defined(CFG_ADC_LEVEL_KEY_EN) || defined(CFG_RES_IO_KEY_SCAN)
	/* Init keys*/
	KeyInit();
#endif
	/* Init battery */
//	BatteryServiceInit();

	/* Init device */
	InitDeviceDetect();

#ifdef CFG_RES_FLASHFS_EN
	if(!FlashFSInit())		 //FS文件系统初始化 需要数秒 
	{
		ResourceRegister(AppResourceFlashFs);
	}
#elif defined(CFG_FUNC_RECORD_FLASH)
	SPIM_Init(0, 0);
	SPI_Flash_Init();
	uint16_t FlashID = SPI_Flash_ReadMID();
	DBG("SPI_Flash_ReadMID=%x\n", FlashID);

	if(FlashID != 0)
	{
		ResourceRegister(AppResourceFlash);
	}
#endif
#ifdef DEVICE_SERVICE_DELETE_EN	
	MessageContext		msgSend;
	msgSend.msgId		= MSG_DEVICE_SERVICE_CREATED;
	MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
	deviceServiceCt.serviceState = TaskStateReady;
	TimeOutSet(&ScanTimer, 10);
#endif
	return 0;
}

static void DeviceServiceDeinit(void)
{
	MessageContext		msgSend;

	/* Key deinit*/
//	KeyDeinit();
//
//	/* Battery deinit */
//	BatteryDeinit();
//
//	/* Device detect deinit */
//	DeviceDetectDeinit();

	deviceServiceCt.serviceState = TaskStateNone;

	/* Send message to main app */
	msgSend.msgId		= MSG_DEVICE_SERVICE_STOPPED;
	MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
}
extern uint32_t SysemMipsPercent;
//sleep前device 暂停，会收到keytable更换msg
#ifdef DEVICE_SERVICE_DELETE_EN	
void DeviceServiceEntrance(MessageContext	msgRecv)
{

	//MessageContext		msgRecv;
	static MessageContext		msgSend;
	static uint32_t			Plug;
//	static uint8_t				MsgTimeOut = DEVICE_SERVICE_TIMEOUT;
//	TIMER				ScanTimer;
#ifdef BT_TWS_SUPPORT
	static bool                is_need_send_to_master;
#endif

#if defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)
	static bool				MipsLog = TRUE;
#endif

#else
static void DeviceServiceEntrance(void * param)
{
	MessageContext		msgRecv;
	MessageContext		msgSend;
	uint32_t			Plug;
	uint8_t				MsgTimeOut = DEVICE_SERVICE_TIMEOUT;
	TIMER				ScanTimer;
	bool                is_need_send_to_master;
#ifdef CFG_FUNC_DEBUG_EN
	bool				MipsLog = TRUE;
#endif
//	ResourceRegister(AppResourceDac);//默认开启，配合缺省APP模式
	
	/* Send message to main app */
	msgSend.msgId		= MSG_DEVICE_SERVICE_CREATED;
	MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
	deviceServiceCt.serviceState = TaskStateReady;
	TimeOutSet(&ScanTimer, 10);

	while(1)
#endif
	{	
#ifdef	CFG_FUNC_POWERKEY_EN
#if		POWERKEY_MODE == POWERKEY_MODE_PUSH_BUTTON
		if(IsTimeOut(&ScanTimer))//10ms
		{
			static int Cnt = 180;
			static bool IsPowerKeyTrig = FALSE;

			if(SystemPowerKeyDetect())
			{
				IsPowerKeyTrig = TRUE;
				APP_DBG("PowerKey Trig\n");
			}
			if(IsPowerKeyTrig)
			{
				if(BACKUP_PowerKeyPinStateGet() == FALSE)
				{
					Cnt--;
					if(Cnt == 0)
					{
						msgSend.msgId		= MSG_POWERDOWN;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
				}
				else
				{
					IsPowerKeyTrig = FALSE;
					Cnt = 200;
				}
			}
		}
#endif
#endif
#ifndef DEVICE_SERVICE_DELETE_EN
		MessageRecv(deviceServiceCt.msgHandle, &msgRecv, MsgTimeOut);
#endif
		//APP_DBG("Device service run\n");
		switch(msgRecv.msgId)
		{
			case MSG_TASK_START:
				if(deviceServiceCt.serviceState == TaskStateReady)
				{
					deviceServiceCt.serviceState = TaskStateRunning;

					msgSend.msgId		= MSG_DEVICE_SERVICE_STARTED;
					MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					ResourcePreSet();
				}
				break;

			case MSG_TASK_PAUSE:
				deviceServiceCt.serviceState = TaskStatePaused;
				msgSend.msgId		= MSG_DEVICE_SERVICE_PAUSED;
				MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
				break;

			case MSG_TASK_RESUME:
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_RES_IO_KEY_SCAN) || defined(CFG_RES_CODE_KEY_USE)|| defined(CFG_ADC_LEVEL_KEY_EN)
				/* Init keys*/
				KeyInit();
#endif
//				InitDeviceDetect();
				deviceServiceCt.serviceState = TaskStateRunning;
				break;

#ifdef CFG_FUNC_RECORDER_EN		
			case MSG_MEDIA_RECORDER_GO_PAUSED:	
				if(deviceServiceCt.serviceState != TaskStateRunning)
					break;
				if(sRecState == RECORDE_GO)
				{
					AudioCoreSinkDisable(AUDIO_RECORDER_SINK_NUM);
					sRecState = RECORDE_PAUSE;
					APP_DBG("rec pasue \n");
				}
				else 
				{
					AudioCoreSinkEnable(AUDIO_RECORDER_SINK_NUM);
					sRecState = RECORDE_GO;
					APP_DBG("rec GO \n");
				}
				break;
#endif

			case MSG_TASK_STOP:
				DeviceServiceDeinit();
				/*Wait for kill*/
				break;
				
			default:
				break;
		}

		if(deviceServiceCt.serviceState == TaskStateRunning)
		{
#ifdef CFG_FUNC_BREAKPOINT_EN
			BreakPointSave(msgRecv);
#endif
			deviceService_run = 1;

#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_RES_CODE_KEY_USE)|| defined(CFG_ADC_LEVEL_KEY_EN) || defined(CFG_RES_IO_KEY_SCAN)
			/* Key scan*/
			if(IsTimeOut(&ScanTimer))
			{
				TimeOutSet(&ScanTimer, 10);

				msgSend.msgId = KeyScan();
				if(msgSend.msgId != MSG_NONE)
				{
					#ifdef BT_TWS_SUPPORT
					if((GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)&&(GetBtManager()->twsRole == BT_TWS_SLAVE))
					{
						is_need_send_to_master = TRUE;
						switch(msgSend.msgId)
						{
							#ifndef TWS_POWEROFF_MODE_SYNC
							//单箱关机不发送到主机处理
							case MSG_BT_SNIFF:
							case MSG_DEEPSLEEP:
							case MSG_POWER:
							case MSG_POWERDOWN:
								is_need_send_to_master = FALSE;
								MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
								break;
							#endif

							//麦克风音效主从单独控制
							#ifndef TWS_VOLUME_SYNC_ENABLE
							case MSG_MUSIC_VOLDOWN:
							case MSG_MUSIC_VOLUP:
							#endif
							case MSG_EFFECTMODE:
								is_need_send_to_master = FALSE;
								MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
								break;

							//从机不能响应的按键
							case MSG_MODE:
							is_need_send_to_master = FALSE;
							#ifdef TWS_SLAVE_MODE_SWITCH_EN
							MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
							#endif
							break;

							case MSG_BT_HF_REDAIL_LAST_NUM:
							case MSG_BT_HF_VOICE_RECOGNITION:
								is_need_send_to_master = FALSE;
								break;
								
							default:
								break;
						}
						
						if (is_need_send_to_master)
						{
							extern void tws_slave_send_key_msg(uint16_t key_data);
							tws_slave_send_key_msg(msgSend.msgId);
						}
						msgSend.msgId = MSG_NONE;	
					}
					else
					#endif
					{
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
				}
				
#ifdef CFG_FUNC_POWER_MONITOR_EN
				//电池电压采样
				PowerVoltageSampling();
				//电池电量处理
				BatteryScan();
#endif

#ifdef CFG_FUNC_RTC_EN
				RtcStateCtrl();
#endif
				/* Device Detect */
				Plug = DeviceDetect();
				if(Plug & CARDIN_EVENT_BIT)//卡插拔事件
				{
					if(Plug & CARDIN_STATE_BIT)
					{
						ResourceRegister(AppResourceCard | AppResourceCardForPlay);
						msgSend.msgId	= MSG_DEVICE_SERVICE_CARD_IN;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
					else
					{
						if(!(Plug & UDISKIN_STATE_BIT))
						{
							//APP_DBG("U Disk also unplug, so clear the startup flag\n");
							Clear_Error_Code();
						}
						ResourceDeregister(AppResourceCard | AppResourceCardForPlay);
						msgSend.msgId	= MSG_DEVICE_SERVICE_CARD_OUT;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
				}
				if(Plug & LINEIN_EVENT_BIT)//线路插拔事件
				{
					if(Plug & LINEIN_STATE_BIT)
					{
						ResourceRegister(AppResourceLineIn);
						msgSend.msgId	= MSG_DEVICE_SERVICE_LINE_IN;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
					else
					{
						ResourceDeregister(AppResourceLineIn);
						msgSend.msgId	= MSG_DEVICE_SERVICE_LINE_OUT;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
				}
				if(Plug & UDISKIN_EVENT_BIT)//
				{
					if(Plug & UDISKIN_STATE_BIT)
					{
						ResourceRegister(AppResourceUDisk | AppResourceUDiskForPlay);
						msgSend.msgId	= MSG_DEVICE_SERVICE_DISK_IN;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
					else
					{
					  if(!(Plug & CARDIN_STATE_BIT))
					  {
						  //APP_DBG("SD Card also unplug, so clear the startup flag\n");
						  Clear_Error_Code();
					  }
					  ResourceDeregister(AppResourceUDisk | AppResourceUDiskForPlay);
					  msgSend.msgId	= MSG_DEVICE_SERVICE_DISK_OUT;
					  MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
				}
				if(Plug & USB_DEVICE_EVENT_BIT)//
				{
					if(Plug & USB_DEVICE_STATE_BIT)
					{
						ResourceRegister(AppResourceUsbDevice);
						#ifdef CFG_COMMUNICATION_BY_USB
						if(!sDevice_Inserted_Flag)
						#endif
						{						
							if(GetSystemMode() > AppModeRestPlay)
							{
								msgSend.msgId	= MSG_DEVICE_SERVICE_USB_DEVICE_IN;
								MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
							}
						}
					}
					else
					{						
						ResourceDeregister(AppResourceUsbDevice);
						msgSend.msgId	= MSG_DEVICE_SERVICE_USB_DEVICE_OUT;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
					}
				}
#ifdef HDMI_HPD_CHECK_DETECT_EN
				if(Plug & HDMI_HPD_EVENT_BIT)
				{
					if(Plug & HDMI_HPD_STATE_BIT)
					{
						if(mainAppCt.hdmiResetFlg == 1)
						{
							mainAppCt.hdmiResetFlg = 2;
						}
						ResourceRegister(AppResourceHdmiIn);
						msgSend.msgId	= MSG_DEVICE_SERVICE_HDMI_IN;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
						//APP_DBG("&&&&hdmi in...\n");
					}
					else
					{
						if(HDMIExitFlg == 1)
						{
							if(IsTimeOut(&HDMIExitTimer))
							{
								HDMIExitFlg = 0;
								mainAppCt.hdmiResetFlg = 0;
								ResourceDeregister(AppResourceHdmiIn);
								msgSend.msgId	= MSG_DEVICE_SERVICE_HDMI_OUT;
								MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
								//APP_DBG("&&&&hdmi out1...\n");
							}
						}
						//APP_DBG("&&&&hdmi out3...\n");
					}
				}

				if(HDMIExitFlg == 1)
				{
					if(IsTimeOut(&HDMIExitTimer))
					{
						HDMIExitFlg = 0;
						mainAppCt.hdmiResetFlg = 0;
						ResourceDeregister(AppResourceHdmiIn);
						msgSend.msgId	= MSG_DEVICE_SERVICE_HDMI_OUT;
						MessageSend(deviceServiceCt.parentMsgHandle, &msgSend);
						//APP_DBG("&&&&hdmi out2...\n");
					}
				}
#endif
/************************MIPS 监测和警告*********************************/
#if defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)
				#define MIPS_LOG_INTERVAL		10000//ms 注意：高优先级任务持续阻塞device时，会影响mips log输出
				if(GetSysTick1MsCnt() % MIPS_LOG_INTERVAL < MIPS_LOG_INTERVAL / 2)
				{
					if(MipsLog
//						&& SysemMipsPercent < 2000 //此行可以减少log，只在idle较少时打印，持续观测时可屏蔽。
						)
					{
#ifdef ENABLE_COUNT_INSTANT_MCPS
						uint16_t InstantVal = InstantMcpsFull();
						if(SysemMipsPercent < 10000 / 2)
						{
							DBG("Fullload:%d mS ", (int)(InstantVal ? InstantVal : MIPS_LOG_INTERVAL));
						}
#endif
						DBG("MCPS:%d   ", (int)((10000 - SysemMipsPercent) * (Clock_CoreClockFreqGet() / 1000000)) / 10000);
						DBG("RAM:%d\n", (int)(320 * 1024 - osPortRemainMem())/1024);

						MipsLog = FALSE;
					}
				}
				else
					MipsLog = TRUE;
#endif
#if (defined(BT_TWS_SUPPORT) && defined(DEBUG_TWS_PACKET) && ((DEBUG_TWS_PACKET == DEBUG_TWS_DATARATE) || (DEBUG_TWS_PACKET == DEBUG_TWS_DATA_BANDWIDTH)))
	if(DebugTwsLog && DebugTwsDataTotal != 0)
	{
		//APP_DBG("Tws:%dB/S,  %d~%dB/S\n", DebugTwsDataTotal/(DEBUG_SPEED_WIN / 1000), (DebugTwsDataMin * 1000) / DEBUG_COUNT_WIN,  (DebugTwsDataMax * 1000) / DEBUG_COUNT_WIN);
		DebugTwsDataMax = 0;
		DebugTwsDataMin = 0xffffffff;
		DebugTwsDataTotal = 0;
		DebugTwsLog = FALSE;
	}
#endif
/********************************************************************/
			}			
#endif
		}
/**************************录音ram优化方案 *******************************/
#ifdef CFG_FUNC_RECORDER_EN
		if(deviceServiceCt.serviceState == TaskStateRunning)
		{
			if(AudioCore.AudioSink[AUDIO_RECORDER_SINK_NUM].Enable&&sRecState == RECORDE_GO)
			{
				MediaRecorderEncode();
			}
		}	
#endif
/**********************************************************************/
	}
}

/**
 * @brief	Start device service.
 * @param	MessageHandle parentMsgHandle
 * @return
 */
int32_t DeviceServiceCreate(MessageHandle parentMsgHandle)
{
	int32_t		ret = 0;

	ret = DeviceServiceInit(parentMsgHandle);
#ifndef DEVICE_SERVICE_DELETE_EN
	if(!ret)
	{
		deviceServiceCt.taskHandle = NULL;
		xTaskCreate(DeviceServiceEntrance,
				"DeviceService",
				DEVICE_SERVICE_SIZE,
				NULL, DEVICE_SERVICE_PRIO,
				&deviceServiceCt.taskHandle);
		if(deviceServiceCt.taskHandle == NULL)
		{
			ret = -1;
		}
	}
	if(ret)
	{
		APP_DBG("DeviceService create fail!\n");
	}
#endif	
	return ret;
}


/***************************************************************************************
 *
 * APIs
 *
 */

/**
 * @brief	Get message receive handle of audio core manager
 * @param	NONE
 * @return	MessageHandle
 */
MessageHandle GetDeviceMessageHandle(void)
{
	return deviceServiceCt.msgHandle;
}

/**
 * @brief	Start device service.
 * @param	NONE
 * @return  
 */
int32_t DeviceServiceStart(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_START;
	MessageSend(deviceServiceCt.msgHandle, &msgSend);

	return 0;
}

/**
 * @brief	Pause device service.
 * @param	NONE
 * @return  
 */
void DeviceServicePause(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_PAUSE;
	MessageSend(deviceServiceCt.msgHandle, &msgSend);
	return ;
}

/**
 * @brief	Resume device service.
 * @param	NONE
 * @return  
 */
void DeviceServiceResume(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_RESUME;
	MessageSend(deviceServiceCt.msgHandle, &msgSend);
	return ;
}

/**
 * @brief	Stop device service.
 * @param	NONE
 * @return  
 */
void DeviceServiceStop(void)
{
	return ;
}

/**
 * @brief	Stop device service.
 * @param	NONE
 * @return  
 */
void DeviceServiceKill(void)
{
	//task
	if(deviceServiceCt.taskHandle != NULL)
	{
		vTaskDelete(deviceServiceCt.taskHandle);
		deviceServiceCt.taskHandle = NULL;
	}

	//Msgbox
	if(deviceServiceCt.msgHandle != NULL)
	{
		MessageDeregister(deviceServiceCt.msgHandle);
		deviceServiceCt.msgHandle = NULL;
	}
	//PortFree
}


#ifdef CFG_FUNC_BREAKPOINT_EN
void BreakPointSave(MessageContext msgRec)
{
	static bool IsBackUpFlag = FALSE;
	BP_SYS_INFO *pBpSysInfo;
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
	BP_PLAYER_INFO *pBpPlayInfo;
#endif
#ifdef CFG_APP_RADIOIN_MODE_EN
	BP_RADIO_INFO *pBpRadioInfo;
#endif

	switch(msgRec.msgId)
	{
		case MSG_DEVICE_SERVICE_BP_SYS_INFO:
			IsBackUpFlag = TRUE;
			TimeOutSet(&TimerBreakPoint, 500);
			pBpSysInfo = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);
			pBpSysInfo->CurModuleId  = mainAppCt.appCurrentMode;
			pBpSysInfo->MusicVolume  = mainAppCt.MusicVolume;
#ifdef CFG_APP_BT_MODE_EN
			pBpSysInfo->HfVolume     = mainAppCt.HfVolume;
#endif
			pBpSysInfo->EffectMode   = mainAppCt.EffectMode;			
			pBpSysInfo->MicVolume    = mainAppCt.MicVolume;
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
			pBpSysInfo->EqMode		 = mainAppCt.EqMode;
#endif	
			pBpSysInfo->ReverbStep   = mainAppCt.ReverbStep;
#ifdef CFG_FUNC_MIC_TREB_BASS_EN			
			pBpSysInfo->MicBassStep     = mainAppCt.MicBassStep;
			pBpSysInfo->MicTrebStep     = mainAppCt.MicTrebStep;
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN			
			pBpSysInfo->MusicBassStep     = mainAppCt.MusicBassStep;
			pBpSysInfo->MusicTrebStep     = mainAppCt.MusicTrebStep;
#endif
#ifdef CFG_FUNC_SOUND_REMIND
			//pBpSysInfo->SoundRemindOn = mainAppCt.SoundRemindOn;
			//pBpSysInfo->LanguageMode = mainAppCt.LanguageMode;
#endif
			break;

#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
		case MSG_DEVICE_SERVICE_BP_PLAYER_INFO:
			IsBackUpFlag = TRUE;
			if(gpMediaPlayer == NULL)
			{
				return;
			}
			TimeOutSet(&TimerBreakPoint, 500);
			pBpPlayInfo = (BP_PLAYER_INFO *)BP_GetInfo(BP_PLAYER_INFO_TYPE);
#if defined(CFG_APP_CARD_PLAY_MODE_EN)
			if(GetSystemMode() == AppModeCardAudioPlay && gpMediaPlayer != NULL)
			{
				pBpPlayInfo->PlayCardInfo.DirSect = gpMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayCardInfo.FirstClust = gpMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayCardInfo.FileSize = gpMediaPlayer->PlayerFile.obj.objsize;
			}
#endif
#if defined(CFG_APP_USB_PLAY_MODE_EN)
			if(GetSystemMode() == AppModeUDiskAudioPlay && gpMediaPlayer != NULL)
			{
				pBpPlayInfo->PlayUDiskInfo.DirSect = gpMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayUDiskInfo.FirstClust = gpMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayUDiskInfo.FileSize = gpMediaPlayer->PlayerFile.obj.objsize;
			}
#endif
#ifdef CFG_FUNC_LRC_EN
			pBpPlayInfo->LrcFlag = gpMediaPlayer->LrcFlag;
#endif
			pBpPlayInfo->PlayMode = gpMediaPlayer->CurPlayMode;
#ifdef	FUNC_SPECIFY_FOLDER_PLAY_EN	
			SaveStoryVar((void*)&(pBpPlayInfo->StoryInformation));
#endif
			break;

#ifdef BP_PART_SAVE_TO_NVM
		case MSG_DEVICE_SERVICE_BP_PLAYER_INFO_2NVM:
			pBpPlayInfo = (BP_PLAYER_INFO *)BP_GetInfo(BP_PLAYER_INFO_TYPE);
#if defined(CFG_APP_CARD_PLAY_MODE_EN)
			if(GetSystemMode() == AppModeCardAudioPlay && gpMediaPlayer != NULL)
			{
				pBpPlayInfo->PlayCardInfo.PlayTime = (uint16_t)(gpMediaPlayer->CurPlayTime);
				pBpPlayInfo->PlayCardInfo.DirSect = gpMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayCardInfo.FirstClust = gpMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayCardInfo.FileSize = gpMediaPlayer->PlayerFile.obj.objsize;
			}
#endif
#if defined(CFG_APP_USB_PLAY_MODE_EN)
			if(GetSystemMode() == AppModeUDiskAudioPlay && gpMediaPlayer != NULL)
			{
				pBpPlayInfo->PlayUDiskInfo.PlayTime = (uint16_t)(gpMediaPlayer->CurPlayTime);
				pBpPlayInfo->PlayUDiskInfo.DirSect = gpMediaPlayer->PlayerFile.dir_sect;
				pBpPlayInfo->PlayUDiskInfo.FirstClust = gpMediaPlayer->PlayerFile.obj.sclust;
				pBpPlayInfo->PlayUDiskInfo.FileSize = gpMediaPlayer->PlayerFile.obj.objsize;
			}
#endif
#ifdef	FUNC_SPECIFY_FOLDER_PLAY_EN	
			SaveStoryVar((void*)&(pBpPlayInfo->StoryInformation));
#endif
			BP_SaveInfo(1);
			return;
#endif
#endif//defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)

		case MSG_DEVICE_SERVICE_BP_RADIO_INFO:
#ifdef CFG_APP_RADIOIN_MODE_EN
			IsBackUpFlag = TRUE;
			{
				uint32_t i;
				if(sRadioControl == NULL)
				{
					return;
				}
				APP_DBG("save radio\n");
				TimeOutSet(&TimerBreakPoint, 500);
				pBpRadioInfo = (BP_RADIO_INFO *)BP_GetInfo(BP_RADIO_INFO_TYPE);
				//BP_SET_ELEMENT(pBpSysInfo->Volume, gSys.Volume);
				//BP_SET_ELEMENT(pBpSysInfo->Eq, gSys.Eq);
				pBpRadioInfo->CurBandIdx = sRadioControl->CurFreqArea<<6;
				pBpRadioInfo->CurFreq = sRadioControl->Freq;
				pBpRadioInfo->StationCount = sRadioControl->ChlCount;

				if(sRadioControl->ChlCount > 0)
				{
					for(i = 0; i < sRadioControl->ChlCount; i++)
					{
						pBpRadioInfo->StationList[i] = sRadioControl->Channel[i];
					}
				}
			}
#endif
			break;
//		case MSG_DEVICE_SERVICE_BP_ALL_INFO:
//			IsBackUpFlag = TRUE;
//			pBpSysInfo = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);
//			pBpPlayInfo = (BP_PLAYER_INFO *)BP_GetInfo(BP_PLAYER_INFO_TYPE);
//#ifdef CFG_APP_RADIOIN_MODE_EN
//			pBpRadioInfo = (BP_RADIO_INFO *)BP_GetInfo(BP_RADIO_INFO_TYPE);
//#endif
//			break;
		default:
			break;
	}

	if(!IsBackUpFlag || !IsTimeOut(&TimerBreakPoint))
	{
		return;
	}
	//APP_DBG("Save BreakPoint Info\n");
	BP_SaveInfo(0);
	IsBackUpFlag = FALSE;
}
#endif

#ifdef CFG_FUNC_RECORDER_EN		
void EncoderServicePause(void)
{
	MessageContext		msgSend;
	if(deviceServiceCt.msgHandle != NULL)
	{
		if(IsRecoding())
		{
			msgSend.msgId = MSG_MEDIA_RECORDER_GO_PAUSED;
			MessageSend(deviceServiceCt.msgHandle, &msgSend);
		}
	}
}

void SetRecState(uint32_t state)//0: go 1:pause
{
	sRecState = state;
}
#endif

