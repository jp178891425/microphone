/**
 **************************************************************************************
 * @file    bt_manager.c
 * @brief   management of all bluetooth event and apis
 *
 * @author  Halley
 * @version V1.1.0
 *
 * $Created: 2016-07-18 16:24:11$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "type.h"
#include "bt_manager.h"
#include "bt_ddb_flash.h"
#ifdef CFG_APP_CONFIG
#include "bt_stack_api.h"
#include "app_config.h"
#include "main_task.h"
#endif
#include "debug.h"
#include "rtos_api.h"
#include "bt_app_func.h"
#include "bt_ddb_flash.h"
#include "app_message.h"
#include "bt_app_interface.h"
#include "bt_stack_service.h"
#ifdef BT_TWS_SUPPORT
#include "mode_switch_api.h"
extern uint32_t gBtTwsSniffLinkLoss;
#include "bt_tws_app_func.h"
#include "ble_app_func.h"
#endif

BT_MANAGER_ST		btManager;
uint32_t BtAllProfileConnectState(void);
extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;

BT_CHECK_EVENT_LIST btCheckEventList = BT_EVENT_NONE;
uint32_t btEventListB0Count = 0; //BT_EVENT_AVRCP_DISCONNECT
uint32_t btEventListB1Count = 0; //BT_EVENT_L2CAP_LINK_DISCONNECT
uint32_t btEventListB1State = 0;
uint32_t btEventListB2Count = 0; //BT_EVENT_AVRCP_CONNECT
uint32_t btEventListCount = 0;

uint32_t btReConProtectCnt; //开机蓝牙回连和主动连接冲突时,保护时间5s;对方远端设备先发起连接,5s内不能发起回连
uint32_t gBtReconBusyCnt = 0;//busy检测冲突次数维护寄存器

/***********************************************************************************
 * 配置可见性函数接口
 **********************************************************************************/
/*static void BtSetAccessModeApi(BtAccessMode accessMode)
{
	if(GetBtDeviceConnState() != accessMode)
	{
		BTSetAccessMode(accessMode);
	}
}
*/
void BtSetAccessModeMsg(BtAccessMode accessMode, uint16_t delay_time)
{
	btManager.btAccessModeSt.accessMode = accessMode;
	if(delay_time == 0)
	{
		btManager.btAccessModeSt.delayTime = 1;
	}
	else
	{
		btManager.btAccessModeSt.delayTime = delay_time;
	}
	APP_DBG("Set AccessMode: %x, delay:%d\n", btManager.btAccessModeSt.accessMode, btManager.btAccessModeSt.delayTime);

	//配置的状态和现状一致时,则清除
	if(btManager.deviceConMode == accessMode)
	{
		btManager.btAccessModeSt.delayTime = 0;
	}
}

/***********************************************************************************
 * 
 **********************************************************************************/
#ifdef BT_TWS_SUPPORT
extern uint32_t gBtEnterDeepSleepFlag;
uint32_t SReconnectTwsFlag=0;//slave回连master标志

bool BtReconProcessCheck(void);
bool BtTwsReconProcessCheck(void);
extern bool tws_connect_cmp(uint8_t * addr);

/***********************************************************************************
 * 确认连接条件
 * return: 
 * 0=reject
 * 1=master
 * 2=slave
 **********************************************************************************/
#define BT_CON_REJECT		0
#define BT_CON_MASTER		1
#define BT_CON_SLAVE		2
#define BT_CON_UNKNOW		3

uint8_t *get_cur_tws_addr(void)
{
	return &btManager.btTwsDeviceAddr[0];
}

uint32_t confirm_connecting_condition(uint8_t *addr)
{
	extern bool is_tws_device(uint8_t *remote_addr);

#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)||(TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER))
	if(btManager.btLastAddrUpgradeIgnored || GetHfpState() > BT_HFP_STATE_CONNECTED)
	{
		APP_DBG("****** mv test box is connected ***\n");
		BtSetAccessModeMsg(BtAccessModeNotAccessible, 1);
		return BT_CON_REJECT; //1. 测试盒连接上设备后,不允许其他设备再次连接进行测试 2. 主机在进行来电响铃或通话，拒绝其他设备连接
	}

	if(is_tws_device(addr))
	{
		if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
		{
			APP_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}

		if((btManager.btLinkState == 1)||(btManager.a2dpState >= BT_A2DP_STATE_CONNECTED)||(btManager.avrcpState >= BT_AVRCP_STATE_CONNECTED))
		{
			//BT_DBG("phone connected, tws Role:Master\n");
			//return BT_CON_MASTER;
			if((memcmp(addr, btManager.btTwsDeviceAddr, 6) == 0)&&(btManager.twsFlag))
			{
				if(GetBtManager()->twsRole == BT_TWS_MASTER)
				{
					BT_DBG("phone connected, tws Role:Master\n");
					return BT_CON_MASTER;
				}
				else
				{
					BT_DBG("role not match, reject\n");
					return BT_CON_REJECT;
				}
			}
			else
			{
			#if (TWS_PAIRING_MODE != CFG_TWS_ROLE_RANDOM)//tws_auto_pair
				if(btManager.twsEnterPairingFlag)
				{
					BT_DBG("phone connected, role:master\n");
					return BT_CON_MASTER;
				}
				else
				{
					BT_DBG("phone connected, reject\n");
					return BT_CON_REJECT;
				}
			#else
				BT_DBG("phone connected, tws Role:Master\n");
				return BT_CON_MASTER;
			#endif
			}
		}

		if((GetBtManager()->twsFlag)&&(GetBtManager()->twsRole == BT_TWS_MASTER))
		{
		#ifdef BT_TWS_FUNCTION_KEY_SWITCH			
			if(!btManager.TwsFunctionEnable)
				return BT_CON_REJECT;
		#endif	
			if(memcmp(addr, GetBtManager()->btTwsDeviceAddr, 6) == 0)
			{
				/*if(btManager.btTwsReconnectTimer.timerFlag)
				{
					//主机正在回连从机,避免出错,拒绝从机的连接
					APP_DBG("reject, waiting for master connect slave\n");
					return 0;
				}*/
				APP_DBG("BT_TWS_MASTER: tws device connecting, Cur_Role:Master\n");
				return BT_CON_MASTER;
			}
			else if(btManager.twsEnterPairingFlag)
			{
				APP_DBG("BT_TWS_MASTER: pairing:  tws device connecting, Cur_Role:Master\n");
				return BT_CON_MASTER;
			}
			else
			{
				APP_DBG("BT_TWS_MASTER: tws device connecting, Cur_Role:Slave\n");
				return BT_CON_SLAVE;
			}
		}
		else if((GetBtManager()->twsFlag)&&(GetBtManager()->twsRole == BT_TWS_SLAVE))
		{
		#ifdef BT_TWS_FUNCTION_KEY_SWITCH
			if(!btManager.TwsFunctionEnable)
				return BT_CON_REJECT;
		#endif	
			//APP_DBG("BT_TWS_SLAVE: tws device connecting, Cur_Role:Slave\n");
			if(btManager.btTwsReconnectTimer.timerFlag)
			{
				APP_DBG("cancel phone reconnecting\n");
				BtStopReconnectTwsReg();
			}
			if(btManager.btTwsReconnectTimer.timerFlag)
			{
				BtStopReconnectTws();
			}

			if(btManager.twsEnterPairingFlag)
			{
				APP_DBG("PAIRING: tws device connecting, Cur_Role:Master\n");
				return BT_CON_MASTER;
			}
			else
			{
				APP_DBG("BT_TWS_SLAVE: tws device connecting, Cur_Role:Slave\n");
				return BT_CON_SLAVE;
			}
		}
		else
		{
			//APP_DBG("BT_TWS_SLAVE: tws device connecting, Cur_Role:unknow\n");
			//return BT_CON_UNKNOW;
			APP_DBG("PAIRING: tws device connecting, Cur_Role:Master\n");
			return BT_CON_MASTER;
		}
	}
	else
	{
		
#ifdef BT_FAST_POWER_ON_OFF_FUNC
		if(!IsBtAudioMode())
		{
			return BT_CON_REJECT;
		}
		else
#endif
		if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
		{
			return BT_CON_REJECT;
		}
		
		return BT_CON_MASTER;
	}
#elif(TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if(is_tws_device(addr))
	{
		if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
		{
			APP_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}
		else
		{
			if(btManager.btTwsReconnectTimer.timerFlag)
			{
				//主机正在回连从机,避免出错,拒绝从机的连接
				//APP_DBG("Soundbar Master: reject, waiting for master connect slave\n");
				//return 0;
				if(btManager.twsRole == BT_TWS_MASTER)
					return BT_CON_MASTER;
				else 
					return BT_CON_SLAVE;
			}
			APP_DBG("tws device connecting\n");
			return BT_CON_MASTER;
		}
	}
	
#ifdef BT_FAST_POWER_ON_OFF_FUNC
#ifdef CFG_APP_CONFIG
	if(!IsBtAudioMode())
	{
		return BT_CON_REJECT;
	}
	else
#endif
	{
		if((btManager.btLinkState == 1)
			||(GetA2dpState() > BT_A2DP_STATE_NONE)
			||(GetHfpState() > BT_HFP_STATE_NONE)
			||(GetAvrcpState() > BT_AVRCP_STATE_NONE)
			)
			return BT_CON_REJECT;
		else 
			return BT_CON_MASTER;
	}
#else //BT_FAST_POWER_ON_OFF_FUNC
	if((btManager.btLinkState == 1)
		||(GetA2dpState() > BT_A2DP_STATE_NONE)
		||(GetHfpState() > BT_HFP_STATE_NONE)
		||(GetAvrcpState() > BT_AVRCP_STATE_NONE)
		)
		return BT_CON_REJECT;
	else 
		return BT_CON_MASTER;
#endif//BT_FAST_POWER_ON_OFF_FUNC

#elif(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(is_tws_device(addr))
	{
		if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
		{
			APP_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}
		else
		{
			APP_DBG("tws device connecting\n");
			return BT_CON_MASTER;
		}
	}
	else
	{
#ifdef BT_FAST_POWER_ON_OFF_FUNC
#ifdef CFG_APP_CONFIG
		if(!IsBtAudioMode())
		{
			return BT_CON_REJECT;
		}
		else
#endif
		{
			if((btManager.btLinkState == 1)
				||(GetA2dpState() > BT_A2DP_STATE_NONE)
				||(GetHfpState() > BT_HFP_STATE_NONE)
				||(GetAvrcpState() > BT_AVRCP_STATE_NONE)
				)
				return BT_CON_REJECT;
			else 
				return BT_CON_SLAVE;
		}
#else //BT_FAST_POWER_ON_OFF_FUNC
		if((btManager.btLinkState == 1)
			||(GetA2dpState() > BT_A2DP_STATE_NONE)
			||(GetHfpState() > BT_HFP_STATE_NONE)
			||(GetAvrcpState() > BT_AVRCP_STATE_NONE)
			)
			return BT_CON_REJECT;
		else 
			return BT_CON_SLAVE;
#endif //BT_FAST_POWER_ON_OFF_FUNC
	}

#else //(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	return BT_CON_SLAVE;
#endif
}


/***********************************************************************************
 * 
 **********************************************************************************/
uint8_t* app_bt_page_state(void)
{
	if(btManager.btReconnectType == RECONNECT_DEVICE)
	{
		btManager.btReconDelayFlag = 1;
		//BtStopReconnectReg();
		return btManager.btDdbLastAddr;
	}
	else if(btManager.btReconnectType == RECONNECT_TWS)
	{
		btManager.btReconDelayFlag = 1;
		//BtStopReconnectTwsReg();
		return btManager.btTwsDeviceAddr;
	}
	else
	{
		return NULL;
	}
}

/***********************************************************************************
 * 
 **********************************************************************************/
bool load_local_device_addr(uint8_t *addr)
{
	memcpy(addr, btManager.btDevAddr, 6);
	return 1;
}

bool load_tws_paired_addr(uint8_t *addr)
{
	memcpy(addr, btManager.btTwsDeviceAddr, 6);
	return 1;
}

void load_tws_filter_infor(uint8_t *infor)
{
	memcpy(infor, btManager.TwsFilterInfor, 6);
}
#endif //#ifdef BT_TWS_SUPPORT

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtLinkStateConnect(void)
{
#ifdef BT_PROFILE_BQB_ENABLE
	if(!btManager.btLinkState)
#else
	if((!btManager.btLinkState)
		&&(GetA2dpState() >= BT_A2DP_STATE_CONNECTED)
		&& (GetAvrcpState() >= BT_AVRCP_STATE_CONNECTED)
		)
#endif
	{
		btManager.btLinkState = 1;
		btManager.fristBtLinkState = 1;
		BtMidMessageSend(MSG_BT_MID_STATE_CONNECTED, 0);
#ifndef BT_TWS_SUPPORT
		btReConProtectCnt = 0;
#endif

		#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)&&defined(BT_TWS_SUPPORT))//tws_auto_pair
		app_bt_device_connect(1);
		#endif
		
		if(btManager.ddbUpdate)
		{
			btManager.ddbUpdate = 0;
			if(!btManager.btLastAddrUpgradeIgnored)
			{
				//save total device info to flash
				SaveTotalDevRec2Flash(1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM/*one total rec block size*/,
							 	GetCurTotaBtRecNum());
			}
		}

#ifdef BT_TWS_SUPPORT
		btManager.btReconDelayFlag = 0;
	#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)//tws_auto_pair
		if(btManager.twsState != BT_TWS_STATE_CONNECTED)
		{
			BTSetAccessMode(BtAccessModeGeneralAccessible);
		}
		else
		{
			BTSetAccessMode(BtAccessModeNotAccessible);
		}
		btManager.twsRole = BT_TWS_MASTER;//连上手机的设备做Master
	#else
		BtAccessModeUpdate(GetBtDeviceConnState());
	#endif
	
	#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
		if((btManager.twsFlag)&&(btManager.twsRole != BT_TWS_MASTER))
		{
			//BtDdb_ClearTwsDeviceAddrList();
		}

		if((btManager.twsState == BT_TWS_STATE_CONNECTED)||(btManager.twsRole == BT_TWS_SLAVE))
		{
			BtStopReconnectTwsReg();
		}
	#endif
#endif
	}

#ifdef BT_TWS_SUPPORT
	GetBtManager()->btReconnectDeviceFlag &= ~(RECONNECT_DEVICE);
#endif
}

void BtLinkStateDisconnect(void)
{
	if( btManager.btLinkState
		&& (GetAvrcpState() == BT_AVRCP_STATE_NONE)
		&& (GetA2dpState() == BT_A2DP_STATE_NONE)
		&& (GetHfpState() == BT_HFP_STATE_NONE)
	)
	{
		btManager.btLastAddrUpgradeIgnored = 0;
		btManager.btLinkState = 0;
		btManager.fristBtLinkState = 0;
		BtStopReconnect();

		BtMidMessageSend(MSG_BT_MID_STATE_DISCONNECT, 0);

#ifdef BT_TWS_SUPPORT		
		if(gBtEnterDeepSleepFlag == 0){
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
		}
		#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)//tws_auto_pair
		app_bt_device_connect(0);
		BtBroadcastNameUpdate();//20230519
		#endif
#endif

		//在L2CAP异常断开所有协议时，开始启动回连机制,30s超时
		if(btCheckEventList & BT_EVENT_L2CAP_LINK_DISCONNECT)
		{
			btEventListB1Count = btEventListCount;
			btEventListB1Count += 30000;

			btManager.btReconnectTryCount = 3;
			btManager.btReconnectIntervalTime = 2;
			btManager.btReconnectTimer.timerFlag = TIMER_USED;
			btManager.btReconnectedFlag = 0;
			//BtReconnectProfile();
			TimeOutSet(&btManager.btReconnectTimer.timerHandle, 1000);
			btManager.btReconnectTimer.timerFlag |= TIMER_STARTED;
		}
	}
}

bool GetBtLinkState(void)
{
	return btManager.btLinkState;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtConnectCtrl(void)
{
	APP_DBG("bt connect ctrl\n");
	
	if((GetBtManager()->btDdbLastAddr[0]==0)
		&&(GetBtManager()->btDdbLastAddr[1]==0)
		&&(GetBtManager()->btDdbLastAddr[2]==0)
		&&(GetBtManager()->btDdbLastAddr[3]==0)
		&&(GetBtManager()->btDdbLastAddr[4]==0)
		&&(GetBtManager()->btDdbLastAddr[5]==0))
	{
		APP_DBG("Last Addr is NULL\n");
		return ;
	}
	
	//reconnect remote device
	APP_DBG("Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			APP_DBG("%x ", GetBtManager()->btDdbLastAddr[i]);
		}
	}
	APP_DBG("\n");

	//开启回连行为
	BtStartReconnectDevice(1);
}

void BtDisconnectCtrl(void)
{
	APP_DBG("bt disconnect ctrl\n");

	//1.终止回连行为
	if(btManager.btReconnectTimer.timerFlag)
	{
		BtStopReconnect();
	}

	//2.断开已连接上的profile
	//BTDisconnect();
	BTHciDisconnectCmd(btManager.btDdbLastAddr);//断开命令改为HCI DISCONNECT CMD
}

#ifdef BT_TWS_SUPPORT
void BtTwsCancelReconnect(void)
{
	//此函数内不能加打印信息
	if(btManager.btTwsReconnectTimer.timerFlag)
	{
		btManager.btTwsReconnectTimer.timerFlag = TIMER_UNUSED;
		btManager.btTwsReconnectTryCount = 0;
		btManager.btTwsReconnectIntervalTime = 0;
		btManager.btReconnectType &= ~RECONNECT_TWS;
	}
	BtDdb_ClearTwsDeviceRecord();

	if(btManager.btReconnectTimer.timerFlag)
		BtReconnectCB();
}
#endif

void BtCancelConnect(void)
{
	//此函数内不能加打印信息
	if(btManager.btReconnectTimer.timerFlag)
	{
		btManager.btReconnectTimer.timerFlag = TIMER_UNUSED;
		btManager.btReconnectTryCount = 0;
		btManager.btReconnectIntervalTime = 0;
		btManager.btReconnectedFlag = 0;
		gBtReconBusyCnt = 0;
#ifdef BT_USER_STATE_DISPLAY
		btManager.btReconnectType &= ~RECONNECT_DEVICE;
		SetBtUserState(BT_USER_STATE_PREPAIR);
#endif
	}
#ifdef BT_TWS_SUPPORT
	if(btManager.btTwsReconnectTimer.timerFlag)
		BtReconnectTwsCB();
#endif
}

#ifndef BT_TWS_SUPPORT
//此函数底层调用
void BtCancelReconnect(void)
{
	if(btManager.btReconnectTimer.timerFlag)
	{
		btManager.btReconnectTimer.timerFlag = TIMER_UNUSED;
		btManager.btReconnectTryCount = 0;
		btManager.btReconnectIntervalTime = 0;
		btManager.btReconnectedFlag = 0;

		btReConProtectCnt = 1;
		gBtReconBusyCnt = 0;
	}
}
#else
//此函数底层调用
void BtCancelReconnect(void)
{
}
#endif

/*****************************************************************************************
 * 链路断连
 ****************************************************************************************/
//此函数底层调用
void BtLmAclLinkDisCb(uint8_t *addr)
{
	btManager.btLastAddrUpgradeIgnored = 0;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void SetBtCurConnectFlag(uint8_t state)
{
	btManager.btCurConnectFlag = state;
}

uint8_t GetBtCurConnectFlag(void)
{
	return btManager.btCurConnectFlag;
}

#ifdef BT_TWS_SUPPORT
#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
void BtAccessModeSet_TwsPeerMaster(BtAccessMode accessMode)
{
	SetBtDeviceConnState(accessMode);
	
	if(BtTwsPairingState())
	{
		APP_DBG("CFG_TWS_PEER: pairing...access mode:%d\n", accessMode);
		return;
	}
	
	if(btManager.btConStateProtectCnt)
	{
		APP_DBG("CFG_TWS_PEER: ...access mode:%d\n", accessMode);
		if(btManager.btAccessModeEnable)
		{
			SetBtStackState(BT_STACK_STATE_READY);
			btManager.btAccessModeEnable = 0;
			BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
			#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
			return;
			#endif
		}
		
		if((btManager.twsFlag) && (btManager.twsRole == BT_TWS_MASTER))
		{
			if(accessMode != BtAccessModeNotAccessible)
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 200);
			}
		}
		else if((btManager.twsFlag) && (btManager.twsRole == BT_TWS_SLAVE))
		{
			if(accessMode != BtAccessModeConnectableOnly)
			{
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 200);
			}
		}
		return;
	}

	if(accessMode == BtAccessModeNotAccessible)
	{
		APP_DBG("CFG_TWS_PEER: ACCESS MODE: Bluetooth no discover no connectable.\r\n");

		if(btManager.btLastAddrUpgradeIgnored)
			return;
		
		if(btManager.btAccessModeEnable)
		{
			SetBtStackState(BT_STACK_STATE_READY);
			btManager.btAccessModeEnable = 0;
			BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
		}
		else
		{
			if(GetSystemMode() > 2)//working
			{
				// 手机和TWS都连上，设为不可见不可连
				if((btManager.btLinkState)&&(btManager.twsState == BT_TWS_STATE_CONNECTED))
					return;

				if(btManager.btLinkState)
				{
					// 连上手机且作为TWS主机连接过 ，设为仅可连
					if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_MASTER))
					{
						BtSetAccessModeMsg(BtAccessModeConnectableOnly, 200);
					}
					return;
				}

				if(btManager.twsState == BT_TWS_STATE_CONNECTED)
				{
					#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
					if(!IsBtAudioMode())
					{
						return;
					}
					#endif

					if(btManager.twsRole != BT_TWS_SLAVE)
					{
						if(btManager.btScanDisable == 0){
							BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
						}else{
							BtSetAccessModeMsg(BtAccessModeConnectableOnly, 200);
						}
					}
					return;
				}

				if((!IsBtAudioMode())&&(!IsBtTwsSlaveMode()))
				{
					if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_MASTER))
					{
						#if (CFG_TWS_ONLY_IN_BT_MODE == FALSE)
						BtSetAccessModeMsg(BtAccessModeConnectableOnly, 200);
						#endif
					}
				}
				else
				{
					if(IsBtTwsSlaveMode())
					{
						if(btManager.btScanDisable == 0){
							BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
						}else{
							BtSetAccessModeMsg(BtAccessModeConnectableOnly, 200);
						}
					}

					//if((btManager.btReconnectTimer.timerFlag == 0)&&(btManager.btTwsReconnectTimer.timerFlag == 0))
					{
						if(btManager.btScanDisable == 0){
							BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
						}else{
							BtSetAccessModeMsg(BtAccessModeConnectableOnly, 200);
						}
					}
				}
			}
		}
	}
	else if(accessMode == BtAccessModeDiscoverbleOnly)
	{
		APP_DBG("CFG_TWS_PEER: ACCESS MODE: only Discoverable\r\n");
	}
	else if(accessMode == BtAccessModeConnectableOnly)
	{
		APP_DBG("CFG_TWS_PEER: ACCESS MODE: only Connectable\r\n");

		if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_SLAVE)&&(SReconnectTwsFlag))
		{
			BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			return;
		}

		if(!IsBtAudioMode())
		{
			#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
			if(GetSystemMode() != AppModeBtHfPlay)
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
			#else
			if(IsBtTwsSlaveMode()&&(btManager.twsState != BT_TWS_STATE_CONNECTED))
			{
				if(btManager.btScanDisable == 0)
				{
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				}
			}
			#endif
			return;
		}

		if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_SLAVE_TIMEOUT)
			return;

		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			if(btManager.twsRole == BT_TWS_MASTER)
			{
				if(btManager.btLinkState == 0)
				{
					if(btManager.btScanDisable == 0)
					{
						BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
					}
				}
				else
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
			}
			else
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
		}
		else
		{
			if(btManager.btLinkState == 0)
			{
				if(btManager.btScanDisable == 0)
				{
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				}
			}
			else if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_MASTER))
			{
				return;
			}
			else if(btManager.twsEnterPairingFlag)
			{
				return;
			}
			else
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
		}
	}
	else //if(accessMode == BtAccessModeGeneralAccessible)
	{
		APP_DBG("CFG_TWS_PEER: ACCESS MODE: WaitForPairing/Connection\r\n");

		btManager.btAccessModeEnable = 0;

		if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			SetBtStackState(BT_STACK_STATE_READY);
		}

		if((btManager.btLastAddrUpgradeIgnored)&&(btManager.btLinkState))
		{
			BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			return;
		}

	#if (TWS_PAIRING_MODE != CFG_TWS_ROLE_RANDOM)
		if(btManager.btScanDisable == 1)
		{
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 20);
			return;
		}

		if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_SLAVE)&&(SReconnectTwsFlag))
		{
			if((BtAllProfileConnectState()==0))
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
			return;
		}
	#endif

	#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
		#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC) && (TWS_PAIRING_MODE != CFG_TWS_ROLE_RANDOM)
		if((!IsBtAudioMode()) && (mainAppCt.appCurrentMode != AppModeBtHfPlay))
		{
			BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			return;
		}
		#endif

		if(gBtTwsAppCt->btTwsPairingStart)
			return;

		//TWS组网成功
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			//从设备不可被搜索不可被发现
			if(btManager.twsRole == BT_TWS_SLAVE)
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
			//主设备连上手机后,不可被搜索不可被发现
			else
			{
				if(btManager.btLinkState)
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
			}
		}
#if (TWS_PAIRING_MODE != CFG_TWS_ROLE_RANDOM)
		else  //TWS未组网
		{
			if(btManager.btLinkState)
			{
				if(GetBtManager()->twsFlag)
				{
					if(btManager.twsRole == BT_TWS_MASTER)
					{
						BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
					}
					else
					{
						BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
					}
				}
				else
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
			}
			else
			{
				if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_SLAVE_TIMEOUT)
				{
					BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
				}
			}
		}
#endif
	#else //#if (CFG_TWS_ONLY_IN_BT_MODE == DISABLE)
		if((gBtTwsAppCt->btTwsPairingStart)&&(btManager.btLinkState == 0))
			return;

		//TWS组网成功
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			//从设备不可被搜索不可被发现
			if(btManager.twsRole == BT_TWS_SLAVE)
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
			//主设备连上手机后,不可被搜索不可被发现
			else
			{
				#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
				if((!IsBtAudioMode()) && (mainAppCt.appCurrentMode != AppModeBtHfPlay))
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
					return;
				}
				#endif

				if(btManager.btLinkState)
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
			}
		}
#if (TWS_PAIRING_MODE != CFG_TWS_ROLE_RANDOM)
		else  //TWS未组网
		{
			#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
			if((!IsBtAudioMode()) && (mainAppCt.appCurrentMode != AppModeBtHfPlay))
			{
				if((btManager.twsFlag) && (btManager.twsRole != BT_TWS_SLAVE))
				{
					BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
				}
				else
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
				return;
			}
			#endif

			if(btManager.btLinkState)
			{
				if(GetBtManager()->twsState<BT_TWS_STATE_CONNECTED)
				{
					BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
				}
				else
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
			}
			else
			{
				if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_SLAVE_TIMEOUT)
				{
					BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
				}
			}
		}
	#endif //CFG_TWS_ONLY_IN_BT_MODE
#endif
	}
}
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
void BtAccessModeSet_TwsPeerSlave(BtAccessMode accessMode)
{
	if(accessMode == BtAccessModeNotAccessible)
	{
		APP_DBG("CFG_TWS_PEER_SLAVE: ACCESS MODE: Bluetooth no discover no connectable.\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_NONE);
        
		if(btManager.btLastAddrUpgradeIgnored)
			return;
		
		if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			SetBtStackState(BT_STACK_STATE_READY);
		}

		#ifdef BT_FAST_POWER_ON_OFF_FUNC
		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
		}
		else
		{
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
		}
		#else
		if(btManager.btAccessModeEnable)
		{
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
		}
		#endif
	}
	else if(accessMode == BtAccessModeDiscoverbleOnly)
	{
		APP_DBG("CFG_TWS_PEER_SLAVE: ACCESS MODE: only Discoverable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_DISCOVERIBLE);
	}
	else if(accessMode == BtAccessModeConnectableOnly)
	{
		APP_DBG("CFG_TWS_PEER_SLAVE: ACCESS MODE: only Connectable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_CONNECTABLE);
		
		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;

			if(GetBtManager()->btReconnectDeviceFlag & RECONNECT_DEVICE)
			{
				BtReconnectDevice();
			}
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			return;
		}
		else
		{
			if(GetSystemMode() == AppModeTwsSlavePlay)
				return;

			if(btManager.twsState == BT_TWS_STATE_NONE)
			{
				if(btManager.btScanDisable == 0)
				{
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				}
			}

			#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
			if(!IsBtAudioMode())
			{
				return;
			}
			#endif
		}
	}
	else
	{
		APP_DBG("CFG_TWS_PEER_SLAVE: ACCESS MODE: WaitForPairing/Connection\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_ALL);
		
		if(btManager.btLastAddrUpgradeIgnored)
			return;
		
		if(btManager.btScanDisable == 1)
		{
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 20);
			return;
		}
		
		btManager.btAccessModeEnable = 0;
		//peer slave
		if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			SetBtStackState(BT_STACK_STATE_READY);
		}
		
		if(GetSystemMode() == AppModeTwsSlavePlay)
		{
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			return;
		}

		//TWS组网成功
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			//主设备连上手机后,不可被搜索不可被发现
			#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
			if(!IsBtAudioMode())
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				return;
			}
			#endif
			
			if(btManager.btLinkState)
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
		}
	}
}
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
void BtAccessModeSet_TwsRoleSlave(BtAccessMode accessMode)
{
	if(accessMode == BtAccessModeNotAccessible)
	{
		APP_DBG("CFG_TWS_ROLE_SLAVE: ACCESS MODE: Bluetooth no discover no connectable.\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_NONE);
        
		if(btManager.btLastAddrUpgradeIgnored)
			return;
		
		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;
			SetBtStackState(BT_STACK_STATE_READY);
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
		}
	}
	else if(accessMode == BtAccessModeDiscoverbleOnly)
	{
		APP_DBG("CFG_TWS_ROLE_SLAVE: ACCESS MODE: only Discoverable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_DISCOVERIBLE);
	}
	else if(accessMode == BtAccessModeConnectableOnly)
	{
		APP_DBG("CFG_TWS_ROLE_SLAVE: ACCESS MODE: only Connectable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_CONNECTABLE);
		
		btManager.btAccessModeEnable = 0;

		#if defined(CFG_TWS_ROLE_SLAVE_TEST)
		if (btManager.twsSoundbarSlaveTestFlag)
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 50);
		#endif
		
		if(Bt_sniff_sniff_start_state_get())
		{
			BleScanParamConfig_Sniff();
			//sniff下linkloss,slave发起1次回连,确认是否异常断开
			//tws_master_connect(&btManager.btTwsDeviceAddr[0]);
			//BtTwsConnectApi(); //slave disconnect 已经执行
			return;
		}
		
		if((btManager.twsState == BT_TWS_STATE_NONE)&&(!btManager.twsSbSlaveDisable))
			tws_slave_simple_pairing_ready();
	}
	else
	{
		APP_DBG("CFG_TWS_ROLE_SLAVE: ACCESS MODE: WaitForPairing/Connection\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_ALL);
		
		if(btManager.btLastAddrUpgradeIgnored)
			return;
		
		if(btManager.btScanDisable == 1)
		{
			#if defined(CFG_TWS_ROLE_SLAVE_TEST)
			if (btManager.twsSoundbarSlaveTestFlag)
			{
				return;
			}
			#else
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 20);
			return;
			#endif
		}
		
		btManager.btAccessModeEnable = 0;
		
		if(btManager.twsSoundbarSlaveTestFlag)
			return;

		BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);

		if((btManager.twsState == BT_TWS_STATE_NONE)&&(!btManager.twsSbSlaveDisable))
			tws_slave_simple_pairing_ready();
	}
}
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
void BtAccessModeSet_TwsRoleMaster(BtAccessMode accessMode)
{
	if(accessMode == BtAccessModeNotAccessible)
	{
		APP_DBG("CFG_TWS_ROLE_MASTER: ACCESS MODE: Bluetooth no discover no connectable.\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_NONE);

		if(btManager.btLastAddrUpgradeIgnored)
			return;

		if(gBtEnterDeepSleepFlag)
		{
			if(btManager.twsState == BT_TWS_STATE_NONE)
			{
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			}
			return;
		}

		if(Bt_sniff_sniff_start_state_get())
			return;

		if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			SetBtStackState(BT_STACK_STATE_READY);
		}

		#ifdef BT_FAST_POWER_ON_OFF_FUNC
		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
		}
		else
		{
			if(btManager.twsState == BT_TWS_STATE_NONE)
			{
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			}
		}
		#else
		if(btManager.btAccessModeEnable)
		{
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
		}
		#endif
	}
	else if(accessMode == BtAccessModeDiscoverbleOnly)
	{
		APP_DBG("CFG_TWS_ROLE_MASTER: ACCESS MODE: only Discoverable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_DISCOVERIBLE);
	}
	else if(accessMode == BtAccessModeConnectableOnly)
	{
		APP_DBG("CFG_TWS_ROLE_MASTER: ACCESS MODE: only Connectable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_CONNECTABLE);

		if(gBtEnterDeepSleepFlag)
			return;

		if(Bt_sniff_sniff_start_state_get())
		{
			if(btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
			return;
		}

		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;

			ble_advertisement_data_update();
			if(GetBtManager()->btReconnectDeviceFlag & RECONNECT_DEVICE)
			{
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				BtReconnectDevice();
			}
		}
		else
		{
			#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
			if(!IsBtAudioMode())
				return;
			#endif
			
			if(btManager.btLinkState == 0)
			{
				if(btManager.btScanDisable == 0)
				{
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				}
			}	
		}
	}
	else
	{
		APP_DBG("CFG_TWS_ROLE_MASTER: ACCESS MODE: WaitForPairing/Connection\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_ALL);

		if(btManager.btLastAddrUpgradeIgnored)
			return;

		if(btManager.btScanDisable == 1)
		{
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 20);
			return;
		}

		if(gBtEnterDeepSleepFlag)
			return;

		if(Bt_sniff_sniff_start_state_get())
		{
			if(btManager.twsState == BT_TWS_STATE_CONNECTED)
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			else
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			return;
		}

		if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			SetBtStackState(BT_STACK_STATE_READY);
		}
		
		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;
			#ifdef BT_FAST_POWER_ON_OFF_FUNC
			BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
			#endif
			
			ble_advertisement_data_update();
			if(GetBtManager()->btReconnectDeviceFlag & RECONNECT_DEVICE)
			{
				BtReconnectDevice();
			}
		}
		else
		{
			if(btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				//TWS组网成功
				#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
				if(!IsBtAudioMode())
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
					return;
				}
				#endif
				if(btManager.btLinkState)
				{
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
				else
				{
					if(btManager.btReconnectType == RECONNECT_DEVICE)
					{
						if(btManager.btReconnectTimer.timerFlag)
							BtReconnectCB();
					}
				}
			}
			else
			{
				//TWS未组网
				#if defined(CFG_APP_CONFIG) && defined(BT_FAST_POWER_ON_OFF_FUNC)
				if(!IsBtAudioMode())
				{
					BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
					return;
				}
				#endif
				
				if(btManager.btLinkState)
				{
					BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
				}
				else
				{
					if(btManager.btReconnectType == RECONNECT_DEVICE)
					{
						if(btManager.btReconnectTimer.timerFlag)
							BtReconnectCB();
					}
				}
			}
		}
	}
}
#endif

void BtAccessModeUpdate(BtAccessMode accessMode)
{
	#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
	BtAccessModeSet_TwsPeerMaster(accessMode);
	#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtAccessModeSet_TwsPeerMaster(accessMode);
	#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	BtAccessModeSet_TwsRoleMaster(accessMode);
	#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	BtAccessModeSet_TwsRoleSlave(accessMode);
	#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	BtAccessModeSet_TwsPeerSlave(accessMode);
	#endif
}

#else //BT_TWS_SUPPORT

void BtAccessModeUpdate(BtAccessMode accessMode)
{
	if(accessMode == BtAccessModeNotAccessible)
	{
		APP_DBG("ACCESS MODE: Bluetooth no discover no connectable.\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_NONE);

		if(btManager.btLastAddrUpgradeIgnored)
			return;
		
		#ifdef BT_FAST_POWER_ON_OFF_FUNC
		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;
			BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
			SetBtStackState(BT_STACK_STATE_READY);
		}
		#else
		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200); // < General discoverable and connectable >
		}
		#endif
	}
	else if(accessMode == BtAccessModeDiscoverbleOnly)
	{
		APP_DBG("ACCESS MODE: only Discoverable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_DISCOVERIBLE);
	}
	else if(accessMode == BtAccessModeConnectableOnly)
	{
		APP_DBG("ACCESS MODE: only Connectable\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_CONNECTABLE);
		
		if(btManager.btLinkState)
		{
			BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
		}
		else
		{
			if(btManager.btScanDisable == 0)
			{
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 50);
			}
		}
	}
	else
	{
		APP_DBG("ACCESS MODE: WaitForPairing/Connection\r\n");
		SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_ALL);
		
		if(btManager.btLastAddrUpgradeIgnored)
			return;
		
		if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			SetBtStackState(BT_STACK_STATE_READY);
		}
		
		if(btManager.btScanDisable == 1)
		{
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 20);
			return;
		}

		if(btManager.btAccessModeEnable)
		{
			btManager.btAccessModeEnable = 0;
			#ifdef BT_FAST_POWER_ON_OFF_FUNC
			BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
			#endif
		}
		BtReconnectDevice();
	}
}
#endif

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtStackCallback(BT_STACK_CALLBACK_EVENT event, BT_STACK_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_COMMON_STACK_INITIALIZED:
		{
			APP_DBG("BT_STACK_EVENT_COMMON_STACK_INITIALIZED\n");
			SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_NONE);

			#ifndef BT_SNIFF_ENABLE
			extern void Set_rwip_sleep_enable(bool flag);
			Set_rwip_sleep_enable(0);
			#endif

			#if !defined(CFG_BT_BACKGROUND_RUN_EN) || !defined(CFG_APP_USB_AUDIO_MODE_EN)
			BtMidMessageSend(MSG_BT_MID_STACK_INIT, 0);
			#endif
		}
		break;

		case BT_STACK_EVENT_COMMON_STACK_FREE_MEM_SIZE:
		{
			//APP_DBG("stack mem:%ld, %ld\n", param->params.stackMemParams.stackMallocMemSize, param->params.stackMemParams.stackFreeMemSize);
		}
		break;

		case BT_STACK_EVENT_COMMON_STACK_OUT_OF_MEM_ERR:
		{
			APP_DBG("!!!ERR: BT_STACK_EVENT_COMMON_STACK_FREE_MEM_SIZE\n");
		}
		break;

		case BT_STACK_EVENT_COMMON_INQUIRY_RESULT:
		{
#ifndef BT_TWS_SUPPORT
			APP_DBG("Inquriy Result:\n");
			APP_DBG("\t%02x:%02x:%02x:%02x:%02x:%02x",
					(param->params.inquiryResult.addr)[0],
					(param->params.inquiryResult.addr)[1],
					(param->params.inquiryResult.addr)[2],
					(param->params.inquiryResult.addr)[3],
					(param->params.inquiryResult.addr)[4],
					(param->params.inquiryResult.addr)[5]);
			APP_DBG("\t %d dB", param->params.inquiryResult.rssi);
			APP_DBG("\t extLen = %d , ext = %s \n",param->params.inquiryResult.extRespLen, param->params.inquiryResult.extResp);
#endif
		}
		break;

		case BT_STACK_EVENT_COMMON_INQUIRY_COMPLETE:
		{
			APP_DBG("BT_STACK_EVENT_COMMON_INQUIRY_COMPLETE\n");
		}
		break;

		case BT_STACK_EVENT_COMMON_INQUIRY_CANCELED:
		{
			APP_DBG("BT_STACK_EVENT_COMMON_INQUIRY_CANCELED\n");
		}
		break;


		case BT_STACK_EVENT_COMMON_MODE_CHANGE:
			APP_DBG("mode - %d, addrs -  %02x:%02x:%02x:%02x:%02x:%02x\n",
					param->params.modeChange.mode,
					(param->params.modeChange.addr)[0],
					(param->params.modeChange.addr)[1],
					(param->params.modeChange.addr)[2],
					(param->params.modeChange.addr)[3],
					(param->params.modeChange.addr)[4],
					(param->params.modeChange.addr)[5]);
#ifdef BT_TWS_SUPPORT
			if(tws_connect_cmp(param->params.modeChange.addr) == 0)
			{
				GetBtManager()->twsMode = param->params.modeChange.mode;
#endif
				if(param->params.modeChange.mode == BTLinkModeSniff)
				{
#ifdef BT_SNIFF_ENABLE

#ifndef BT_TWS_SUPPORT
		          	extern unsigned int gSniffInterval_his;
					extern unsigned char notacc_f;

					gSniffInterval_his = 0;//sniff成功删除记录值
					notacc_f = 0;
#endif//BT_TWS_SUPPORT
					Bt_sniff_sniff_start();
//					if(GetBtManager()->twsRole == BT_TWS_SLAVE)
//					{
//						SysDeepsleepStandbyStatus();
//					}
#endif
//					SwitchModeTo(APP_MODE_IDLE);
				}
				else
				{
#ifdef BT_SNIFF_ENABLE
					Bt_sniff_sniff_stop();
//					BtPlayWakeupConfig();
#endif
				}
#ifdef BT_TWS_SUPPORT
			}
#endif
		break;

		case BT_STACK_EVENT_COMMON_GET_REMDEV_NAME:
		{
			if(param->params.remDevName.name == NULL)
				break;
			
			APP_DBG("Remote Device:\n");
			APP_DBG("\t%02x:%02x:%02x:%02x:%02x:%02x",
					(param->params.remDevName.addr)[0],
					(param->params.remDevName.addr)[1],
					(param->params.remDevName.addr)[2],
					(param->params.remDevName.addr)[3],
					(param->params.remDevName.addr)[4],
					(param->params.remDevName.addr)[5]);
			
			memset(btManager.remoteName, 0, 40);
			btManager.remoteNameLen = 0;
			
			if((param->params.remDevName.nameLen) && (param->params.remDevName.name))
			{
				if(param->params.remDevName.nameLen < 40)
				{
					btManager.remoteNameLen = param->params.remDevName.nameLen;
				}
				else
				{
					btManager.remoteNameLen = 40;
				}
				memcpy(btManager.remoteName, param->params.remDevName.name, btManager.remoteNameLen);
			}
			APP_DBG("\t nameLen = %d , name = %s \n",btManager.remoteNameLen, btManager.remoteName);
			

			if((param->params.remDevName.name[0] == 'M')
				&& (param->params.remDevName.name[1] == 'V')
				&& (param->params.remDevName.name[2] == '_')
				&& (param->params.remDevName.name[3] == 'B')
				&& (param->params.remDevName.name[4] == 'T')
				&& (param->params.remDevName.name[5] == 'B')
				&& (param->params.remDevName.name[6] == 'O')
				&& (param->params.remDevName.name[7] == 'X')
				)
			{
                if(btManager.btLinkState)
                {
				    printf("phone connected, disconnect btbox\n");
    				btManager.btLastAddrUpgradeIgnored = 0;
                    BTHciDisconnectCmd((uint8_t *)param->params.remDevName.addr);
                }
                else
                {
    				printf("connect btbox\n");
    				btManager.btLastAddrUpgradeIgnored = 1;
                }
			}
		}
		break;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case BT_STACK_EVENT_COMMON_ACCESS_MODE:
		BtAccessModeUpdate(param->params.accessMode);
		break;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case BT_STACK_EVENT_COMMON_CONNECTION_ABORTED:
			APP_DBG("BT_STACK_EVENT_COMMON_CONNECTION_ABORTED\n");
			//在L2CAP异常断开所有协议时，开始启动回连机制,30s超时
			btEventListB1State = 0;
#if defined(CFG_APP_CONFIG)
			{
				#include "bt_play_mode.h"
				if(GetBtPlayState() == 1)
					btEventListB1State = 1;
			}
#endif
			btCheckEventList |= BT_EVENT_L2CAP_LINK_DISCONNECT;
			btEventListB1Count = btEventListCount;
			btEventListB1Count += 30000;
		break;

		case BT_STACK_EVENT_COMMON_PAGE_TIME_OUT:
			APP_DBG("BT_STACK_EVENT_COMMON_PAGE_TIME_OUT\n");
#ifdef BT_TWS_SUPPORT
			if(btManager.btConStateProtectCnt)
			{
				btManager.btConStateProtectCnt=0;
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			}
			
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
			if(btManager.btReconnectType == RECONNECT_DEVICE)
			{
				if(!BtTwsReconProcessCheck())
				{
					if(btManager.btReconnectTimer.timerFlag)
						BtReconnectCB();
				}
			}
			else if(btManager.btReconnectType == RECONNECT_TWS)
			{
				if(!BtReconProcessCheck())
				{
					if(btManager.btTwsReconnectTimer.timerFlag)
					{
						if((param->errorCode == 0x0d)&&(btManager.twsRole == BT_TWS_SLAVE))
						{
							BtStopReconnectTws();
						}
						else
						{
							BtReconnectTwsCB();
						}
					}
				}
			}
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
			if(btManager.btReconnectType == RECONNECT_DEVICE)
			{
				if(btManager.btReconnectTimer.timerFlag)
					BtReconnectCB();
			}
#endif//(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
#else
			BtReconnectCB();
#endif//defined(BT_TWS_SUPPORT)

#if (defined(BT_TWS_SUPPORT) && (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE))
			//gBtTwsSniffLinkLoss = 0;
			if(!btManager.twsSbSlaveDisable)
			{
				if((btManager.twsEnterPairingFlag)&&(gBtTwsAppCt->btTwsEvent&BT_TWS_EVENT_SIMPLE_PAIRING))
					tws_slave_start_simple_pairing();
				else
					tws_slave_simple_pairing_ready();
			}
#endif// defined(BT_TWS_SUPPORT) && (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
		break;

////////////////////////////////////////////////////////////////////////////////
		case BT_STACK_EVENT_TRUST_LIST_ADD_NEW_RECORD:
		{
			APP_DBG("A new device has been added into db : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n",
					(param->params.bd_addr)[0],
					(param->params.bd_addr)[1],
					(param->params.bd_addr)[2],
					(param->params.bd_addr)[3],
					(param->params.bd_addr)[4],
					(param->params.bd_addr)[5]);
		}
		break;

		case BT_STACK_EVENT_TRUST_LIST_DELETE_RECORD:
		break;

		case BT_STACK_EVENT_COMMON_BB_LOST_CONNECTION:
		{
#ifdef BT_TWS_SUPPORT
			APP_DBG("LINK LOST\n");
#else
			//connection timeout
			if(param->errorCode == 0x08)
			{
				APP_DBG("BB LOST\n");
#endif //#ifdef BT_TWS_SUPPORT

				if(!btManager.btLastAddrUpgradeIgnored)
				{
					#ifdef BT_TWS_SUPPORT
					//tws已经连接,并且为从,则不能回连主机
					if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
						break;
					#endif
					BtStartReconnectDevice(2);
				}
			
			#ifdef BT_TWS_SUPPORT
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			#endif//#ifdef BT_TWS_SUPPORT

#ifndef BT_TWS_SUPPORT
			}
#endif //#ifndef BT_TWS_SUPPORT
		}
		break;

#ifdef BT_TWS_SUPPORT
		case BT_STACK_EVENT_COMMON_TWS_LOST_CONNECTION:
		{
			APP_DBG("[TWS]LINK LOST\n");
			
#ifdef CFG_TWS_SOUNDBAR_APP
			{
				extern void sniff_lmpsend_set(uint8_t set);
				sniff_lmpsend_set(0);
			}
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
			if(GetSystemMode() == AppModeTwsSlavePlay)
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_BT_TWS_LINKLOSS;
				MessageSend(GetTwsSlaveMessageHandle(), &msgSend);
				break;
			}
#endif //TWS_PAIRING_MODE ==

#ifdef BT_SNIFF_ENABLE
			if(Bt_sniff_sniff_start_state_get())
			{
				//Bt_sniff_sniff_stop();
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50); 
				break;
			}
#endif//BT_SNIFF_ENABLE
			
			if(btManager.btLinkState)
			{
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			}
			else
			{
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
				//BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
#else
				//BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
				/*if(btManager.twsSimplePairingCfg)
				{
					//slave等待超时后进入可被搜索可被连接状态
					gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_SLAVE_TIMEOUT;
					gBtTwsAppCt->btTwsSlaveTimeoutCount = (BT_TWS_RECONNECT_TIMEOUT/2);
				}*/
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
#endif
			}
			//BtTwsLinkLoss();
		}
		break;
		
		case BT_STACK_EVENT_COMMON_TWS_IS_FOUND:
		{	
			btManager.remoteNameLen = param->params.inquiryResult.extResp[0] - 1;
			if(btManager.remoteNameLen < BT_NAME_SIZE)
				memcpy(btManager.remoteName, &param->params.inquiryResult.extResp[2], btManager.remoteNameLen);
			else
				memcpy(btManager.remoteName, &param->params.inquiryResult.extResp[2], BT_NAME_SIZE);
			
			APP_DBG("[TWS]tws device is found, connecting: %x:%x:%x:%x:%x:%x		nameLen: %d		name: %s\n", 
			param->params.inquiryResult.addr[0], param->params.inquiryResult.addr[1], param->params.inquiryResult.addr[2], 
			param->params.inquiryResult.addr[3], param->params.inquiryResult.addr[4], param->params.inquiryResult.addr[5],
			btManager.remoteNameLen, &btManager.remoteName[0]);

			BtTwsPairingConnect(param->params.inquiryResult.addr);
		}
		break;

		case BT_STACK_EVENT_COMMON_TWS_PAIRING_STOP:
			printf("BT_STACK_EVENT_COMMON_TWS_PAIRING_STOP\n");
			if((btManager.btLinkState)&&(btManager.twsState == BT_TWS_STATE_CONNECTED))
			{
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			}
			else if(btManager.btLinkState)
			{
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			}
			else
			{
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			}
		break;

		case BT_STACK_EVENT_COMMON_TWS_PAIRING_TIMEOUT:
			APP_DBG("[TWS]Pairing timeout.\n");
			CheckBtTwsPairing();
		
#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
			if(!BtReconProcessCheck())
			{
				if(btManager.btReconnectType == RECONNECT_TWS)
					BtReconnectTwsCB();
			}
#endif
		break;

#else //#ifdef BT_TWS_SUPPORT

		case BT_STACK_EVENT_SIMPLE_PAIRING_FAILURE:
		{
			//手机端取消配对，系统开机回连，手机端拒绝，会反馈该msg
			//可以停止回连，并可选择清除配对记录
			APP_DBG("SIMPLE PAIRING_FAILURE\n");
			if(btManager.btReconnectTimer.timerFlag)
			{
				BtStopReconnect();

#ifdef BT_AUTO_CLEAR_LAST_PAIRING_LIST
				if(memcmp(GetBtManager()->btDdbLastAddr, param->params.bd_addr, 6) == 0)
				{
					memset(GetBtManager()->btDdbLastAddr, 0, 6);
					BtDdb_LastBtAddrErase();
				}
#endif
			}
		}
		break;
#endif //#ifdef BT_TWS_SUPPORT
		default:
		break;
	}
}

/**************************************************************************************
 **************************************************************************************/
void ClearBtManagerReg(void)
{
	memset(&btManager, 0, sizeof(BT_MANAGER_ST));
}

BT_MANAGER_ST * GetBtManager(void)
{
	return &btManager;
}

#ifndef BT_TWS_SUPPORT
extern uint8_t BtNumericalDispEnable;
/***********************************************************************************
 * 配置蓝牙security功能，1是打开，0是关闭(系统默认关闭)，
 这个函数调用需要在bt stack初始化之前
 **********************************************************************************/
void BtNumericalDisplayFuncEnable(uint8_t val)
{
	BtNumericalDispEnable = val;
}
/***********************************************************************************
 * 校验手机发过来的key，客户自定义，
 校验完成后调用BtNumericalAccecptOrReject来决定是否连接
 **********************************************************************************/
void BtNumericalVerify(uint32_t val)
{
	APP_DBG("SEC Key: %ld\n", val);
}
#endif

/***********************************************************************************
 * 手动设置蓝牙可见性 1=关闭可见性; 0=开启可见性(默认配置)
 **********************************************************************************/
void SetBtScanDisable(uint8_t flag)
{
	btManager.btScanDisable = flag;
	if(btManager.btScanDisable)
		BtSetAccessModeMsg(BtAccessModeConnectableOnly, 10);
	else
		BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 10);
}

/***********************************************************************************
 *
 **********************************************************************************/
void SetBtStackState(BT_STACK_STATE state)
{
	GetBtManager()->stackState = state;
}

BT_STACK_STATE GetBtStackState(void)
{
	return GetBtManager()->stackState;
}

bool SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE mode)
{
	GetBtManager()->deviceConMode = mode;

	return TRUE;
}

BT_DEVICE_CONNECTION_MODE GetBtDeviceConnState(void)
{
	return GetBtManager()->deviceConMode;
}
/**
* Bt device connected profile manage. 
*/
void SetBtConnectedProfile(uint16_t connectedProfile)
{
	btManager.btConnectedProfile |= connectedProfile;

	SetBtCurConnectFlag(1);
	
	//连接到一个蓝牙设备的一个profile时,就更新最后1次连接的蓝牙记录信息
	if(GetNumOfBtConnectedProfile() == 1)
	{
		//配对地址一致,不需要重复保存
		if(memcmp(btManager.btDdbLastAddr, btManager.remoteAddr, 6) == 0)
		{
			if(btManager.btDdbLastProfile & connectedProfile)
				return ;
		}
		
		btManager.btDdbLastProfile = (uint8_t)btManager.btConnectedProfile;
		//BP10主动回连上次连接的设备，被新的手机连接，发现MAC地址不一致，则停止回连
		if(btManager.btReconnectTimer.timerFlag)
		{
			BtStopReconnect();
		}

		memcpy(btManager.btDdbLastAddr, btManager.remoteAddr, 6);
		BtDdb_UpgradeLastBtAddr(btManager.remoteAddr, btManager.btDdbLastProfile);
	}
	else
	{
		if((btManager.btDdbLastProfile & connectedProfile) == 0 )
		{
			btManager.btDdbLastProfile |= connectedProfile;
			BtDdb_UpgradeLastBtProfile(btManager.remoteAddr, btManager.btDdbLastProfile);
		}
	}
}

void SetBtDisconnectProfile(uint16_t disconnectProfile)
{
	btManager.btConnectedProfile &= ~disconnectProfile;

	if(!btManager.btConnectedProfile)
	{
		SetBtCurConnectFlag(0);
	}
}

uint16_t GetBtConnectedProfile(void)
{
	return btManager.btConnectedProfile;
}

uint8_t GetNumOfBtConnectedProfile(void)
{
	uint8_t		number_of_profile = 0;

	if(btManager.btConnectedProfile & BT_CONNECTED_A2DP_FLAG)
	{
		number_of_profile++;
	}

	if(btManager.btConnectedProfile & BT_CONNECTED_HFP_FLAG)
	{
		number_of_profile++;
	}

	if(btManager.btConnectedProfile & BT_CONNECTED_SPP_FLAG)
	{
		number_of_profile++;
	}
		
	if(btManager.btConnectedProfile & BT_CONNECTED_AVRCP_FLAG)
	{
		number_of_profile++;
	}

	return 	number_of_profile;
}

/*param: Addr is the address of remote device
  *return: 0->success, BT host allow connect, other->the reson of fail
  */
//不生效
uint8_t BtConnectDecide(uint8_t *Addr)
{
#if 0	// 客户根据自己的需求实现过滤代码
	uint8_t HPHONE[6] = {0xA6, 0x48, 0x80, 0xFA, 0xA1, 0x30};

	if(memcmp(Addr, HPHONE, 6) == 0)
	{
		APP_DBG("reject the connect request\n");
		return 1;
	}
#endif
	return 0;
}

/*param: NULL
  *return: 0->success, allow Master/Slave connect, other->the reson of fail
  */
uint8_t BtConnectConfirm(void)
{
	if(mainAppCt.appCurrentMode != AppModeBtAudioPlay)
	{
		// 假设主机刚发起了回连操作就立马插入SD/U盘，就会出现切换到SLAVE后主副机依然回连组网成功
		// 这个和CFG_TWS_ONLY_IN_BT_MODE的设计理念不相符，所以加上这个判断
		return 1;
	}

	return 0;
}

//仅仅针对连接DEVICE/phone
uint32_t BtAllProfileConnectState(void)
{
	uint32_t reconnectProfile = GetSupportProfiles();
	reconnectProfile &= (BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP | BT_PROFILE_SUPPORTED_AVRCP);

	if((reconnectProfile & BT_PROFILE_SUPPORTED_HFP)&&(GetHfpState() >= BT_HFP_STATE_CONNECTED))
	{
		reconnectProfile &= ~BT_PROFILE_SUPPORTED_HFP;
	}
	
	if((reconnectProfile & BT_PROFILE_SUPPORTED_A2DP)&&(GetA2dpState() >= BT_A2DP_STATE_CONNECTED))
	{
		reconnectProfile &= ~BT_PROFILE_SUPPORTED_A2DP;
	}

	if((reconnectProfile & BT_PROFILE_SUPPORTED_AVRCP)&&(GetAvrcpState() >= BT_AVRCP_STATE_CONNECTED))
	{
		reconnectProfile &= ~BT_PROFILE_SUPPORTED_AVRCP;
	}
	return reconnectProfile;
}

bool CheckBtAllProfileIsConnected(void)
{
	bool ret = 0;
	uint32_t reconnectProfile = GetSupportProfiles();
	reconnectProfile &= (BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP | BT_PROFILE_SUPPORTED_AVRCP);

	if((reconnectProfile & BT_PROFILE_SUPPORTED_HFP)&&(GetHfpState() >= BT_HFP_STATE_CONNECTED))
	{
		reconnectProfile &= ~BT_PROFILE_SUPPORTED_HFP;
	}
	
	if((reconnectProfile & BT_PROFILE_SUPPORTED_A2DP)&&(GetA2dpState() >= BT_A2DP_STATE_CONNECTED))
	{
		reconnectProfile &= ~BT_PROFILE_SUPPORTED_A2DP;
	}

	if((reconnectProfile & BT_PROFILE_SUPPORTED_AVRCP)&&(GetAvrcpState() >= BT_AVRCP_STATE_CONNECTED))
	{
		reconnectProfile &= ~BT_PROFILE_SUPPORTED_AVRCP;
	}

	if(reconnectProfile == 0)
	{
		BtStopReconnect();
		ret = 1;
	}
	
	return ret;
	
}

extern bool BtReconnectStartIsReady(void);
void BtReconnectDevice(void)
{
#ifndef BT_TWS_SUPPORT
	//发起回连前,有新设备主动连接,避免冲突,不发起回连
	if(btReConProtectCnt)
		return;
#endif

	if(btManager.BtPowerOnFlag)
	{
		//某些时候回连过程中,ACL链路连接成功,出现profile连接异常,则继续发起连接,直到连接超时
		BtReconnectCB();
		return;
	}
#if  defined(CFG_APP_CONFIG)
	if(!BtReconnectStartIsReady())
	{
		GetBtManager()->btReconnectDelayCount = 1;//等待开始回连
		return;
	}
#endif
	btManager.BtPowerOnFlag = 1;
	//reconnect last bluetooth device
	if((GetBtManager()->btDdbLastAddr[0]==0)
		&&(GetBtManager()->btDdbLastAddr[1]==0)
		&&(GetBtManager()->btDdbLastAddr[2]==0)
		&&(GetBtManager()->btDdbLastAddr[3]==0)
		&&(GetBtManager()->btDdbLastAddr[4]==0)
		&&(GetBtManager()->btDdbLastAddr[5]==0))
	{
		APP_DBG("Last Addr is NULL\n");
		return ;
	}
	
	//reconnect remote device
	APP_DBG("Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			APP_DBG("%x ", GetBtManager()->btDdbLastAddr[i]);
		}
	}
	APP_DBG("\n");

	BtStartReconnectDevice(1);
}

extern bool IsBtReconnectReady(void);

static void BtReconnectDelay(void)
{
	APP_DBG("Busy\n");
	TimeOutSet(&btManager.btReconnectTimer.timerHandle, 500);
	gBtReconBusyCnt++;

	btManager.btReconnectTimer.timerFlag |= TIMER_STARTED;
}

extern uint8_t BB_link_state_is_used(uint8_t *addr);
extern int32_t lm_env_link_state(uint8_t *addr);
uint32_t BtReconnectProfile(void)
{
	uint32_t reconnectProfile = 0;
	reconnectProfile = GetSupportProfiles();
	reconnectProfile &= (BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP | BT_PROFILE_SUPPORTED_AVRCP);

	//printf("BtReconnectProfile\n");
	if((GetA2dpState() == BT_A2DP_STATE_NONE)&&(GetAvrcpState() == BT_AVRCP_STATE_NONE)
#if (BT_HFP_SUPPORT == ENABLE)
		&&(GetHfpState() == BT_HFP_STATE_NONE)
#endif
		)
	{
		//当前链路存在,则应用层停止发起新的连接,等待超时后再发起连接
		if(lm_env_link_state(btManager.btDdbLastAddr))
		{
			printf("link exist, delay2s reconnect device...\n");
			BtReconnectDelay();
			return 0;
		}
	}

	//当前链路已被使用,并且不是当前的设备,则停止回连流程
	if(BB_link_state_is_used(btManager.btDdbLastAddr))
	{
		APP_DBG("!!!  lm link used, stop reconnect...\n");
		BtStopReconnect();
		return 0;
	}

	//在回连手机时,打开手机蓝牙功能;安卓手机会自动回连;
	//某些安卓手机回连非常快,会导致sdp channel复用,导致异常;
	//在发起回连时,确定sdp server是不是被使用,未使用则开始进行回连
	if((reconnectProfile & BT_PROFILE_SUPPORTED_HFP)&&(GetHfpState() < BT_HFP_STATE_CONNECTED) &&(btManager.btDdbLastProfile & BT_PROFILE_SUPPORTED_HFP))
	{
		if(!IsBtReconnectReady() && (gBtReconBusyCnt<=3))
		{
			BtReconnectDelay();
			return reconnectProfile;
		}
		BtHfpConnect(GetBtManager()->btDdbLastAddr);
	}
	else if((reconnectProfile & BT_PROFILE_SUPPORTED_A2DP)&&(GetA2dpState() < BT_A2DP_STATE_CONNECTED) &&(btManager.btDdbLastProfile & BT_PROFILE_SUPPORTED_A2DP))
	{
		reconnectProfile &= ~BT_PROFILE_SUPPORTED_HFP;
		
		if(!IsBtReconnectReady()  && (gBtReconBusyCnt<=3))
		{
			BtReconnectDelay();
			return reconnectProfile;
		}

		extern uint32_t gL2capCloseChannelCnt;
		if(gL2capCloseChannelCnt)
		{
			printf("+++ l2cap close channel, delay......\n");
			BtReconnectDelay();
			return reconnectProfile;
		}
		
		BtA2dpConnect(GetBtManager()->btDdbLastAddr);
	}
	else if((reconnectProfile & BT_PROFILE_SUPPORTED_AVRCP)&&(GetAvrcpState() != BT_AVRCP_STATE_CONNECTED) &&(btManager.btDdbLastProfile & BT_PROFILE_SUPPORTED_AVRCP))
	{
		reconnectProfile &= ~(BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP);
		
		if(!IsBtReconnectReady() && (gBtReconBusyCnt<=3))
		{
			BtReconnectDelay();
			return reconnectProfile;
		}
		BtAvrcpConnect(GetBtManager()->btDdbLastAddr);
	}
	else
	{
		reconnectProfile &= ~(BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP | BT_PROFILE_SUPPORTED_AVRCP);
		BtStopReconnect();
	}

	return reconnectProfile;
}

#ifdef BT_TWS_SUPPORT
void BtWaitingForReconDevice(void)
{
	if(btManager.btLinkState)
		return;

	APP_DBG("BtWaitingForReconDevice\n");
	btManager.btReconnectTryCount = BT_POR_TRY_COUNTS;
	if(btManager.btReconnectTryCount == 0)
		return;
	
	btManager.btReconnectIntervalTime = BT_POR_INTERNAL_TIME;
	btManager.btReconnectTimer.timerFlag = TIMER_USED;
	btManager.btReconnectedFlag = 0;
}
#endif

//1.在开机的时候，需要回连连接过的设备
//2.在BB连接丢失(设备拿远后)，需要回连设备
void BtStartReconnectDevice(uint8_t mode)
{
	gBtReconBusyCnt = 0;
	if(mode == 1)
	{
#ifdef BT_TWS_SUPPORT
		if(btManager.btReconnectType == RECONNECT_DEVICE)
			return;
#endif		
		//power on: reconnect device
		btManager.btReconnectTryCount = BT_POR_TRY_COUNTS;
		if(btManager.btReconnectTryCount == 0)
			return;
		btManager.btReconnectIntervalTime = BT_POR_INTERNAL_TIME;
		btManager.btReconnectTimer.timerFlag = TIMER_USED;
		btManager.btReconnectedFlag = 0;
#ifdef BT_TWS_SUPPORT
		{
			btManager.btReconnectType = RECONNECT_DEVICE;
#endif
			BtReconnectProfile();
#ifdef BT_TWS_SUPPORT
		}
#endif
	}
	else if(mode == 2)
	{
		//bb lost: reconnect device
		btManager.btReconnectTryCount = BT_BLR_TRY_COUNTS;
		if(btManager.btReconnectTryCount == 0)
			return;
		
		btManager.btReconnectIntervalTime = BT_BLR_INTERNAL_TIME;
		btManager.btReconnectTimer.timerFlag = TIMER_USED;
		btManager.btReconnectedFlag = 0;
#ifdef BT_TWS_SUPPORT
		{
			btManager.btReconnectType = RECONNECT_DEVICE;
#endif
			BtReconnectProfile();
#ifdef BT_TWS_SUPPORT
		}
#endif
	}
#ifdef BT_USER_STATE_DISPLAY
	SetBtUserState(BT_USER_STATE_RECON);
#endif
}

void BtStartReconnectProfile(void)
{
	if(!btManager.btReconnectTimer.timerFlag)
	{
		//某些手机不主动连接AVRCP，需要主动发起AVRCP连接请求
		/*if((GetAvrcpState() < BT_AVRCP_STATE_CONNECTED)&&(GetA2dpState()>=BT_A2DP_STATE_CONNECTED)
			#if (BT_HFP_SUPPORT == ENABLE)
			&&(GetHfpState()>=BT_HFP_STATE_CONNECTED)
			#endif
		)
		{
			btManager.btReconnectedFlag = 1;
			TimeOutSet(&btManager.btReconnectTimer.timerHandle, 50);
			btManager.btReconnectTimer.timerFlag |= TIMER_USED;
			btManager.btReconnectTimer.timerFlag |= TIMER_STARTED;
		}*/
		return;
	}

	if(CheckBtAllProfileIsConnected())
		return;
	
	btManager.btReconnectedFlag = 1;
	TimeOutSet(&btManager.btReconnectTimer.timerHandle, 50);//200);
	btManager.btReconnectTimer.timerFlag |= TIMER_STARTED;
}

//1.在连接设备成功后，需要停止回连
//2.在连接次数超时后，需要停止回连
//3.在有其他设备连接成功后，停止回连
void BtStopReconnectReg(void)
{
	if(btManager.btReconnectTimer.timerFlag == TIMER_UNUSED)
		return;
	
	APP_DBG("BtStopReconnectReg\n");
	btManager.btReconnectTimer.timerFlag = TIMER_UNUSED;
	btManager.btReconnectTryCount = 0;
	btManager.btReconnectIntervalTime = 0;
#ifdef BT_TWS_SUPPORT
	btManager.btReconnectType &= ~RECONNECT_DEVICE;
#endif
	btManager.btReconnectedFlag = 0;
	gBtReconBusyCnt = 0;
}

void BtStopReconnect(void)
{
	btManager.btReconnectTimer.timerFlag = TIMER_UNUSED;
	btManager.btReconnectTryCount = 0;
	btManager.btReconnectIntervalTime = 0;
	btManager.btReconnectedFlag = 0;
	gBtReconBusyCnt = 0;
#ifdef BT_TWS_SUPPORT
	btManager.btReconnectType &= ~RECONNECT_DEVICE;
	APP_DBG("BtStopReconnect\n");

	if((btManager.btLinkState)&&(btManager.twsState > BT_TWS_STATE_NONE))
	{
		BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
	}
	else if((BtAllProfileConnectState()==0)&&(btManager.twsFlag)&&(btManager.twsRole == BT_TWS_SLAVE))
	{
		BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
	}
	else if(btManager.btLinkState)
	{
		BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
	}
	else
	{
		BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
	}

	if((BtAllProfileConnectState()==0)&&(btManager.twsFlag)&&(btManager.twsRole == BT_TWS_SLAVE))
	{
		if(btManager.btTwsReconnectTimer.timerFlag)
		{
			btManager.btTwsReconnectTimer.timerFlag = TIMER_UNUSED;
			btManager.btTwsReconnectTryCount = 0;
			btManager.btTwsReconnectIntervalTime = 0;
			btManager.btReconnectType &= ~RECONNECT_TWS;
			SReconnectTwsFlag = 0;
			APP_DBG("BtStopReconnect\n");
		}
	}
	else
		BtTwsReconProcessCheck();
#else //#ifdef BT_TWS_SUPPORT
	APP_DBG("BtStopReconnect\n");
	//当L2CAP异常回连事件触发，连接成功或者超时停止回连时，清除相应的状态标识
	if(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT)
	{
		btCheckEventList &= ~BT_EVENT_L2CAP_LINK_DISCONNECT;
		btEventListB1Count = 0;
		if((GetAvrcpState() > BT_AVRCP_STATE_NONE)
		&& (GetA2dpState() > BT_A2DP_STATE_NONE)
		&& btEventListB1State)
		{
			BTCtrlPlay();
		}

		btEventListB1State = 0;
	}
#endif//#ifdef BT_TWS_SUPPORT
}


void BtReconnectCB(void)
{
	if(!btManager.btReconnectTimer.timerFlag)
		return;

	if(btManager.btReconnectTryCount)
	{
		btManager.btReconnectTryCount--;
		TimeOutSet(&btManager.btReconnectTimer.timerHandle, btManager.btReconnectIntervalTime*1000);
		btManager.btReconnectTimer.timerFlag |= TIMER_STARTED;
		APP_DBG("BtReConnect will be started in %d seconds\n", btManager.btReconnectIntervalTime);
	}
	else
	{
#ifdef BT_USER_STATE_DISPLAY
		SetBtUserState(BT_USER_STATE_PREPAIR);
#endif
		BtStopReconnect();
	}
}

void CheckBtReconnectTimer(void)
{
	btManager.btReconnectTimer.timerFlag &= ~TIMER_STARTED;
#ifdef BT_TWS_SUPPORT
	{
		btManager.btReconnectType = RECONNECT_DEVICE;
#endif
		if(!BtReconnectProfile())
		{
			if(btManager.btReconnectedFlag)
			{
				BtStopReconnect();
			}
		}
		
#ifdef BT_TWS_SUPPORT
	}
#endif
}

#ifdef BT_TWS_SUPPORT
bool BtReconProcessCheck(void)
{
	if(!btManager.twsFlag)
	{
		if(btManager.btReconnectTimer.timerFlag)
		{
			btManager.btReconnectTimer.timerFlag = TIMER_UNUSED;
			btManager.btReconnectTryCount = 0;
			btManager.btReconnectIntervalTime = 0;
			btManager.btReconnectedFlag = 0;
			
			btManager.btReconnectType &= ~RECONNECT_DEVICE;
			APP_DBG("BtStopReconnect\n");
		}
		return 0;
	}
	if((btManager.btReconnectTimer.timerFlag)&&(btManager.btReconnectTryCount))
	{
		if(btManager.btReconDelayFlag)
		{
			btManager.btReconDelayFlag = 0;
			return 0;
		}
		
		btManager.btReconnectTryCount--;
		btManager.btReconnectType = RECONNECT_DEVICE;
		APP_DBG("phone reconnect again...\n");
		if(!BtReconnectProfile())
		{
			if(btManager.btReconnectedFlag)
			{
				BtStopReconnect();
			}
			return 0;
		}
		return 1;
	}
	else
	{
		if(btManager.btReconnectTimer.timerFlag)
		{
			btManager.btReconnectTimer.timerFlag = TIMER_UNUSED;
			btManager.btReconnectTryCount = 0;
			btManager.btReconnectIntervalTime = 0;
			btManager.btReconnectedFlag = 0;
			
			btManager.btReconnectType &= ~RECONNECT_DEVICE;
			APP_DBG("BtStopReconnect\n");
		}
		return 0;
	}
}

////////////////////////////////////////////

void BtReconnectTws_Slave(void)
{
#ifdef BT_TWS_FUNCTION_KEY_SWITCH		
	if(!btManager.TwsFunctionEnable)
		return;
#endif

	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		return;
	
	if(btManager.TwsPowerOnFlag)
		return;
	btManager.TwsPowerOnFlag = 1;

	//reconnect remote device
	APP_DBG("tws slave Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			APP_DBG("%x ", GetBtManager()->btTwsDeviceAddr[i]);
		}
	}
	APP_DBG("\n");

	if(btManager.btTwsReconnectTimer.timerFlag)
		return;
	
	btManager.btTwsReconnectTryCount = BT_TWS_TRY_COUNTS;
	if(btManager.btTwsReconnectTryCount == 0)
		return;
	
	btManager.btTwsReconnectIntervalTime = BT_TWS_INTERNAL_TIME;
	btManager.btTwsReconnectTimer.timerFlag = TIMER_USED;
	btManager.btReconnectType = RECONNECT_TWS;
	
	TimeOutSet(&btManager.btTwsReconnectTimer.timerHandle, btManager.btTwsReconnectIntervalTime*1000);
	btManager.btTwsReconnectTimer.timerFlag |= TIMER_STARTED;

	SReconnectTwsFlag=1;
}

//tws
void BtReconnectTws(void)
{
#ifdef BT_TWS_FUNCTION_KEY_SWITCH		
	if(!btManager.TwsFunctionEnable)
		return;
#endif	

	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		return;
	
	if(btManager.TwsPowerOnFlag)
		return;
	btManager.TwsPowerOnFlag = 1;

	//reconnect remote device
	APP_DBG("tws Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			APP_DBG("%x ", GetBtManager()->btTwsDeviceAddr[i]);
		}
	}
	APP_DBG("\n");

	BtStartReconnectTws(BT_TWS_TRY_COUNTS, BT_TWS_INTERNAL_TIME);
}

void BtWaitingForReconTws(void)
{
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		return;
	
	btManager.btTwsReconnectTryCount = BT_TWS_TRY_COUNTS;
	if(btManager.btTwsReconnectTryCount == 0)
		return;
	
	btManager.btTwsReconnectIntervalTime = BT_TWS_INTERNAL_TIME;
	btManager.btTwsReconnectTimer.timerFlag = TIMER_USED;

	//btManager.btReconnectType |= RECONNECT_TWS;
}

//1.在开机的时候，需要回连连接过的设备
//2.在BB连接丢失(设备拿远后)，需要回连设备
void BtStartReconnectTws(uint8_t tryCount, uint8_t interval)
{
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		return;

	if(tryCount == 0)
		return;
	
	btManager.btTwsReconnectTryCount = tryCount;
	btManager.btTwsReconnectIntervalTime = interval;
	btManager.btTwsReconnectTimer.timerFlag = TIMER_USED;

	btManager.btReconnectType = RECONNECT_TWS;
	
	APP_DBG("BtStartReconnectTws: tws reconnect again...\n");
	BtTwsConnectApi();
}

//1.在连接设备成功后，需要停止回连
//2.在连接次数超时后，需要停止回连
//3.在有其他设备连接成功后，停止回连
void BtStopReconnectTwsReg(void)
{
	if(btManager.btTwsReconnectTimer.timerFlag == TIMER_UNUSED)
		return;
	
	APP_DBG("BtStopReconnectTwsReg\n");
	btManager.btTwsReconnectTimer.timerFlag = TIMER_UNUSED;
	btManager.btTwsReconnectTryCount = 0;
	btManager.btTwsReconnectIntervalTime = 0;
	btManager.btReconnectType &= ~RECONNECT_TWS;

	SReconnectTwsFlag = 0;
}
void BtStopReconnectTws(void)
{
	btManager.btTwsReconnectTimer.timerFlag = TIMER_UNUSED;
	btManager.btTwsReconnectTryCount = 0;
	btManager.btTwsReconnectIntervalTime = 0;
	btManager.btReconnectType &= ~RECONNECT_TWS;

#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
	if(SReconnectTwsFlag)
	{
		SReconnectTwsFlag = 0;
	#ifdef BT_FAST_POWER_ON_OFF_FUNC
	#if defined(CFG_APP_CONFIG)
		if(!IsBtAudioMode())
		{
			BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
		}
		else
	#endif
		{
			if(btManager.btLinkState)
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			else
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
		}
	#else //#ifdef BT_FAST_POWER_ON_OFF_FUNC
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			if(btManager.btLinkState)
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			else
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
		}
		else
		{
			BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
		}
	#endif//BT_FAST_POWER_ON_OFF_FUNC
	}
	else
	{
#ifdef BT_FAST_POWER_ON_OFF_FUNC
#if defined(CFG_APP_CONFIG)
		if(!IsBtAudioMode())
		{
			if(btManager.twsRole == BT_TWS_MASTER)
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
			else
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
		}
		else
#endif
		{
			if(btManager.btLinkState)
				BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
			else
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
		}
#endif//BT_FAST_POWER_ON_OFF_FUNC
	}
#endif//TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER
	APP_DBG("BtStopReconnect-Tws\n");

	BtReconProcessCheck();
}


void BtReconnectTwsCB(void)
{
	if(!btManager.btTwsReconnectTimer.timerFlag)
		return;
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
	if(!btManager.TwsFunctionEnable)
		return;
#endif

	if(btManager.btTwsReconnectTryCount)
	{
		btManager.btTwsReconnectTryCount--;
		TimeOutSet(&btManager.btTwsReconnectTimer.timerHandle, btManager.btTwsReconnectIntervalTime*1000);
		btManager.btTwsReconnectTimer.timerFlag |= TIMER_STARTED;
		APP_DBG("[TWS] BtReConnect will be started in %d seconds\n", btManager.btTwsReconnectIntervalTime);
	}
	else
	{
		BtStopReconnectTws();
	}
}

void CheckBtReconnectTwsTimer(void)
{
	btManager.btTwsReconnectTimer.timerFlag &= ~TIMER_STARTED;

	//如果有HFP正在工作(例如打电话、接电话)则停止回连
	if(btManager.twsState > BT_TWS_STATE_NONE || GetHfpState() >= BT_HFP_STATE_INCOMING)
	{
		BtStopReconnectTws();
	}
	else
	{
		APP_DBG("CheckBtReconnectTwsTimer: tws reconnect again...\n");
		btManager.btReconnectType = RECONNECT_TWS;
		tws_master_connect(GetBtManager()->btTwsDeviceAddr);
	}
}

bool BtTwsReconProcessCheck(void)
{
	//如果有HFP正在工作(例如打电话、接电话)则停止回连
	if((btManager.btTwsReconnectTimer.timerFlag)&&(btManager.btTwsReconnectTryCount)&&(btManager.twsState == BT_TWS_STATE_NONE)
			&&(GetHfpState() < BT_HFP_STATE_INCOMING))
	{
		if(btManager.btReconDelayFlag)
		{
			btManager.btReconDelayFlag = 0;
			return 0;
		}
		
		/*btManager.btTwsReconnectTryCount--;
		BT_DBG("BtTwsReconProcessCheck: tws reconnect again...\n");
		btManager.btReconnectType = RECONNECT_TWS;
		tws_master_connect(GetBtManager()->btTwsDeviceAddr);
		*/
#ifdef BT_TWS_FUNCTION_KEY_SWITCH
		if(!btManager.TwsFunctionEnable)
			return 0;
#endif
		//在需要同时连接TWS设备和手机时,则在连接手机超时后,等待个空闲时间,再发起TWS回连
		btManager.btReconnectType = RECONNECT_TWS;
		BtReconnectTwsCB();
		return 1;
	}
	else
	{
		if(btManager.btTwsReconnectTimer.timerFlag)
		{
			btManager.btTwsReconnectTimer.timerFlag = TIMER_UNUSED;
			btManager.btTwsReconnectTryCount = 0;
			btManager.btTwsReconnectIntervalTime = 0;
			btManager.btReconnectType &= ~RECONNECT_TWS;
			SReconnectTwsFlag = 0;
			APP_DBG("BtStopReconnect-Tws\n");
		}
		return 0;
	}
}
#endif //#ifdef BT_TWS_SUPPORT

#ifdef BT_SNIFF_ENABLE

static _BT_SNIFF_CTRL Bt_sniff_state_data;
void Bt_sniff_state_init(void)
{
	Bt_sniff_state_data.bt_sleep_enter = 0;
	Bt_sniff_state_data.bt_sniff_start = 0;
}

//进入stack deepsleep flag

void Bt_sniff_sleep_enter(void)
{
	Bt_sniff_state_data.bt_sleep_enter = 1;
}

void Bt_sniff_sleep_exit(void)
{
	Bt_sniff_state_data.bt_sleep_enter = 0;
}

uint8_t Bt_sniff_sleep_state_get(void)
{
	return Bt_sniff_state_data.bt_sleep_enter;
}

//进入sniff flag
void Bt_sniff_sniff_start(void)
{
#ifndef BT_TWS_SUPPORT
	Bt_sniff_fastpower_dis();
#endif
	Bt_sniff_state_data.bt_sniff_start = 1;
}

void Bt_sniff_sniff_stop(void)
{
	Bt_sniff_state_data.bt_sniff_start = 0;
}

uint8_t Bt_sniff_sniff_start_state_get(void)
{
	return Bt_sniff_state_data.bt_sniff_start;
}
#ifndef BT_TWS_SUPPORT
//启动蓝牙时用不用fastpower//启动sniff EN，K29 change mode，关闭
void Bt_sniff_fastpower_dis(void)
{
	Bt_sniff_state_data.bt_sleep_fastpower_f = 1;
}

void Bt_sniff_fastpower_en(void)
{
	Bt_sniff_state_data.bt_sleep_fastpower_f = 0;
}

uint8_t Bt_sniff_fastpower_get(void)
{
	return Bt_sniff_state_data.bt_sleep_fastpower_f;
}
#endif//#ifndef BT_TWS_SUPPORT
#endif //BT_SNIFF_ENABLE

