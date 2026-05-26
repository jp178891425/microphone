/**
 **************************************************************************************
 * @file    main_task.c
 * @brief   Program Entry 
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
#include "type.h"
#include "app_config.h"
#include "timeout.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "dma.h"
#include "clk.h"
#include "main_task.h"
#include "timer.h"
#include "otg_detect.h"
#include "irqn.h"
#include "watchdog.h"
#include "dac.h"
#include "dac_interface.h"
#include "audio_adc.h"
#include "adc_interface.h"
#ifdef CFG_RES_FLASHFS_EN
#include "file.h"
#endif
#include "breakpoint.h"
#include "audio_vol.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "device_service.h"
#include "device_detect.h" 
#include "bt_stack_service.h"
#ifdef CFG_FUNC_RECORDER_FLASHFS
#include "playback_service.h"
#endif
#include "remind_sound_service.h"
#include "shell.h"
#include "ctrlvars.h"
#include "communication.h"
#include "mode_switch_api.h"
#include "media_play_mode.h"
#include "flash_boot.h"
#include "breakpoint.h"
#include "sadc_interface.h"
#include "adc.h"
#include "otg_device_hcd.h"
#include "spdif.h"
#include "otg_device_standard_request.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "adc_key.h"
#include "delay.h"
#include "ir_key.h"
#include "rtc_alarm.h"
#include "rtc.h"
#include "i2s.h"
#include "i2s_interface.h"
#include "uarts_interface.h"
#include "bt_ddb_flash.h"
#include "bb_api.h"
#include "bt_config.h"
#include "bt_stack_api.h"
#include "recorder_service.h"
#include "hdmi_in_api.h"
#include "bt_play_mode.h"
#include "bt_hf_mode.h"
#include "bt_config.h"
#include "efuse.h"
#include "rest_mode.h"
#include "resampler_polyphase.h"
#include "linein_mode.h"
#include "rtc_ctrl.h"
#include "soft_watch_dog.h"
#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
#include "spdif_interface.h"
#define SPDIF_OUT_FIFO_LEN  CFG_PARA_MAX_SAMPLES_PER_FRAME*8*2
uint8_t SpdifFifo[SPDIF_OUT_FIFO_LEN];
#endif
#ifdef CFG_FUNC_USB_MIX_MODE
#include "usb_audio_api.h"
#endif
#ifdef CFG_FUNC_I2S_MIX_MODE
#include "i2s_api.h"
#endif
#ifdef CFG_FUNC_LINE_MIX_MODE
#include "line_api.h"
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
#include "spdif_mix_api.h"
#endif
#ifdef BT_TWS_SUPPORT
#include "bt_tws_api.h"
#include "bt_tws_app_func.h"
extern uint8_t tws_link_status_get(void);//bt lib
#endif
#ifdef CFG_FUNC_DISPLAY_EN
#include "display_service.h"
#endif
extern   HDMIInfo  			 *gHdmiCt;


#if FLASH_BOOT_EN
void start_up_grate(uint32_t UpdateResource);
bool IsUpdataFileExist(void);
#endif
void SystemGotoPowerDown(void);
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
extern void LineInModeEffectConfig(void);
extern void AudioEffectsInit_LineIn(void);
#endif
void SysVarInit(void);
void SleepMainAppTask(void);
inline void WakeupMainAppTask(void);
inline void StartModeExit(void);
static void SelectModeBeforeEnter(void);
uint16_t AudioDAC0DataSetNull(void* Buf, uint16_t Len);
uint16_t AudioDAC0DataSpaceLenGetNull(void);
#ifdef CFG_RES_AUDIO_I2SOUT_EN
void AudioI2sOutParamsSet(void);
#endif

#ifdef BT_TWS_SUPPORT
#define MAIN_APP_TASK_STACK_SIZE		768//512//
#else
#define MAIN_APP_TASK_STACK_SIZE		768//1024
#endif
#define MAIN_APP_TASK_PRIO				4
#define MAIN_APP_TASK_SLEEP_PRIO		6 //进入deepsleep 需要相对其他task最高优先级。
#ifdef CFG_FUNC_USB_MIX_MODE
#define MAIN_NUM_MESSAGE_QUEUE			32
#else
#define MAIN_NUM_MESSAGE_QUEUE			10
#endif

#define SHELL_TASK_STACK_SIZE			512//1024
#define SHELL_TASK_PRIO					2
#define	MODE_JITTER_TIME				50
#define	MODE_ENTER_TIME					5000
#define	MODE_EXIT_TIME					3000
#define	MODE_WORK_TIME					0xfffffff 
#define MAIN_APP_MSG_TIMEOUT			50	/* 50 ms */

#ifdef CFG_APP_REST_MODE_EN//两个宏不能同时用
	#ifdef CFG_FUNC_REMIND_DEEPSLEEP
		#error	"Please donot use CFG_FUNC_REMIND_DEEPSLEEP!!!"
	#endif
#endif

//void mv_speex_init(void);

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
#ifdef CFG_APP_HDMIIN_MODE_EN
	5,//PERIPHERAL_ID_TIMER3,			//2
#else
	255,//PERIPHERAL_ID_TIMER3,			//2
#endif
#if defined(CFG_RES_AUDIO_SPDIFOUT_EN)
	255,//PERIPHERAL_ID_SDIO_RX,			//3
	255,//PERIPHERAL_ID_SDIO_TX,			//4
#else
	255,//PERIPHERAL_ID_SDIO_RX,			//3
	255,//PERIPHERAL_ID_SDIO_TX,			//4
#endif
	255,//PERIPHERAL_ID_UART0_RX,		//5
	255,//PERIPHERAL_ID_TIMER1,			//6
	255,//PERIPHERAL_ID_TIMER2,			//7
#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	4,//PERIPHERAL_ID_SDPIF_RX, 	//8 SPDIF_RX /TX same chanell
	4,//PERIPHERAL_ID_SDPIF_TX, 	//8 SPDIF_RX /TX same chanell
#else	
	255,//PERIPHERAL_ID_SDPIF_RX,		//8 SPDIF_RX /TX same chanell
	255,//PERIPHERAL_ID_SDPIF_TX,		//8 SPDIF_RX /TX same chanell
#endif
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
#ifdef  CFG_RES_AUDIO_I2S0IN_EN 
	4,//PERIPHERAL_ID_I2S0_RX,			//21
#else 
	255,//PERIPHERAL_ID_I2S0_RX,		//21
#endif	
#if	(defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S_PORT==0))
	7,//PERIPHERAL_ID_I2S0_TX,			//22
#elif defined(CFG_RES_AUDIO_I2S0OUT_EN)
	5,//PERIPHERAL_ID_I2S0_TX,			//22	
#else	
	255,//PERIPHERAL_ID_I2S0_TX,		//22
#endif	
#ifdef  CFG_RES_AUDIO_I2S1IN_EN 
	6,//PERIPHERAL_ID_I2S1_RX,			//23
#else
	255,//PERIPHERAL_ID_I2S1_RX,		//23
#endif
#if	(defined(CFG_RES_AUDIO_I2SOUT_EN )&&(CFG_RES_I2S_PORT==1)) || defined(CFG_RES_AUDIO_I2S1OUT_EN)
	7,	//PERIPHERAL_ID_I2S1_TX,		//24
#else
	255,//PERIPHERAL_ID_I2S1_TX,		//24
#endif
	255,//PERIPHERAL_ID_PPWM,			//25
	255,//PERIPHERAL_ID_ADC,     		//26
	255,//PERIPHERAL_ID_SOFTWARE,		//27
};

//软件标记 登记表
uint32_t		SoftwareFlag;

uint8_t gEnterBtHfMode = 0;
uint8_t gEnterBtRecordMode = 0;

MainAppContext	mainAppCt;


TIMER ModeTimer;//避免模式切换频繁，以便充分接收（串行）消息

uint16_t EndModeExit_set = MSG_NONE;//判断是否为蓝牙退出mainapp
uint8_t Bt_sniff_sniff_start_state_get(void);

static AppMode NextModeProcess(AppMode StartMode);

#ifdef CFG_COMMUNICATION_BY_UART
uint8_t UartRxBuf[1024] = {0};
uint8_t UartTxBuf[1024] = {0};
#endif

#ifdef CFG_RES_IR_NUMBERKEY
bool Number_select_flag = 0;
uint16_t Number_value = 0;
TIMER Number_selectTimer;
#endif

#ifdef CFG_FUNC_REMIND_MIX_MODE
void RemindMixTaskInit(void);
uint16_t AudioRemindMixDataGet(void* Buf, uint16_t Len);
uint16_t AudioRemindMixDataLenGet(void);
void AudioMixRemind_Release(void);
void AudioMixRemind_ResMalloc(uint16_t SampleLen);
void AudioMixRemind_ConfigInit(void);
#endif
#ifdef	CFG_FUNC_REMIND_DEEPSLEEP
//初始化休眠提示音后，标记放完提示音后，该进入什么case 操作
MessageId PowerStateMsgHandle = MSG_NONE;
#endif

#ifdef CFG_FUNC_USB_MIX_MODE
//以下个变量为临时变量，后续需要修改，临时解决办法，用于解决BT初始过程，USB声卡枚举失败问题
uint32_t UsbDeviceInitFlag = 0;//是否已经准备枚举
uint32_t APPModeInitOKflag = 0;//功能进入应用模式
//uint32_t UsbDeviceInFlag;//标记设备插入，枚举之后清零
uint32_t UsbDlyCheck = 0;//功能进入应用模式
#endif

#ifdef CFG_FUNC_USB_MIDE_EN
	void usb_midi_play_demo(void);
#endif

static uint8_t ResMallocFlag = 0;

uint8_t GetSysResMallocStatus(void)
{
	return ResMallocFlag;
}

static int32_t MainAppInit(void)
{
	memset(&mainAppCt, 0, sizeof(MainAppContext));

	mainAppCt.msgHandle = MessageRegister(MAIN_NUM_MESSAGE_QUEUE);
	mainAppCt.state = TaskStateCreating;
	mainAppCt.MState = ModeStateNone;
	mainAppCt.appBackupMode = AppModeIdle;
	mainAppCt.appCurrentMode = AppModeIdle;
	mainAppCt.appTargetMode = AppModeIdle;
	return 0;
}


//接收下层service created消息，完毕后start这些servcie
static void MainAppServiceCreating(uint16_t msgId)
{
	if(msgId == MSG_AUDIO_CORE_SERVICE_CREATED)
	{
		APP_DBG("MainApp:AudioCore service created\n");
		mainAppCt.AudioCoreSync = TRUE;
	}
	else if(msgId == MSG_DEVICE_SERVICE_CREATED)
	{
		APP_DBG("MainApp:Device service created\n");
		mainAppCt.DeviceSync = TRUE;
	}
#ifdef CFG_FUNC_DISPLAY_EN
	if(msgId == MSG_DISPLAY_SERVICE_CREATED)
	{
		APP_DBG("MainApp:Display service created\n");
		mainAppCt.DisplaySync = TRUE;
	}
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(msgId == MSG_REMIND_SOUND_SERVICE_CREATED)
	{
		APP_DBG("MainApp:Remind service created\n");
		mainAppCt.RemindSoundSync = TRUE;
	}
#endif

	if(mainAppCt.AudioCoreSync
		&& mainAppCt.DeviceSync
#ifdef CFG_FUNC_DISPLAY_EN
		&& mainAppCt.DisplaySync
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
		&& mainAppCt.RemindSoundSync
#endif
		)
	{
		DeviceServiceStart();
		mainAppCt.DeviceSync = FALSE;
		AudioCoreServiceStart();
		mainAppCt.AudioCoreSync = FALSE;
#ifdef CFG_FUNC_DISPLAY_EN
		DisplayServiceStart();
		mainAppCt.DisplaySync = FALSE;
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
		RemindSoundServiceStart();
		mainAppCt.RemindSoundSync = FALSE;
#endif
		mainAppCt.state = TaskStateReady;
	}
}


//接收下层service started，完毕后准备模式切换。
static void MainAppServiceStarting(uint16_t msgId)
{
	if(msgId == MSG_AUDIO_CORE_SERVICE_STARTED)
	{
		APP_DBG("MainApp:AudioCore service started\n");
		mainAppCt.AudioCoreSync = TRUE;
	}
	else if(msgId == MSG_DEVICE_SERVICE_STARTED)
	{
		APP_DBG("MainApp:Device service started\n");
		mainAppCt.DeviceSync = TRUE;
	}
#ifdef CFG_FUNC_DISPLAY_EN
	else if(msgId == MSG_DISPLAY_SERVICE_STARTED)
	{
		APP_DBG("MainApp:Display service started\n");
		mainAppCt.DisplaySync = TRUE;
	}
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	else if(msgId == MSG_REMIND_SOUND_SERVICE_STARTED)
	{
		APP_DBG("MainApp:Remind service started\n");
		mainAppCt.RemindSoundSync = TRUE;
	}
#endif

	if(mainAppCt.AudioCoreSync
		&& mainAppCt.DeviceSync
#ifdef CFG_FUNC_DISPLAY_EN
		&& mainAppCt.DisplaySync
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
		&& mainAppCt.RemindSoundSync
#endif
		)
	{
		mainAppCt.DeviceSync = FALSE;
		mainAppCt.AudioCoreSync = FALSE;
#ifdef CFG_FUNC_DISPLAY_EN
		mainAppCt.DisplaySync = FALSE;
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
		mainAppCt.RemindSoundSync = FALSE;
#endif

		//等待开机提示音和device扫描时段。在waiting模式内实施。
		SoftFlagRegister(SoftFlagWaitDetect);//此标记期间target不改变。但响应deepsleep等退出/重入。
#ifdef CFG_FUNC_REMIND_SOUND_EN
		{
			RemindSoundServiceItemRequest(SOUND_REMIND_KAIJI, REMIND_ATTR_SYS);
			extern uint8_t Report_Error_Code(void);
			if(Report_Error_Code() == UPDAT_OK) 
			{
				RemindSoundServiceItemRequest(SOUND_REMIND_UPDATAOK, REMIND_ATTR_SYS);
			}
			else if(Report_Error_Code() == NEEDLESS_UPDAT)
			{
				RemindSoundServiceItemRequest(SOUND_REMIND_NOUPDATA, REMIND_ATTR_SYS);
			}else
			{
				//升级失败  后续也可以加一个升级失败的提示音
			}
		}
#else
#if defined(CFG_FUNC_REMIND_MIX_MODE)
				{
					extern uint8_t Report_Error_Code(void);
					if(Report_Error_Code() == UPDAT_OK) 
					{
						RemindMixServiceItemRequest(SOUND_REMIND_UPDATAOK, 0);
					}
					else if(Report_Error_Code() == NEEDLESS_UPDAT)
					{
						RemindMixServiceItemRequest(SOUND_REMIND_NOUPDATA, 0);
					}
					else
					{
						//升级失败	后续也可以加一个升级失败的提示音
					}
				}
#endif
#endif

		mainAppCt.appCurrentMode = AppModeWaitingPlay;
		mainAppCt.appTargetMode = AppModeWaitingPlay;
		;//启动Waiting模式，播提示音。结束后启动首个有效模式-含断电记忆
		//ModeStateSet(ModeCreate);
		if(ModeStateSet(ModeCreate))
		{
			mainAppCt.MState = ModeStateEnter;
			TimeOutSet(&ModeTimer, MODE_ENTER_TIME);//启动模式的 超时保护
			mainAppCt.state = TaskStateRunning;
		}
		else
		{
			MessageContext		msgSend;
			mainAppCt.MState = ModeStateExit;
			msgSend.msgId		= MSG_MAINAPP_NEXT_MODE;
			//APP_DBG("enter mode err, go to next mode\n");
			MessageSend(mainAppCt.msgHandle, &msgSend);
		}
	}
}

uint32_t gBtEnterDeepSleepFlag = 0;//
#ifdef CFG_FUNC_DEEPSLEEP_EN
static void MainAppServicePause(void)
{
	if(mainAppCt.state == TaskStateRunning)//其他状态不响应
	{
#if (defined(CFG_APP_BT_MODE_EN) && defined(CFG_BT_BACKGROUND_RUN_EN))
		if(GetBtStackCt())
		{
			if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
			{
				BtDisconnectCtrl();
			}
			
			if(GetBtManager()->btReconnectTimer.timerFlag)
			{
				BtStopReconnect();
			}
#ifdef BT_TWS_SUPPORT
 			if(GetBtManager()->btTwsReconnectTimer.timerFlag)
 			{
 				BtStopReconnectTws();
 			}
#endif
		}

#endif
		DeviceServicePause();
		mainAppCt.DeviceSync = FALSE;
		AudioCoreServicePause();
		mainAppCt.AudioCoreSync = FALSE;
#ifdef CFG_FUNC_DISPLAY_EN
		DisplayServicePause();
		mainAppCt.DisplaySync = FALSE;
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
		RemindSoundServicePause();
		mainAppCt.RemindSoundSync = FALSE;
#endif
#ifdef CFG_FUNC_USB_MIX_MODE
		void UsbDeviceSleep(void);
		UsbDeviceSleep();//
		UsbDeviceInitFlag = 0;//是否已经准备枚举
		//uint32_t UsbDeviceInFlag;//标记设备插入，枚举之后清零
		UsbDlyCheck = 0;//功能进入应用模式
#endif

		mainAppCt.state = TaskStatePausing;
		//Work态需发起退出，None直接暂停，Enter等~ed消息或超时执行exit，Exit等~ed消息或超时后置Pause态
		if(mainAppCt.MState == ModeStateWork)
		{
			StartModeExit();
		}
	}
}

static void MainAppServicePauseBt(void)
{
	if(mainAppCt.state == TaskStateRunning)//其他状态不响应
	{
		DeviceServicePause();
		mainAppCt.DeviceSync = FALSE;
		AudioCoreServicePause();
		mainAppCt.AudioCoreSync = FALSE;
#ifdef CFG_FUNC_DISPLAY_EN
		DisplayServicePause();
		mainAppCt.DisplaySync = FALSE;
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
		RemindSoundServicePause();
		mainAppCt.RemindSoundSync = FALSE;
#endif
		mainAppCt.state = TaskStatePausing;
		//Work态需发起退出，None直接暂停，Enter等~ed消息或超时执行exit，Exit等~ed消息或超时后置Pause态
		if(mainAppCt.MState == ModeStateWork)
		{
			StartModeExit();
		}
	}
}

//接收下层service Pausing，完毕后准备模式切换。
static void MainAppServicePausing(uint16_t msgId)
{
	if(msgId == MSG_AUDIO_CORE_SERVICE_PAUSED)
	{
		APP_DBG("MainApp:AudioCore service paused\n");
		mainAppCt.AudioCoreSync = TRUE;
	}

	if(msgId == MSG_DEVICE_SERVICE_PAUSED)
	{
		APP_DBG("MainApp:Device service paused\n");
		mainAppCt.DeviceSync = TRUE;
	}

#ifdef CFG_FUNC_DISPLAY_EN
	else if(msgId == MSG_DISPLAY_SERVICE_PAUSED)
	{
		APP_DBG("MainApp:Display service paused\n");
		mainAppCt.DisplaySync = TRUE;
	}
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	else if(msgId == MSG_REMIND_SOUND_SERVICE_PAUSED)
	{
		APP_DBG("MainApp:Remind service paused\n");
		mainAppCt.RemindSoundSync = TRUE;
	}
#endif

	if(mainAppCt.DeviceSync //mainAppCt.AudioCoreSync &&
		#ifdef CFG_FUNC_REMIND_SOUND_EN
		&& mainAppCt.RemindSoundSync
		#endif
		#ifdef CFG_FUNC_DISPLAY_EN
		&& mainAppCt.DisplaySync
		#endif
		&& (mainAppCt.MState == ModeStateNone || mainAppCt.MState == ModeStatePause))
	{

#if (defined(CFG_APP_BT_MODE_EN) && defined(CFG_BT_BACKGROUND_RUN_EN))
	#if (defined (BT_TWS_SUPPORT) && (TWS_PAIRING_MODE != CFG_TWS_ROLE_SLAVE)) || (!defined (BT_TWS_SUPPORT))
		if(msgId != MSG_BTSTACK_DEEPSLEEP)
		{
			uint8_t btDisconnectTimeout = 0;
			uint32_t btDisconnectComplete = 0;
			gBtEnterDeepSleepFlag = 1;

			APP_DBG("confirm bt disconnect\n");
			while(btDisconnectComplete < 4)
			{
				//2s timeout
				vTaskDelay(50);
				btDisconnectTimeout++;
				if(btDisconnectTimeout>=40)
					break;

				if((btDisconnectTimeout % 10)==0)
				{
					if(GetBtLinkState())
						BtDisconnectCtrl();
					else
						btDisconnectComplete=4;
				}
				btDisconnectComplete++;
			}
			APP_DBG("kill bt\n");
			//Kill bt stack service
			BtStackServiceKill();
			vTaskDelay(10);
			//bb reset
			rwip_reset();
			BT_IntDisable();
			vTaskDelay(10);
		}
	#endif
#endif

		APP_DBG("MainApp:Sleep\n");
		AudioCoreSinkMute(0,TRUE,TRUE);
		AudioCoreSinkDisable(0);

		#if defined(BT_SNIFF_ENABLE)&&(TWS_PAIRING_MODE != CFG_TWS_ROLE_SLAVE)
		if(msgId != MSG_BTSTACK_DEEPSLEEP)
		#endif
		{
			vTaskPrioritySet(mainAppCt.taskHandle, MAIN_APP_TASK_SLEEP_PRIO);//设定最高优先级
		}

		DMA_ChannelAllocTableSet((uint8_t*)DmaChannelMap);
		SleepMainAppTask();
#ifdef CFG_FUNC_MAIN_DEEPSLEEP_EN
		Reset_McuSystem();//针对上电后默认进入deepsleep应用场景，建议走系统复位的流程
#endif
		SoftFlagDeregister(SoftFlagWakeUpSouceIsCEC);

#ifdef BT_TWS_SUPPORT
	#if (TWS_PAIRING_MODE != CFG_TWS_ROLE_SLAVE)
		if(msgId != MSG_BTSTACK_DEEPSLEEP)
		{
			DeepSleeping();
		}
		else
		{
			#if defined(BT_SNIFF_ENABLE) && defined(CFG_APP_BT_MODE_EN)
			BtSniffADDAReadySet(0);
			DeepSleeping_BT();
			#endif
		}
	#else
		{
			BtSniffADDAReadySet(0);
			DeepSleeping_BT();
		}
	#endif
#else
		if(msgId != MSG_BTSTACK_DEEPSLEEP)
		{
			DeepSleeping();
		}
		#ifdef	BT_SNIFF_ENABLE
		else
		{
			//Efuse_ReadDataDisable();//睡前需要改1V2，读1V2需要efuse
			SysDeepsleepStart();
			vTaskPrioritySet(mainAppCt.taskHandle, MAIN_APP_TASK_SLEEP_PRIO);//设定最高优先级
			//记录休眠前的状态
			{
				extern BTSNIFF_GET_DEFAULT_CLKCONFIG_t sniff_get_clkconfig;
				sniff_get_clkconfig.get1V2 = Power_LDO12Get();
				sniff_get_clkconfig.sys_div = Clock_SysClkDivGet();
				sniff_get_clkconfig.core_div = Clock_CoreClkDivGet();
				sniff_get_clkconfig.sys_clk = Clock_PllFreqGet();
			}
			while(Bt_sniff_sniff_start_state_get())//没退出sniff就不转maintask。
			{
				vTaskDelay(1);
			}
			Bt_sniff_change_pllclk();//时钟切换逻辑
			vTaskPrioritySet(mainAppCt.taskHandle, MAIN_APP_TASK_PRIO);//设定最高优先级
			Clock_APllLock(240000);
			Efuse_ReadDataEnable();
			SysDeepsleepStop();
		}
		#endif//BT_SNIFF_ENABLE
#endif

		gBtEnterDeepSleepFlag = 0;

#ifdef CFG_FUNC_WAKEUP_MCU_RESET
		Reset_McuSystem();
#endif
		WakeupMainAppTask();
		if(msgId != MSG_BTSTACK_DEEPSLEEP)
		{
			vTaskPrioritySet(mainAppCt.taskHandle, MAIN_APP_TASK_PRIO);
		}
		
#ifdef BT_SNIFF_ENABLE
		WDG_Enable(WDG_STEP_4S);
#endif

		DeviceServiceResume();
		AudioCoreServiceResume();
#ifdef CFG_FUNC_DISPLAY_EN
		DisplayServiceResume();
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
		RemindSoundServiceResume();
#endif

		if(msgId != MSG_BTSTACK_DEEPSLEEP)
		{
			#if defined(CFG_APP_BT_MODE_EN) && defined(CFG_BT_BACKGROUND_RUN_EN) && !defined(CFG_TWS_SOUNDBAR_APP)
			WDG_Feed();
			BtStackServiceStart();
			WDG_Feed();
			#endif
		}

		SoftFlagDeregister(SoftFlagDeepSleepRequest);
#if defined(BT_TWS_SUPPORT) 
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
		TwsCfg.IsRemindSyncEn = 1;
		if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
		{
			if(btManager.twsState != BT_TWS_STATE_CONNECTED)
			{
				mainAppCt.appCurrentMode = mainAppCt.appBackupMode;
				mainAppCt.appTargetMode = mainAppCt.appBackupMode;
			}
			else
			{
				mainAppCt.appTargetMode = mainAppCt.appCurrentMode;
			}
			mainAppCt.appBackupMode = AppModeIdle;//清理后备模式
			SoftFlagDeregister(SoftFlagPlayback);//安全性清理
		}
#else
		if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
		{
			mainAppCt.appCurrentMode = mainAppCt.appBackupMode;
			mainAppCt.appTargetMode = mainAppCt.appBackupMode;
			mainAppCt.appBackupMode = AppModeIdle;//清理后备模式
			SoftFlagDeregister(SoftFlagPlayback);//安全性清理
		}
#endif
		else 
#endif
                if(SoftFlagGet(SoftFlagBtHfDelayEnterSleep))
		{
			mainAppCt.appTargetMode = AppModeBtAudioPlay;
			mainAppCt.appCurrentMode = mainAppCt.appTargetMode;
			mainAppCt.appBackupMode = AppModeIdle;//清理后备模式
		}
		else if(ModeResumeMask(mainAppCt.appCurrentMode)
			|| (mainAppCt.appCurrentMode == AppModeWaitingPlay && !SoftFlagGet(SoftFlagWaitModeMask)))
		{
			if(SoftFlagGet(SoftFlagWakeUpSouceIsCEC))
				mainAppCt.appCurrentMode = AppModeHdmiAudioPlay;
			else
				mainAppCt.appCurrentMode = NextModeProcess(mainAppCt.appCurrentMode);
			mainAppCt.appTargetMode = mainAppCt.appCurrentMode;
			mainAppCt.appBackupMode = AppModeIdle;//清理后备模式
			SoftFlagDeregister(SoftFlagPlayback);//安全性清理
		}
		else
		{
			if(SoftFlagGet(SoftFlagWakeUpSouceIsCEC))
				mainAppCt.appCurrentMode = AppModeHdmiAudioPlay;
			mainAppCt.appTargetMode = mainAppCt.appCurrentMode;
		}
		SoftFlagDeregister(SoftFlagBtHfDelayEnterSleep);
		SoftFlagDeregister(SoftFlagWakeUpSouceIsCEC);
#if defined(BT_TWS_SUPPORT) 
	#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
		AudioDAC_SoftMute(DAC0, TRUE, TRUE); //防止soundbar从机唤醒po音
	#else
		#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
		vTaskDelay(1000); //延时等从机也起来后再一起报提示音
		#endif
		
		#ifdef CFG_FUNC_REMIND_WAKEUP
		if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
		{
			mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
			mainAppCt.appTargetMode = mainAppCt.appCurrentMode = AppModeWaitingPlay;
		}
		SoftFlagRegister(SoftFlagWaitDetect);
		RemindSoundServiceItemRequest(SOUND_REMIND_KAIJI, REMIND_ATTR_SYS);
		#endif
	#endif
#else
	#ifdef CFG_FUNC_REMIND_WAKEUP
		if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
		{
			mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
			mainAppCt.appTargetMode = mainAppCt.appCurrentMode = AppModeWaitingPlay;
		}
#ifdef CFG_FUNC_REMIND_SOUND_EN
		RemindSoundServiceItemRequest(SOUND_REMIND_KAIJI, REMIND_ATTR_SYS);
#elif defined CFG_FUNC_REMIND_MIX_MODE
		RemindMixServiceItemRequest(SOUND_REMIND_KAIJI, TRUE);
#endif
	#endif
#endif
		ModeStateSet(ModeCreate);
		mainAppCt.MState = ModeStateEnter;
		TimeOutSet(&ModeTimer, MODE_ENTER_TIME);//启动模式的 超时保护
		mainAppCt.state = TaskStateRunning;
		WDG_Feed();
		DelayMs(200);//旨在消除一声'po'音，和mic有关，可根据实际效果微调延时。
		AudioCoreSinkEnable(0);
		AudioCoreSinkUnmute(0,TRUE,TRUE);
		SystemVolSet();
	}
}
#endif

//参数模式，选择下一个有效模式，有模式后备时，后备优先。
//backup不能提前清理，有backup条件下，下一个模式是backup或backup的下一个模式。只在发起Enter时清理。
static AppMode NextModeProcess(AppMode StartMode)
{
	AppMode FindMode;

	if(mainAppCt.appBackupMode != AppModeIdle)
	{
		if(CheckModeResource(mainAppCt.appBackupMode))
		{
			FindMode = mainAppCt.appBackupMode;
		}
		else
		{
			FindMode = NextAppMode(mainAppCt.appBackupMode);
		}
	}
	else
	{
		FindMode = NextAppMode(mainAppCt.appCurrentMode);
	}
	return FindMode;
}

/**
 * @func        MsgProcessModeResource
 * @brief       处理模式资源相关事件消息，按键改变模式也归此处理，避免消息串行引起的模式频繁切换
 * @param       uint16_t msgId  
 * @Output      None
 * @return      bool TRUE：appTargetMode更改，FALSE：appTargetMode不变。
 * @Others      此函数更新targetmode，重点是模式选择的策略预定义。具体是否切换currentMode取决于MState模式状态机
 * Record
 */
static bool MsgProcessModeResource(uint16_t msgId)
{
	AppMode AppModes = AppModeWaitingPlay;
	switch(msgId)
	{
		case MSG_DEVICE_SERVICE_LINE_IN:
		case MSG_DEVICE_SERVICE_CARD_IN:
		case MSG_DEVICE_SERVICE_DISK_IN:
#if 1//ndef CFG_COMMUNICATION_BY_USB //USB HID在线调音则USB device 不做后插先播功能
		case MSG_DEVICE_SERVICE_USB_DEVICE_IN:
#endif
		case MSG_DEVICE_SERVICE_HDMI_IN:
			if(mainAppCt.appCurrentMode == AppModeRestPlay)
			{
				break;
			}
			if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
			{
				break;
			}
#if	defined(CFG_APP_BT_MODE_EN)&&(BT_HFP_SUPPORT == ENABLE)
			extern uint32_t getBtHfModeEnterFlag(void);
			if ((mainAppCt.appCurrentMode == AppModeBtHfPlay) //蓝牙通话过程中，不调整
				|| (mainAppCt.appCurrentMode == AppModeBtRecordPlay) //蓝牙K歌录音过程中，不调整
				|| (getBtHfModeEnterFlag()))
			{
				break;
			}
#endif
			if(!SoftFlagGet(SoftFlagRecording | SoftFlagPlayback) //回放或录音期间，不调整。
				&& (mainAppCt.appCurrentMode != AppModeUsbDevicePlay)
#ifdef CFG_APP_USB_PHONE_MODE_EN
				&& (mainAppCt.appCurrentMode != AppModeUsbPhone)
#endif
				)
			{
				if(!SoftFlagGet(SoftFlagWaitModeMask))// 非强制Waiting阶段
				{
#ifndef CFG_FUNC_APP_MODE_AUTO
					AppModes = FindModeByPlugInMsg(msgId);
#else
					if(mainAppCt.appTargetMode == AppModeWaitingPlay)//无音源时，有任何新音源都要查找模式，和后插先播无关。
					{
						AppModes = FindModeByPlugInMsg(msgId);
					}
#endif
				}
				if(AppModes != mainAppCt.appTargetMode && AppModes != AppModeWaitingPlay)
				{
					mainAppCt.appTargetMode = AppModes;
					if(mainAppCt.appCurrentMode != AppModeWaitingPlay)//无需后退。
					{
						if( (mainAppCt.appCurrentMode != AppModeCardPlayBack)
							&&( mainAppCt.appCurrentMode != AppModeUDiskPlayBack)
							&&( mainAppCt.appCurrentMode != AppModeFlashFsPlayBack) )//上一个模式为录音回放 不进行记录
						{
							mainAppCt.appBackupMode = mainAppCt.appCurrentMode;//后插先播只做一层模式后退
						}
					}
					return TRUE;
				}
			}
			break;
		case MSG_DEVICE_SERVICE_CARD_OUT:
		case MSG_DEVICE_SERVICE_LINE_OUT:
		case MSG_DEVICE_SERVICE_DISK_OUT:
#if 1//ndef CFG_COMMUNICATION_BY_USB //USB在线调音则USB device 不做后插先播功能
		case MSG_DEVICE_SERVICE_USB_DEVICE_OUT:	
#endif
		case MSG_DEVICE_SERVICE_HDMI_OUT:
			if(!CheckModeResource(mainAppCt.appCurrentMode))
			{
				AppModes  = NextModeProcess(mainAppCt.appCurrentMode);
				if(AppModes != mainAppCt.appTargetMode)
				{
					mainAppCt.appTargetMode = AppModes;
					APP_DBG("\nMainApp:Find New:%s @:%s  DevOut:%d ",ModeNameStr(AppModes), ModeNameStr(mainAppCt.appTargetMode), msgId);
					return TRUE;
				}
			}
			break;
			
#ifdef CFG_APP_BT_MODE_EN
#ifdef BT_AUTO_ENTER_PLAY_MODE
		case MSG_DEVICE_SERVICE_BTPLAY_IN:
			APP_DBG("MSG: MSG_DEVICE_SERVICE_BTPLAY_IN\n");
			if(mainAppCt.appCurrentMode != AppModeBtAudioPlay)
			{
				mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				mainAppCt.appTargetMode = AppModeBtAudioPlay;
				return TRUE;
			}
			break;
#endif

#if (BT_HFP_SUPPORT == ENABLE)
		case MSG_DEVICE_SERVICE_BTHF_IN:
			APP_DBG("MSG: MSG_DEVICE_SERVICE_BTHF_IN\n");
			if((mainAppCt.appCurrentMode != AppModeBtHfPlay)&&(!gEnterBtHfMode))
			{
				if(!gEnterBtRecordMode)
					mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				mainAppCt.appTargetMode = AppModeBtHfPlay;
				gEnterBtHfMode = 1;
				return TRUE;
			}
			break;
			
		case MSG_DEVICE_SERVICE_BTHF_OUT:
			APP_DBG("MSG: MSG_DEVICE_SERVICE_BTHF_OUT\n");
			if((mainAppCt.appCurrentMode == AppModeBtHfPlay)&&(gEnterBtHfMode))
			{
				gEnterBtHfMode = 0;
				if((mainAppCt.appBackupMode != AppModeIdle)&&(mainAppCt.appBackupMode != AppModeBtHfPlay))
				{
					mainAppCt.appTargetMode = mainAppCt.appBackupMode;
				}
				else
				{
					mainAppCt.appTargetMode = AppModeBtAudioPlay;
				}
				return TRUE;
			}
			break;
			
			case MSG_DEVICE_SERVICE_BTRECORD_IN:
				APP_DBG("MSG: MSG_DEVICE_SERVICE_BTRECORD_IN\n");
				if((mainAppCt.appCurrentMode != AppModeBtRecordPlay)&&(!gEnterBtRecordMode))
				{
					mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
					mainAppCt.appTargetMode = AppModeBtRecordPlay;
					gEnterBtRecordMode = 1;
					return TRUE;
				}
				break;
				
			case MSG_DEVICE_SERVICE_BTRECORD_OUT:
				APP_DBG("MSG: MSG_DEVICE_SERVICE_BTRECORD_OUT\n");
				if((mainAppCt.appCurrentMode == AppModeBtRecordPlay)&&(gEnterBtRecordMode))
				{
					gEnterBtRecordMode = 0;
					if((mainAppCt.appBackupMode != AppModeIdle)&&(mainAppCt.appBackupMode != AppModeBtRecordPlay))
					{
						mainAppCt.appTargetMode = mainAppCt.appBackupMode;
					}
					else
					{
						mainAppCt.appTargetMode = AppModeBtAudioPlay;
					}
					return TRUE;
				}
				break;
#endif

#ifdef BT_TWS_SUPPORT
		case MSG_DEVICE_SERVICE_TWS_SLAVE_CONNECTED:
			APP_DBG("MSG: MSG_DEVICE_SERVICE_TWS_SLAVE_CONNECT\n");
			if(mainAppCt.appCurrentMode != AppModeTwsSlavePlay)
			{
				mainAppCt.appTargetMode = AppModeTwsSlavePlay;
				mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				return TRUE;
			}
			break;
			
		case MSG_DEVICE_SERVICE_TWS_SLAVE_DISCONNECT:
			APP_DBG("MSG: MSG_DEVICE_SERVICE_TWS_SLAVE_DISCONNECT\n");
			if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
			{
				if((mainAppCt.appBackupMode != AppModeIdle)&&(mainAppCt.appBackupMode != AppModeTwsSlavePlay))
				{
					mainAppCt.appTargetMode = mainAppCt.appBackupMode;
				}
				else
				{
					mainAppCt.appTargetMode = AppModeBtAudioPlay;
				}
				return TRUE;
			}
			break;
#endif

#endif
#ifdef CFG_FUNC_ALARM_EN
		  case MSG_RTC_ALARMING:
		  		AppModes = AppModeRestPlay;
				if(AppModes != mainAppCt.appTargetMode)
				{
					mainAppCt.appTargetMode = AppModes;
					if(mainAppCt.appTargetMode == mainAppCt.appBackupMode)
					{
						mainAppCt.appBackupMode = AppModeIdle;//清理
					}
					return TRUE;
				}
		  		break;
#endif

		case MSG_MODE:
#ifdef CFG_APP_BT_MODE_EN
#ifdef BT_TWS_SUPPORT
			#ifdef TWS_SLAVE_MODE_SWITCH_EN
			if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
			{
				tws_link_disconnect();
			}
			#else
			if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
			{
				return FALSE;
			}
			#endif
	#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE) || (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER))
			if((GetBtManager()->twsRole == BT_TWS_SLAVE)&&(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED))
			{
				//return FALSE;
				//extern void tws_link_disconnect(void);
				//tws_link_disconnect();
				BtTwsDisconnectApi();
			}
	#else
			#ifndef TWS_SLAVE_MODE_SWITCH_EN
			if((GetBtManager()->twsRole == BT_TWS_SLAVE)&&(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED))
			{
				//从机组网后,不能进行模式切换,除非先断开组网
				return FALSE;
			}
			#endif
	#endif
#endif

#if (BT_HFP_SUPPORT == ENABLE)
			if(mainAppCt.appCurrentMode == AppModeBtHfPlay) //蓝牙通话过程中，不能强行切换模式
			{
				return FALSE;
			}
			extern uint32_t getBtHfModeEnterFlag(void);
			if (getBtHfModeEnterFlag())
			{
				return FALSE;
			}
#endif
#ifdef BT_RECORD_FUNC_ENABLE
			if(mainAppCt.appCurrentMode == AppModeBtRecordPlay)
			{
				return FALSE;
			}					
#endif

			if(SoftFlagGet(SoftFlagEffectChange))
			{
				break;//屏蔽
			}
#endif
			AppModes = NextModeProcess(mainAppCt.appCurrentMode);
			if(AppModes != mainAppCt.appTargetMode)
			{
				mainAppCt.appTargetMode = AppModes;
				if(mainAppCt.appTargetMode == mainAppCt.appBackupMode)
				{
					mainAppCt.appBackupMode = AppModeIdle;//清理
				}
				return TRUE;
			}
			break;
#ifdef CFG_APP_REST_MODE_EN
		case MSG_POWER:
#ifdef CFG_APP_BT_MODE_EN
#if (BT_HFP_SUPPORT == ENABLE)
			if(mainAppCt.appCurrentMode == AppModeBtHfPlay) //蓝牙通话过程中，不能强行切换模式
			{
				return FALSE;
			}
#endif
#ifdef BT_RECORD_FUNC_ENABLE
			if(mainAppCt.appCurrentMode == AppModeBtRecordPlay)
			{
				return FALSE;
			}
#endif
#endif
			AppModes = AppModeRestPlay;
			if(AppModes != mainAppCt.appTargetMode)
			{
//				RestModeEnable();
//				RestBackupAppSet(mainAppCt.appTargetMode);

				if(!ModeResumeMask(mainAppCt.appCurrentMode))
				{
					mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				}
				mainAppCt.appTargetMode = AppModes;
//				SoftFlagRegister(SoftFlagDeepSleepRequest);//告知reset mode要关机了

				if(mainAppCt.appTargetMode == mainAppCt.appBackupMode)
				{
					mainAppCt.appBackupMode = AppModeIdle;//清理
				}
				return TRUE;
			}
			break;
#endif
#ifdef CFG_FUNC_RECORDER_EN
		case MSG_REC_PLAYBACK:
			if((mainAppCt.appCurrentMode == AppModeUDiskPlayBack 
				|| mainAppCt.appCurrentMode == AppModeCardPlayBack
				|| mainAppCt.appCurrentMode == AppModeFlashFsPlayBack
				|| mainAppCt.appCurrentMode == AppModeFlashPlayBack)
			   && mainAppCt.appBackupMode != AppModeIdle)
			{
				mainAppCt.appTargetMode = NextModeProcess(mainAppCt.appBackupMode);
				return TRUE;
			}
			if(mainAppCt.appCurrentMode == AppModeRestPlay)
			{
				break;
			}
			if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
			{
				break;
			}
#ifdef CFG_APP_BT_MODE_EN
#ifdef BT_RECORD_FUNC_ENABLE
			if(mainAppCt.appCurrentMode == AppModeBtRecordPlay)
			{
				break;
			}					
#endif
#if (BT_HFP_SUPPORT == ENABLE)
			if(mainAppCt.appCurrentMode == AppModeBtHfPlay) //蓝牙通话过程中，不能进入录音回放模式
			{
				return FALSE;
			}
#endif
#endif
#ifdef CFG_FUNC_RECORD_SD_UDISK
#ifdef CFG_FUNC_RECORD_UDISK_FIRST
			if(CheckModeResource(AppModeUDiskPlayBack))
			{

				mainAppCt.appTargetMode = AppModeUDiskPlayBack;
				if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
				{
					mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				}
				SoftFlagRegister(SoftFlagPlayback);
				APP_DBG("MainApp:target:%s\n", "AppModeUDiskPlayBack");
				return TRUE;
			}
			else if( CheckModeResource(AppModeCardPlayBack))
			{
				mainAppCt.appTargetMode = AppModeCardPlayBack;
				if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
				{
					mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				}
				SoftFlagRegister(SoftFlagPlayback);
				APP_DBG("MainApp:target:%s\n", "AppModeCardPlayBack");
				return TRUE;
			}
			break;
#else //CFG_FUNC_RECORD_UDISK_FIRST
			if(CheckModeResource(AppModeCardPlayBack))
			{

				mainAppCt.appTargetMode = AppModeCardPlayBack;
				if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
				{
					mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				}
				SoftFlagRegister(SoftFlagPlayback);
				APP_DBG("MainApp:target:%s\n", "AppModeCardPlayBack");
				return TRUE;
			}
			else if(CheckModeResource(AppModeUDiskPlayBack))
			{
				mainAppCt.appTargetMode = AppModeUDiskPlayBack;
				if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
				{
					mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
				}
				SoftFlagRegister(SoftFlagPlayback);
				APP_DBG("MainApp:target:%s\n", "AppModeUDiskPlayBack");
				return TRUE; 
			}
			break;
#endif//CFG_FUNC_RECORD_UDISK_FIRST
#elif defined(CFG_FUNC_RECORD_FLASHFS)
		if(CheckModeResource(AppModeFlashFsPlayBack))
		{
			mainAppCt.appTargetMode = AppModeFlashFsPlayBack;
			if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
			{
				mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
			}
			SoftFlagRegister(SoftFlagPlayback);
			APP_DBG("MainApp:target:%s\n", "AppModeFlashFsPlayBack");
			return TRUE;
		}
#elif defined(CFG_FUNC_RECORD_FLASH)
		if(CheckModeResource(AppModeFlashPlayBack))
		{
			mainAppCt.appTargetMode = AppModeFlashPlayBack;
			if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
			{
				mainAppCt.appBackupMode = mainAppCt.appCurrentMode;
			}
			SoftFlagRegister(SoftFlagPlayback);
			APP_DBG("target:%s\n", "AppModeFlashPlayBack");
			return TRUE;
		}
#endif //CFG_FUNC_RECORD_SD_UDISK
#endif//CFG_FUNC_RECORDER_EN



#ifdef BT_PROFILE_BQB_ENABLE
		case MSG_AVRCP_STOP:
			APP_DBG("MSG_AVRCP_STOP\n");
			BTCtrlStop();
			break;

		case MSG_A2DP_CONNECT:
			APP_DBG("MSG_A2DP_CONNECT\n");
			A2dpConnect(btManager.btDdbLastAddr);
			break;

		case MSG_AVRCP_CONNECT:
			APP_DBG("MSG_AVRCP_CONNECT\n");
			AvrcpConnect(btManager.btDdbLastAddr);
		break;

		case MSG_HFP_CONNECT:
			APP_DBG("MSG_HFP_CONNECT\n");
			HfpConnect(btManager.btDdbLastAddr);
			break;

		case MSG_SCO_CONNECT:
			APP_DBG("MSG_SCO_CONNECT\n");
			HfpAudioConnect();
		break;

		case MSG_CLR_PAIRED_LIST:
			APP_DBG("MSG_CLR_PAIRED_LIST\n");
			BtDdb_ClearAllPairedInfo();
			break;

		case MSG_BT_BQB_AVDTP_SMG:
			{
				BTBqbAvdtpSmgSet(1);
				APP_DBG("enable AVDTP/SNK/ACP/SIG/SMG/BI-05-C BI-33-C test\n");
			}
			break;

		case MSG_BT_BQB_AVDTP_SMG_BI38C:
			{
				BTBqbAvdtpSmgBI38CSet(1);
				APP_DBG("enable AVDTP/SNK/ACP/SIG/SMG/BI-38-C test\n");
			}
			break;
#endif
		default:
			break;	
	}
	return FALSE;
}

static void AfterWaitingMode(void)
{
	mainAppCt.appTargetMode = AppModeWaitingPlay ;//AppModeBtAudioPlay;//设初始值
	if(mainAppCt.appBackupMode != AppModeIdle)//有记忆
	{
#ifdef BT_TWS_SUPPORT
		if((GetBtManager()->twsState == BT_TWS_STATE_CONNECTED) && (GetBtManager()->twsRole == BT_TWS_SLAVE))
			mainAppCt.appBackupMode = AppModeTwsSlavePlay;
#endif
		APP_DBG("Backup:%d\n", mainAppCt.appBackupMode);
		mainAppCt.appTargetMode = NextModeProcess(mainAppCt.appBackupMode);
		APP_DBG("\n MainApp:select 0:%s\n", ModeNameStr(mainAppCt.appTargetMode));
	}
	else
	{
		mainAppCt.appTargetMode = FindPriorityMode();
		APP_DBG("\n MainApp:select 1:%s\n", ModeNameStr(mainAppCt.appTargetMode));
	}
	if(mainAppCt.appTargetMode != AppModeWaitingPlay)//注意：此后断电记忆模式可能抢占
	{
		ModeStateSet(ModeStop);
		mainAppCt.MState = ModeStateExit;
		TimeOutSet(&ModeTimer, MODE_EXIT_TIME);//启动模式的 超时保护
	}
	//没有合适模式时，继续waiting，不退出。
}

//完成exit后，实施Enter前模式判断。
static void SelectModeBeforeEnter(void)
{
	if(!CheckModeResource(mainAppCt.appTargetMode))//再次音源确认，以防消息丢失，引起无效的模式进入
	{
		mainAppCt.appTargetMode = NextModeProcess(mainAppCt.appCurrentMode);
		mainAppCt.appBackupMode = AppModeIdle;//清理后备模式
		SoftFlagDeregister(SoftFlagPlayback);//安全性清理
	}
	else if(mainAppCt.appBackupMode != AppModeIdle && (!CheckModeResource(mainAppCt.appBackupMode) || mainAppCt.appBackupMode == mainAppCt.appTargetMode))
	{//例：播卡，卡回放，拔卡，此时要进播卡的下一个模式，需对backup清理。
		mainAppCt.appBackupMode = AppModeIdle;
		SoftFlagDeregister(SoftFlagPlayback);//安全性清理
	}
	if(SoftFlagGet(SoftFlagDeepSleepRequest | SoftFlagWaitDetect) && mainAppCt.appTargetMode != AppModeWaitingPlay)
	{
		if(!ModeResumeMask(mainAppCt.appCurrentMode))
		{
			mainAppCt.appBackupMode = mainAppCt.appTargetMode;
		}
		mainAppCt.appTargetMode = AppModeWaitingPlay;
	}
	if(mainAppCt.appTargetMode == AppModeWaitingPlay)
	{
	
		mainAppCt.appCurrentMode = mainAppCt.appTargetMode = AppModeWaitingPlay;//强制启动闲置模式
		vTaskDelay(10);//补丁，旨在让idle任务运行，被删除task释放资源，减少碎片化
		
		APP_DBG("\nMainApp:Start:%s\n", ModeNameStr(mainAppCt.appCurrentMode));
		ModeStateSet(ModeCreate);//此处还应考虑初始化失败后的模式更换？
		mainAppCt.MState = ModeStateEnter;
		TimeOutSet(&ModeTimer, MODE_ENTER_TIME);//启动模式的 超时保护
	}
	else
	{
		mainAppCt.appCurrentMode = mainAppCt.appTargetMode;
		vTaskDelay(10);//补丁，旨在让idle任务运行，被删除task释放资源，减少碎片化
		
		APP_DBG("\nMainApp:Start:%s\n", ModeNameStr(mainAppCt.appCurrentMode));
		//ModeStateSet(ModeCreate);//此处还应考虑初始化失败后的模式更换？
		if(ModeStateSet(ModeCreate))
		{
			mainAppCt.MState = ModeStateEnter;
			TimeOutSet(&ModeTimer, MODE_ENTER_TIME);//启动模式的 超时保护
		}
		else
		{
			MessageContext		msgSend;
			mainAppCt.MState = ModeStateExit;
			msgSend.msgId		= MSG_MAINAPP_NEXT_MODE;
			//APP_DBG("enter mode err, go to next mode\n");
			MessageSend(mainAppCt.msgHandle, &msgSend);	
		}
	}
}

void StartModeExit(void)
{
	if(IsAudioPlayerMute() == FALSE)
	{
		AudioPlayerMute();
	}
	ModeStateSet(ModeStop);
	mainAppCt.MState = ModeStateExit;
	TimeOutSet(&ModeTimer, MODE_EXIT_TIME);//超时维护	
}


void EndModeExit(void)
{
	SoftFlagDeregister(SoftFlagDecoderMask | SoftFlagDecoderSwitch);//安全性清理
#ifdef BT_TWS_SUPPORT
		//tws_master_mute_send(TRUE);//kill 中 app source dis后，主机直接静音，从机需要配合。
#endif
	ModeStateSet(ModeKill);
	//vTaskDelay(10);//补丁，旨在让idle任务运行，被删除task释放资源，减少碎片化
	SoftFlagDeregister(SoftFlagRecording | SoftFlagPlayback | SoftFlagEffectChange);//清理
	MessageContext		MsgSend;
	MsgSend.msgId = MSG_APP_EXIT;
	MessageSend(GetDeviceMessageHandle(), &MsgSend);//通知device更换keymaptable
#ifdef CFG_FUNC_DEEPSLEEP_EN
	if(mainAppCt.state == TaskStatePausing)//deepsleep请求阶段，mode进入暂停态。
	{
		vTaskDelay(10);//补丁，旨在让idle任务运行，被删除task释放资源，减少碎片化
		mainAppCt.MState = ModeStatePause;
		MainAppServicePausing(EndModeExit_set);
	}
	else
#endif
	{
		SelectModeBeforeEnter();
	#if (defined(CFG_APP_USB_AUDIO_MODE_EN))&&(defined(CFG_COMMUNICATION_BY_USB))
		if(GetUSBAudioExitFlag())
		{
			SetUSBAudioExitFlag(FALSE);
			SetDeviceDetectVarReset();
		}
	#endif
	}
}

//音频模式播放时，对模式相关消息的判断和消抖
static void StateAtAppRunning(bool Change)
{
	if(Change && !SoftFlagGet(SoftFlagWaitModeMask))//app播放阶段有模式新请求
	{
		APP_DBG("MainApp:Work change Jitter!\n");
		TimeOutSet(&ModeTimer, MODE_JITTER_TIME);
	}

	if(IsTimeOut(&ModeTimer) && (mainAppCt.appCurrentMode != mainAppCt.appTargetMode)
#ifdef CFG_FUNC_REMIND_SOUND_EN
			&& RemindSoundEmptySys()
#endif
			)
	{
		if(SoftFlagGet(SoftFlagEffectChange))
		{
			APP_DBG("MainApp:Wait FrameChage\n");
			TimeOutSet(&ModeTimer, MODE_JITTER_TIME);
		}
		else
		{
			StartModeExit();
		}
	}
}

/**
 * @func        MainAppModeProcess
 * @brief       模式的进入、退出、运行
 * @param       uint16_t Msg  
 * @Output      None
 * @return      None
 * @Others      此函数主要以mode的状态机，结合定时器消抖和保护，实现启用app,created/started 结束stopped,kill Msg同步，
 * @Others      要点：work状态期间消息处理与转发
 * Record
 */
static void MainAppModeProcess(uint16_t Msg)
{
	MessageContext	msgSend;
#ifdef CFG_FUNC_DISPLAY_EN
	MessageContext	DispmsgSend;
#endif
	bool	TargetModeChange = FALSE;

	if(!SoftFlagGet(SoftFlagWaitModeMask))//启动阶段，不自动检测模式变化。
	{
		TargetModeChange = MsgProcessModeResource(Msg);
	}
//声卡功能检测，底层已经消抖
#ifdef CFG_FUNC_USB_MIX_MODE
	if(Msg == MSG_DEVICE_SERVICE_USB_DEVICE_IN)
	{

		UsbDlyCheck= 2;

	}
	else if(Msg == MSG_DEVICE_SERVICE_USB_DEVICE_OUT)
	{
		extern void UsbDeviceDisable(void);
		UsbDeviceDisable();
		UsbDeviceInitFlag = 0;
	}
	if((UsbDlyCheck>0)&& (APPModeInitOKflag>0))
	{
		extern void UsbDeviceEnable(void);
		UsbDlyCheck--;
		if(UsbDlyCheck==0)
		{
			if(UsbDeviceInitFlag == 0)
			{
				UsbDeviceEnable();
				UsbDeviceInitFlag = 1;
			}
		}
	}
#endif	
	switch(mainAppCt.MState)
	{
			//过渡态，如有新的TargetMode不做消抖
		case ModeStateEnter: 
			if(IsTimeOut(&ModeTimer))
			{
				APP_DBG("\nMainApp:Enter %s timeout，care for ram.\n", ModeNameStr(mainAppCt.appCurrentMode));
				StartModeExit();
			}
			else
			{
				switch(AppStateMsgAck(Msg))
				{	
						//模式已创建 处理
					case MSG_APP_CREATED:
						if(mainAppCt.state == TaskStatePausing)
						{
							StartModeExit();
						}
						else if(mainAppCt.appCurrentMode == mainAppCt.appTargetMode || SoftFlagGet(SoftFlagWaitModeMask))
						{
							ModeStateSet(ModeStart);
						}
						else
						{
							StartModeExit();
							APP_DBG("\nMainApp:exiting %s to %s After Created", ModeNameStr(mainAppCt.appCurrentMode), ModeNameStr(mainAppCt.appTargetMode));
						}
						break;

						//模式已启动 处理
					case MSG_APP_STARTED:
						if(mainAppCt.state == TaskStatePausing)
						{
							StartModeExit();
						}
						else if(mainAppCt.appCurrentMode == mainAppCt.appTargetMode || SoftFlagGet(SoftFlagWaitModeMask))//说明切入进程中targety没有变化,waiting模式除外
						{
							APP_DBG("MainApp:Mode Started\n");
#ifdef CFG_FUNC_REMIND_SOUND_EN
							if(!RemindSoundEmptySys() && !SoftFlagGet(SoftFlagDecoderApp))
							{
								RemindSoundServicePlay();
							}
#endif
							mainAppCt.MState = ModeStateWork;
							TimeOutSet(&ModeTimer, MODE_WORK_TIME);
							msgSend.msgId = Msg;
							MessageSend(GetDeviceMessageHandle(), &msgSend);//通知device更换keymaptable	
                            #ifdef CFG_FUNC_DISPLAY_EN
							DispmsgSend.msgId = MSG_DISPLAY_SERVICE_DEV;
							MessageSend(GetDisplayMessageHandle(), &DispmsgSend);
                            #endif
						}
						else//target有变化，则进入退出进程
						{
							StartModeExit();
						}
						break;
				}
			}
			break;

			//稳定播控状态，系统消息基本在此处理，播控消息转当前mode App。
		case ModeStateWork:
			AudioPlayerMenuCheck();
			switch(Msg)
			{
					//模式故障处理
				case MSG_MEDIA_PLAY_MODE_FAILURE://任务已启动，可以stop流程;like:SD/Udisk空、初始化失败
					mainAppCt.appTargetMode = NextModeProcess(mainAppCt.appCurrentMode);
#ifdef	CFG_FUNC_DEV_ERROR_MODE
					//避免设备故障的模式，反复进入后失败退出。特别是只有一个可用模式时。
					if(mainAppCt.appCurrentMode == AppModeCardAudioPlay)
					{
						ResourceDeregister(AppResourceCardForPlay);
					}
					else if(mainAppCt.appCurrentMode == AppModeUDiskAudioPlay)
					{
						ResourceDeregister(AppResourceUDiskForPlay);
					}
#endif
					StartModeExit();
					APP_DBG("MainApp:MEDIA_PLAY_MODE_FAILURE\n");
					break;

				case MSG_WAITING_PERMISSION_MODE:
#ifdef	CFG_FUNC_REMIND_DEEPSLEEP
					if(SoftFlagGet(SoftFlagDeepSleepRequest))
					{
						SoftFlagDeregister(SoftFlagDeepSleepRequest);
						if(PowerStateMsgHandle == MSG_DEEPSLEEP)
						{
							msgSend.msgId = MSG_TASK_DEEPSLEEP;
							PowerStateMsgHandle = MSG_NONE;
						}
						else if(PowerStateMsgHandle == MSG_BTSTACK_DEEPSLEEP)
						{
							msgSend.msgId = MSG_TASK_BTSNIFF;
							PowerStateMsgHandle = MSG_NONE;
						}
						else if(PowerStateMsgHandle == MSG_POWERDOWN)
						{
							msgSend.msgId = MSG_POWERCHAGE_POWERDOWN;
							PowerStateMsgHandle = MSG_NONE;
						}
						//msgSend.msgId = MSG_TASK_DEEPSLEEP;
						MessageSend(mainAppCt.msgHandle, &msgSend);
					}
					else
#endif
					{
						AfterWaitingMode();
					}
					break;

				default: //非系统消息转发给app
					//AudioPlayerMenuCheck();
					if(Msg != MSG_NONE && GetAppMessageHandle() != NULL)
					{
					
					//	APP_DBG("MSG=%x\n",Msg);
						msgSend.msgId = Msg;
						MessageSend(GetAppMessageHandle(), &msgSend);
					}


					break;
			}
			StateAtAppRunning(TargetModeChange);
			break;

				//过渡态，如有新的TargetMode不做消抖
		case ModeStateExit:
			if(IsTimeOut(&ModeTimer))
			{
				APP_DBG("\nMainApp:Exit %s timeout，care for task.\n", ModeNameStr(mainAppCt.appCurrentMode));
				EndModeExit();
			}
			else if(AppStateMsgAck(Msg) == MSG_APP_STOPPED)
			{
				APP_DBG("MainApp:exit stopped OK !\n");
				EndModeExit();
			}
			break;
		default://case ModeStatePause:
//			if(mainAppCt.appTargetMode != AppModeIdle && CheckModeResource(mainAppCt.appTargetMode))
//			{
//				mainAppCt.appCurrentMode = mainAppCt.appTargetMode;
//				APP_DBG("\nMainApp:Enter:%s\n", ModeNameStr(mainAppCt.appCurrentMode));
//				ModeStateSet(ModeCreate);//此处还应考虑初始化失败后的模式更换？
//				mainAppCt.MState = ModeStateEnter;
//				TimeOutSet(&ModeTimer, MODE_ENTER_TIME);//启动模式的 超时保护
//			}
			break;
	}
}

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
extern void SystemAppSourceVolSet(void);
#define AudioCoreAppSourceVolSetZero()		AudioCoreSourceVolSet(APP_SOURCE_NUM,0,0)
#define AudioCoreAppSourceVolRecover()		SystemAppSourceVolSet()
#endif

//各模式下的通用消息处理, 共有的提示音在此处理，因此要求调用次API前，确保APP running状态。避免解码器没准备好。
void CommonMsgProccess(uint16_t Msg)
{
#ifdef CFG_FUNC_DISPLAY_EN
	MessageContext	msgSend;
#endif
	switch(Msg)
	{
		case MSG_MENU://菜单键
			APP_DBG("menu key\n");
			AudioPlayerMenu();
			break;

		case MSG_MUTE:
			APP_DBG("MSG_MUTE\n");
			#ifdef  CFG_APP_HDMIIN_MODE_EN
			if(GetSystemMode() == AppModeHdmiAudioPlay)
			{
				if(IsHDMISourceMute() == TRUE)
					HDMISourceUnmute();
				else
					HDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteStatus = IsHDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteflag = 2;
			}
			else
			#endif
			{
				AudioPlayerMute();
			}
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MUTE;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		case MSG_MAIN_VOL_UP:
			SystemVolUp();
			APP_DBG("MSG_MAIN_VOL_UP\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_VOL;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		case MSG_MAIN_VOL_DW:
			SystemVolDown();
			APP_DBG("MSG_MAIN_VOL_DW\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_VOL;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		case MSG_MUSIC_VOLUP:
			AudioMusicVolUp();
#ifdef BT_TWS_SUPPORT
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
#endif
			APP_DBG("MSG_MUSIC_VOLUP\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MUSIC_VOL;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		case MSG_MUSIC_VOLDOWN:
			AudioMusicVolDown();
#ifdef BT_TWS_SUPPORT
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
#endif
			APP_DBG("MSG_MUSIC_VOLDOWN\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MUSIC_VOL;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		#if CFG_RES_MIC_SELECT
		case MSG_MIC_VOLUP:
			AudioMicVolUp();
			APP_DBG("MSG_MIC_VOLUP\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MIC_VOL;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		case MSG_MIC_VOLDOWN:
			AudioMicVolDown();
			APP_DBG("MSG_MIC_VOLDOWN\n");
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_MIC_VOL;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;
		#endif

#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_PLAY_SYNC_VOLUME_CHANGED:
			APP_DBG("MSG_BT_PLAY_SYNC_VOLUME_CHANGED\n");
#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
			AudioMusicVolSet(GetBtSyncVolume());
#ifdef BT_TWS_SUPPORT
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
#endif
#endif
			break;
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
		#ifdef CFG_FUNC_MIC_KARAOKE_EN
		case MSG_MIC_EFFECT_UP:
			if(mainAppCt.ReverbStep < MAX_MIC_REVB_STEP)
			{
				mainAppCt.ReverbStep++;
			}
			else
			{
				mainAppCt.ReverbStep = 0;
			}
			ReverbStepSet(mainAppCt.ReverbStep);
			APP_DBG("MSG_MIC_EFFECT_UP\n");
			APP_DBG("ReverbStep = %d\n", mainAppCt.ReverbStep);
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;
		#endif

		#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
		case MSG_EQ:
			APP_DBG("MSG_EQ\n");
			if(mainAppCt.EqMode < EQ_MODE_VOCAL_BOOST)
			{
				mainAppCt.EqMode++;
			}
			else
			{
				mainAppCt.EqMode = EQ_MODE_FLAT;
			}
			APP_DBG("EqMode = %d\n", mainAppCt.EqMode);
			#ifdef BT_TWS_SUPPORT
			if((GetBtManager()->twsRole != BT_TWS_SLAVE) || (GetBtManager()->twsState != BT_TWS_STATE_CONNECTED))
			#endif
			{
				#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
				EqModeSet(mainAppCt.EqMode);
				#endif
			}
			#ifdef BT_TWS_SUPPORT
			tws_eq_mode_send(mainAppCt.EqMode);
			#endif

			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_EQ;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;
		#endif

		#ifdef CFG_FUNC_MIC_KARAOKE_EN
		case MSG_EFFECTMODE:
			if(mainAppCt.MState == ModeStateWork && !SoftFlagGet(SoftFlagEffectChange))
			{
				APP_DBG("MSG_EFFECTMODE\n");
				if(mainAppCt.appCurrentMode == AppModeBtHfPlay)//蓝牙通话模式下，不支持音效模式切换
				{
					break;
				}
				if(mainAppCt.appCurrentMode == AppModeUsbPhone)//usb通话模式下，不支持音效模式切换
				{
					break;
				}
#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
				mainAppCt.EffectMode++;
				extern uint8_t flash_effect_total;
				if(mainAppCt.EffectMode >= flash_effect_total)
				{
					mainAppCt.EffectMode = 0;
				}

#else
				if(mainAppCt.EffectMode < EFFECT_MODE_WaWaYin)
				{
					mainAppCt.EffectMode++;
				}
				else
				{
					mainAppCt.EffectMode = EFFECT_MODE_HunXiang;
				}
#endif
				APP_DBG("EffectMode = %d\n", mainAppCt.EffectMode);
				switch(mainAppCt.EffectMode)
				{
					case EFFECT_MODE_HunXiang:
#ifdef CFG_FUNC_REMIND_SOUND_EN
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindSoundServiceItemRequest(SOUND_REMIND_LIUXINGH,REMIND_ATTR_NORMAL);
#else
	#ifdef CFG_FUNC_REMIND_MIX_MODE
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindMixServiceItemRequest(SOUND_REMIND_LIUXINGH,TRUE);
	#endif
#endif
						break;
					case EFFECT_MODE_DianYin:
#ifdef CFG_FUNC_REMIND_SOUND_EN
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindSoundServiceItemRequest(SOUND_REMIND_DIANYIN,REMIND_ATTR_NORMAL);
#else
	#ifdef CFG_FUNC_REMIND_MIX_MODE
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindMixServiceItemRequest(SOUND_REMIND_DIANYIN,TRUE);
	#endif
#endif
						break;
					case EFFECT_MODE_MoYin:
#ifdef CFG_FUNC_REMIND_SOUND_EN
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindSoundServiceItemRequest(SOUND_REMIND_MOYIN,REMIND_ATTR_NORMAL);
#else
	#ifdef CFG_FUNC_REMIND_MIX_MODE
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindMixServiceItemRequest(SOUND_REMIND_MOYIN,TRUE);
	#endif
#endif
						break;
					case EFFECT_MODE_HanMai:
#ifdef CFG_FUNC_REMIND_SOUND_EN
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindSoundServiceItemRequest(SOUND_REMIND_HANMAI,REMIND_ATTR_NORMAL);
#else
	#ifdef CFG_FUNC_REMIND_MIX_MODE
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindMixServiceItemRequest(SOUND_REMIND_HANMAI,TRUE);
	#endif
#endif
						break;
					case EFFECT_MODE_NanBianNv:
#ifdef CFG_FUNC_REMIND_SOUND_EN
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindSoundServiceItemRequest(SOUND_REMIND_NVSHEN,REMIND_ATTR_NORMAL);
#else
	#ifdef CFG_FUNC_REMIND_MIX_MODE
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindMixServiceItemRequest(SOUND_REMIND_NVSHEN,TRUE);
	#endif
#endif
						break;
					case EFFECT_MODE_NvBianNan:
#ifdef CFG_FUNC_REMIND_SOUND_EN
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindSoundServiceItemRequest(SOUND_REMIND_NANSHEN,REMIND_ATTR_NORMAL);
#else
	#ifdef CFG_FUNC_REMIND_MIX_MODE
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindMixServiceItemRequest(SOUND_REMIND_NANSHEN,TRUE);
	#endif
#endif
						break;
					case EFFECT_MODE_WaWaYin:
#ifdef CFG_FUNC_REMIND_SOUND_EN
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindSoundServiceItemRequest(SOUND_REMIND_WAWAYIN,REMIND_ATTR_NORMAL);
#else
	#ifdef CFG_FUNC_REMIND_MIX_MODE
						gCtrlVars.remind_type = REMIND_TYPE_KEY;
						RemindMixServiceItemRequest(SOUND_REMIND_WAWAYIN,TRUE);
					#endif
#endif
						break;

					default:
						break;
				}
				AudioCoreAppSourceVolSetZero();
#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
				AudioEffectModeSel(mainAppCt.EffectMode, 2);//0=init hw,1=effect,2=hw+effect
#else
				AudioEffectModeSel(mainAppCt.EffectMode, 1);//0=init hw,1=effect,2=hw+effect
#endif
				{
					extern TIMER EffectChangeTimer;
					TimeOutSet(&EffectChangeTimer, 500);//临时修改方案，保证音效模式频繁切换时，能规避死机现象(恢复为500ms超时等待处理)
				}
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			break;

		case MSG_MIC_FIRST:
			APP_DBG("MSG_MIC_FIRST\n");
			#ifdef CFG_FUNC_SHUNNING_EN
			gCtrlVars.ShunningMode = !gCtrlVars.ShunningMode;
			APP_DBG("ShunningMode = %d\n", gCtrlVars.ShunningMode);
			#endif
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_SHUNNING;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;
		#endif

		#ifdef CFG_FUNC_MIC_TREB_BASS_EN
		case MSG_MIC_TREB_UP:
			APP_DBG("MSG_MIC_TREB_UP\n");
			if(mainAppCt.MicTrebStep < MAX_BASS_TREB_GAIN)
			{
				mainAppCt.MicTrebStep++;
			}
			APP_DBG("MicTrebStep = %d\n", mainAppCt.MicTrebStep);
			MicBassTrebAjust(mainAppCt.MicBassStep, mainAppCt.MicTrebStep);
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_TRE;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MIC_TREB_DW:
			APP_DBG("MSG_MIC_TREB_DW\n");
			if(mainAppCt.MicTrebStep)
			{
				mainAppCt.MicTrebStep--;
			}
			APP_DBG("MicTrebStep = %d\n", mainAppCt.MicTrebStep);
			MicBassTrebAjust(mainAppCt.MicBassStep, mainAppCt.MicTrebStep);
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_TRE;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MIC_BASS_UP:
			APP_DBG("MSG_MIC_BASS_UP\n");
			if(mainAppCt.MicBassStep < MAX_BASS_TREB_GAIN)
			{
				mainAppCt.MicBassStep++;
			}
			APP_DBG("MicBassStep = %d\n", mainAppCt.MicBassStep);
			MicBassTrebAjust(mainAppCt.MicBassStep, mainAppCt.MicTrebStep);
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_BAS;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MIC_BASS_DW:
			APP_DBG("MSG_MIC_BASS_DW\n");
			if(mainAppCt.MicBassStep)
			{
				mainAppCt.MicBassStep--;
			}
			APP_DBG("MicBassStep = %d\n", mainAppCt.MicBassStep);
			MicBassTrebAjust(mainAppCt.MicBassStep, mainAppCt.MicTrebStep);
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_BAS;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;
		#endif

		#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
		case MSG_MUSIC_TREB_UP:
			APP_DBG("MSG_MUSIC_TREB_UP\n");
			if(mainAppCt.MusicTrebStep < MAX_BASS_TREB_GAIN)
			{
				mainAppCt.MusicTrebStep++;
			}
			APP_DBG("MusicTrebStep = %d\n", mainAppCt.MusicTrebStep);
			#ifdef BT_TWS_SUPPORT
			if((GetBtManager()->twsRole != BT_TWS_SLAVE) || (GetBtManager()->twsState != BT_TWS_STATE_CONNECTED))
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}
			#ifdef BT_TWS_SUPPORT
			tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MUSIC_TREB_DW:
			APP_DBG("MSG_MUSIC_TREB_DW\n");
			if(mainAppCt.MusicTrebStep)
			{
				mainAppCt.MusicTrebStep--;
			}
			APP_DBG("MusicTrebStep = %d\n", mainAppCt.MusicTrebStep);
			#ifdef BT_TWS_SUPPORT
			if((GetBtManager()->twsRole != BT_TWS_SLAVE) || (GetBtManager()->twsState != BT_TWS_STATE_CONNECTED))
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}
			#ifdef BT_TWS_SUPPORT
			tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MUSIC_BASS_UP:
			APP_DBG("MSG_MUSIC_BASS_UP\n");
			if(mainAppCt.MusicBassStep < MAX_BASS_TREB_GAIN)
			{
				mainAppCt.MusicBassStep++;
			}
			APP_DBG("MusicBassStep = %d\n", mainAppCt.MusicBassStep);
			#ifdef BT_TWS_SUPPORT
			if((GetBtManager()->twsRole != BT_TWS_SLAVE) || (GetBtManager()->twsState != BT_TWS_STATE_CONNECTED))
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}
			#ifdef BT_TWS_SUPPORT
			tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;

		case MSG_MUSIC_BASS_DW:
			APP_DBG("MSG_MUSIC_BASS_DW\n");
			if(mainAppCt.MusicBassStep)
			{
				mainAppCt.MusicBassStep--;
			}
			APP_DBG("MusicBassStep = %d\n", mainAppCt.MusicBassStep);
			#ifdef BT_TWS_SUPPORT
			if((GetBtManager()->twsRole != BT_TWS_SLAVE) || (GetBtManager()->twsState != BT_TWS_STATE_CONNECTED))
			#endif
			{
				MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			}
			#ifdef BT_TWS_SUPPORT
			tws_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			#endif
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;
		#endif

		case MSG_3D:
			APP_DBG("MSG_3D\n");
			#if CFG_AUDIO_EFFECT_MUSIC_3D_EN
			gCtrlVars.music_threed_unit.three_d_en = !gCtrlVars.music_threed_unit.three_d_en;
			#endif
			#if CFG_AUDIO_EFFECT_MUSIC_3D_PLUS_EN
			gCtrlVars.music_threed_plus_unit.three_d_en = !gCtrlVars.music_threed_plus_unit.three_d_en;
			#endif
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_3D;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		case MSG_VOCAL_CUT:
			APP_DBG("MSG_VOCAL_CUT\n");
			gCtrlVars.vocal_cut_unit.vocal_cut_en = !gCtrlVars.vocal_cut_unit.vocal_cut_en;
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_VOCAL_CUT;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

		case MSG_VB:
			APP_DBG("MSG_VB\n");
			#if CFG_AUDIO_EFFECT_MUSIC_VIRTUAL_BASS_EN
			gCtrlVars.music_vb_unit.vb_en = !gCtrlVars.music_vb_unit.vb_en;
			#endif
			#ifdef CFG_FUNC_DISPLAY_EN
            msgSend.msgId = MSG_DISPLAY_SERVICE_VB;
            MessageSend(GetDisplayMessageHandle(), &msgSend);
			#endif
			break;

#endif

		case MSG_REC_MUSIC:
			SetRecMusic(0);
			break;

		#ifdef CFG_FUNC_RTC_EN
		case MSG_RTC_SET_TIME:
			RTC_ServiceModeSelect(0,0);
			break;

		case MSG_RTC_SET_ALARM:
			RTC_ServiceModeSelect(0,1);
			break;

		case MSG_RTC_DISP_TIME:
			RTC_ServiceModeSelect(0,2);
			break;

		case MSG_RTC_UP:
			RTC_RtcUp();
			break;

		case MSG_RTC_DOWN:
			RTC_RtcDown();
			break;

		#ifdef CFG_FUNC_SNOOZE_EN
		case MSG_RTC_SNOOZE:
			if(mainAppCt.AlarmRemindStart)
			{
				mainAppCt.AlarmRemindCnt = 0;
				mainAppCt.AlarmRemindStart = 0;
				mainAppCt.SnoozeOn = 1;
				mainAppCt.SnoozeCnt = 0;
			}
			break;
		#endif

		#endif //end of  CFG_FUNC_RTC_EN

        #ifdef BT_TWS_SUPPORT
		case MSG_BT_TWS_OUT_MODE:
            APP_DBG("MSG_BT_TWS_OUT_MODE\n");
			extern void TwsOutModeSel(void);
			TwsOutModeSel();
			break;
		#endif

		case MSG_REMIND1:
			#ifdef CFG_FUNC_REMIND_MIX_MODE
			gCtrlVars.remind_type = REMIND_TYPE_BACKGROUND;
			RemindMixServiceItemRequest(SOUND_REMIND_ZHANGSHE,TRUE);
            #else
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			gCtrlVars.remind_type = REMIND_TYPE_BACKGROUND;
			RemindSoundServiceItemRequest(SOUND_REMIND_ZHANGSHE,REMIND_ATTR_NORMAL);
			#endif
			#endif
			break;

		case MSG_MIDI_TEST:
           #ifdef CFG_FUNC_USB_MIDE_EN
			usb_midi_play_demo();
            #endif
			break;

		case MSG_DEVICE_SERVICE_BATTERY_LOW:
			//RemindSound request
			APP_DBG("MSG_DEVICE_SERVICE_BATTERY_LOW\n");
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			RemindSoundServiceItemRequest(SOUND_REMIND_DLGUODI, REMIND_ATTR_NORMAL);
			#endif
			#ifdef CFG_FUNC_REMIND_MIX_MODE
			RemindMixServiceItemRequest(SOUND_REMIND_DLGUODI, FALSE);
			#endif
			break;

#ifdef CFG_APP_BT_MODE_EN
		#ifdef POWER_ON_BT_ACCESS_MODE_SET
		case MSG_BT_OPEN_ACCESS:
			GetBtManager()->btScanDisable = !GetBtManager()->btScanDisable;	
			DBG("GetBtManager()->btScanDisable = %d\n", GetBtManager()->btScanDisable);
			BtAccessModeUpdate(GetBtDeviceConnState());
			break;
		#endif

		//蓝牙连接断开消息,用于提示音
		case MSG_BT_STATE_CONNECTED:
			APP_DBG("[BT_STATE]:BT Connected...\n");
#if BT_HID_SUPPORT == ENABLE
			if(!Get_hid_connect())
				HidConnect(GetBtManager()->remoteAddr);
#endif
			//异常回连过程中，不提示连接断开提示音
			if(btManager.btDutModeEnable)
				break;

			if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(RemindSoundServiceItemRequest(SOUND_REMIND_CONNECT, REMIND_ATTR_SYS))
				{
					if(!SoftFlagGet(SoftFlagWaitBtRemindEnd)&&SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						SoftFlagRegister(SoftFlagWaitBtRemindEnd);
					}
				}
				else
				#endif
				#ifdef CFG_FUNC_REMIND_MIX_MODE
				if(RemindMixServiceItemRequest(SOUND_REMIND_CONNECT, FALSE))
				{
					if(!SoftFlagGet(SoftFlagWaitBtRemindEnd)&&SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						SoftFlagRegister(SoftFlagWaitBtRemindEnd);
					}
				}
				else
				#endif
				{
					if(SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						MessageContext		msgSend;
						SoftFlagDeregister(SoftFlagDelayEnterBtHf);

						msgSend.msgId = MSG_DEVICE_SERVICE_ENTER_BTHF_MODE;
						MessageSend(GetMainMessageHandle(), &msgSend);
					}
				}
			}
			break;

		case MSG_BT_STATE_DISCONNECT:
			APP_DBG("[BT_STATE]:BT Disconnected...\n");

			if(btManager.btDutModeEnable)
				break;

			APP_DBG("Bt Disconnect Reason:");
			switch(GetBtDisconReason())
			{
				case BT_DISCON_REASON_REMOTE_DIS:
					APP_DBG("BT_DISCON_REASON_REMOTE_DIS\n");
					break;

				case BT_DISCON_REASON_LOCAL_DIS:
					APP_DBG("BT_DISCON_REASON_LOCAL_DIS\n");
					break;

				case BT_DISCON_REASON_LINKLOSS:
					APP_DBG("BT_DISCON_REASON_LINKLOSS\n");
					break;

				default:
					break;
			}

			//异常回连过程中，不提示连接断开提示音
			if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				if((mainAppCt.appCurrentMode != AppModeBtHfPlay)\
					&&(mainAppCt.appCurrentMode != AppModeBtRecordPlay)\
					&&(mainAppCt.appCurrentMode != AppModeWaitingPlay))
				{
					// #ifdef CFG_FUNC_REMIND_MIX_MODE
					// RemindMixServiceItemRequest(SOUND_REMIND_DISCONNE, FALSE);
					// #else	
					// RemindSoundServiceItemRequest(SOUND_REMIND_DISCONNE, REMIND_ATTR_NORMAL);
					// #endif
				}
				else
					SoftFlagRegister(SoftFlagDiscDelayMask);
			}
			break;
#endif

#ifdef BT_TWS_SUPPORT
		case MSG_BT_TWS_AUDIO_START:
			#if defined (CFG_FUNC_REMIND_SOUND_EN) || defined (CFG_FUNC_REMIND_MIX_MODE)
			if(SoftFlagGet(SoftFlagTwsRemind))
			{
				if(GetBtManager()->twsRole == BT_TWS_MASTER)
				{
					#ifdef CFG_FUNC_REMIND_MIX_MODE
					RemindMixServiceItemRequest(SOUND_REMIND_TWS_CONN, FALSE);
					#else	
					RemindSoundServiceItemRequest(SOUND_REMIND_TWS_CONN, REMIND_ATTR_SYS);
					#endif
				}
				SoftFlagDeregister(SoftFlagTwsRemind);
			}
			#endif
			break;
			
		case MSG_BT_TWS_DISCONNECT_REMIND:
			#if defined (CFG_FUNC_REMIND_SOUND_EN) || defined (CFG_FUNC_REMIND_MIX_MODE)
			if(GetSystemMode() == AppModeTwsSlavePlay)
			{
				SoftFlagRegister(SoftFlagTwsDisconnectRemind);
			}
			else
			{
				#ifdef CFG_FUNC_REMIND_MIX_MODE
				RemindMixServiceItemRequest(SOUND_REMIND_TWS_DISC, FALSE);
				#else	
				RemindSoundServiceItemRequest(SOUND_REMIND_TWS_DISC, REMIND_ATTR_SYS);
				#endif
			}
			#endif
			break;
#endif

		default:
			#ifdef CFG_ADC_LEVEL_KEY_EN
			AdcLevelMsgProcess(Msg);
			#endif
			break;
	}
}

extern void rwip_reset(void);

//test, sam 20190702
void AudioResRelease(void);
bool AudioResMalloc(uint16_t SampleLen);
void AudioResInit(void);
void SamplesFrameUpdata(void);
#ifdef	CFG_FUNC_REMIND_DEEPSLEEP
void PowerStateChange()
{
	//再次注释上方客户可以做一些进入休眠的操作。
	if(!SoftFlagGet(SoftFlagDeepSleepRequest))
	{
		SoftFlagRegister(SoftFlagDeepSleepRequest | SoftFlagWaiting);
		StartModeExit();
	}
}
#endif

#ifdef BT_TWS_SUPPORT
#ifndef BT_TWS_FUNCTION_KEY_SWITCH
int16_t tws_audio_core_buf[512*2];//512sample 立体声
uint16_t TWSDataGet(void* Buf, uint16_t Samples);
uint16_t TWSDataLenGet(void);

void tws_slave_fifo_clear(void)
{
	memset(tws_audio_core_buf,0,sizeof(tws_audio_core_buf));
}
#endif
#endif



static void MainAppTaskEntrance(void * param)
{
	int16_t i;
	uint16_t dac_fifo_len;
	uint16_t dacx_fifo_len;
	MessageContext		msg;

	DelayMsFunc = (DelayMsFunction)vTaskDelay; //提高Os条件下驱动层延时函数精度，非OS默认使用DelayMs
	DMA_ChannelAllocTableSet((uint8_t*)DmaChannelMap);
#ifdef CFG_FUNC_MAIN_DEEPSLEEP_EN


	WDG_Feed();
	vTaskDelay(3000); //调试阶段务必保持大延时，以防芯片锁死SW，无法再次下载，除非有flashboot
	WDG_Feed();
	vTaskDelay(2000);
	APP_DBG("Main:DeepSleep\n");
//#ifdef CFG_PARA_WAKEUP_SOURCE_IR
//	IRKeyInit();//此处为IR配置初始化 
////#endif
	SoftFlagDeregister(SoftFlagWakeUpSouceIsCEC);
	DeepSleeping();
#endif
	SarADC_Init();

	//For OTG check
#if defined(CFG_FUNC_UDISK_DETECT)
#if defined(CFG_FUNC_USB_DEVICE_EN)
 	OTG_PortSetDetectMode(1,1);
#else
 	OTG_PortSetDetectMode(1,0);
#endif
#else
#if defined(CFG_FUNC_USB_DEVICE_EN)
 	OTG_PortSetDetectMode(0,1);
#else
 	OTG_PortSetDetectMode(0,0);
#endif
#endif
 	Timer_Config(TIMER2,1000,0);
 	Timer_Start(TIMER2);
 	NVIC_EnableIRQ(Timer2_IRQn);

#ifdef CFG_FUNC_BREAKPOINT_EN
 	BP_LoadInfo();
#endif

	SysVarInit();
	
	SoftFlagDeregister(SoftFlagMask);

	CtrlVarsInit();//音效初始化参数，后续需要调整	
	
#ifdef CFG_APP_HDMIIN_MODE_EN
	HDMI_CEC_DDC_Init();
#endif

#ifdef BT_TWS_SUPPORT
#ifndef BT_TWS_FUNCTION_KEY_SWITCH
	extern void TWS_Param_Init(void);
	TWS_Param_Init();
#endif
#endif
	///////////////////////////////AudioCore/////////////////////////////////////////
	mainAppCt.AudioCore =  (AudioCoreContext*)&AudioCore;
	memset(mainAppCt.AudioCore, 0, sizeof(AudioCoreContext));
	mainAppCt.SamplesPreFrame = CFG_PARA_SAMPLES_PER_FRAME;//默认数据帧大小
	mainAppCt.SampleRate = CFG_PARA_SAMPLE_RATE;

#ifdef CFG_FUNC_USB_MIX_MODE
	APPModeInitOKflag = 0;
	UsbDevicePowerInit();
	UsbDevicePlayDeInit();
#endif
	
#ifdef CFG_FUNC_I2S_MIX_MODE
	AudioI2S_Release();
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
	AudioSpdif_Release();
#endif
    #if CFG_AUDIO_EFFECT_PITCH_DECTOR_EN
	mainAppCt.SamplesPreFrame = CFG_PARA_PITCH_DECT_SAMPLES_PER_FRAME;//默认数据帧大小
    #endif
	if(!AudioResMalloc(mainAppCt.SamplesPreFrame))
	{
		APP_DBG("Res Error!\n");
		return;
	}
	
#if defined (BT_TWS_SUPPORT) && !defined (CFG_FUNC_MIC_KARAOKE_EN)
	// TWS对相位要求较高的方案需要关掉karaoke; 并加大4倍dac的缓存保证相位
	dac_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2 *2 * 4;
	dacx_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2 * 4;
#else
	dac_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2 *2;
	dacx_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2;
#endif

	//DAC init
#if defined (CFG_RES_AUDIO_DAC0_EN) && defined (CFG_RES_AUDIO_DACX_EN)
	AudioDAC_Init(ALL, mainAppCt.SampleRate, (void*)mainAppCt.DACFIFO, dac_fifo_len, (void*)mainAppCt.DACXFIFO, dacx_fifo_len);
#elif defined (CFG_RES_AUDIO_DAC0_EN)
	AudioDAC_Init(DAC0, mainAppCt.SampleRate, (void*)mainAppCt.DACFIFO, dac_fifo_len , NULL, 0);
#elif defined (CFG_RES_AUDIO_DACX_EN)
	AudioDAC_Init(DAC1, mainAppCt.SampleRate, NULL, 0, (void*)mainAppCt.DACXFIFO, dacx_fifo_len);
#else
	APP_DBG("DAC0 DACX All Disable!!!!!!!!!!\n");
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	AudioSpdif_OutInit(GPIOA28, mainAppCt.SampleRate, SpdifFifo, SPDIF_OUT_FIFO_LEN);
#endif

#ifdef BT_TWS_SUPPORT
#ifndef BT_TWS_FUNCTION_KEY_SWITCH
	mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].Enable = 1;
	mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].FuncDataGet = TWSDataGet;
	mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].FuncDataGetLen = TWSDataLenGet;
	mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].IsSreamData = 0;
	mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].PcmFormat = 2;
	mainAppCt.AudioCore->AudioSource[TWS_SOURCE_NUM].PcmInBuf = (int16_t *)tws_audio_core_buf;//(int16_t *)mainAppCt.Source0Buf_ADC1;
//	AudioCoreSourceDisable(TWS_SOURCE_NUM);
#endif
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	 AudioI2sOutParamsSet();
#endif

	//Mic1 analog  = Soure0.
	AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	AudioADC_MicBias1Enable(1);

#if (defined(BT_TWS_SUPPORT) || CFG_RES_MIC_SELECT)
	AudioADC_DynamicElementMatch(ADC1_MODULE, TRUE, TRUE);
	AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1);
	AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2);
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 15, 4);//0db bypass
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 15, 4);

	//Mic1   digital
	AudioADC_DigitalInit(ADC1_MODULE, mainAppCt.SampleRate, (void*)mainAppCt.ADCFIFO, mainAppCt.SamplesPreFrame * 2 * 2 * 2);

#ifdef CFG_FUNC_LINE_MIX_MODE
	AudioADC_DigitalInit(ADC0_MODULE, mainAppCt.SampleRate, (void*)mainAppCt.Fifo_Line, mainAppCt.SamplesPreFrame * 2 * 2 * 2);

	mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].FuncDataGet = AudioLine_DataGet;
	mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].FuncDataGetLen = AudioLine_DataLenGet;
	mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].IsSreamData = TRUE;
	mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].PcmFormat = 2;//
	mainAppCt.AudioCore->AudioSource[LINE_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source2Buf_Line;
	AudioCoreSourceEnable(LINE_SOURCE_NUM);
	AudioCoreSourceMute(LINE_SOURCE_NUM,1,1);
#endif

	//Soure0.
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].FuncDataGet = AudioADC1DataGet;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].FuncDataGetLen = AudioADC1DataLenGet;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].IsSreamData = TRUE;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].PcmFormat = 2;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source0Buf_ADC1;
	AudioCoreSourceEnable(MIC_SOURCE_NUM);
#endif
	
	//sink0
	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].Enable = 0;
    #ifdef CFG_RES_AUDIO_DAC0_EN
	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].FuncDataSet = AudioDAC0DataSet;
	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].FuncDataSpaceLenGet = AudioDAC0DataSpaceLenGet;
    #else
	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].FuncDataSet = AudioDAC0DataSetNull;
	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].FuncDataSpaceLenGet = AudioDAC0DataSpaceLenGetNull;
    #endif
//	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].FuncDataSet = AudioDAC0DataSet;
//	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].FuncDataSpaceLenGet = AudioDAC0DataSpaceLenGet;
	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink0Buf_DAC0;
	mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmFormat = 2;
	AudioCoreSinkEnable(AUDIO_DAC0_SINK_NUM);

#ifdef CFG_FUNC_REMIND_MIX_MODE
    mainAppCt.AudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].Enable = 0;
    mainAppCt.AudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].FuncDataGet = AudioRemindMixDataGet;
    mainAppCt.AudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].FuncDataGetLen = AudioRemindMixDataLenGet;
    mainAppCt.AudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].IsSreamData = 0;
    mainAppCt.AudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].PcmFormat = 2;//test mic audio effect
    mainAppCt.AudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source2Buf_MixRemind;
#endif
#ifdef CFG_RES_AUDIO_DACX_EN
	//不改变原来结构，sink2 用于DAC-X，可以根据实际情况调整
	mainAppCt.AudioCore->AudioSink[AUDIO_DACX_SINK_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSink[AUDIO_DACX_SINK_NUM].FuncDataSet = AudioDAC1DataSet;
	mainAppCt.AudioCore->AudioSink[AUDIO_DACX_SINK_NUM].FuncDataSpaceLenGet = AudioDAC1DataSpaceLenGet;
	mainAppCt.AudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_DACX;
	mainAppCt.AudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmFormat = 1;
	AudioCoreSinkEnable(AUDIO_DACX_SINK_NUM);
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	mainAppCt.AudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].Enable = 0;
	mainAppCt.AudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].FuncDataSet = AudioSpdifTXDataSet;
	mainAppCt.AudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].FuncDataSpaceLenGet = AudioSpdifTXDataSpaceLenGet;
	mainAppCt.AudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_Spdif; //sink audiocore 内buf 1 frame。
	AudioCoreSinkEnable(AUDIO_SPDIF_SINK_NUM);
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	//不改变原来结构，sink2 用于I2S OUTs，可以根据实际情况调整
	mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].Enable = 0;
#if CFG_RES_I2S_MODE == 1
	AudioCore.AudioSink[AUDIO_I2SOUT_SINK_NUM].SreamDataState = 1;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
#endif
#if (CFG_RES_I2S_PORT == 0)
	mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].FuncDataSet = AudioI2S0_DataSet;
	mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].FuncDataSpaceLenGet = AudioI2S0_DataSpaceLenGet;
#else
	mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].FuncDataSet = AudioI2S1_DataSet;
	mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].FuncDataSpaceLenGet = AudioI2S1_DataSpaceLenGet;
#endif
	mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_I2S;
	mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmFormat = 2;
	AudioCoreSinkEnable(AUDIO_I2SOUT_SINK_NUM);
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	AudioEffectModeSel(mainAppCt.EffectMode, 2);//0=init hw,1=effect,2=hw+effect
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
	AudioI2S_ConfigInit();
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
	 AudioSpdif_ConfigInit();
#endif
	////Audio Core音量配置
	SystemVolSet();
	
	for( i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
	   mainAppCt.AudioCore->AudioSource[i].PreGain = 4095;//默认使用4095， 0dB
	}

#ifdef CFG_FUNC_REMIND_MIX_MODE
	RemindMixTaskInit();
#endif

	AudioCoreServiceCreate(mainAppCt.msgHandle);
	mainAppCt.AudioCoreSync = FALSE;
	DeviceServiceCreate(mainAppCt.msgHandle);
	mainAppCt.DeviceSync = FALSE;
#ifdef CFG_FUNC_DISPLAY_EN
	DisplayServiceCreate(mainAppCt.msgHandle);
	mainAppCt.DisplaySync = FALSE;
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	RemindSoundServiceCreate(mainAppCt.msgHandle);
	mainAppCt.RemindSoundSync = FALSE;
#endif

#ifdef CFG_FUNC_ALARM_EN
	mainAppCt.AlarmFlag = FALSE;
#endif

#ifndef CFG_APP_BT_MODE_EN
	volatile unsigned long *p;
	uint32_t temp = 0;
	p = (volatile unsigned long *)0x40021070;//volatile unsigned long *
	temp = *p;
	temp &=  0xFFFFFC00;
	temp |=  BT_TRIM;
	temp |=  (BT_TRIM << 5);
	if((BT_TRIM >= 0) && (BT_TRIM <= 0xff))
	{
		*(volatile unsigned long *)0x40021070 = temp;
	}else{
		printf("BT_TRIM is erro not writ!!!");
	}
	
#endif

#if (defined(CFG_APP_BT_MODE_EN) && defined(CFG_BT_BACKGROUND_RUN_EN))
	//将蓝牙任务创建移至此处,以便优先申请协议栈使用的内存空间,不影响其他的任务; 开机睡眠时，蓝牙stack再次开，避免上电马上退出。
	BtStackServiceStart();//蓝牙设备驱动serivce 启动失败时，目前是挂起，无同步消息。
//	IRKeyInit();//clk源被改？
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	{
		extern TIMER EffectChangeTimer;
		TimeOutSet(&EffectChangeTimer, 120);
	}
#endif
	APP_DBG("MainApp:run\n");
#ifdef DEVICE_SERVICE_DELETE_EN
	uint32_t sysTick_temp;
	extern uint32_t gSysTick;
	sysTick_temp = gSysTick;
#endif	
#ifdef SOFT_WACTH_DOG_ENABLE
	SWD_task_register(SWD_MainTask_ID);
#endif
	while(1)
	{
#ifdef SOFT_WACTH_DOG_ENABLE
		SWD_task_reset(SWD_MainTask_ID);
#endif
		WDG_Feed();
#ifdef BT_TWS_SUPPORT
#if defined(BT_SNIFF_ENABLE) && defined(CFG_APP_BT_MODE_EN)
		tws_sniff_check_adda_process();//检测sniff后adda是否恢复的轮询
#endif
#endif
#ifdef DEVICE_SERVICE_DELETE_EN
		MessageRecv(mainAppCt.msgHandle, &msg, 2);
		extern void DeviceServiceEntrance(MessageContext msgRecv);
		DeviceServiceEntrance(msg);
		if(!msg.msgId && ((sysTick_temp+MAIN_APP_MSG_TIMEOUT) < gSysTick))
			continue;
		sysTick_temp = gSysTick;
#else
		MessageRecv(mainAppCt.msgHandle, &msg, MAIN_APP_MSG_TIMEOUT);
#endif
#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
		extern void AudioEffSaveToFlash(void);
		AudioEffSaveToFlash();
#endif	
#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
        extern uint32_t  Silence_Power_Off_Time;
        if(!Silence_Power_Off_Time)
		{
			Silence_Power_Off_Time = SILENCE_POWER_OFF_DELAY_TIME;
			APP_DBG("power off\n");
			#ifdef	CFG_FUNC_POWERKEY_EN
			msg.msgId =  MSG_POWERDOWN;
			#endif

			#ifdef	CFG_FUNC_DEEPSLEEP_EN
			msg.msgId =  MSG_DEEPSLEEP;
			#endif
        }
#endif
#if FLASH_BOOT_EN
		//设备经过播放，预搜索mva包登记，可升级。拔出时取消登记。
		#ifndef FUNC_UPDATE_CONTROL
		extern uint8_t Report_Error_Code(void);
		if((Report_Error_Code() == UPDAT_OK) || (Report_Error_Code() == NEEDLESS_UPDAT) )
		{
			;
		}
		else if(SoftFlagGet(SoftFlagMvaInCard) && GetSystemMode() == AppModeCardAudioPlay)
		{
			APP_DBG("MainApp:updata file exist in Card\n");
			#ifdef FUNC_OS_EN
			if(SDIOMutex != NULL)
			{
				osMutexLock(SDIOMutex);
			}
			#endif
			start_up_grate(AppResourceCard);
			#ifdef FUNC_OS_EN
			if(SDIOMutex != NULL)
			{
				osMutexUnlock(SDIOMutex);
			}
			#endif
		}
		else if(SoftFlagGet(SoftFlagMvaInUDisk) && GetSystemMode() == AppModeUDiskAudioPlay)
		{
			APP_DBG("MainApp:updata file exist in Udisk\n");
			#ifdef FUNC_OS_EN
			if(UDiskMutex != NULL)
			{
				//osMutexLock(UDiskMutex);
				while(osMutexLock_1000ms(UDiskMutex) != 1)
				{
					WDG_Feed();
				}
			}
			#endif
			start_up_grate(AppResourceUDisk);
			#ifdef FUNC_OS_EN
			if(UDiskMutex != NULL)
			{
				osMutexUnlock(UDiskMutex);
			}
			#endif
		}
		#endif


//		uint8_t cmd = 0;
//		if(UART0_RecvByte(&cmd))
//		{
//			if(cmd == 'y')
//			{
//				start_up_grate(0xFFFFFFFF);
//			}
//		}
#endif

		switch(msg.msgId)
		{
#ifdef CFG_FUNC_REMIND_SOUND_EN
			case MSG_REMIND_SOUND_SERVICE_CREATED:
#endif
#ifdef CFG_FUNC_DISPLAY_EN
			case MSG_DISPLAY_SERVICE_CREATED:
#endif
			case MSG_DEVICE_SERVICE_CREATED:
			case MSG_AUDIO_CORE_SERVICE_CREATED:	
				MainAppServiceCreating(msg.msgId);
				break;
			
#ifdef CFG_FUNC_REMIND_SOUND_EN
			case MSG_REMIND_SOUND_SERVICE_STARTED:
#endif
#ifdef CFG_FUNC_DISPLAY_EN
			case MSG_DISPLAY_SERVICE_STARTED:
#endif
			case MSG_DEVICE_SERVICE_STARTED:
			case MSG_AUDIO_CORE_SERVICE_STARTED:
				MainAppServiceStarting(msg.msgId);
				break;
#if FLASH_BOOT_EN
			case MSG_DEVICE_SERVICE_CARD_OUT:
				SoftFlagDeregister(SoftFlagMvaInCard);//清理mva包标记
				break;
			
			case MSG_DEVICE_SERVICE_DISK_OUT:
				SoftFlagDeregister(SoftFlagMvaInUDisk);//清理mva包标记
				break;
			
			case MSG_UPDATE:
				#ifdef FUNC_UPDATE_CONTROL
				APP_DBG("MainApp:UPDATE MSG\n");
				//设备经过播放，预搜索mva包登记，可升级。拔出时取消登记。
				if(SoftFlagGet(SoftFlagMvaInCard) && GetSystemMode() == AppModeCardAudioPlay)
				{
					APP_DBG("MainApp:updata file exist in Card\n");
					start_up_grate(AppResourceCard);
				}
				else if(SoftFlagGet(SoftFlagMvaInUDisk) && GetSystemMode() == AppModeUDiskAudioPlay)
				{
					APP_DBG("MainApp:updata file exist in Udisk\n");
					start_up_grate(AppResourceUDisk);
				}
				#endif
				break;
#endif

#ifdef	CFG_FUNC_POWERKEY_EN
			case MSG_POWERDOWN:
#ifdef BT_TWS_SUPPORT
				//主从同步关机
				TwsSyncPowerOff(msg.msgId);
#endif				
#ifdef	CFG_FUNC_REMIND_DEEPSLEEP
				PowerStateMsgHandle = MSG_POWERDOWN;
				PowerStateChange();
				break;
			case MSG_POWERCHAGE_POWERDOWN:
#endif
				APP_DBG("MainApp:POWERDOWN\n");
				//消息转发给当前APP，APP保存现场，进入Stop模式，回消息给到main
				//SystemPowerDown();
				SystemGotoPowerDown();
				break;
#endif
#ifdef CFG_FUNC_DEEPSLEEP_EN
			case MSG_DEEPSLEEP:
#ifdef CFG_APP_HDMIIN_MODE_EN
				if(GetSystemMode() == AppModeHdmiAudioPlay)
				{
					if(SoftFlagGet(SoftFlagDeepSleepMsgIsFromTV) == 0)//非电视端发来的关机
					{
						HDMI_CEC_ActivePowerOff(200);
					}
					SoftFlagDeregister(SoftFlagDeepSleepMsgIsFromTV);
				}
#endif
#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT == ENABLE))
				if(GetSystemMode() == AppModeBtHfPlay)
				{
					if(SoftFlagGet(SoftFlagBtHfDelayEnterSleep) == 0)//通话过程中关机
					{
						gEnterBtHfMode = 0;
						ExitBtHfMode();
						SoftFlagRegister(SoftFlagBtHfDelayEnterSleep);
					}
				}
				extern void SetBtHfModeEnterFlag(uint32_t flag);
				SetBtHfModeEnterFlag(0);
#endif
#ifdef BT_TWS_SUPPORT
				//主从同步关机
				TwsSyncPowerOff(msg.msgId);
#endif
#ifdef	CFG_FUNC_REMIND_DEEPSLEEP
				PowerStateMsgHandle = MSG_DEEPSLEEP;
				PowerStateChange();
				break;
			case MSG_TASK_DEEPSLEEP:
#endif
				EndModeExit_set = MSG_NONE;
				MainAppServicePause();
				break;
			case MSG_BTSTACK_DEEPSLEEP:
#ifdef	CFG_FUNC_REMIND_DEEPSLEEP
				PowerStateMsgHandle = MSG_BTSTACK_DEEPSLEEP;
				PowerStateChange();
				break;
			case MSG_TASK_BTSNIFF:
#endif
				vTaskDelay(500);
				EndModeExit_set = MSG_BTSTACK_DEEPSLEEP;
				MainAppServicePauseBt();
				break;
			case MSG_AUDIO_CORE_SERVICE_PAUSED:
			case MSG_DEVICE_SERVICE_PAUSED:
			case MSG_REMIND_SOUND_SERVICE_PAUSED:
			case MSG_DISPLAY_SERVICE_PAUSED:
				if(mainAppCt.state == TaskStatePausing)// 状态限制
				{
					MainAppServicePausing(msg.msgId);
				}
				break;
#endif

#ifdef CFG_SOFT_POWER_KEY_EN
			case MSG_SOFT_POWER:
				SoftKeyPowerOff();
				break;
#endif

#ifdef CFG_APP_BT_MODE_EN
			case MSG_BT_ENTER_DUT_MODE:
				if(mainAppCt.appCurrentMode == AppModeBtAudioPlay)
					BtEnterDutModeFunc();
				break;

#ifdef BT_SNIFF_ENABLE
			case MSG_BT_SNIFF:
	#ifdef BT_TWS_SUPPORT
		#if((TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE))
				if(GetBtManager()->twsState > BT_TWS_STATE_NONE)
				{
					if(tws_link_status_get() == 0)
						break;
				}
				if(GetBtManager()->twsState == BT_TWS_STATE_NONE)
				{
					sniff_lmpsend_set(0);
					MessageContext		msgSend;
					msgSend.msgId		= MSG_DEEPSLEEP;
					MessageSend(GetMainMessageHandle(), &msgSend);
					break;
				}
					
				if(sniff_lmpsend_get() == 0)
				{
					sniff_lmpsend_set(1);
					if(GetBtManager()->twsState > BT_TWS_STATE_NONE)
					{
						#ifdef CFG_APP_HDMIIN_MODE_EN
						if(GetSystemMode() == AppModeHdmiAudioPlay)
						{
							if(SoftFlagGet(SoftFlagDeepSleepMsgIsFromTV) == 0)//非电视端发来的关机
							{
								HDMI_CEC_ActivePowerOff(200);
							}
							SoftFlagDeregister(SoftFlagDeepSleepMsgIsFromTV);
						}
						#endif
						
						if(GetBtManager()->twsRole == BT_TWS_MASTER)
						{
							extern void tws_stop_transfer(void);//发起sniff请求，主从停止TWS传输，主从都会进入 tws_stop_callback().
							tws_stop_transfer();
						}
						else
						{
							extern void tws_slave_send_cmd_sniff(void);
							tws_slave_send_cmd_sniff();
						}
					}
					else
					{
						sniff_lmpsend_set(0);
						MessageContext		msgSend;
						msgSend.msgId		= MSG_DEEPSLEEP;
						MessageSend(GetMainMessageHandle(), &msgSend);
					}
				}
			break;
		#else
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_DEEPSLEEP;
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
			break;
		#endif
	#else
			{
		#ifdef CFG_APP_HDMIIN_MODE_EN
				if(GetSystemMode() == AppModeHdmiAudioPlay)
				{
					if(SoftFlagGet(SoftFlagDeepSleepMsgIsFromTV) == 0)//非电视端发来的关机
					{
						HDMI_CEC_ActivePowerOff(200);
					}
					SoftFlagDeregister(SoftFlagDeepSleepMsgIsFromTV);
				}
		#endif
				//BtStartEnterSniffMode();
			}
			break;
	#endif
			break;

#endif//BT_SNIFF_ENABLE

			case MSG_BTSTACK_BB_ERROR:
				APP_DBG("bb and bt stack reset\n");
				RF_PowerDownBySw();
				WDG_Feed();
				//reset bb and bt stack
				rwip_reset();
				BtStackServiceKill();
				WDG_Feed();
				vTaskDelay(50);
				RF_PowerDownByHw();
				WDG_Feed();
				//reset bb and bt stack
				BtStackServiceStart();
				break;


#if (BT_HFP_SUPPORT == ENABLE)
				case MSG_DEVICE_SERVICE_ENTER_BTHF_MODE:
				{
					EnterBtHfMode();
				}
				break;
#endif
#ifdef BT_AUTO_ENTER_PLAY_MODE
			case MSG_BT_A2DP_STREAMING:
				//播放歌曲时,有模式切换需求,则在此消息中开始进行模式切换操作
				if((mainAppCt.appCurrentMode != AppModeBtAudioPlay)&&(mainAppCt.appCurrentMode != AppModeBtHfPlay))
				{
					MessageContext		msgSend;
					
					APP_DBG("Enter Bt Audio Play Mode...\n");
					ResourceRegister(AppResourceBtPlay);
					
					// Send message to main app
					msgSend.msgId		= MSG_DEVICE_SERVICE_BTPLAY_IN;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				break;
#endif

/////////////////////////////////////////////////////////////////////////////////
//KEY: TWS FUNCTION
#ifdef BT_TWS_SUPPORT
			case MSG_BT_TWS_MASTER_CONNECTED:
			case MSG_BT_TWS_SLAVE_CONNECTED:
				break;
			#ifdef BT_TWS_FUNCTION_KEY_SWITCH
			case MSG_BT_TWS_ON:
				APP_DBG("MSG_BT_TWS_ON\n");
				if(SoftFlagGet(SoftFlagRecording))
				{
					APP_DBG("Recording,Do Not On TWS!!\n");
					break;
				}								
				tws_function_switch();
				break;
			#endif
			case MSG_BT_TWS_PAIRING:
#ifdef BT_TWS_FUNCTION_KEY_SWITCH			
				if(!btManager.TwsFunctionEnable)
					break;
#endif				
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
				if(mainAppCt.appCurrentMode == AppModeTwsSlavePlay)
				{					
					if(tws_connect_state_get() == TWS_PAIR_PAIRING)
						break;
					if(gBtTwsAppCt->btTwsPairingStart == 1)
					{
						APP_DBG("CFG_TWS_PEER_SLAVE: Exit pairing loop\n");
						extern unsigned char gLmpCmdStart;
						BtTwsExitPeerPairingMode();
						if(gLmpCmdStart == 0)
						{
							gLmpCmdStart = 1;
						}
						if(tws_connect_state_get() == TWS_PAIR_INQUIRY)
						{
							ME_CancelInquiry();
							//BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
						}
					}
					TwsSlaveModeExit();
					break;
				}
				else
				{	
					if(!btManager.btLinkState)
					{
						#include "tws_slave_mode.h"
						#include "bt_tws_api.h"
						if(btManager.twsState == BT_TWS_STATE_NONE)
						{
							BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
							TwsSlaveModeEnter();
							BtTwsEnterPeerPairingMode();
							break;
						}
					}
					
					if(btManager.twsState == BT_TWS_STATE_CONNECTED)
					{
						tws_link_disconnect();
					}
				}
			
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
				#if (TWS_SIMPLE_PAIRING_SUPPORT == ENABLE)
				if(!btManager.twsSbSlaveDisable)
				{
					//1.断开TWS
					if(btManager.twsState == BT_TWS_STATE_CONNECTED)
					{
						tws_link_disconnect();
					}
					//2.关闭BLE SCAN
					tws_slave_stop_simple_pairing();
					tws_slave_simple_pairing_end();
					btManager.twsSbSlaveDisable = 1;
				}
				else
				{
					btManager.twsSbSlaveDisable = 0;
					//1.开启BLE SCAN
					tws_slave_start_simple_pairing();
				}
				#else
				if(btManager.twsState == BT_TWS_STATE_CONNECTED)
					BtTwsDeviceDisconnect();
				else
					BtTwsEnterSimplePairingMode();
				#endif
#else
#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
				if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
				{
					BtTwsDeviceDisconnect();
					break;
				}
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
				if(gBtTwsAppCt->btTwsPairingStart == 1)
				{
					APP_DBG("CFG_TWS_PEER_MASTER: Exit pairing loop\n");
					GetBtManager()->twsStopConnect = 1;
					extern unsigned char gLmpCmdStart;
					BtTwsExitPeerPairingMode();
					if(gLmpCmdStart == 0)
					{
						gLmpCmdStart = 1;
					}
					else
					{
						ME_CancelInquiry();
					}
					break;
				}
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
				// 预留
#endif
				if(mainAppCt.appCurrentMode != AppModeBtAudioPlay)
					break;
				
			#if (TWS_PAIRING_WHEN_PHONE_CONNECTED_SUPPORT == DISABLE)
				if(btManager.btLinkState)
				{
					//BtDisconnectCtrl();
					break;
				}
			#endif
				BtStopReconnectReg();
				BtStopReconnectTwsReg();

				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				
			#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
				if(btManager.btLinkState)
				{
					APP_DBG("doublekey pair start!!!!!!!\n");
					extern void tws_start_pairing_doubleKey(void);
					extern void BtBroadcastNameUpdate(void);
					extern uint32_t doubleKeyCnt;
					tws_start_pairing_doubleKey();

					doubleKeyCnt = 30000;//30s 超时
					BtBroadcastNameUpdate();
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
					break;
				}
				BtTwsEnterPairingMode();
			#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
				BtTwsEnterSimplePairingMode();
			#else //(TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
				BtTwsEnterPeerPairingMode();
			#endif
#else

				if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
				{
#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE))
#ifdef TWS_SOUNDBAR_MASTER_DISCONNECT_ENABLE
					BtDdb_ClearTwsDeviceAddrList();  //soundbar master主动断开TWS连接,清除TWS记录,避免重复连接
#endif
#endif
					BtTwsDeviceDisconnect();
					break;
				}
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
				if(gBtTwsAppCt->btTwsPairingStart == 1)
				{
					APP_DBG("CFG_TWS_PEER_MASTER: Exit pairing loop\n");
					BtTwsExitPeerPairingMode();
					ME_CancelInquiry();
					break;
				}
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
				// 预留
#endif
				
				if(mainAppCt.appCurrentMode != AppModeBtAudioPlay)
					break;

			#if (TWS_PAIRING_WHEN_PHONE_CONNECTED_SUPPORT == DISABLE)
				if(btManager.btLinkState)
				{
					break;
				}
			#endif
				
			#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
				if(btManager.btLinkState)
				{
					APP_DBG("doublekey pair start!!!!!!!\n");
					extern void tws_start_pairing_doubleKey(void);
					extern void BtBroadcastNameUpdate(void);
					extern uint32_t doubleKeyCnt;
					tws_start_pairing_doubleKey();

					doubleKeyCnt = 30000;//30s 超时
					BtBroadcastNameUpdate();
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
					break;
				}
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				BtTwsEnterPairingMode();
			#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				BtTwsEnterSimplePairingMode();
			#else //(TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
				BtTwsEnterPeerPairingMode();
			#endif
#endif
#endif
				break;

			case MSG_BT_TWS_RECONNECT:
				BtTwsDeviceReconnect();
				break;
			
			case MSG_BT_TWS_DISCONNECT:
				BtTwsDeviceDisconnect();
				break;

			case MSG_BT_TWS_CLEAR_PAIRED_LIST:
				BtDdb_ClearTwsDeviceAddrList();
				break;

			case MSG_BT_CLEAR_PAIRED_LIST:
				BtDdb_Erase();
				break;

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
			//Soundbar Slave进入可以校频偏状态
			case MSG_BT_SOUNDBAR_SLAVE_TEST_MODE:
				if(btManager.twsState == BT_TWS_STATE_NONE)
				{
					if(!btManager.twsSoundbarSlaveTestFlag)
					{
						btManager.twsSoundbarSlaveTestFlag = 1;
						APP_DBG("Soundbar Slave Enter Test State\n");
						BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 50);
					}
					else
					{
						btManager.twsSoundbarSlaveTestFlag = 0;
						APP_DBG("Soundbar Slave Exit Test State\n");
						BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
						
					}
				}
				break;
#endif
#endif
/////////////////////////////////////////////////////////////////////////////////
#endif

		}
		//工作态 或 deepsleep前的退出过程
		if(mainAppCt.state == TaskStateRunning ||(mainAppCt.state == TaskStatePausing && (mainAppCt.MState != ModeStatePause || mainAppCt.MState != ModeStateNone)))
		{
			if(msg.msgId == MSG_AUDIO_CORE_EFFECT_CHANGE)
			{
				uint8_t mute_f = 0;

				ResMallocFlag = 1;
				APP_DBG("msg.msgId == MSG_AUDIO_CORE_EFFECT_CHANGE\n");
#ifdef CFG_RES_AUDIO_DAC0_EN
#ifdef BT_TWS_SUPPORT
				mute_f = ((*(volatile unsigned long *) 0x4002E018) & 0x03);
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
				mute_f = 0; //soundbar从机默认打开mute
#endif
				//tws_master_mute_send(TRUE);
#endif
				AudioDAC_DigitalMute(DAC0, TRUE, TRUE);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
				AudioDAC_DigitalMute(DAC1, TRUE, TRUE);
#endif
				vTaskDelay(15);

				gCtrlVars.audio_effect_init_flag = 1;
				AudioEffectsDeInit();//释放音效内存
				AudioEffectsInit();//重新分配音效内存，
				if((mainAppCt.appCurrentMode != AppModeBtHfPlay)
				&&(mainAppCt.appCurrentMode != AppModeUsbPhone))
				{
					AudioEffectParamSync();//音效参数更新后，需要再同步相关操作参数设置
				}

				if(GetAudioCoreServiceState() == TaskStatePaused)
				{
					AudioCoreServiceResume();
				}
				gCtrlVars.audio_effect_init_flag = 0;

#ifdef BT_RECORD_FUNC_ENABLE
				{
					extern void BtRecordAudioResInitOk(void);
					if(GetSystemMode() == AppModeBtRecordPlay)
						BtRecordAudioResInitOk();
				}
#endif
					vTaskDelay(15);
#ifdef CFG_RES_AUDIO_DAC0_EN
					if(!mute_f)
						AudioDAC_DigitalMute(DAC0, FALSE, FALSE);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
					AudioDAC_DigitalMute(DAC1, FALSE, FALSE);

#endif
				ResMallocFlag = 0;
				gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取

#if defined(BT_SNIFF_ENABLE) && defined(CFG_APP_BT_MODE_EN)
				if(sniff_wakeup_get() == 1)
				{
					//##__退出sniff，标志位恢复__##
					sniff_wakeup_set(0);
					sniff_lmpsend_set(0);
					//##____________________##
#ifdef BT_TWS_SUPPORT
					if(GetBtManager()->twsRole == BT_TWS_MASTER)
					{
			//				SniffMasterADDAReady();//主机向从机发送ADDA已经启动,在BtTws_Slave_RecvData中收到
						extern void tws_master_send_ready(void);
						tws_master_send_ready();
					}
					else if(GetBtManager()->twsRole == BT_TWS_SLAVE)
					{
						extern void tws_slave_send_ready(void);
						tws_slave_send_ready();
			//				SysSniffSlaveADDAReady();
					}
#endif
				}
#endif//BT_SNIFF_ENABLE
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
                AudioCoreAppSourceVolRecover();
#endif	

#ifdef CFG_FUNC_USB_MIX_MODE
				APPModeInitOKflag = 1;
#endif			
			}
			//动态内存大小调整
			if(msg.msgId == MSG_AUDIO_CORE_FRAME_SIZE_CHANGE)
			{
				AudioEffectsDeInit();//释放音效内存
				SoftFlagRegister(SoftFlagEffectChange);
				APP_DBG("msg.msgId == MSG_AUDIO_CORE_FRAME_SIZE_CHANGE\n");
				ResMallocFlag = 1;
				mainAppCt.SourcesMuteState = 0;
				for(i = 0; i<AUDIO_CORE_SOURCE_MAX_NUM; i++)
				{
					mainAppCt.SourcesMuteState <<= 1;
						//APP_DBG("mainAppCt.AudioCore->AudioSource[%d].LeftMuteFlag =%d\n", i, mainAppCt.AudioCore->AudioSource[i].LeftMuteFlag);
					if(mainAppCt.AudioCore->AudioSource[i].LeftMuteFlag == TRUE)
					{
						mainAppCt.SourcesMuteState |= 0x01;
					}
					//APP_DBG("mainAppCt.SourcesMuteState = %x\n", mainAppCt.SourcesMuteState);
					AudioCoreSourceMute(i, 1, 1);
				}
				vTaskDelay(10);

				AudioADC_DigitalMute(ADC1_MODULE, 1, 1);
				vTaskDelay(10);
#ifdef CFG_RES_AUDIO_DAC0_EN
				AudioDAC_DigitalMute(DAC0, TRUE, TRUE);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
				AudioDAC_DigitalMute(DAC1, TRUE, TRUE);
#endif
				vTaskDelay(15);

				if(GetAudioCoreServiceState() == TaskStatePaused)
				{
					MessageContext		msgSend;
					msgSend.msgId		= MSG_AUDIO_CORE_SERVICE_PAUSED;
					MessageSend(GetMainMessageHandle(), &msgSend);
				}
				else
				{
					AudioCoreServicePause();
				}
			}
			if(msg.msgId == MSG_AUDIO_CORE_SERVICE_PAUSED && SoftFlagGet(SoftFlagEffectChange))
			{
				if(GetAppMessageHandle() != NULL)
				{
					MessageContext		msgSend;
					msgSend.msgId = MSG_APP_RES_RELEASE;
					MessageSend(GetAppMessageHandle(), &msgSend);
				}
			}
			if(msg.msgId == MSG_APP_RES_RELEASE_SUC && SoftFlagGet(SoftFlagEffectChange))
			{
				APP_DBG("MSG_APP_RES_RELEASE_SUC\n");
				AudioResRelease();
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
				AudioEffectsDeInit();
#endif
				SamplesFrameUpdata();//AudioCore Pause 住之后才能切换系统的帧大小
				if(!AudioResMalloc(mainAppCt.SamplesPreFrame))
				{
					APP_DBG("Res Error!\n");
					return;
				}
				{
					MessageContext		msgSend;
					if(GetAppMessageHandle() != NULL)
					{
						msgSend.msgId = MSG_APP_RES_MALLOC;
						MessageSend(GetAppMessageHandle(), &msgSend);
					}
				}
			}
			if(msg.msgId == MSG_APP_RES_MALLOC_SUC && SoftFlagGet(SoftFlagEffectChange))
			{
				APP_DBG("MSG_APP_RES_MALLOC_SUC\n");
				AudioResInit();
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
				gCtrlVars.audio_effect_init_flag = 1;
				AudioEffectsInit();
				if((mainAppCt.appCurrentMode != AppModeBtHfPlay)&&(mainAppCt.appCurrentMode != AppModeUsbPhone))
				{
					AudioEffectParamSync();//音效参数更新后，需要再同步相关操作参数设置
				}
				gCtrlVars.audio_effect_init_flag = 0;
				gCtrlVars.AutoRefresh = 1;//////调音时模式发生改变，上位机会自动读取音效数据，1=允许上位读，0=不需要上位机读取
#endif				
				{
					MessageContext		msgSend;
					if(GetAppMessageHandle() != NULL)
					{
						msgSend.msgId = MSG_APP_RES_INIT;
						MessageSend(GetAppMessageHandle(), &msgSend);
#ifdef CFG_FUNC_USB_MIX_MODE
						APPModeInitOKflag = 1;
#endif
					}
				}
			#ifdef BT_TWS_SUPPORT
				extern uint32_t g_tws_need_init;
				g_tws_need_init = 1;
			#endif
			}
			if(msg.msgId == MSG_APP_RES_INIT_SUC && SoftFlagGet(SoftFlagEffectChange))
			{
				APP_DBG("(msg.msgId == MSG_APP_RES_INIT_SUC)\n");
				AudioCoreDeinit();
				AudioCoreServiceResume();

				AudioADC_DigitalMute(ADC1_MODULE, 0, 0);
				vTaskDelay(10);

				for(i = 0; i<AUDIO_CORE_SOURCE_MAX_NUM; i++)
				{
					if((mainAppCt.SourcesMuteState & 0x01) == 0x00)//sourcr 端原来处于unmute状态，还原source状态
					{
						#if defined(CFG_APP_LINEIN_MODE_EN) || defined(CFG_APP_RADIOIN_MODE_EN)
						if((mainAppCt.appCurrentMode == AppModeLineAudioPlay) || (mainAppCt.appCurrentMode == AppModeRadioAudioPlay))
						{
#ifdef BT_TWS_SUPPORT
							if((AUDIO_CORE_SOURCE_MAX_NUM - 1 - i != APP_SOURCE_NUM) && (AUDIO_CORE_SOURCE_MAX_NUM - 1 - i != TWS_SOURCE_NUM))
							{
								AudioCoreSourceUnmute(AUDIO_CORE_SOURCE_MAX_NUM - 1 - i, 1, 1);
							}
							else
#endif
							{
								#if defined(CFG_APP_LINEIN_MODE_EN)
								if(mainAppCt.appCurrentMode == AppModeLineAudioPlay)
								{
									if(LineInSourceIsNeedCurrentUnmute())
									{
										LineInSourceUnmuteSet();
									}
									if(AUDIO_CORE_SOURCE_MAX_NUM - 1 - i != APP_SOURCE_NUM) AudioCoreSourceUnmute(AUDIO_CORE_SOURCE_MAX_NUM - 1 - i, 1, 1);
								}
								#endif

								#if defined(CFG_APP_RADIOIN_MODE_EN)
								if(mainAppCt.appCurrentMode == AppModeRadioAudioPlay)
								{
									extern bool RadioPlaySourceIsNeedCurrentUnmute(void);
									extern void RadioPlaySourceUnmuteSet(void);
									if(RadioPlaySourceIsNeedCurrentUnmute())
									{
										RadioPlaySourceUnmuteSet();
									}
									if(AUDIO_CORE_SOURCE_MAX_NUM - 1 - i != APP_SOURCE_NUM) AudioCoreSourceUnmute(AUDIO_CORE_SOURCE_MAX_NUM - 1 - i, 1, 1);
								}
								#endif
							}
						}
						else
						{
							AudioCoreSourceUnmute(AUDIO_CORE_SOURCE_MAX_NUM - 1 - i, 1, 1);
						}
						#else
							AudioCoreSourceUnmute(AUDIO_CORE_SOURCE_MAX_NUM - 1 - i, 1, 1);
						#endif
						//APP_DBG("i = %d, mainAppCt.SourcesMuteState = %x\n", i, mainAppCt.SourcesMuteState);
					}
					mainAppCt.SourcesMuteState >>= 1;
				}
#ifdef BT_TWS_SUPPORT
				//if(tws_link_status_get() == 0)
#endif
				{
					vTaskDelay(10);
#ifdef CFG_RES_AUDIO_DAC0_EN
					AudioDAC_DigitalMute(DAC0, FALSE, FALSE);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
					AudioDAC_DigitalMute(DAC1, FALSE, FALSE);
				
#endif
				}
                ResMallocFlag = 0;
                SoftFlagDeregister(SoftFlagEffectChange);
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
                AudioCoreAppSourceVolRecover();
#endif				
			}
			
			MainAppModeProcess(msg.msgId);

#ifdef	CFG_FUNC_AUDIO_EFFECT_EN
			if(mainAppCt.appCurrentMode != AppModeWaitingPlay)
			{
				void EffectChange(void);
				EffectChange();
			}
#endif
		}
	}
}

/***************************************************************************************
 *
 * APIs
 *
 */

int32_t MainAppTaskStart(void)
{
	MainAppInit();
#ifdef CFG_FUNC_SHELL_EN
	shell_init();
	xTaskCreate(mv_shell_task, "SHELL", SHELL_TASK_STACK_SIZE, NULL, SHELL_TASK_PRIO, NULL);
#endif
	xTaskCreate(MainAppTaskEntrance, "MainApp", MAIN_APP_TASK_STACK_SIZE, NULL, MAIN_APP_TASK_PRIO, &mainAppCt.taskHandle);
	return 0;
}

MessageHandle GetMainMessageHandle(void)
{
	return mainAppCt.msgHandle;
}


uint32_t GetSystemMode(void)
{
	return mainAppCt.appCurrentMode;
}

uint32_t IsBtAudioMode(void)
{
	return (mainAppCt.appCurrentMode == AppModeBtAudioPlay);
}

uint32_t IsBtTwsSlaveMode(void)
{
	return (mainAppCt.appCurrentMode == AppModeTwsSlavePlay);
}

void SoftFlagRegister(uint32_t SoftEvent)
{
	SoftwareFlag |= SoftEvent;
	return ;
}

void SoftFlagDeregister(uint32_t SoftEvent)
{
	SoftwareFlag &= ~SoftEvent;
	return ;
}

bool SoftFlagGet(uint32_t SoftEvent)
{
	return SoftwareFlag & SoftEvent ? TRUE : FALSE;
}

void SysVarInit(void)
{
	int16_t i;
#ifdef CFG_FUNC_BREAKPOINT_EN
	BP_SYS_INFO *pBpSysInfo;

	pBpSysInfo = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);

	#ifdef CFG_FUNC_MAIN_DEEPSLEEP_EN
	if(SoftFlagGet(SoftFlagWakeUpSouceIsCEC))
	{
		pBpSysInfo->CurModuleId = AppModeHdmiAudioPlay;
	}
	SoftFlagDeregister(SoftFlagWakeUpSouceIsCEC);
	#endif

	if(!ModeResumeMask(pBpSysInfo->CurModuleId))
	{
		mainAppCt.appBackupMode = pBpSysInfo->CurModuleId;//上电必先Waiting，断电记忆模式使用backup恢复。
		if((mainAppCt.appBackupMode <= AppModeWaitingPlay) || (mainAppCt.appBackupMode >= AppModeCardPlayBack))
		{
			mainAppCt.appBackupMode = AppModeBtAudioPlay;
		}
	}
	//mainAppCt.appBackupMode = AppModeBtAudioPlay;
	APP_DBG("MainApp:%d,%d\n", mainAppCt.appCurrentMode, pBpSysInfo->CurModuleId);

	#ifdef BT_TWS_SUPPORT
	extern uint32_t tws_delay;
	if(pBpSysInfo->CurModuleId != AppModeBtAudioPlay)
	{
		tws_delay = 20;
	}
	else
	{
		tws_delay = TWS_STATRT_PLAY_FRAM;
	}
	#endif

	mainAppCt.MusicVolume = pBpSysInfo->MusicVolume;
	if(mainAppCt.MusicVolume > CFG_PARA_MAX_VOLUME_NUM) 
	{
		mainAppCt.MusicVolume = CFG_PARA_MAX_VOLUME_NUM;
	}
	APP_DBG("MusicVolume:%d,%d\n", mainAppCt.MusicVolume, pBpSysInfo->MusicVolume);	
	
	#ifdef CFG_FUNC_MIC_KARAOKE_EN
	mainAppCt.EffectMode = pBpSysInfo->EffectMode;
	if((mainAppCt.EffectMode > EFFECT_MODE_WaWaYin) || (mainAppCt.EffectMode <= 10))
	{
		mainAppCt.EffectMode = EFFECT_MODE_HunXiang;
	}
	#else
	mainAppCt.EffectMode = EFFECT_MODE_NORMAL;
	#endif
	
	#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
	extern uint8_t flash_effect_total;
	flash_effect_total = sizeof(FLASH_EFFECT_TAB)/sizeof(FLASH_EFFECT_TAB[0])-2;//no aec
	mainAppCt.EffectMode = pBpSysInfo->EffectMode;
	if(mainAppCt.EffectMode > flash_effect_total)
	{
		mainAppCt.EffectMode = 0;
	}
	#endif	
	APP_DBG("EffectMode:%d,%d\n", mainAppCt.EffectMode, pBpSysInfo->EffectMode);
	
	mainAppCt.MicVolume = pBpSysInfo->MicVolume;
	if(mainAppCt.MicVolume > CFG_PARA_MAX_VOLUME_NUM) 
	{
		mainAppCt.MicVolume = CFG_PARA_MAX_VOLUME_NUM;
	}
	mainAppCt.MicVolumeBak = mainAppCt.MicVolume;
	APP_DBG("MicVolume:%d,%d\n", mainAppCt.MicVolume, pBpSysInfo->MicVolume);

	#ifdef CFG_APP_BT_MODE_EN
	mainAppCt.HfVolume = pBpSysInfo->HfVolume;
	if(mainAppCt.HfVolume > CFG_PARA_MAX_VOLUME_NUM) 
	{
		mainAppCt.HfVolume = CFG_PARA_MAX_VOLUME_NUM;
	}
	APP_DBG("HfVolume:%d,%d\n", mainAppCt.HfVolume, pBpSysInfo->HfVolume);
	#endif
	
	#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	mainAppCt.EqMode = pBpSysInfo->EqMode;
	if(mainAppCt.EqMode > EQ_MODE_VOCAL_BOOST)
	{
		mainAppCt.EqMode = EQ_MODE_FLAT;
	}
	#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN    
	mainAppCt.EqModeBak = mainAppCt.EqMode;
	mainAppCt.EqModeFadeIn = 0;
	mainAppCt.eqSwitchFlag = 0;
	#endif
	APP_DBG("EqMode:%d,%d\n", mainAppCt.EqMode, pBpSysInfo->EqMode);
	#endif

	mainAppCt.ReverbStep = pBpSysInfo->ReverbStep;
    if(mainAppCt.ReverbStep > MAX_MIC_REVB_STEP)
	{
		mainAppCt.ReverbStep = MAX_MIC_REVB_STEP;
	}
	mainAppCt.ReverbStepBak = mainAppCt.ReverbStep;
	APP_DBG("ReverbStep:%d,%d\n", mainAppCt.ReverbStep, pBpSysInfo->ReverbStep);
	
	#ifdef CFG_FUNC_MIC_TREB_BASS_EN	
    mainAppCt.MicBassStep = pBpSysInfo->MicBassStep;
    if(mainAppCt.MicBassStep > MAX_BASS_TREB_GAIN)
	{
		mainAppCt.MicBassStep = 7;
	}
    mainAppCt.MicTrebStep = pBpSysInfo->MicTrebStep;
    if(mainAppCt.MicTrebStep > MAX_BASS_TREB_GAIN)
	{
		mainAppCt.MicTrebStep = 7;
	}
	mainAppCt.MicTrebStepBak = mainAppCt.MicTrebStep;
	mainAppCt.MicBassStepBak = mainAppCt.MicBassStep;
	APP_DBG("MicTrebStep:%d,%d\n", mainAppCt.MicTrebStep, pBpSysInfo->MicTrebStep);
	APP_DBG("MicBassStep:%d,%d\n", mainAppCt.MicBassStep, pBpSysInfo->MicBassStep);
	#endif

	#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN	
    mainAppCt.MusicBassStep = pBpSysInfo->MusicBassStep;
    if(mainAppCt.MusicBassStep > MAX_BASS_TREB_GAIN)
	{
		mainAppCt.MusicBassStep = 7;
	}
    mainAppCt.MusicTrebStep = pBpSysInfo->MusicTrebStep;
    if(mainAppCt.MusicTrebStep > MAX_BASS_TREB_GAIN)
	{
		mainAppCt.MusicTrebStep = 7;
	}
	APP_DBG("MusicTrebStep:%d,%d\n", mainAppCt.MusicTrebStep, pBpSysInfo->MusicTrebStep);
	APP_DBG("MusicBassStep:%d,%d\n", mainAppCt.MusicBassStep, pBpSysInfo->MusicBassStep);
	#endif

#else
	mainAppCt.appBackupMode = AppModeBtAudioPlay;		  
	mainAppCt.MusicVolume = CFG_PARA_MAX_VOLUME_NUM;
	#ifdef CFG_APP_BT_MODE_EN
	mainAppCt.HfVolume = CFG_PARA_MAX_VOLUME_NUM;
	#endif
	#ifdef CFG_FUNC_MIC_KARAOKE_EN
	mainAppCt.EffectMode = EFFECT_MODE_HunXiang;
	#else
	mainAppCt.EffectMode = EFFECT_MODE_NORMAL;
	#endif
	mainAppCt.MicVolume = CFG_PARA_MAX_VOLUME_NUM;
	
	#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	mainAppCt.EqMode = EQ_MODE_FLAT;
 	#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN    
    mainAppCt.EqModeBak = mainAppCt.EqMode;
	mainAppCt.eqSwitchFlag = 0;
	#endif
	#endif
	mainAppCt.ReverbStep = MAX_MIC_DIG_STEP;

	#ifdef CFG_FUNC_MIC_TREB_BASS_EN	
	mainAppCt.MicBassStep = 7;
	mainAppCt.MicTrebStep = 7;
	#endif	

	#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN	
	mainAppCt.MusicBassStep = 7;
	mainAppCt.MusicTrebStep = 7;
	#endif	

#endif

	#ifdef CFG_APP_HDMIIN_MODE_EN
	mainAppCt.hdmiArcOnFlg = 1;
	mainAppCt.hdmiResetFlg = 0;
	#endif
    for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		mainAppCt.gSysVol.AudioSinkVol[i] = CFG_PARA_MAX_VOLUME_NUM;
	}

	for(i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(i == MIC_SOURCE_NUM)
		{
			mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM] = mainAppCt.MicVolume;
		}
		else if(i == APP_SOURCE_NUM)
		{
			mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.MusicVolume;
		}
		else if(i == REMIND_SOURCE_NUM)
		{
			#if CFG_PARAM_FIXED_REMIND_VOL
			mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = CFG_PARAM_FIXED_REMIND_VOL;
			#else
			mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = mainAppCt.MusicVolume;
			#endif
		}
		#ifdef CFG_FUNC_REMIND_MIX_MODE
		else if(i == MIX_REMIND_SOURCE_NUM)
		{
			#if CFG_PARAM_FIXED_REMIND_VOL
			mainAppCt.gSysVol.AudioSourceVol[MIX_REMIND_SOURCE_NUM] = CFG_PARAM_FIXED_REMIND_VOL;
			#else
			mainAppCt.gSysVol.AudioSourceVol[MIX_REMIND_SOURCE_NUM] = mainAppCt.MusicVolume;
			#endif
		}
		#endif
		#ifdef BT_TWS_SUPPORT
		else if(i == TWS_SOURCE_NUM)
		{
			mainAppCt.gSysVol.AudioSourceVol[TWS_SOURCE_NUM] = mainAppCt.MusicVolume;
		}
		#endif
		else
		{
			mainAppCt.gSysVol.AudioSourceVol[i] = CFG_PARA_MAX_VOLUME_NUM;		
		}
	}
	mainAppCt.gSysVol.MuteFlag = FALSE;	
	mainAppCt.muteFlagPre = FALSE;
	mainAppCt.ReceiveDeepSleepFlag = FALSE;
}

#ifdef	CFG_FUNC_POWERKEY_EN
void SystemGotoPowerDown(void)
{
	MessageContext		msgSend;

	msgSend.msgId = MSG_TASK_POWERDOWN;
	MessageSend(GetAppMessageHandle(), &msgSend);
}
#endif

#if defined(CFG_FUNC_DEEPSLEEP_EN) || defined(CFG_FUNC_MAIN_DEEPSLEEP_EN)
void SleepMainAppTask(void)
{
	AudioCoreSourceDisable(MIC_SOURCE_NUM);
//	OTG_PortSetDetectMode(0,0);//禁用usb/device 扫描驱动

#ifdef DISP_DEV_7_LED
	NVIC_DisableIRQ(Timer6_IRQn);
#endif
	NVIC_DisableIRQ(Timer2_IRQn);
	WDG_Disable();
//	OTG_PowerDown(); //usb device 声卡 等 禁止。
	OTG_DeepSleepBackup();
#ifdef CFG_FUNC_UDISK_DETECT
	UDiskRemovedFlagSet(TRUE);
#endif
	ADC_Disable();//按键扫描
	AudioADC_PowerDown();
	AudioADC_VcomConfig(2);//注意，VCOM会和DAC配置重叠。参数2，PowerDown VCOM

	SPDIF_AnalogModuleDisable();//spdif,HDMI
#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioDAC_PowerDown(DAC0);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	AudioDAC_PowerDown(DAC1);
#endif
}


inline void WakeupMainAppTask(void)
{
#if defined (BT_TWS_SUPPORT) && !defined (CFG_FUNC_MIC_KARAOKE_EN)
	// TWS对相位要求较高的方案需要关掉karaoke; 并加大dac的缓存保证相位
	uint16_t dac_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2 *2 * 4;
	uint16_t dacx_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2 * 4;
#else
	uint16_t dac_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2 *2;
	uint16_t dacx_fifo_len = mainAppCt.SamplesPreFrame * 2 * 2;
#endif

#ifdef DISP_DEV_7_LED
	NVIC_EnableIRQ(Timer6_IRQn);
#endif

	//DAC init
#if defined (CFG_RES_AUDIO_DAC0_EN) && defined (CFG_RES_AUDIO_DACX_EN)
	AudioDAC_Init(ALL, mainAppCt.SampleRate, (void*)mainAppCt.DACFIFO, dac_fifo_len, (void*)mainAppCt.DACXFIFO, dacx_fifo_len);
#elif defined (CFG_RES_AUDIO_DAC0_EN)
	AudioDAC_Init(DAC0, mainAppCt.SampleRate, (void*)mainAppCt.DACFIFO, dac_fifo_len , NULL, 0);
#elif defined (CFG_RES_AUDIO_DACX_EN)
	AudioDAC_Init(DAC1, mainAppCt.SampleRate, NULL, 0, (void*)mainAppCt.DACXFIFO, dacx_fifo_len);
#else
	APP_DBG("DAC0 DACX All Disable!!!!!!!!!!\n");
#endif

	//Mic1  analog 对应AudioADC_PowerDown(); 注意dac要先配。
	AudioADC_AnaInit();
	AudioADC_VcomConfig(1);//MicBias en
	AudioADC_MicBias1Enable(1);
#if (defined(BT_TWS_SUPPORT) || CFG_RES_MIC_SELECT)
	AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1);
	AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2);

	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, 15, 4);//0db bypass
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, 15, 4);

	//Mic1   digital
	AudioADC_DigitalInit(ADC1_MODULE, mainAppCt.SampleRate, (void*)mainAppCt.ADCFIFO, mainAppCt.SamplesPreFrame * 2 * 2 * 2);
	AudioCoreSourceEnable(MIC_SOURCE_NUM);
#endif
//	SPDIF_AnalogModuleDisable();//spdif,HDMI

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	AudioI2sOutParamsSet();
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
    AudioI2S_ConfigInit();
#endif

#ifdef CFG_FUNC_LINE_MIX_MODE
    AudioLine_ConfigInit();
#endif
#ifdef CFG_FUNC_SPDIF_MIX_MODE
	 AudioSpdif_ConfigInit();
#endif
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_FUNC_POWER_MONITOR_EN)
	SarADC_Init();
#endif

#ifdef CFG_RES_ADC_KEY_USE	
	AdcKeyInit();
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IR) && defined(CFG_RES_IR_KEY_USE)
	IRKeyInit();
#endif
	OTG_WakeupResume();
 	Timer_Config(TIMER2,1000,0);
 	Timer_Start(TIMER2);
 	Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
	NVIC_EnableIRQ(Timer2_IRQn);
	
#ifdef CFG_FUNC_USB_MIX_MODE
	void UsbDeviceInitSet(uint8_t sta);
	UsbDeviceInitSet(0);
	OTG_DeviceInit();
	NVIC_EnableIRQ(Usb_IRQn);
#endif

#if defined(CFG_FUNC_USB_DEVICE_EN) || defined(CFG_FUNC_UDISK_DETECT)
	vTaskDelay(100);
	OTG_PortLinkCheck();
#if (defined(CFG_APP_USB_AUDIO_MODE_EN))&&(defined(CFG_COMMUNICATION_BY_USB))
	SetDeviceDetectVarReset_Deepsleep();
#endif
#endif
}

#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
void AudioI2sOutParamsSet(void)
{
	I2SParamCt i2s_set;
	DBG("%s\n",__func__);
	i2s_set.IsMasterMode=CFG_RES_I2S_MODE;// 0:master 1:slave
	i2s_set.SampleRate=mainAppCt.SampleRate;
	i2s_set.I2sFormat=I2S_FORMAT_I2S;
	i2s_set.I2sBits=I2S_LENGTH_16BITS;
	i2s_set.I2sTxRxEnable=1;
#if (CFG_RES_I2S_PORT == 0)
	i2s_set.TxPeripheralID=PERIPHERAL_ID_I2S0_TX;
#else
	i2s_set.TxPeripheralID=PERIPHERAL_ID_I2S1_TX;
#endif
	i2s_set.TxBuf=(void*)mainAppCt.I2SFIFO;
#ifdef BT_TWS_SUPPORT
#if((TWS_AUDIO_OUT_PATH == TWS_IIS0_OUT) || (TWS_AUDIO_OUT_PATH == TWS_IIS1_OUT) )
	i2s_set.TxLen=mainAppCt.SamplesPreFrame * 2 * 2 * 2 * 4;
#else
	i2s_set.TxLen=mainAppCt.SamplesPreFrame * 2 * 2 * 2;
#endif
#else
	i2s_set.TxLen=mainAppCt.SamplesPreFrame * 2 * 2 * 2;
#endif
#if (CFG_RES_I2S_IO_PORT==0)
//i2s0  group_gpio0
	GPIO_PortAModeSet(GPIOA0, 9);// mclk out
	GPIO_PortAModeSet(GPIOA1, 6);// lrclk
	GPIO_PortAModeSet(GPIOA2, 5);// bclk
	GPIO_PortAModeSet(GPIOA3, 7);// dout
	GPIO_PortAModeSet(GPIOA4, 1);// din
	GPIO_RegOneBitSet(GPIO_A_PD, GPIOA3);//set do pull down  
//i2s0  group_gpio0
#else //lif (CFG_RES_I2S_IO_PORT==1)
//i2s1  group_gpio1 
	GPIO_PortAOutDsSet(GPIOA7,  GPIO_PortA_OUTDS_24MA);
	GPIO_PortAModeSet(GPIOA7, 5);//mclk out
	GPIO_PortAModeSet(GPIOA8, 1);//lrclk
	GPIO_PortAModeSet(GPIOA9, 2);//bclk
	GPIO_PortAModeSet(GPIOA10, 4);//do
	GPIO_PortAModeSet(GPIOA11, 2);//di
	GPIO_RegOneBitSet(GPIO_A_PD, GPIOA10);//set do pull down
//i2s1  group_gpio1
#endif
#if (CFG_RES_I2S_PORT == 0)
	AudioI2S_Init(I2S0_MODULE, &i2s_set);
#else
	AudioI2S_Init(I2S1_MODULE, &i2s_set);
#endif
}
#endif

//mic channel: 
void AudioADC1ParamsSet(uint32_t sampleRate)
{
#if CFG_RES_MIC_SELECT
	AudioCoreSourceDisable(MIC_SOURCE_NUM);
	AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	AudioADC_MicBias1Enable(1);
	#if 0//PGA相关参数统一由调音参数更新
	AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1);
	AudioADC_PGASel(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2);
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT, LINEIN3_LEFT_OR_MIC1, gain, gainBoostSel);
	AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_RIGHT, LINEIN3_RIGHT_OR_MIC2, gain, gainBoostSel);
    #endif
	
	//Mic1	 digital
	memset(mainAppCt.ADCFIFO, 0, mainAppCt.SamplesPreFrame * 2 * 2 * 2);
	AudioADC_DigitalInit(ADC1_MODULE, sampleRate, (void*)mainAppCt.ADCFIFO, mainAppCt.SamplesPreFrame * 2 * 2 * 2);
#endif
}

//用于蓝牙HF通话时,MIC数据上传,退出该状态,恢复到之前状态
void ResumeAudioCoreMicSource(void)
{
#if CFG_RES_MIC_SELECT
	AudioADC1ParamsSet(mainAppCt.SampleRate);
	
	//Soure0.
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].Enable = 1;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].FuncDataGet = AudioADC1DataGet;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].FuncDataGetLen = AudioADC1DataLenGet;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].IsSreamData = TRUE;
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].PcmFormat = 2;//test mic audio effect
	mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source0Buf_ADC1;
	AudioCoreSourceEnable(MIC_SOURCE_NUM);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	AudioDAC_DigitalMute(DAC1, TRUE, TRUE);
	vTaskDelay(20);	
	AudioDAC_Pause(DAC1);
	AudioDAC_Disable(DAC1);
	AudioDAC_Reset(DAC1);
	AudioDAC_SampleRateChange(DAC1, CFG_PARA_SAMPLE_RATE);//恢复
	AudioDAC_Enable(DAC1);
	AudioDAC_Run(DAC1);
	AudioDAC_DigitalMute(DAC1, FALSE, FALSE);
#endif

#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioDAC_DigitalMute(DAC0, TRUE, TRUE);
//此处主从同步问题忽略，效果待定。
	vTaskDelay(20);
	AudioDAC_Pause(DAC0);
	AudioDAC_Disable(DAC0);
	AudioDAC_Reset(DAC0);
	AudioDAC_SampleRateChange(DAC0, CFG_PARA_SAMPLE_RATE);//恢复
	AudioDAC_Enable(DAC0);
	AudioDAC_Run(DAC0);
	AudioDAC_DigitalMute(DAC0, FALSE, FALSE);
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	AudioI2sOutParamsSet();
#endif
}


void AudioDACSinkBufClear(void)
{
	uint16_t AudioCoreSinkBufLen = mainAppCt.SamplesPreFrame * 2 * 2;
	memset(mainAppCt.Sink0Buf_DAC0, 0, AudioCoreSinkBufLen);

#ifdef CFG_RES_AUDIO_DACX_EN
	memset(mainAppCt.Sink2Buf_DACX, 0, AudioCoreSinkBufLen);
#endif
}

void EffectPcmBufMalloc(uint32_t SampleLen);
void EffectPcmBufRelease(void);
//帧大小切换，需要释放所有音频通路相关的内存资源
//AudioCore需要暂停
//重新申请相关音频通路的资源
void AudioResRelease(void)
{
	//uint32_t i;
    APP_DBG("AudioResRelease\n");
#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioDAC_Disable(DAC0);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	AudioDAC_Disable(DAC1);
#endif

#if defined(CFG_RES_AUDIO_I2SOUT_EN)
	//I2S hold
	//I2S DMA Disable
#if CFG_RES_I2S_IO_PORT == 0
	I2S_ModuleDisable(I2S0_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S0_TX);
#else
	I2S_ModuleDisable(I2S1_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S1_TX);
#endif
#endif

#if (defined(BT_TWS_SUPPORT) || CFG_RES_MIC_SELECT)
	AudioADC_Disable(ADC1_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_ADC1_RX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_ADC1_RX);
	
#endif

#ifdef CFG_RES_AUDIO_DAC0_EN
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC0_TX, DMA_ERROR_INT);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_AUDIO_DAC1_TX, DMA_ERROR_INT);
#endif
#ifdef CFG_RES_AUDIO_DAC0_EN
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC0_TX);
#endif
#ifdef CFG_RES_AUDIO_DACX_EN
	DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC1_TX);
#endif
#ifdef CFG_FUNC_LINE_MIX_MODE
	AudioADC_Disable(ADC0_MODULE);
#endif
#ifdef CFG_RES_AUDIO_DAC0_EN
	AudioDAC_Reset(DAC0);
#endif
#ifdef CFG_RES_AUDIO_DACX_EN
	AudioDAC_Reset(DAC1);
#endif

#ifdef CFG_FUNC_RECORDER_EN
	ReleaseRecorderCtSink1Buf();
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	if(mainAppCt.Sink2Buf_DACX != NULL)
	{
		APP_DBG("Sink2Buf_DACX\n");
		osPortFree(mainAppCt.Sink2Buf_DACX);
		mainAppCt.Sink2Buf_DACX = NULL;
	}
#endif
	if(mainAppCt.Sink0Buf_DAC0 != NULL)
	{
		APP_DBG("Sink0Buf_DAC0\n");
		osPortFree(mainAppCt.Sink0Buf_DAC0);
		mainAppCt.Sink0Buf_DAC0 = NULL;
	}
#ifdef CFG_RES_AUDIO_I2SOUT_EN
	//I2S Sink2Buf_I2S
	if(mainAppCt.Sink2Buf_I2S != NULL)
	{
		APP_DBG("Sink2Buf_I2S\n");
		osPortFree(mainAppCt.Sink2Buf_I2S);
		mainAppCt.Sink2Buf_I2S = NULL;
	}
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	if(mainAppCt.Sink2Buf_Spdif != NULL)
	{
		DBG("Sink2Buf_SPDIF\n");
		osPortFree(mainAppCt.Sink2Buf_Spdif);
		mainAppCt.Sink2Buf_Spdif = NULL;
	}
#endif

#if (defined(BT_TWS_SUPPORT) || CFG_RES_MIC_SELECT)
	if(mainAppCt.Source0Buf_ADC1 != NULL)
	{
		APP_DBG("Source0Buf_ADC1\n");
		osPortFree(mainAppCt.Source0Buf_ADC1);
		mainAppCt.Source0Buf_ADC1 = NULL;
	}
	if(mainAppCt.ADCFIFO != NULL)
	{
		APP_DBG("ADC1FIFO\n");
		osPortFree(mainAppCt.ADCFIFO);
		mainAppCt.ADCFIFO = NULL;
	}
#endif
#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(mainAppCt.I2SFIFO != NULL)
	{
		APP_DBG("I2SFIFO\n");
		osPortFree(mainAppCt.I2SFIFO);
		mainAppCt.I2SFIFO = NULL;
	}
#endif
#ifdef CFG_RES_AUDIO_DACX_EN
	if(mainAppCt.DACXFIFO != NULL)
	{
		APP_DBG("DACXFIFO\n");
		osPortFree(mainAppCt.DACXFIFO);
		mainAppCt.DACXFIFO = NULL;
	}
#endif
#ifdef CFG_RES_AUDIO_DAC0_EN
	if(mainAppCt.DACFIFO != NULL)
	{
		APP_DBG("DACFIFO\n");
		osPortFree(mainAppCt.DACFIFO);
		mainAppCt.DACFIFO = NULL;
	}
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	EffectPcmBufRelease();
#endif

#ifdef CFG_FUNC_USB_MIX_MODE
	UsbDevicePlayDeInit();
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
	AudioI2S_Release();
#endif

#ifdef CFG_FUNC_LINE_MIX_MODE
	AudioLine_Release();
#endif

#ifdef CFG_FUNC_SPDIF_MIX_MODE
    AudioSpdif_Release();
#endif

#ifdef CFG_FUNC_REMIND_MIX_MODE
		AudioMixRemind_Release();
#endif

	APP_DBG("AudioResRelease ok\n");
	APP_DBG(" AudioResRelease remain mem size: %dK\n",(320 * 1024 - osPortRemainMem())/1024);
}

bool AudioResMalloc(uint16_t SampleLen)
{
#if defined (BT_TWS_SUPPORT) && !defined (CFG_FUNC_MIC_KARAOKE_EN)
	// TWS对相位要求较高的方案需要关掉karaoke; 并加大4倍dac的缓存保证相位
	uint16_t Dac0FifoLen = SampleLen * 2 * 2 * 2 * 4;   //立体声8倍大小于帧长，单位byte
	uint16_t DacxFifoLen = SampleLen * 2 * 2 *4;        //单声到4倍大小于帧长，单位byte
	uint16_t AudioCoreBufStereo = SampleLen * 2 * 2 *4; //AudioCore接口均按立体声来处理
#else
	uint16_t Dac0FifoLen = SampleLen * 2 * 2 * 2;        //立体声8倍大小于帧长，单位byte
	uint16_t DacxFifoLen = SampleLen * 2 * 2;            //单声到4倍大小于帧长，单位byte
	uint16_t AudioCoreBufStereo = SampleLen * 2 * 2;     //AudioCore接口均按立体声来处理
#endif

	uint16_t FifoLenStereo = SampleLen * 2 * 2 * 2;      //立体声8倍大小于帧长，单位byte
	//uint16_t FifoLenMono = SampleLen * 2 * 2;            //单声到4倍大小于帧长，单位byte
	uint16_t AudioCoreBufLen = SampleLen * 2 * 2;        //AudioCore接口均按立体声来处理

	APP_DBG("mainAppCt.SamplesPreFrame = %d\n", SampleLen);
	APP_DBG("Audio Res Malloc from End\n");

	//////////申请DMA fifo
#ifdef CFG_RES_AUDIO_DAC0_EN
	mainAppCt.DACFIFO = (uint32_t*)osPortMallocFromEnd(Dac0FifoLen);//DAC fifo
	if(mainAppCt.DACFIFO != NULL)
	{
		memset(mainAppCt.DACFIFO, 0, Dac0FifoLen);
	}
	else
	{
		APP_DBG("malloc DACFIFO error\n");
		return FALSE;
	}
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	mainAppCt.DACXFIFO = (uint32_t*)osPortMallocFromEnd(DacxFifoLen);//DACX fifo
	if(mainAppCt.DACXFIFO != NULL)
	{
		memset(mainAppCt.DACXFIFO, 0, DacxFifoLen);
	}
	else
	{
		APP_DBG("malloc DACXFIFO error\n");
		return FALSE;
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	#ifdef BT_TWS_SUPPORT
	mainAppCt.I2SFIFO = (uint32_t*)osPortMallocFromEnd(FifoLenStereo * 4);//I2S fifo
	if(mainAppCt.I2SFIFO != NULL)
	{
		memset(mainAppCt.I2SFIFO, 0, FifoLenStereo * 4);
	}
	#else
	mainAppCt.I2SFIFO = (uint32_t*)osPortMallocFromEnd(FifoLenStereo);//I2S fifo
	if(mainAppCt.I2SFIFO != NULL)
	{
		memset(mainAppCt.I2SFIFO, 0, FifoLenStereo);
	}
	#endif
	else
	{
		APP_DBG("malloc I2SFIFO error\n");
		return FALSE;
	}
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	mainAppCt.Sink2Buf_Spdif = (uint16_t*)osPortMallocFromEnd(AudioCoreBufLen);//stereo
	if(mainAppCt.Sink2Buf_Spdif != NULL)
	{
		memset(mainAppCt.Sink2Buf_Spdif, 0, AudioCoreBufLen);
	}
	else
	{
		DBG("malloc Sink2Buf_SPDIF error\n");
		return FALSE;
	}
#endif

#if (defined(BT_TWS_SUPPORT) || CFG_RES_MIC_SELECT)
	mainAppCt.ADCFIFO = (uint32_t*)osPortMallocFromEnd(FifoLenStereo);//ADC fifo
	if(mainAppCt.ADCFIFO != NULL)
	{
		memset(mainAppCt.ADCFIFO, 0, FifoLenStereo);
	}
	else
	{
		APP_DBG("malloc ADCFIFO error\n");
		return FALSE;
	}

	///////////////////////////////AudioCore/////////////////////////////////////////
	//mic
	mainAppCt.Source0Buf_ADC1 = (uint16_t*)osPortMallocFromEnd(AudioCoreBufLen);//stereo
	if(mainAppCt.Source0Buf_ADC1 != NULL)
	{
		memset(mainAppCt.Source0Buf_ADC1, 0, AudioCoreBufLen);
	}
	else
	{
		APP_DBG("malloc Source0Buf_ADC1 error\n");
		return FALSE;
	}
#endif

#ifdef CFG_RES_AUDIO_DAC0_EN
	mainAppCt.Sink0Buf_DAC0 = (uint16_t*)osPortMallocFromEnd(AudioCoreBufStereo);//stereo
	if(mainAppCt.Sink0Buf_DAC0 != NULL)
	{
		memset(mainAppCt.Sink0Buf_DAC0, 0, AudioCoreBufStereo);
	}
	else
	{
		APP_DBG("malloc Sink0Buf_DAC0 error\n");
		return FALSE;
	}
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	mainAppCt.Sink2Buf_DACX = (uint16_t*)osPortMallocFromEnd(AudioCoreBufStereo);//mono
	if(mainAppCt.Sink2Buf_DACX != NULL)
	{
		memset(mainAppCt.Sink2Buf_DACX, 0, AudioCoreBufStereo);
	}
	else
	{
		APP_DBG("malloc Sink2Buf_DACX error\n");
		return FALSE;
	}
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	mainAppCt.Sink2Buf_I2S = (uint16_t*)osPortMallocFromEnd(AudioCoreBufLen);//stereo
	if(mainAppCt.Sink2Buf_I2S != NULL)
	{
		memset(mainAppCt.Sink2Buf_I2S, 0, AudioCoreBufLen);
	}
	else
	{
		APP_DBG("malloc Sink2Buf_I2S error\n");
		return FALSE;
	}
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_EN
	EffectPcmBufMalloc(SampleLen);
	EffectPcmBufClear(SampleLen);
#endif

#ifdef CFG_FUNC_USB_MIX_MODE
#ifdef CFG_FUNC_PITCH_DET_EN
	if(!mainAppCt.EnterPitchDetFlag)
#endif
	UsbDevicePlayInit(SampleLen);//mainAppCt.SamplesPreFrame);
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
#ifdef CFG_FUNC_PITCH_DET_EN
	if(!mainAppCt.EnterPitchDetFlag)
#endif
	AudioI2S_ResMalloc(SampleLen);
#endif

#ifdef CFG_FUNC_LINE_MIX_MODE
#ifdef CFG_FUNC_PITCH_DET_EN
	if(!mainAppCt.EnterPitchDetFlag)
#endif
	AudioLine_ResMalloc(SampleLen);
#endif

#ifdef CFG_FUNC_SPDIF_MIX_MODE
	AudioSpdif_ResMalloc(SampleLen);
#endif

#ifdef CFG_FUNC_RECORDER_EN
	if(!MallocRecorderCtSink1Buf(SampleLen))
		return FALSE;
#endif

#ifdef CFG_FUNC_REMIND_MIX_MODE
		AudioMixRemind_ResMalloc(SampleLen);
#endif
	APP_DBG("\n");
	return TRUE;
}

//音频通路资源初始化
void AudioResInit(void)
{
	APP_DBG("AudioResInit\n");
#if (defined(BT_TWS_SUPPORT) || CFG_RES_MIC_SELECT)
	if(mainAppCt.Source0Buf_ADC1 != NULL)
	{
		mainAppCt.AudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source0Buf_ADC1;
	}
#endif
	if(mainAppCt.Sink0Buf_DAC0 != NULL)
	{
		mainAppCt.AudioCore->AudioSink[AUDIO_DAC0_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink0Buf_DAC0;
	}

#ifdef CFG_FUNC_RECORDER_EN
	AudioRecResInit();
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	if(mainAppCt.Sink2Buf_DACX != NULL)
	{
		mainAppCt.AudioCore->AudioSink[AUDIO_DACX_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_DACX;
	}
#endif
#ifdef CFG_RES_AUDIO_I2SOUT_EN
	if(mainAppCt.Sink2Buf_I2S != NULL)
	{
		mainAppCt.AudioCore->AudioSink[AUDIO_I2SOUT_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_I2S;
	}
#endif

#ifdef CFG_RES_AUDIO_SPDIFOUT_EN
	if(mainAppCt.Sink2Buf_Spdif != NULL)
	{
		mainAppCt.AudioCore->AudioSink[AUDIO_SPDIF_SINK_NUM].PcmOutBuf = (int16_t*)mainAppCt.Sink2Buf_Spdif;
	}
#endif

#ifdef CFG_RES_AUDIO_DAC0_EN
	//DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC0_TX);

#if defined (BT_TWS_SUPPORT) && !defined (CFG_FUNC_MIC_KARAOKE_EN)
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC0_TX, mainAppCt.SamplesPreFrame * 2 * 2*4, (void*)mainAppCt.DACFIFO, mainAppCt.SamplesPreFrame * 2 * 2 * 2*4);
#else
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC0_TX, mainAppCt.SamplesPreFrame * 2 * 2, (void*)mainAppCt.DACFIFO, mainAppCt.SamplesPreFrame * 2 * 2 * 2);
#endif

	DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_DAC0_TX);
	AudioDAC_Enable(DAC0);
#endif

#ifdef CFG_RES_AUDIO_DACX_EN
	//DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_DAC1_TX);
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_DAC1_TX, mainAppCt.SamplesPreFrame * 2, (void*)mainAppCt.DACXFIFO, mainAppCt.SamplesPreFrame * 2 * 2);
	DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_DAC1_TX);
	AudioDAC_Enable(DAC1);
#endif

#ifdef CFG_RES_AUDIO_I2SOUT_EN
	AudioI2sOutParamsSet();
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
    AudioI2S_ConfigInit();
#endif

#ifdef CFG_FUNC_LINE_MIX_MODE
    AudioLine_ConfigInit();
#endif

#ifdef CFG_FUNC_SPDIF_MIX_MODE
    AudioSpdif_ConfigInit();
#endif

#if (defined(BT_TWS_SUPPORT) || CFG_RES_MIC_SELECT)
	//DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_ADC1_RX);
	DMA_CircularConfig(PERIPHERAL_ID_AUDIO_ADC1_RX, mainAppCt.SamplesPreFrame * 2 * 2, (void*)mainAppCt.ADCFIFO, mainAppCt.SamplesPreFrame * 2 * 2 * 2);
	if(AudioADC_IsOverflow(ADC1_MODULE))
	{
		AudioADC_OverflowClear(ADC1_MODULE);
	}
	AudioADC_Clear(ADC1_MODULE);
    DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_ADC1_RX);
    AudioADC_LREnable(ADC1_MODULE, 1, 1);
    AudioADC_Enable(ADC1_MODULE);
	#ifdef CFG_FUNC_REMIND_MIX_MODE
    AudioMixRemind_ConfigInit();
    #endif
#endif
}

void SamplesFrameUpdataMsg(void)//发现帧变化，发送消息
{
	MessageContext		msgSend;
	APP_DBG("SamplesFrameUpdataMsg\n");

	msgSend.msgId		= MSG_AUDIO_CORE_FRAME_SIZE_CHANGE;
    MessageSend(mainAppCt.msgHandle, &msgSend);
}

void SamplesFrameUpdata(void)
{
	mainAppCt.SamplesPreFrame = gCtrlVars.SamplesPerFrame;
}

void EffectUpdataMsg(void)
{
	MessageContext		msgSend;
	APP_DBG("EffectUpdataMsg\n");

	msgSend.msgId		= MSG_AUDIO_CORE_EFFECT_CHANGE;
	MessageSend(mainAppCt.msgHandle, &msgSend);
}

uint16_t AudioDAC0DataSetNull(void* Buf, uint16_t Len)
{
     Buf = Buf;
     Len = Len;
     return 0;
}

uint16_t AudioDAC0DataSpaceLenGetNull(void)
{
	return 512*8;
}

void TwsSyncPowerOff(uint16_t msgId)
{
#ifdef BT_TWS_SUPPORT
	BtTwsExitPeerPairingMode();

	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		#ifdef TWS_POWEROFF_MODE_SYNC
		if (btManager.twsRole == BT_TWS_MASTER)
		{
			void tws_sync_power_off_msg(uint16_t msg);
			tws_sync_power_off_msg(msgId);
			btManager.twsState = BT_TWS_STATE_NONE;		
		}
		#else
		tws_link_disconnect();
		#endif
	}
#endif
}

void PowerOffMessage(void)
{
	MessageContext		msgSend;

#if	defined(CFG_FUNC_BACKUP_EN) && (POWERKEY_MODE == POWERKEY_MODE_PUSH_BUTTON)
	msgSend.msgId = MSG_POWERDOWN;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_POWERDOWN\n");
#elif defined(CFG_SOFT_POWER_KEY_EN)
	msgSend.msgId = MSG_SOFT_POWER;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_SOFT_POWEROFF\n");
#elif defined(CFG_FUNC_DEEPSLEEP_EN) 
	msgSend.msgId = MSG_DEEPSLEEP;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_DEEPSLEEP\n");
#else
	msgSend.msgId = MSG_POWER;
	APP_DBG("msgSend.msgId = MSG_POWER\n");
#endif

	MessageSend(GetMainMessageHandle(), &msgSend);
}

void MainTaskMsgSend(uint16_t msgId)
{
	MessageContext		msgSend;
	msgSend.msgId = msgId;
	MessageSend(GetMainMessageHandle(), &msgSend);
}


