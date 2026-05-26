/**
 **************************************************************************************
 * @file    bt_stack_service.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2018-2-9 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "gpio.h" //for BOARD
#include "debug.h"
#include "rtos_api.h"
#include "app_message.h"
#include "uarts.h"
#include "uarts_interface.h"
#include "dma.h"
#include "timeout.h"
#include "irqn.h"
//#include "ble_api.h"
#include "bt_config.h"
#include "bt_stack_api.h"
#include "bt_app_func.h"
#include "bt_stack_service.h"
#include "bt_play_mode.h"
#include "bt_play_api.h"
#include "bt_hf_mode.h"
#include "bt_app_interface.h"
#include "ble_api.h"
#include "ble_app_func.h"
#include "bt_avrcp_api.h"
#include "bt_manager.h"
#include "mode_switch_api.h"
#include "main_task.h"
#include "bt_pbap_api.h"
#include "bt_platform_interface.h"
#include "audio_core_service.h"
#include "bb_api.h"
#include "clk.h"
#include "reset.h"
#include "remind_sound_service.h"
#include "bt_ddb_flash.h"
#include "bt_record_mode.h"
#include "backup.h"
#include "powercontroller.h"
#include "sys.h"
#include "deepsleep.h"
#include "soft_watch_dog.h"

#ifdef BT_TWS_SUPPORT
#include "bt_tws_app_func.h"
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)//tws_auto_pair
const uint8_t gBtNameNull[] = {' '};
#endif
#endif
#ifdef CFG_APP_BT_MODE_EN

#ifdef CFG_BT_BACKGROUND_RUN_EN
uint8_t gBtHostStackMemHeap[BT_STACK_MEM_SIZE];
#endif
//BR/EDR STACK SERVICE
#ifdef BT_TWS_SUPPORT
#define BT_STACK_SERVICE_STACK_SIZE		900//768//
#define BT_STACK_SERVICE_PRIO			4
#else
#define BT_STACK_SERVICE_STACK_SIZE		768//1024
#define BT_STACK_SERVICE_PRIO			4
#endif
#define BT_STACK_NUM_MESSAGE_QUEUE		10

#ifndef BT_TWS_SUPPORT
//USER SERVICE //用来处理协议栈callback中用户需要处理的msg
#define BT_USER_SERVICE_STACK_SIZE		256
#define BT_USER_SERVICE_PRIO			3
#define BT_USER_NUM_MESSAGE_QUEUE		10
#endif

#ifdef MVA_BT_OBEX_UPDATE_FUNC_SUPPORT
	#define BT_OBEX_SERVICE_STACK_SIZE		512
	#define BT_OBEX_SERVICE_PRIO			4
	xTaskHandle			bt_obex_taskHandle;
#endif

typedef struct _BtStackServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	TaskState			serviceState;

	uint8_t				serviceWaitResume;	//1:蓝牙不在后台运行时,开启通话,退出播放模式,不能kill蓝牙协议栈

	uint8_t				bbErrorMode;
	uint32_t			bbErrorType;
#ifdef BT_TWS_SUPPORT
	uint32_t			btEnterSniffStep;
	uint32_t			btExitSniffReconPhone;
#endif
}BtStackServiceContext;

static BtStackServiceContext	*btStackServiceCt = NULL;
BtStackServiceContext		gBtStackServiceCt;

BT_CONFIGURATION_PARAMS		*btStackConfigParams = NULL;
BT_CONFIGURATION_PARAMS		gBtStackConfigParams;

//static uint32_t bbIsrCnt = 0;

//蓝牙通话模式/通话录音模式相关参数
extern uint8_t gEnterBtHfMode;
extern uint8_t gEnterBtRecordMode;

//蓝牙服务sniff 服务task。
#ifdef BT_SNIFF_ENABLE

typedef struct _BtUserServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	TaskState			serviceState;

}BtUserServiceContext;


#if(defined(BT_SNIFF_ENABLE) && !defined(BT_TWS_SUPPORT))
static BtUserServiceContext	*btUserServiceCt = NULL;
BtUserServiceContext		gBtUserServiceCt;
#endif

#endif	//BT_SNIFF_ENABLE

static void BtRstStateCheck(void);

void BtScanPageStateCheck(void);
void BtScanPageStateSet(BT_SCAN_PAGE_STATE state);

/***********************************************************************************
 * 蓝牙库延时函数
 **********************************************************************************/
void BtOsDelayMs(uint8_t delayCnt)
{
	vTaskDelay(delayCnt);
}

/***********************************************************************************
 * 
 **********************************************************************************/
void BtTwsPowerDownProcess(void)
{
	
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	
	mainAppCt.ReceiveDeepSleepFlag = TRUE;
	
	msgHandle = GetMainMessageHandle();
	msgSend.msgId = MSG_DEEPSLEEP;
	MessageSend(msgHandle, &msgSend);

}

/***********************************************************************************
 * 
 **********************************************************************************/
extern int32_t BtDeviceTrimValueSet(uint8_t TrimValue);
void BtGetInfo(uint8_t *bt_info,uint8_t type)
{
	uint8_t i;
	//APP_DBG("**********\nLocal Device Infor:\n");

	if(type==0)//bt name
	{
		for(i = 0; i < BT_NAME_SIZE;i++)//bt name
		{
			if(btStackConfigParams->bt_LocalDeviceName[i]=='\0')
			{
				break;
			}
			bt_info[i] = btStackConfigParams->bt_LocalDeviceName[i];
		}
#if (BLE_SUPPORT == ENABLE)
		for(i = 0; i < BT_NAME_SIZE;i++)//ble name
		{
			if(btStackConfigParams->ble_LocalDeviceName[i]=='\0')
			{
				break;
			}
			bt_info[i+BT_NAME_SIZE] = btStackConfigParams->ble_LocalDeviceName[i];
		}
#endif
	}
//------------------------------------//
	if(type==1)//bt mac  APP_DBG("FlashBtAddress(NAP-UAP-LAP):");
	{
		for(i = 0; i < BT_ADDR_SIZE;i++)//bt mac
		{
			bt_info[i] = btStackConfigParams->bt_LocalDeviceAddr[i];
		}
#if (BLE_SUPPORT == ENABLE)
		for(i = 0; i < BT_ADDR_SIZE;i++)//ble mac
		{
			bt_info[i+BT_ADDR_SIZE] = btStackConfigParams->ble_LocalDeviceAddr[i];
		}
#endif
	}
	//-----------------------------------------------//
	//APP_DBG("Freq trim:0x%x\n", btStackConfigParams->bt_trimValue);

}
/***********************************************************************************
 * 蓝牙测试盒校准频偏完成回调函数
 **********************************************************************************/
void BtFreqOffsetAdjustComplete(unsigned char offset)
{
	int8_t ret = 0;
	APP_DBG("++++++[BT_OFFSET]  offset:0x%x ++++++\n", offset);

	btManager.btLastAddrUpgradeIgnored = 1;

	//判断是否和当前默认值一致,不一致更新保存到flash
	if(offset != btStackConfigParams->bt_trimValue)
	{
		btStackConfigParams->bt_trimValue = offset;
	
		//save to flash
		//ret = BtDdb_SaveBtConfigurationParams(btStackConfigParams);
		ret = BtDeviceTrimValueSet(btStackConfigParams->bt_trimValue);

		if(ret)
			APP_DBG("[BT_OFFSET]update Error!!!\n");
		else
			APP_DBG("$$$[BT_OFFSET] update $$$\n");
	}

	//清除最后1次配对记录
	BtDdb_LastBtAddrErase();
}

/***********************************************************************************
 * 蓝牙middleware的消息传递入口函数
 **********************************************************************************/
void BtMidMessageManage(BtMidMessageId messageId, uint8_t Param)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;

	switch(messageId)
	{
		case MSG_BT_MID_UART_RX_INT:
			msgHandle = GetBtStackServiceMsgHandle();
			msgSend.msgId = MSG_BTSTACK_RX_INT;
			MessageSend(msgHandle, &msgSend);
			break;

		case MSG_BT_MID_ACCESS_MODE_IDLE:
			BtReconnectDevice();
			break;

		case MSG_BT_MID_STACK_INIT:
			{
				//此处配置协议栈初始化完成后，是否进入到蓝牙可被搜索可被连接状态;
				//1=进入到可被搜索可被连接状态;  0=进入到不可被搜索不可被连接状态
				GetBtManager()->btAccessModeEnable = 1;
				#ifdef POWER_ON_BT_ACCESS_MODE_SET
				GetBtManager()->btScanDisable = 1;
				#else
				GetBtManager()->btScanDisable = 0;
				#endif
#ifdef BT_TWS_SUPPORT
			#if (CFG_TWS_ONLY_IN_BT_MODE == DISABLE)
				if(GetBtManager()->twsFlag)
					GetBtManager()->btConStateProtectCnt = 1;
				else
			#endif
					GetBtManager()->btConStateProtectCnt = 0;
#endif
			}
			break;

		case MSG_BT_MID_STATE_CONNECTED:
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_BT_STATE_CONNECTED;
				MessageSend(GetMainMessageHandle(), &msgSend);
#ifdef BT_USER_STATE_DISPLAY
				SetBtUserState(BT_USER_STATE_CONNECTED);
#endif
				SetBtPlayState(BT_PLAYER_STATE_STOP);
			}
			break;
		
		case MSG_BT_MID_STATE_DISCONNECT:
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_BT_STATE_DISCONNECT;
				MessageSend(GetMainMessageHandle(), &msgSend);
#ifdef BT_USER_STATE_DISPLAY
				SetBtUserState(BT_USER_STATE_DISCONNECTED);
#endif
				SetBtPlayState(BT_PLAYER_STATE_STOP);
			}
			break;

		case MSG_BT_MID_STATE_FAST_ENABLE:
//#ifdef BT_FAST_POWER_ON_OFF_FUNC
			if(GetSystemMode() == AppModeBtAudioPlay)
			{
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
#if (defined(BT_TWS_SUPPORT) && ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)))
				if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_SLAVE))
					BtReconnectTws_Slave();
#endif
			}
			else
			{
#if (defined(BT_TWS_SUPPORT) && ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)) && (CFG_TWS_ONLY_IN_BT_MODE == DISABLE))
				if(btManager.twsFlag)
				{
					if(btManager.twsRole == BT_TWS_SLAVE)
					{
						BtReconnectTws_Slave();
					}
					else 
					{
						BtReconnectTws();
					}
				}
#endif
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_CLOSING);
			}
//#endif
			break;

//////////////////////////////////////////////////////////////////////////////////////////////////
//AVRCP
		case MSG_BT_MID_PLAY_STATE_CHANGE:
			if((Param == BT_PLAYER_STATE_PLAYING)&&(GetA2dpState() == BT_A2DP_STATE_STREAMING))
			{
				msgHandle = GetMainMessageHandle();
				msgSend.msgId		= MSG_BT_A2DP_STREAMING;
				MessageSend(msgHandle, &msgSend);
			}
			
			msgHandle = GetBtPlayMessageHandle();
			if(msgHandle == NULL)
				break;

			if(GetBtPlayState() == Param)
				break;
			
			SetBtPlayState(Param);

			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_PLAY_STATE_CHANGED;
			MessageSend(msgHandle, &msgSend);
			break;

		case MSG_BT_MID_STREAM_PAUSE:
			msgHandle = GetBtPlayMessageHandle();
			if(msgHandle == NULL)
				break;
			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_PLAY_STREAM_PASUE;
			MessageSend(msgHandle, &msgSend);
			break;

		case MSG_BT_MID_VOLUME_CHANGE:		
#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
			msgHandle =GetMainMessageHandle();// GetBtPlayMessageHandle();
			//if(msgHandle == NULL)
			//	break;
			
			SetBtSyncVolume(Param);
			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_PLAY_SYNC_VOLUME_CHANGED;
			MessageSend(msgHandle, &msgSend);
#endif
			break;

//////////////////////////////////////////////////////////////////////////////////////////////////
//HFP
#if (BT_HFP_SUPPORT == ENABLE)
		case MSG_BT_MID_HFP_TASK_RESUME:
			BtHfModeRunningResume();
			break;

		//通话数据格式更新
		case MSG_BT_MID_HFP_CODEC_TYPE_UPDATE:
			BtHfCodecTypeUpdate(Param);
			break;

#if defined(CFG_FUNC_REMIND_SOUND_EN) || defined(CFG_FUNC_REMIND_MIX_MODE)
		//通话模式下呼入电话播放来电提示音
		case MSG_BT_MID_HFP_PLAY_REMIND:
			msgHandle = GetBtHfMessageHandle();
			if(msgHandle == NULL)
				break;
#if (CFG_BT_RING_MODE >= RING_NUMBER_REMIND)
			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_HF_MODE_REMIND_PLAY;
			MessageSend(msgHandle, &msgSend);
#endif
			break;
		
		//通话模式下停止播放提示音
		case MSG_BT_MID_HFP_PLAY_REMIND_END:
			msgHandle = GetBtHfMessageHandle();
			if(msgHandle == NULL)
				break;
#if defined(CFG_FUNC_REMIND_SOUND_EN)
			RemindSoundServiceEnd();
#endif
#if defined(CFG_FUNC_REMIND_MIX_MODE)
			RemindMixStop();
#endif
			break;
#endif

#ifdef BT_RECORD_FUNC_ENABLE
		case MSG_BT_MID_HFP_RECORD_MODE_ENTER://进入通话录音模式
			{
				if(!gEnterBtHfMode)
					EnterBtRecordMode();
			}
			break;
	
		case MSG_BT_MID_HFP_RECORD_MODE_EXIT://退出通话录音模式
			{
				ExitBtRecordMode();
			}
			break;
			
		case MSG_BT_MID_HFP_RECORD_MODE_DEREGISTER://注销通话录音模式
			{
				if(GetSystemMode() == AppModeBtRecordPlay)
				{
					BtRecordModeDeregister();
				}
			}
			break;
#endif

#endif
		case MSG_BT_MID_HFP_CONNECTED:
			if(mainAppCt.appCurrentMode != AppModeBtHfPlay)
			{
				gEnterBtHfMode = 0;
				gEnterBtRecordMode = 0;
			}
			break;
		
#if BT_AVRCP_VOLUME_SYNC
		case MSG_BT_MID_AVRCP_PANEL_KEY:
			{
				MessageContext		msgSend={0};
				switch(Param)
				{
				case 65://vol +
					msgSend.msgId=MSG_MUSIC_VOLUP;
				break;
	
				case 66:////vol -
					msgSend.msgId=MSG_MUSIC_VOLDOWN;
				break;
				/*		
					case 68:
						msgSend.msgId=MSG_PLAY;
					break;
					case 70://pause
						msgSend.msgId=MSG_PAUSE;
					break;
					case 75:
						msgSend.msgId=MSG_NEXT;
					break;
					case 76:
						msgSend.msgId=MSG_PRE;
					break;
					default:
					break;*/
				}
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
			break;
#endif
		
//////////////////////////////////////////////////////////////////////////////////////////////////
		default:
			break;
	}
}

extern uint32_t btReConProtectCnt;
#ifdef BT_TWS_SUPPORT
extern unsigned short ME_CancelInquiry(void);
#endif
#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE) //20240515
uint32_t gGetBtMediaInforCnt = 0;
#endif
static void CheckBtEventTimer(void)
{
#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
	//获取蓝牙歌曲和歌词信息 //20240515
	if(gGetBtMediaInforCnt)
	{
		gGetBtMediaInforCnt = 0;
		BTCtrlGetMediaInfor();
	}
#endif

#if (TWS_CONNECTE_WHEN_ACTIVE_DISCONNECTION_SUPPORT == ENABLE)
	if(tws_active_disconnection_get() == 1)
	{
		BtDdb_ClearTwsDeviceAddrList();	
		tws_active_disconnection_set(0);
		
		APP_DBG("tws disconnect\n");
		BtTwsDisconnectApi();
	}
#endif
#ifdef BT_TWS_SUPPORT
	if(GetBtManager()->twsStopConnect == 1)
	{
		BtTwsExitPeerPairingMode();
		ME_CancelInquiry();	
		GetBtManager()->twsStopConnect = 0;
	}
#endif
	//开机蓝牙回连和主动连接冲突时,保护时间5s;对方远端设备先发起连接,5s内不能发起回连
	if(btReConProtectCnt)
	{
		btReConProtectCnt++;
		if(btReConProtectCnt>=5000)
		{
			btReConProtectCnt = 0;
		}
	}

	//获取蓝牙播放状态
	if(GetBtManager()->avrcpPlayStatusTimer.timerFlag)
	{
		if(IsTimeOut(&GetBtManager()->avrcpPlayStatusTimer.timerHandle))
		{
			BT_A2DP_STATE state = GetA2dpState();
			if(state == BT_A2DP_STATE_STREAMING)
			{
				BTCtrlGetPlayStatus();
				TimerStart_BtPlayStatus();
			}
			else
			{
				TimerStop_BtPlayStatus();
			}
		}
	}

	if(GetBtManager()->btReconnectDelayCount)
	{
		if (!SoftFlagGet(SoftFlagDecoderRemind))
		{
			GetBtManager()->btReconnectDelayCount++;
			if(GetBtManager()->btReconnectDelayCount>200)
			{
				GetBtManager()->btReconnectDelayCount = 0;
				BtReconnectDevice();
			}
		}
	}
	
	if(GetBtManager()->btReconnectTimer.timerFlag & TIMER_STARTED)
	{
		if(IsTimeOut(&GetBtManager()->btReconnectTimer.timerHandle))
		{
			CheckBtReconnectTimer();
		}
	}
#ifdef BT_TWS_SUPPORT
	if(GetBtManager()->btTwsReconnectTimer.timerFlag & TIMER_STARTED)
	{
		if(IsTimeOut(&GetBtManager()->btTwsReconnectTimer.timerHandle))
		{
			CheckBtReconnectTwsTimer();
		}
	}
#endif

//#if (defined(BT_FAST_POWER_ON_OFF_FUNC)&&defined(CFG_BT_BACKGROUND_RUN_EN))
	BtScanPageStateCheck();
//#endif

	BtRstStateCheck();

#if(defined(BT_SNIFF_ENABLE) && defined(BT_TWS_SUPPORT))
	BtStartEnterSniffStep();
	BtExitSniffReconnectPhone();
#endif //BT_SNIFF_ENABLE
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/**
 * @brief	Get message receive handle of bt stack manager
 * @param	NONE
 * @return	MessageHandle
 */
MessageHandle GetBtStackServiceMsgHandle(void)
{
	if(!btStackServiceCt)
		return NULL;
	
	return btStackServiceCt->msgHandle;
}

TaskState GetBtStackServiceState(void)
{
	if(!btStackServiceCt)
		return 0;
	
	return btStackServiceCt->serviceState;
}

void update_btDdb(uint8_t addr)
{
	btStackConfigParams->bt_LocalDeviceAddr[3] = addr;
	BtDdb_SaveBtConfigurationParams(btStackConfigParams);
	BtDdb_Erase();
}

static void CheckBtErrorState(void)
{
	btEventListCount++;
	if(btCheckEventList)
	{
		//防止手机端不主动断开AVRCP,在手机端断开A2DP后,设置5S超时,然后主动断开AVRCP
		if((btCheckEventList&BT_EVENT_AVRCP_DISCONNECT)&&(btEventListCount == btEventListB0Count))
		{
			APP_DBG("[btCheckEventList]: BT_EVENT_AVRCP_DISCONNECT\n");
			if(GetAvrcpState() > BT_AVRCP_STATE_NONE)
			{
				AvrcpDisconnect();
			}
			btCheckEventList &= ~BT_EVENT_AVRCP_DISCONNECT;
			btEventListB0Count = 0;
		}

		//当接收到BT_STACK_EVENT_COMMON_CONNECTION_ABORTED事件,当前链路出现异常,会主动断开手机,需要主动回连手机,并自动播放歌曲
		//超时时间为30s
		if((btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT)&&(btEventListCount == btEventListB1Count))
		{
			APP_DBG("[btCheckEventList]: BT_EVENT_L2CAP_LINK_DISCONNECT\n");
			
			btCheckEventList &= ~BT_EVENT_L2CAP_LINK_DISCONNECT;
			btEventListB0Count = 0;
		}

		if((btCheckEventList&BT_EVENT_AVRCP_CONNECT)&&(btEventListB2Count))
		{
			btEventListB2Count--;
			if(!btEventListB2Count)
			{
				btCheckEventList &= ~BT_EVENT_AVRCP_CONNECT;
				btEventListB2Count = 0;
				if((GetAvrcpState() < BT_AVRCP_STATE_CONNECTED)&&(GetA2dpState() >= BT_A2DP_STATE_CONNECTED))
				{
					BtAvrcpConnect(GetBtManager()->remoteAddr);
				}
			}
		}
	}
}

//此函数lib中调用，客户可根据自己条件配置
void BtCntClkSet(void)
{
#if (BT_SNIFF_CLK_SEL == BT_SNIFF_HOSC_CLK)
	//HOSC 32K
	Clock_BTDMClkSelect(OSC_32K_MODE);
#ifdef BT_TWS_SUPPORT
	Clock_OSC32KClkSelect(LOSC_32K_MODE);
#else
	Clock_OSC32KClkSelect(HOSC_DIV_32K_CLK_MODE);
#endif
	Clock_32KClkDivSet(Clock_OSCClkDivGet());  //如果这里出现问题，请检查SystemClockInit，不要在这里直接改——Tony
	Clock_BBCtrlHOSCInDeepsleep(0);//禁止baseband进入sniff后硬件自动关闭HOSC 24M

#elif (BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK)
	//RC
	sniff_rc_init_set();//Rc 初始化参数
	//RC 32K
	Clock_BTDMClkSelect(RC_CLK32_MODE);//select rc_clk_32k
	Clock_32KClkDivSet(750);     //set osc_clk_32k = 24M/32K=750

	Clock_RcCntWindowSet(30);//64-1  --  32K/64 = 500

	Clock_RC32KClkDivSet( Clock_RcFreqGet(1) / ((uint32_t)(32*1000)) );
	Clock_RcFreqCntOneTimeStart();

	Clock_BBCtrlHOSCInDeepsleep(1);//Deepsleep时,BB接管HOSC

#elif (BT_SNIFF_CLK_SEL == BT_SNIFF_LOSC_CLK)
	//btclk freq set
	BACKUP_32KEnable(OSC32K_SOURCE);
	sniff_cntclk_set(1);//sniff cnt clk 32768 Hz default not use

	Clock_BTDMClkSelect(OSC_32K_MODE);
	Clock_OSC32KClkSelect(LOSC_32K_MODE);
	Clock_BBCtrlHOSCInDeepsleep(0);//禁止baseband进入sniff后硬件自动关闭HOSC 24M

#endif
}


#ifdef BT_TWS_SUPPORT
void BtSetFreqTrim(unsigned char value);
uint8_t BtGetFreqTrim(void);
#define     ADR_OSC_CTRL                                               (0x40021070)
typedef struct _ST_OSC_CTRL {
      unsigned long HOSC_CAP_XI                :  5;
      unsigned long HOSC_CAP_XO                :  5;
      unsigned long HOSC_CS                    :  4;
      unsigned long HOSC_LDO_TRIMMING          :  5;
      unsigned long HOSC_LDO_TEST              :  1;
      unsigned long HOSC_LDO_EN                :  1;
      unsigned long HOSC_EN                    :  1;
      unsigned long BB_CTRL_HOSC_EN            :  1;
} ST_OSC_CTRL __ATTRIBUTE__(BITBAND);

#define SREG_OSC_CTRL                       (*(volatile ST_OSC_CTRL *) ADR_OSC_CTRL)

typedef struct TWS_CAP_INFO_
{
uint8_t val;
uint32_t count;
}TWS_CAP_INFO;

TWS_CAP_INFO cap_info[5];
uint32_t cap_set_setup = 0;
uint32_t rx_count=0;
#endif

void acl_rx_callback(int8_t s)
{
#ifdef BT_TWS_SUPPORT
#ifndef CFG_FUNC_MIC_KARAOKE_EN
	if(GetBtManager()->twsRole != BT_TWS_SLAVE)
#endif
	{
		return;
	}
	rx_count++;
	if(( (rx_count%20)==0) && (cap_set_setup == 0) )
	{
		uint8_t cap = BtGetFreqTrim();
		if(s < 0)
		{
			cap++;
			BtSetFreqTrim(cap);
		}
		else if(s > 0)
		{
			cap--;
			BtSetFreqTrim(cap);
		}
		else
		{
			int i;
			uint8_t xio = BtGetFreqTrim();
			for(i=0;i<5;i++)
			{
				if(cap_info[i].val == xio)
				{
					//找到了合适的位置
					cap_info[i].count++;
					break;
				}
			}
			if(i == 5)//没有找到
			{
				uint32_t min = 0xFFFFFFFF;
				int min_nidex = 0;
				for(i=0;i<5;i++)
				{
					if(cap_info[i].count <= min)
					{
						min = cap_info[i].count;
						min_nidex = i;
					}
				}
				cap_info[min_nidex].count = 1;
				cap_info[min_nidex].val = xio;
			}
			uint32_t max = 0;
			int max_nidex = 0;
			for(i=0;i<5;i++)
			{
				if(cap_info[i].count >= max)
				{
					max = cap_info[i].count;
					max_nidex = i;
				}
			}
			if( (max > 0) && (cap_info[max_nidex].count > 100) )
			{
				BtSetFreqTrim(cap_info[max_nidex].val);
				cap_set_setup = 1;
			}
		}
	}
#endif
}

uint32_t g_tws_need_init;
static void BtStackServiceEntrance(void * param)
{
	MessageContext		msgRecv;
#if 0
	BtBbParams bbParams;
	
	//load bt stack all params
	LoadBtConfigurationParams();
	
	//BB init
	ConfigBtBbParams(&bbParams);
	Bt_init((void*)&bbParams);

	//host memory init
	SetBtPlatformInterface(&pfiOS, &pfiBtDdb);

#ifdef BT_TWS_SUPPORT
#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	Other_confirm_Callback_Set(NULL);
#else
	Other_confirm_Callback_Set(BtConnectConfirm);
#endif
#endif
#endif//BT_TWS_SUPPORT

#ifdef CFG_BT_BACKGROUND_RUN_EN
	//在蓝牙开启后台运行时,host的内存采用数组,避免存在申请/释放带来碎片化的风险
	BTStackMemAlloc(BT_STACK_MEM_SIZE, gBtHostStackMemHeap, 0);
#else
	BTStackMemAlloc(BT_STACK_MEM_SIZE, NULL, 1);//PTS测试时需要开大内存
#endif
#endif
	APP_DBG("BtStackServiceEntrance.\n");

	//BR/EDR init
	if(!BtStackInit())
	{
		APP_DBG("error init bt device\n");
		//出现初始化异常时,蓝牙协议栈任务挂起
		while(1)
		{
			MessageRecv(btStackServiceCt->msgHandle, &msgRecv, 0xFFFFFFFF);
		}
	}
	else
	{
		APP_DBG("bt device init success!\n");
	}

	//BLE init
#if (BLE_SUPPORT == ENABLE)
	{
		InitBlePlaycontrolProfile();
		
		if(!InitBleStack(&g_playcontrol_app_context, &g_playcontrol_profile))
		{
			APP_DBG("error ble stack init\n");
		}
#ifdef BT_TWS_SUPPORT
	#ifdef CFG_TWS_SOUNDBAR_APP
		TwsPeerBlePairingRegist(0);
	#else
		TwsPeerBlePairingRegist(1);
	#endif
#endif
	}
#endif
#ifdef SOFT_WACTH_DOG_ENABLE
	SWD_task_register(SWD_BtStackTask_ID);
#endif

	vTaskPrioritySet(xTaskGetCurrentTaskHandle(), BT_STACK_SERVICE_PRIO);

	while(1)
	{
#ifdef SOFT_WACTH_DOG_ENABLE
		SWD_task_reset(SWD_BtStackTask_ID);
#endif
		//在蓝牙协议栈当前所有的事情处理完后,才会挂起任务
		//if(!HasBtDataToProccess())
		{
			MessageRecv(btStackServiceCt->msgHandle, &msgRecv, 1);
#ifdef BT_SNIFF_ENABLE

	#if (BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK)
		//RC时钟校准
		if(Clock_RcFreqCntOneTimeReady())
		{
		   Clock_RcFreqCntOneTimeGet(1);
		   Clock_RcFreqCntOneTimeStart();
		}
	#endif//BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK
#endif//BT_SNIFF_ENABLE
		}

		switch(msgRecv.msgId)
		{
			case MSG_BTSTACK_BB_ERROR:
				{
					MessageContext		msgSend;
					MessageHandle 		msgHandle;
					msgHandle = GetMainMessageHandle();
					msgSend.msgId = MSG_BTSTACK_BB_ERROR;

					MessageSend(msgHandle, &msgSend);

					if(btStackServiceCt->bbErrorMode == 1)
					{
						APP_DBG("BT ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
					}
					else if(btStackServiceCt->bbErrorMode == 2)
					{
						APP_DBG("BLE ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
					}
				}
				break;
#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM) && defined(BT_TWS_SUPPORT))//tws_auto_pair
			case MSG_BTSTACK_BROADCAST_NAME_UPDATE:
				{
					extern void BtSetDeviceName(uint8_t *name, uint8_t len);
					extern uint32_t doubleKeyCnt;
					//蓝牙名称下发到协议栈
					if(btManager.btLinkState && btManager.twsState != BT_TWS_STATE_CONNECTED && doubleKeyCnt)
					{
						BtSetDeviceName(gBtNameNull, 1);
					}
					else
					{
						BtSetDeviceName(btStackConfigParams->bt_LocalDeviceName, strlen(btStackConfigParams->bt_LocalDeviceName));
					}
				}
				break;
#endif
#if ((BLE_SUPPORT == ENABLE) && defined(BT_TWS_SUPPORT))
			case MSG_BTSTACK_BLE_ADV_SCAN:
				APP_DBG("MSG_BTSTACK_BLE_ADV_SCAN\n");
				ble_adv_scan_set();
				break;

			case MSG_BTSTACK_BLE_ONLY_ADV:
				APP_DBG("MSG_BTSTACK_BLE_ONLY_ADV\n");
				ble_only_adv_set();
				break;

			case MSG_BTSTACK_BLE_ONLY_SCAN:
				APP_DBG("MSG_BTSTACK_BLE_ONLY_SCAN\n");
				ble_only_scan_set();
				break;

			case MSG_BTSTACK_BLE_DISABLE_ADV_SCAN:
				APP_DBG("MSG_BTSTACK_BLE_DISABLE_ADV_SCAN\n");
				ble_adv_scan_disable();
				break;

			case MSG_BTSTACK_BLE_ADV_DATA_DELAY_RESUME:
				APP_DBG("MSG_BTSTACK_BLE_ADV_DATA_RESUME\n");
				//ble_advdata_updata();
				btManager.bleAdvDataSetDelayCnt = 500;
				break;
#endif

#ifdef BT_TWS_SUPPORT
			case MSG_BTSTACK_TWS_CONNECT:
				APP_DBG("[BT_STACK_APP]:tws connect: %02x:%02x:%02x:%02x:%02x:%02x\n", 
					btManager.btTwsDeviceAddr[0],
					btManager.btTwsDeviceAddr[1],
					btManager.btTwsDeviceAddr[2],
					btManager.btTwsDeviceAddr[3],
					btManager.btTwsDeviceAddr[4],
					btManager.btTwsDeviceAddr[5]
					);
				if((!btManager.btTwsDeviceAddr[0])&&(!btManager.btTwsDeviceAddr[1])&&(!btManager.btTwsDeviceAddr[2])&&
					(!btManager.btTwsDeviceAddr[3])&&(!btManager.btTwsDeviceAddr[4])&&(!btManager.btTwsDeviceAddr[5])
					)
				{
					APP_DBG("addr error!!!\n");
					break;
				}

				tws_master_connect(btManager.btTwsDeviceAddr);
				break;

			case MSG_BTSTACK_TWS_DISCONNECT:
				APP_DBG("[BT_STACK_APP]:tws disconnect\n");
				tws_link_disconnect();
				break;
				
			
			case MSG_BTSTACK_TWS_SYNC:
				{
					tws_sync_reinit();
					printf("tws_sync_reinit..\n");
				}
				break;
#endif
				
				case MSG_BTSTACK_LOCAL_DEVICE_NAME_UPDATE:
				{
					APP_DBG("MSG_BTSTACK_LOCAL_DEVICE_NAME_UPDATE\n");
					extern void BtSetDeviceName(uint8_t *name, uint8_t len);
					//蓝牙名称下发到协议栈
					BtSetDeviceName(btStackConfigParams->bt_LocalDeviceName, strlen((const char *)(btStackConfigParams->bt_LocalDeviceName)));
					//蓝牙名称保存到本地
					BtDeviceNameSet(btStackConfigParams->bt_LocalDeviceName, strlen((const char *)(btStackConfigParams->bt_LocalDeviceName)));
				}
				break;
		}
		
		rw_main();
		
		BTStackRun();
		CheckBtEventTimer();
		
		//bt异常情况处理
		CheckBtErrorState();

#ifdef BT_TWS_SUPPORT
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
		if(btManager.TwsFunctionEnable)
#endif
		{
			if(GetSystemMode() != AppModeBtHfPlay)
			{
				tws_run_loop();
			}
			BtTwsRunLoop();
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		TwsDoubleKeyPairTimeOut();
#endif
		}
#endif
		AudioCoreServiceMsg();

#ifdef BT_HFP_MODE_DISABLE
		extern uint32_t gHfpCallNoneWaiting;
		if(gHfpCallNoneWaiting)
		{
			gHfpCallNoneWaiting++;
			if(gHfpCallNoneWaiting>=1000) //delay 1000ms
			{
				gHfpCallNoneWaiting=0;
//				printf("HfpCallNoneWaiting end\n");
			}
		}
#endif

#ifdef BT_TWS_SUPPORT
		if(btManager.btConStateProtectCnt)
		{
			btManager.btConStateProtectCnt++;
			if(btManager.btConStateProtectCnt>=5000)
			{
				btManager.btConStateProtectCnt=0;
				APP_DBG("bt connect state protect end...\n");
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			}
		}
		if(g_tws_need_init == 1)
		{
			if(!SoftFlagGet(SoftFlagDecoderRemind))
			{
				if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
				{
					APP_DBG("tws_init...\n");
					AudioPlayerSinkMuteRemind(1);
					g_tws_need_init = 2;
					tws_init();
				}
				else
				{
					g_tws_need_init = 0;
				}
			}
		}
#endif

		if(btManager.btAccessModeSt.delayTime)
		{
			btManager.btAccessModeSt.delayTime--;
			if(btManager.btAccessModeSt.delayTime == 0)
			{
				APP_DBG("====== set access mode: %d\n", btManager.btAccessModeSt.accessMode);
				BTSetAccessMode(btManager.btAccessModeSt.accessMode);
			}
		}
	}
}

/**
 * @brief	Start bluetooth stack service initial.
 * @param	NONE
 * @return	
 */
static bool BtStackServiceInit(void)
{
	APP_DBG("bluetooth stack service init.\n");

	btStackServiceCt = (BtStackServiceContext*)&gBtStackServiceCt;//(BtStackServiceContext*)osPortMalloc(sizeof(BtStackServiceContext));
	if(btStackServiceCt == NULL)
	{
		return FALSE;
	}
	memset(btStackServiceCt, 0, sizeof(BtStackServiceContext));
	
	btStackConfigParams = (BT_CONFIGURATION_PARAMS*)&gBtStackConfigParams;//(BT_CONFIGURATION_PARAMS*)osPortMalloc(sizeof(BT_CONFIGURATION_PARAMS));
	if(btStackConfigParams == NULL)
	{
		return FALSE;
	}
	memset(btStackConfigParams, 0, sizeof(BT_CONFIGURATION_PARAMS));

	btStackServiceCt->msgHandle = MessageRegister(BT_STACK_NUM_MESSAGE_QUEUE);
	if(btStackServiceCt->msgHandle == NULL)
	{
		return FALSE;
	}
	btStackServiceCt->serviceState = TaskStateCreating;

	//register bt middleware message send interface
	BtAppiFunc_MessageSend(BtMidMessageManage);

#ifdef BT_SNIFF_ENABLE
#ifndef BT_TWS_SUPPORT
	//user service
	btUserServiceCt = (BtUserServiceContext*)&gBtUserServiceCt;//(BtUserServiceContext*)osPortMalloc(sizeof(BtUserServiceContext));
	if(btUserServiceCt == NULL)
	{
		return FALSE;
	}
	memset(btUserServiceCt, 0, sizeof(BtUserServiceContext));

	btUserServiceCt->msgHandle = MessageRegister(BT_USER_NUM_MESSAGE_QUEUE);
	if(btUserServiceCt->msgHandle == NULL)
	{
		return FALSE;
	}
#endif//BT_TWS_SUPPORT
#endif//BT_SNIFF_ENABLE

	//bt rf module check
	//TimeOutSet(&btRfTimerHandle, 2000);
#ifdef BT_TWS_SUPPORT
	BtTwsAppInit();
#endif

	return TRUE;
}

/**
 * @brief	Start bluetooth stack service.
 * @param	NONE
 * @return	
 */
 
bool BtStackServiceStart(void)
{
	bool		ret = TRUE;
	if((btStackServiceCt->serviceWaitResume)&&(btStackServiceCt))
	{
		btStackServiceCt->serviceWaitResume = 0;
		return ret;
	}

#ifdef BT_TWS_SUPPORT
#if (BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK)
	extern void sniff_clk_set(int8_t set);
	sniff_clk_set(0);//sniff use rc
#endif
#endif

	memset((uint8_t*)BB_EM_MAP_ADDR, 0, BB_EM_SIZE);//clear em erea
	
	ClearBtManagerReg();

	SetBtStackState(BT_STACK_STATE_INITAILIZING);
	
	ret = BtStackServiceInit();
	if(ret)
	{
		btStackServiceCt->taskHandle = NULL;
		
#if 1
		BtBbParams bbParams;

		//load bt stack all params
		LoadBtConfigurationParams();

		//BB init
		ConfigBtBbParams(&bbParams);
		Bt_init((void*)&bbParams);

		//host memory init
		SetBtPlatformInterface(&pfiOS, &pfiBtDdb);

	#ifdef BT_TWS_SUPPORT
	#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
	#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
		Other_confirm_Callback_Set(NULL);
	#else
		Other_confirm_Callback_Set(BtConnectConfirm);
	#endif
	#endif
	#endif//BT_TWS_SUPPORT

	#ifdef CFG_BT_BACKGROUND_RUN_EN
		//在蓝牙开启后台运行时,host的内存采用数组,避免存在申请/释放带来碎片化的风险
		BTStackMemAlloc(BT_STACK_MEM_SIZE, gBtHostStackMemHeap, 0);
	#else
		BTStackMemAlloc(BT_STACK_MEM_SIZE, NULL, 1);//PTS测试时需要开大内存
	#endif
#endif

		xTaskCreate(BtStackServiceEntrance, 
					"BtStack", 
					BT_STACK_SERVICE_STACK_SIZE, 
					NULL, 
					(BT_STACK_SERVICE_PRIO-1),
					&btStackServiceCt->taskHandle);
		if(btStackServiceCt->taskHandle == NULL)
		{
			ret = FALSE;
		}
#ifdef MVA_BT_OBEX_UPDATE_FUNC_SUPPORT
				void bt_obex_upgrate(void);
				xTaskCreate(bt_obex_upgrate,
									"bt_obex_upgrate",
									BT_OBEX_SERVICE_STACK_SIZE,
									NULL,
									BT_OBEX_SERVICE_PRIO,
									&bt_obex_taskHandle);
				if(bt_obex_taskHandle == NULL)
				{
					ret = FALSE;
				}
#endif
#ifdef BT_SNIFF_ENABLE
#ifndef BT_TWS_SUPPORT
		xTaskCreate(BtUserServiceEntrance,
							"BtUserService",
							BT_USER_SERVICE_STACK_SIZE,
							NULL,
							BT_USER_SERVICE_PRIO,
							&btUserServiceCt->taskHandle);
		if(btUserServiceCt->taskHandle == NULL)
		{
			ret = FALSE;
		}
#endif//BT_TWS_SUPPORT
#endif//BT_SNIFF_ENABLE
	}
	if(!ret)
		APP_DBG("BtStack service create fail!\n");
	return ret;
}

/**
 * @brief	Kill bluetooth stack service.
 * @param	NONE
 * @return	
 */
bool BtStackServiceKill(void)
{
	int32_t ret = 0;
	if(btStackServiceCt == NULL)
	{
		return FALSE;
	}

	//btStackService
	//Msgbox
	if(btStackServiceCt->msgHandle)
	{
		MessageDeregister(btStackServiceCt->msgHandle);
		btStackServiceCt->msgHandle = NULL;
	}
	
	//task
	if(btStackServiceCt->taskHandle)
	{
		vTaskDelete(btStackServiceCt->taskHandle);
		btStackServiceCt->taskHandle = NULL;
	}

	//deregister bt middleware message send interface
	BtAppiFunc_MessageSend(NULL);

#if (BLE_SUPPORT == ENABLE)
	UninitBleStack();
	UninitBlePlaycontrolProfile();
#endif

	//stack deinit
	ret = BtStackUninit();
	if(!ret)
	{
		APP_DBG("Bt Stack Uninit fail!!!\n");
		return FALSE;
	}

	if(btStackConfigParams)
	{
		//osPortFree(btStackConfigParams);
		btStackConfigParams = NULL;
	}
	//
	if(btStackServiceCt)
	{
		//osPortFree(btStackServiceCt);
		btStackServiceCt = NULL;
	}
	APP_DBG("!!btStackServiceCt\n");
	

	return TRUE;
}

//
void BtStackServiceWaitResume(void)
{
	btStackServiceCt->serviceWaitResume = 1;
}

//注:需要判断当前是否在中断中，需要调用不同的消息发送函数接口
extern uint32_t GetIPSR( void );
void WakeupBtStackService(void)
{
/*	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_RX_INT;

	if(GetIPSR())
		MessageSendFromISR(msgHandle, &msgSend);
	else
		MessageSend(msgHandle, &msgSend);
*/
}
void BBMatchReport(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BB_ERROR;
	MessageSend(mainAppCt.msgHandle, &msgSend);
}

void BBErrorReport(uint8_t mode, uint32_t errorType)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	if(btStackServiceCt == NULL)
		return;
	
	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_BB_ERROR;

	btStackServiceCt->bbErrorMode = mode;
	btStackServiceCt->bbErrorType = errorType;

	//isr
	MessageSendFromISR(msgHandle, &msgSend);
}

void BBIsrReport(void)
{
//	bbIsrCnt++;
}

void BT_IntDisable(void)
{
	NVIC_DisableIRQ(18);//BT_Interrupt =18
	NVIC_DisableIRQ(19);//BLE_Interrupt =19
}

void BT_ModuleClose(void)
{
	Reset_RegisterReset(MDM_REG_SEPA);
	Reset_FunctionReset(BTDM_FUNC_SEPA|MDM_FUNC_SEPA|RF_FUNC_SEPA);
	Clock_Module2Disable(ALL_MODULE2_CLK_SWITCH); //close clock
}

#ifdef BT_TWS_SUPPORT
/***********************************************************************************
 * 
 **********************************************************************************/
void BtTwsConnectApi(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_TWS_CONNECT;
	MessageSend(btStackServiceCt->msgHandle, &msgSend);
}

void BtTwsDisconnectApi(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_TWS_DISCONNECT;
	MessageSend(btStackServiceCt->msgHandle, &msgSend);
}
#endif
/***********************************************************************************
 * 
 **********************************************************************************/
uint8_t GetBtStackCt(void)
{
	if(btStackServiceCt)
		return 1;
	else
		return 0;
}

/***********************************************************************************
 * 快速开关蓝牙
 * 断开蓝牙连接，蓝牙进入不可被搜索，不可被连接状态
 **********************************************************************************/
//#if (defined(BT_FAST_POWER_ON_OFF_FUNC)&&defined(CFG_BT_BACKGROUND_RUN_EN))
void BtScanPageStateSet(BT_SCAN_PAGE_STATE state)
{
	btManager.btScanPageState = state;
}

BT_SCAN_PAGE_STATE BtScanPageStateGet(void)
{
	return btManager.btScanPageState;
}

void BtFastPowerOff(void)
{
#ifdef BT_SNIFF_ENABLE
#ifndef BT_TWS_SUPPORT
	if(!Bt_sniff_fastpower_get())
#endif
#endif
	{
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_CLOSING);
	}
#ifdef BT_SNIFF_ENABLE
#ifndef BT_TWS_SUPPORT
	else
	{
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_SNIFF);
	}
#endif
#endif
}

void BtFastPowerOn(void)
{
#ifdef BT_SNIFF_ENABLE
#ifndef BT_TWS_SUPPORT
	if(!Bt_sniff_fastpower_get())
#endif
#endif
	{
		if(btStackServiceCt->serviceWaitResume)
		{
			#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
			if(!btManager.btLinkState)
			{
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			}
			#endif
			btStackServiceCt->serviceWaitResume = 0;
			return;
		}
        #ifdef BT_TWS_SUPPORT
		if(btStackServiceCt->btExitSniffReconPhone)
		{
			BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPEN_WAITING);
		}
		else
		{
			if(GetBtStackState() == BT_STACK_STATE_READY)
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
			else
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_EXIT_DEEPSLEEP_WAITING);
		}
		#else
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
		#endif
	}
#ifdef BT_SNIFF_ENABLE
#ifndef BT_TWS_SUPPORT
	else
	{
		if(Bt_sniff_fastpower_get())
		{
			Bt_sniff_fastpower_en();
		}
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_ENABLE);
	}
#endif
#endif
}

void BtScanPageStateCheck(void)
{
	static uint32_t bt_disconnect_count = 0;
	switch(btManager.btScanPageState)
	{
		case BT_SCAN_PAGE_STATE_CLOSING:
			// If there is a reconnectiong process, stop it
			if(btManager.btReconnectTimer.timerFlag)
			{
				BtStopReconnect();
			}

			#if (defined(BT_TWS_SUPPORT) && (CFG_TWS_ONLY_IN_BT_MODE == ENABLE))
			if(btManager.btTwsReconnectTimer.timerFlag)
			{
				BtStopReconnectTws();
			}
			BtTwsDeviceDisconnectExt();
			#endif
			
#ifdef BT_FAST_POWER_ON_OFF_FUNC
			{
				//加快断连流程
				extern uint8_t GetLmPageState(uint8_t* addr);
				uint8_t gBtLmPageState = GetLmPageState(btManager.btDdbLastAddr);
				if(gBtLmPageState == 1)
				{
					//paging中,取消连接流程
					printf("-------- paging..., cancel\n");
					BTHciCreateConnectionCancel(btManager.btDdbLastAddr);
				}
				else if((gBtLmPageState>=3)&&(GetBtCurConnectFlag() == 0))
				{
					//acl链路已经创建,未连上profile,直接断开acl连接
					printf("acl is connected, disconnect...\n");
					BTHciDisconnectCmd(btManager.btDdbLastAddr);
					BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISCONNECTING);
					break;
				}
			}
			// If there is a bt link, disconnect it
			if(GetBtCurConnectFlag())
			{
				BtDisconnectCtrl();
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISCONNECTING);
				break;
			}
#endif
			//不可被搜索不可被连接
			#if defined (BT_TWS_SUPPORT)&&(CFG_TWS_ONLY_IN_BT_MODE == DISABLE)
			if((GetBtManager()->twsFlag) && (GetBtManager()->twsState == BT_TWS_STATE_NONE))
			{
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			}
			else
			#endif
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
			BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISABLE);
			break;
			
		case BT_SCAN_PAGE_STATE_DISCONNECTING:
			if(bt_disconnect_count > 200)	// wait about 200ms
			{
				if(GetBtDeviceConnState() != BT_DEVICE_CONNECTION_MODE_NONE)
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
					BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISABLE);
				}
				else
				{
					if(GetBtCurConnectFlag())
					{
						BtDisconnectCtrl();
						BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISCONNECTING);
					}
					else
					{
						#if (CFG_TWS_ONLY_IN_BT_MODE == FALSE)
						BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISABLE);
						#endif
					}
				}
				bt_disconnect_count = 0;
			}
			else
				bt_disconnect_count++;
			break;
			
		case BT_SCAN_PAGE_STATE_DISABLE:
#ifdef BT_FAST_POWER_ON_OFF_FUNC
			// double check wether there is a bt link, if any, disconnect again
			if(GetBtCurConnectFlag())
			{
				BtDisconnectCtrl();
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISCONNECTING);
			}
#endif
			break;

		case BT_SCAN_PAGE_STATE_OPENING:
			BtScanPageStateSet(BT_SCAN_PAGE_STATE_ENABLE);
			btManager.BtPowerOnFlag = 0;
#ifdef BT_TWS_SUPPORT
			btManager.TwsPowerOnFlag = 0;
#endif
			//BtReconnectDevice();
#if ((CFG_TWS_ONLY_IN_BT_MODE == ENABLE) || defined(TWS_SLAVE_MODE_SWITCH_EN))
		#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
			if(GetBtManager()->twsFlag)
			{
				GetBtManager()->btConStateProtectCnt = 1;
				if(GetBtManager()->twsRole == BT_TWS_SLAVE)
				{
					//BtReconnectTws();
					BtReconnectTws_Slave();
					BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
					
					if((GetBtManager()->btDdbLastAddr[0])
							||(GetBtManager()->btDdbLastAddr[1])
							||(GetBtManager()->btDdbLastAddr[2])
							||(GetBtManager()->btDdbLastAddr[3])
							||(GetBtManager()->btDdbLastAddr[4])
							||(GetBtManager()->btDdbLastAddr[5]))
					{
						BtWaitingForReconDevice();
					}
					break;
				}
				else
				{
					if(!btManager.btReconnectType)
					{
						BtReconnectTws();
						if((GetBtManager()->btDdbLastAddr[0])
							||(GetBtManager()->btDdbLastAddr[1])
							||(GetBtManager()->btDdbLastAddr[2])
							||(GetBtManager()->btDdbLastAddr[3])
							||(GetBtManager()->btDdbLastAddr[4])
							||(GetBtManager()->btDdbLastAddr[5]))
						{
							BtWaitingForReconDevice();
						}
					}
				}
			}
			else
			{
				BtReconnectDevice();
			}
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			break;
		#endif
#else
		#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE))
			//BtReconnectDevice();
			GetBtManager()->btReconnectDelayCount = 1;
		#elif (TWS_PAIRING_MODE != CFG_TWS_ROLE_SLAVE)
			if(GetBtManager()->twsFlag)
			{
				if(GetBtManager()->twsRole == BT_TWS_SLAVE)
				{
			#ifndef CFG_AUTO_ENTER_TWS_SLAVE_MODE
					//BtReconnectTws();
					BtReconnectTws_Slave();
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
					break;
			#endif

					if(GetCurTotaBtRecNum())
					{
						if(btManager.btReconnectType == RECONNECT_TWS)
							BtWaitingForReconDevice();
						else
							BtReconnectDevice();
					}
				}
				else
				{
					if(btManager.btReconnectType == RECONNECT_TWS)
					{
						BtWaitingForReconDevice();
					}
					else
					{
						if(!btManager.btReconnectType)
						{
							if((GetBtManager()->btDdbLastAddr[0])
								||(GetBtManager()->btDdbLastAddr[1])
								||(GetBtManager()->btDdbLastAddr[2])
								||(GetBtManager()->btDdbLastAddr[3])
								||(GetBtManager()->btDdbLastAddr[4])
								||(GetBtManager()->btDdbLastAddr[5]))
							{
								BtReconnectDevice();
								BtWaitingForReconTws();
							}
							else
							{
								BtReconnectTws();
							}
						}
					}
				}
			}
			else
			{
				BtReconnectDevice();
			}
		#endif
#endif
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			break;
			
		case BT_SCAN_PAGE_STATE_ENABLE:
			break;

#ifdef BT_TWS_SUPPORT
		case BT_SCAN_PAGE_STATE_OPEN_WAITING:
			//sleep唤醒后延时回连手机
			if(btStackServiceCt->btExitSniffReconPhone == 0)
			{
				APP_DBG("BT_SCAN_PAGE_STATE_OPEN_WAITING_END\n");
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
			}
			break;

		case BT_SCAN_PAGE_STATE_EXIT_DEEPSLEEP_WAITING:
			if(GetBtStackState() == BT_STACK_STATE_READY)
			{
				//APP_DBG("BT_SCAN_PAGE_STATE_EXIT_DEEPSLEEP_WAITING_END\n");
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
			}
			break;
#endif
			
		default:
			break;
	}
}
//#endif

/***********************************************************************************
 * 蓝牙恢复出厂设置
 **********************************************************************************/
extern void BTHciCreateConnectionCancel(uint8_t *addr);
static void BtRstStateCheck(void)
{
	switch(btManager.btRstState)
	{
		case BT_RST_STATE_NONE:
			break;
			
		case BT_RST_STATE_START:
			APP_DBG("bt reset start\n");
			// If there is a reconnectiong process, stop it
			if(btManager.btReconnectTimer.timerFlag)
			{
				BtStopReconnect();
			}
			BTHciCreateConnectionCancel(btManager.btDdbLastAddr);//打断正在回连的流程

			// If there is a bt link, disconnect it
			if(GetBtCurConnectFlag())
			{
				BtDisconnectCtrl();
			}

			btManager.btRstState = BT_RST_STATE_WAITING;
			btManager.btRstWaitingCount = 0;
			break;
			
		case BT_RST_STATE_WAITING:
			if(!btManager.btLinkState)
			{
				btManager.btRstState = BT_RST_STATE_FINISHED;
			}
			else if(btManager.btRstWaitingCount>=3000)
			{
				btManager.btRstWaitingCount = 2000;
				//if(GetBtConnectedProfile())
				if(GetBtManager()->btConnectedProfile)
				{
					BtDisconnectCtrl();
				}
				else if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_ALL)
				{
					//BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
					btManager.btRstState = BT_RST_STATE_FINISHED;
				}
			}
			else
			{
				btManager.btRstWaitingCount++;
				/*if((!GetBtCurConnectFlag())&&(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_ALL))
				{
					btManager.btRstState = BT_RST_STATE_FINISHED;
				}*/
			}
			break;
			
		case BT_RST_STATE_FINISHED:
			btManager.btRstWaitingCount = 0;
			memset(btManager.remoteAddr, 0, 6);
			memset(btManager.btDdbLastAddr, 0, 6);
			
			BtDdb_Erase();
			
			btManager.btRstState = BT_RST_STATE_NONE;
			APP_DBG("bt reset complete\n");
			break;
			
		default:
			btManager.btRstState = BT_RST_STATE_NONE;
			break;
	}
}

/***********************************************************************************
 * 开关蓝牙
 * 断开蓝牙连接，删除蓝牙协议栈任务，关闭蓝牙晶振
 **********************************************************************************/
void BtPowerOff(void)
{
	uint8_t btDisconnectTimeout = 0;
	if(!btStackServiceCt)
		return;
	
	APP_DBG("[Func]:Bt off\n");
	
	if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
	{
		while(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			vTaskDelay(10);
			btDisconnectTimeout++;
			if(btDisconnectTimeout>=100)
				break;
		}

		//快速在BT模式和其他模式(共2个模式)切换，需要delay(500);避免蓝牙初始化和反初始化状态未完成导致的错误
		//vTaskDelay(500);
		vTaskDelay(50);
	}
	
	if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
	{
		BtDisconnectCtrl();
	}

	//在蓝牙回连时,需要先取消蓝牙回连行为
	if(GetBtManager()->btReconnectTimer.timerFlag)
	{
		BtStopReconnect();
		vTaskDelay(50);
	}

	//wait for bt disconnect, 2S timeout
	while(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
	{
		vTaskDelay(10);
		btDisconnectTimeout++;
		if(btDisconnectTimeout>=200)
			break;
	}
	
	//bb reset
	rwip_reset();
	BT_IntDisable();
	//Kill bt stack service
	BtStackServiceKill();
	vTaskDelay(10);
	//reset bt module and close bt clock
	BT_ModuleClose();
}

void BtPowerOn(void)
{
	APP_DBG("[Func]:Bt on\n");
	vTaskDelay(50);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	vTaskDelay(50);

	//bt stack restart
	BtStackServiceStart();
}

bool BtReconnectStartIsReady(void)
{
	if(GetSystemMode()>AppModeWaitingPlay)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


#ifdef BT_TWS_SUPPORT
/***********************************************************************************
 * BLE status config
 **********************************************************************************/
void BleAdvScanEnable(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BLE_ADV_SCAN;
	MessageSend(btStackServiceCt->msgHandle, &msgSend);
}

void BleOnlyAdv(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BLE_ONLY_ADV;
	MessageSend(btStackServiceCt->msgHandle, &msgSend);
}

void BleOnlyScan(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BLE_ONLY_SCAN;
	MessageSend(btStackServiceCt->msgHandle, &msgSend);
}

void BleAdvScanDisable(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BLE_DISABLE_ADV_SCAN;
	MessageSend(btStackServiceCt->msgHandle, &msgSend);
}

void BleAdvDataResume(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BLE_ADV_DATA_DELAY_RESUME;
	MessageSend(btStackServiceCt->msgHandle, &msgSend);
}
#endif
/***********************************************************************************
 * 进入蓝牙DUT模式
 **********************************************************************************/
void BtEnterDutModeFunc(void)
{
	uint8_t btDisconnectTimeout = 0;
	if(!GetBtStackCt())
	{
		APP_DBG("Enter dut mode fail\n");
		return;
	}

	if(!btManager.btDutModeEnable)
	{
		btManager.btDutModeEnable = 1;
		
		if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
		{
			BtDisconnectCtrl();
		}
		if(GetBtManager()->btReconnectTimer.timerFlag)
		{
			BtStopReconnect();
		}

#ifdef BT_TWS_SUPPORT
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			tws_link_disconnect();
		}
		if(GetBtManager()->btTwsReconnectTimer.timerFlag)
		{
			BtReconnectTws();
		}
#endif
		
		APP_DBG("confirm bt disconnect\n");
		while(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
		{
			//2s timeout
			vTaskDelay(100);
			btDisconnectTimeout++;
			if(btDisconnectTimeout>=20)
				break;
		}

		APP_DBG("clear all pairing list\n");
		BtDdb_Erase();
		
		APP_DBG("[Enter dut mode]\n");
		BTEnterDutMode();
	}
}

/***********************************************************************************
 * 蓝牙名称的更新
 **********************************************************************************/
void BtLocalDeviceNameUpdate(uint8_t *deviceName)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	if(btStackServiceCt == NULL)
		return;
	
	memset(btStackConfigParams->bt_LocalDeviceName, 0, BT_NAME_SIZE);
	memcpy(btStackConfigParams->bt_LocalDeviceName, deviceName, (strlen((const char *)(deviceName))));
	
	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_LOCAL_DEVICE_NAME_UPDATE;

	MessageSend(msgHandle, &msgSend);
}

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)//tws_auto_pair
/***********************************************************************************
 * 广播的蓝牙名称的更新
 **********************************************************************************/
void BtBroadcastNameUpdate(void)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	if(btStackServiceCt == NULL)
		return;

	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_BROADCAST_NAME_UPDATE;
	MessageSend(msgHandle, &msgSend);
}
#endif
/***********************************************************************************
 * 
 **********************************************************************************/

#else

void WakeupBtStackService(void)
{
}

void BBErrorReport(void)
{
}

void BBIsrReport(void)
{
}

void BtGetInfo(uint8_t *bt_info,uint8_t type)
{
}
#endif

#ifdef BT_TWS_SUPPORT
void tws_slave_ready_callback(uint32_t twsRole)
{
#ifdef BT_SNIFF_ENABLE
	switch(twsRole)
	{

	case 1://s
		{
			uint8_t flag = BtSniffADDAReadyGet();
			BtSniffADDAReadySet(flag | (1<<1));
		}
		break;
	case 0://m
		{
			uint8_t flag = BtSniffADDAReadyGet();
			BtSniffADDAReadySet(flag | (1<<1));
		}
		break;
	}
#endif//BT_SNIFF_ENABLE
}

void tws_master_ready_callback(uint32_t twsRole)
{
#ifdef BT_SNIFF_ENABLE
	switch(twsRole)
	{

	case 1://s
		{
			uint8_t flag = BtSniffADDAReadyGet();
			BtSniffADDAReadySet(flag | (1));
		}
		break;
	case 0://m
		{
			uint8_t flag = BtSniffADDAReadyGet();
			BtSniffADDAReadySet(flag | (1));
		}
		break;

	}
#endif//BT_SNIFF_ENABLE
}
#endif
#ifdef BT_SNIFF_ENABLE

typedef enum
{
	SNIFF_EXIT,
	SNIFF_READY,
	SNIFF_ENTER
}_SNIFF_STATE_t;

_SNIFF_STATE_t	sniff_state = SNIFF_EXIT;

void SniffStateSet(_SNIFF_STATE_t state)
{
	sniff_state = state;
}

_SNIFF_STATE_t SniffStateGet()
{
	return sniff_state;
}

#ifndef BT_TWS_SUPPORT
//Note:蓝牙的callback是协议栈直接回调函数，客户不能在callback中进行太多处理;
//如有需要，则将callback中的event和params通过msg发送到userService中统一进行处理;
MessageHandle GetBtUserServiceMsgHandle(void)
{
	return btUserServiceCt->msgHandle;
}

TaskState GetBtUserServiceState(void)
{
	return btUserServiceCt->serviceState;
}
#endif

//发送蓝牙进入deepsleep的消息，btlib会使用
void SendDeepSleepMsg(void)
{
#ifdef BT_TWS_SUPPORT
	if(Bt_sniff_sniff_start_state_get())
	{
		Bt_sniff_sleep_enter();
	}
#else
	MessageContext		msgSend;

	if(SniffStateGet() == SNIFF_EXIT)
		msgSend.msgId = MSG_BTSTACK_SNIFF_STANDBY;
	else if(SniffStateGet()==SNIFF_ENTER)
	{
		msgSend.msgId = MSG_BTSTACK_SNIFF_ENTER;
	}

	if(GetA2dpState() >= BT_A2DP_STATE_CONNECTED)
	{
		Bt_sniff_sleep_enter();
		MessageSend(btUserServiceCt->msgHandle, &msgSend);
	}
#endif
}

void SysDeepsleepStandbyStatus(void)
{
	MessageContext		msgSend;
#ifdef BT_TWS_SUPPORT
	if(sniff_wakeup_get() == 0)
	{
		sniff_wakeup_set(1);//设置为进入sniff模式
		msgSend.msgId		= MSG_BTSTACK_DEEPSLEEP;
		MessageSend(mainAppCt.msgHandle, &msgSend);
	}
#else
	if(GetA2dpState() >= BT_A2DP_STATE_CONNECTED)
	{
		msgSend.msgId		= MSG_BTSTACK_DEEPSLEEP;
		MessageSend(mainAppCt.msgHandle, &msgSend);
	}
#endif
}

void SysDeepsleepStart(void)
{
#ifndef BT_TWS_SUPPORT
	MessageContext		msgSend;

	msgSend.msgId		= MSG_BTSTACK_SNIFF_ENTER;
	SniffStateSet(SNIFF_ENTER);
	MessageSend(btUserServiceCt->msgHandle, &msgSend);
#endif
}

void SysDeepsleepStop(void)
{
#ifndef BT_TWS_SUPPORT
	MessageContext		msgSend;

	msgSend.msgId		= MSG_BTSTACK_SNIFF_EXIT;
	MessageSend(btUserServiceCt->msgHandle, &msgSend);
#endif
}

//改变PLL 频率
void Bt_sniff_change_pllclk()
{
	extern void LogUartConfig(bool InitBandRate);
	extern BTSNIFF_GET_DEFAULT_CLKCONFIG_t sniff_get_clkconfig;

	GIE_DISABLE();
	SysTickDeInit();
	if(sniff_get_clkconfig.get1V2>1100)
		Power_LDO12Config(sniff_get_clkconfig.get1V2);
	else
		Power_LDO12Config(1250);
	Clock_SysClkDivSet(sniff_get_clkconfig.sys_div);
	Clock_CoreClkDivSet(sniff_get_clkconfig.core_div);
	Clock_PllLock(sniff_get_clkconfig.sys_clk/1000);
	Clock_DeepSleepSysClkSelect(PLL_CLK_MODE,FSHC_PLL_CLK_MODE,FALSE);
	SysTickInitSet();

	Clock_UARTClkSelect(PLL_CLK_MODE);//先切换log clk。避免后续慢速处理
	LogUartConfig(TRUE); //scan不打印时 可屏蔽
	GIE_ENABLE();
	APP_DBG("clk:%d,%d,%d,%d\r\n",sniff_get_clkconfig.sys_div,sniff_get_clkconfig.core_div
			,sniff_get_clkconfig.sys_clk,sniff_get_clkconfig.get1V2);
}

#ifdef BT_TWS_SUPPORT
void send_sniff_msg()
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BT_SNIFF;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

void DisconnectFromPhone()
{
	if((GetA2dpState() < BT_A2DP_STATE_CONNECTED)||(GetHfpState() < BT_HFP_STATE_CONNECTED))//已经断开
		return;

	if(GetA2dpState()>BT_A2DP_STATE_NONE)
	{
		A2dpDisconnect();//主机连接手机的话就先断开
	}
	if(GetHfpState() >= BT_HFP_STATE_CONNECTED)
	{
		BtHfpDisconnect();//主机连接手机的话就先断开
	}

	while((GetA2dpState() >= BT_A2DP_STATE_CONNECTED)||(GetHfpState() >= BT_HFP_STATE_CONNECTED))//等待断开成功
	{
		vTaskDelay(2);
	}


	send_sniff_msg();


//	BTSetRemDevIntoSniffMode(GetBtManager()->btTwsDeviceAddr);//断开退出sniff，然后再进sniff
}



void BTWakeupIOSet()
{
	extern uint8_t sniffiocnt;

	sniffiocnt = 0;//蓝牙按键唤醒计数。
	GPIO_PortAModeSet(GPIOA23,0);
	GPIO_RegOneBitSet(GPIO_A_IE,GPIOA23);
	GPIO_RegOneBitClear(GPIO_A_OE,GPIOA23);
	GPIO_RegOneBitSet(GPIO_A_PU,GPIOA23);
	GPIO_RegOneBitClear(GPIO_A_PD,GPIOA23);
}

void BTSniffSet()
{
	if((Bt_sniff_sniff_start_state_get()))//&&(Bt_sniff_sleep_state_get()))
	{
		Bt_sniff_sleep_exit();

		DisconnectFromPhone();//主机连接手机的话就先断开

//		BTWakeupIOSet();//设置唤醒源。
	}
}

void BtSniffExit_process(void)
{
	if(tws_get_role() == BT_TWS_MASTER)
		BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);

//	BTSetRemDevExitSniffMode(GetBtManager()->btTwsDeviceAddr);
	if(GetBtManager()->twsState > BT_TWS_STATE_NONE)
		BTSetRemDevExitSniffMode(GetBtManager()->btTwsDeviceAddr);
	Bt_sniff_sniff_stop();

}

uint8_t sniffaddaready_f = 0;//bit1:master ok!
							 //bit2:slaver ok!
void BtSniffADDAReadySet(uint8_t set)
{
	sniffaddaready_f = set;
}

uint8_t BtSniffADDAReadyGet()
{
	return sniffaddaready_f;
}

void BtStartEnterSniffMode(void)
{
	//取消Master回连手机功能
	if(btManager.btReconnectTimer.timerFlag)
	{
		BtStopReconnect();
	}
	
	/*if((GetA2dpState() >= BT_A2DP_STATE_CONNECTED) 
				|| (GetHfpState() >= BT_HFP_STATE_CONNECTED) 
				|| (GetAvrcpState() >= BT_AVRCP_STATE_CONNECTED))
				*/
	if(GetBtCurConnectFlag())
	{
		//手动断开
		BtDisconnectCtrl();

		btStackServiceCt->btEnterSniffStep = 1;
	}
	else
	{
		if(tws_get_role() == BT_TWS_MASTER)
		{
			if(Bt_sniff_sniff_start_state_get() == 0)
			{
				APP_DBG("remote device enter sniff mode.\n");
				//DisconnectFromPhone();
				BTSetRemDevIntoSniffMode(GetBtManager()->btTwsDeviceAddr);
			}
			else
			{
				APP_DBG("remote device exit sniff mode.\n");
				//DisconnectFromPhone();//在mode为0时，还能进入此处说明是手机与板子进入了sniffmode
									  //这里直接断开连接。

			}
		}
	}
}

uint32_t btEnterSniffCnt = 0;
void BtStartEnterSniffStep(void)
{
	if(btStackServiceCt->btEnterSniffStep)
	{
		if((btManager.btLinkState == 0)&&(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_ALL))
		{
			BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			btStackServiceCt->btExitSniffReconPhone=0;
			btStackServiceCt->btEnterSniffStep = 0;
			if(tws_get_role() == BT_TWS_MASTER)
			{
				if(Bt_sniff_sniff_start_state_get() == 0)
				{
					APP_DBG("remote device enter sniff mode.\n");
					BTSetRemDevIntoSniffMode(GetBtManager()->btTwsDeviceAddr);
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
				else
				{
					APP_DBG("remote device exit sniff mode.\n");
					//DisconnectFromPhone();//在mode为0时，还能进入此处说明是手机与板子进入了sniffmode
										  //这里直接断开连接。

				}
			}
		}
		else
		{
			btEnterSniffCnt++;
			if(btEnterSniffCnt>=250)
			{
				btEnterSniffCnt=0;
				if((GetA2dpState() >= BT_A2DP_STATE_CONNECTED) 
				|| (GetHfpState() >= BT_HFP_STATE_CONNECTED) 
				|| (GetAvrcpState() >= BT_AVRCP_STATE_CONNECTED))
				{
					BtDisconnectCtrl();
				}
			}
		}
	}
}

void BtExitSniffReconnectFlagSet(void)
{
	btStackServiceCt->btExitSniffReconPhone = 1;
}

void BtExitSniffReconnectPhone(void)
{
	if(btStackServiceCt->btExitSniffReconPhone)
	{
		btStackServiceCt->btExitSniffReconPhone++;
		//if(btStackServiceCt->btExitSniffReconPhone>=250)
		if(btStackServiceCt->btExitSniffReconPhone>=500)
		{
			btStackServiceCt->btExitSniffReconPhone=0;
			
		#ifdef BT_FAST_POWER_ON_OFF_FUNC
			
		#else
			BtConnectCtrl();
		#endif
		}
	}
}
#else

static void BtUserServiceEntrance(void * param)
{
	MessageContext		msgRecv;

	Bt_sniff_state_init();
	SysDeepsleepStop();
	Clock_RcFreqCntOneTimeStart();
//	APP_DBG("______Bt User Service Start\n");
	while(1)
	{
		MessageRecv(btUserServiceCt->msgHandle, &msgRecv, 0xffffffff);

		switch(msgRecv.msgId)
		{
			case MSG_BTSTACK_SNIFF_STANDBY:

				if((Bt_sniff_sniff_start_state_get())&&(Bt_sniff_sleep_state_get()))
				{
//					printf("____MSG_BTSTACK_SNIFF_STANDBY\r\n");
					Bt_sniff_sleep_exit();
					SysDeepsleepStandbyStatus();
					SniffStateSet(SNIFF_READY);
				}
				break;

			case MSG_BTSTACK_SNIFF_ENTER:

				if (Bt_sniff_sniff_start_state_get())
				{
					if(Bt_sniff_sleep_state_get())
					{
						Bt_sniff_sleep_exit();
						BtDeepSleepForUsr();
//						printf("____MSG_BTSTACK_SNIFF_ENTER\r\n");
					}
				}

				break;
			case MSG_BTSTACK_SNIFF_EXIT:
//				printf("____MSG_BTSTACK_SNIFF_EXIT\r\n");
				SniffStateSet(SNIFF_EXIT);

				break;
			case MSG_NONE:

				break;

		}
	}
}
void BtStartEnterSniffMode(void)
{
}
#endif
#else
void SendDeepSleepMsg(void)
{
}

void BtStartEnterSniffMode(void)
{
}

void BtStartEnterSniffStep(void)
{
}

#endif	//BT_SNIFF_ENABLE

