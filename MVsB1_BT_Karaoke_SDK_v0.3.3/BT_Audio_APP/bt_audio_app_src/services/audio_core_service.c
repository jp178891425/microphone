/**
 **************************************************************************************
 * @file    audio_core_service.c
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

#include <string.h>
#include "type.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "main_task.h"
#include "audio_core_service.h"
#include "app_config.h"
#include "audio_core_api.h"
#include "communication.h"
#include "otg_device_standard_request.h"
#include "device_detect.h" 
#include "bt_manager.h"
#include "soft_watch_dog.h"
#ifdef CFG_FUNC_USB_MIX_MODE
#include "otg_detect.h"
extern uint8_t usb_busy;
#endif
#ifdef CFG_FUNC_I2S_MIX_MODE
#include "i2s_api.h"
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
#include "spdif_mix_api.h"
#endif
#ifdef BT_TWS_SUPPORT
#define AUDIO_CORE_SERVICE_SIZE			768
#else
#define AUDIO_CORE_SERVICE_SIZE			512//768//512//1024
#endif

#define AUDIO_CORE_SERVICE_PRIO			4
#define AUDIO_CORE_SERVICE_TIMEOUT		1		/** 1 ms */


#define ACS_NUM_MESSAGE_QUEUE			16

#if (AUDIO_CORE_SERVICE_SIZE <= 512)
#define USE_SYS_STACK	//can free AUDIO_CORE_SERVICE_SIZE*4 btyes RAM
#endif

typedef struct _AudioCoreServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	MessageHandle		parentMsgHandle;

	TaskState			serviceState;
}AudioCoreServiceContext;

static AudioCoreServiceContext		audioCoreServiceCt;

/**
 * @brief	Audio core servcie init
 * @param	NONE
 * @return	0 for success
 */
static int32_t AudioCoreServiceInit(MessageHandle parentMsgHandle)
{
	memset(&audioCoreServiceCt, 0, sizeof(AudioCoreServiceContext));

	/* register message handle */
	audioCoreServiceCt.msgHandle = MessageRegister(ACS_NUM_MESSAGE_QUEUE);
	if(audioCoreServiceCt.msgHandle == NULL)
	{
		return -1;
	}
	audioCoreServiceCt.serviceState = TaskStateCreating;
	audioCoreServiceCt.parentMsgHandle = parentMsgHandle;

	AudioCoreInit();

	return 0;
}

/*static void AudioCoreServiceDeinit(void)
{
	AudioCoreDeinit();
	audioCoreServiceCt.msgHandle = NULL;
	audioCoreServiceCt.serviceState = TaskStateNone;
	audioCoreServiceCt.parentMsgHandle = NULL;
}*/
uint32_t 	IsAudioCorePause = FALSE;
uint32_t 	IsAudioCorePauseMsgSend = FALSE;
#ifdef BT_TWS_SUPPORT
extern 		TaskHandle_t audio_core_handle;
#endif
static void AudioCoreServiceEntrance(void * param)
{
	MessageContext		msgRecv;
	MessageContext		msgSend;

	audioCoreServiceCt.serviceState = TaskStateReady;

	/* Send message to parent */
	msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_CREATED;
	MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);

#ifdef SOFT_WACTH_DOG_ENABLE
	SWD_task_register(SWD_AudioCoreTask_ID);
#endif
#ifdef BT_TWS_SUPPORT
	audio_core_handle = xTaskGetCurrentTaskHandle();
#endif
	while(1)
	{
		MessageRecv(audioCoreServiceCt.msgHandle, &msgRecv, AUDIO_CORE_SERVICE_TIMEOUT);
#ifdef SOFT_WACTH_DOG_ENABLE
		SWD_task_reset(SWD_AudioCoreTask_ID);
#endif
		switch(msgRecv.msgId)
		{
			case MSG_TASK_START:
				if(audioCoreServiceCt.serviceState == TaskStateStarting)
				{
					audioCoreServiceCt.serviceState = TaskStateRunning;

					msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_STARTED;
					MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				}
				break;
				
			case MSG_TASK_PAUSE:
#ifdef BT_TWS_SUPPORT
				audioCoreServiceCt.serviceState = TaskStatePaused;
				msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_PAUSED;
				MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
#endif
				IsAudioCorePause = TRUE;
				IsAudioCorePauseMsgSend = TRUE;
				break;
			
			case MSG_AUDIO_CORE_HOLD:
				APP_DBG("MSG_AUDIO_CORE_HOLD\n");
				audioCoreServiceCt.serviceState = TaskStatePaused;
				msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_PAUSED;
				MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				break;
			
			case MSG_TASK_RESUME:
				IsAudioCorePause = FALSE;
				if(audioCoreServiceCt.serviceState == TaskStatePaused)
				{
					audioCoreServiceCt.serviceState = TaskStateRunning;
				}
				break;
			case MSG_TASK_STOP:
				//Set para
				AudioCoreDeinit();
				//clear msg
				MessageClear(audioCoreServiceCt.msgHandle);
				//Set state
				audioCoreServiceCt.serviceState = TaskStateStopped;
				//reply
				msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_STOPPED;
				MessageSend(audioCoreServiceCt.parentMsgHandle, &msgSend);
				break;

			default:
				break;
		}

		if(audioCoreServiceCt.serviceState == TaskStateRunning)
		{
			AudioCoreRun();
#ifdef CFG_FUNC_I2S_MIX_MODE
			AudioI2S_DataProcess();
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
			AudioSpdif_DataInProcess();
#endif			
#ifdef CFG_FUNC_TSM_EN
           DecoderTsmProcess();
#endif
		}
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
		btManager.TwsAudioCoreExitFlag = FALSE;
#endif

#ifdef CFG_COMMUNICATION_BY_UART
		UART1_Communication_Receive_Process();
#endif

#ifdef CFG_FUNC_USB_MIX_MODE
	    if(OTG_PortDeviceIsLink())
		 {
			if(usb_busy==0)
			{
				OTG_DeviceRequestProcess();
			}
		 }
#else
#ifdef CFG_COMMUNICATION_BY_USB
		if((GetSystemMode() != 0) 
		&& (GetSystemMode() != AppModeUsbDevicePlay)
		&& (GetSystemMode() != AppModeWaitingPlay)
		&& (GetSystemMode() != AppModeUDiskAudioPlay)
		&& (GetSystemMode() != AppModeUsbPhone)
		)
		{
			if(GetDeviceInitState())		
			{
				OTG_DeviceRequestProcess();
			}
		}
#endif
#endif
	}
}


/**
 * @brief	Get message receive handle of audio core manager
 * @param	NONE
 * @return	MessageHandle
 */
MessageHandle GetAudioCoreServiceMsgHandle(void)
{
	return audioCoreServiceCt.msgHandle;
}

TaskState GetAudioCoreServiceState(void)
{
	return audioCoreServiceCt.serviceState;
}

/**
 * @brief	Start audio core service.
 * @param	 NONE
 * @return  
 */
int32_t AudioCoreServiceCreate(MessageHandle parentMsgHandle)
{
	int32_t		ret = 0;
#ifdef	USE_SYS_STACK
	uint32_t*	sys_stack_addr = NULL;

	sys_stack_addr = (uint32_t*)(0x20003900 - AUDIO_CORE_SERVICE_SIZE*4);//将协同stack给audiocore使用
																		 //如果改变sag的栈低，请注意修改。
	memset(sys_stack_addr, 0 ,AUDIO_CORE_SERVICE_SIZE*4);
#endif

	ret = AudioCoreServiceInit(parentMsgHandle);
	if(!ret)
	{
		audioCoreServiceCt.taskHandle = NULL;
#ifdef	USE_SYS_STACK
		if(sys_stack_addr)
		{
			APP_DBG("Define USE_SYS_STACK!!!\r\n");

			xTaskGenericCreate(  AudioCoreServiceEntrance ,
								 "AudioCoreService" ,
								 AUDIO_CORE_SERVICE_SIZE ,
								 NULL ,
								 AUDIO_CORE_SERVICE_PRIO ,
								 &audioCoreServiceCt.taskHandle ,
								 sys_stack_addr ,
								 NULL );
		}
		else
#endif
		{
			xTaskCreate(AudioCoreServiceEntrance,
						"AudioCoreService",
						AUDIO_CORE_SERVICE_SIZE,
						NULL, AUDIO_CORE_SERVICE_PRIO,
						&audioCoreServiceCt.taskHandle);
		}
		if(audioCoreServiceCt.taskHandle == NULL)
		{
			ret = -1;
		}
	}
	if(ret)
	{
		APP_DBG("AudioCoreService create fail!\n");
	}
	return ret;
}

int32_t AudioCoreServiceStart(void)
{
	MessageContext		msgSend;

	audioCoreServiceCt.serviceState = TaskStateStarting;

	msgSend.msgId		= MSG_TASK_START;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
	return 0;
}

int32_t AudioCoreServicePause(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_PAUSE;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
	return 0;
}

int32_t AudioCoreServiceResume(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_RESUME;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
	return 0;
}

//这个api通过发送消息，来唤起audiocore进程执行，优先执行一次
void AudioCoreServiceMsg(void)
{
#ifdef BT_TWS_SUPPORT
	extern uint32_t tws_init_done;
#endif
	if(audioCoreServiceCt.serviceState == TaskStateRunning
#ifdef BT_TWS_SUPPORT
	&&(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED && tws_init_done)
#endif
	)
	{
		MessageContext		msgSend;
		msgSend.msgId		= MSG_NONE;
		MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
	}
	return;
}

/**
 * @brief	Exit audio core service.
 * @param	NONE
 * @return  
 */
void AudioCoreServiceStop(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_STOP;
	MessageSend(audioCoreServiceCt.msgHandle, &msgSend);
}

int32_t AudioCoreServiceKill(void)
{
	//task	先删任务，再删邮箱，收资源
	if(audioCoreServiceCt.taskHandle != NULL)
	{
		vTaskDelete(audioCoreServiceCt.taskHandle);
		audioCoreServiceCt.taskHandle = NULL;
	}
	
	//Msgbox
	if(audioCoreServiceCt.msgHandle != NULL)
	{
		MessageDeregister(audioCoreServiceCt.msgHandle);
		audioCoreServiceCt.msgHandle = NULL;
	}

	//PortFree...
	return 0;
}
