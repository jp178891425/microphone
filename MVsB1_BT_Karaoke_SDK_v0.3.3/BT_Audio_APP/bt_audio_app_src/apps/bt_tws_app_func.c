/**
 **************************************************************************************
 * @file    bt_tws_app_func.c
 * @brief   tws相关函数功能接口
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2019-10-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include "type.h"
#include "app_config.h"
#include "bt_config.h"
//driver
#include "chip_info.h"
#include "debug.h"
//middleware
#include "main_task.h"
#include "bt_manager.h"
//application
#include "bt_tws_app_func.h"
#include "bt_tws_api.h"
#include "ble_app_func.h"
#include "bt_ddb_flash.h"
#include "bt_stack_service.h"

#ifdef BT_TWS_SUPPORT
BtTwsAppCt *gBtTwsAppCt = NULL;
static uint32_t gBtTwsCount = 0;

extern uint32_t gBtTwsDelayConnectCnt;
extern uint16_t BtTwsRandomPairingCnt;
extern uint32_t BtTwsRandomPairingCntBk;
extern uint32_t BtTwsRandomPairingFlag;

void BtTwsAppInit(void)
{
	if(gBtTwsAppCt == NULL)
	{
		gBtTwsAppCt = (BtTwsAppCt *)osPortMalloc(sizeof(BtTwsAppCt));
	}

	if(gBtTwsAppCt)
	{
		memset(gBtTwsAppCt, 0, sizeof(BtTwsAppCt));
	}
}

void BtTwsDeviceReconnect(void)
{
	if(GetBtManager()->twsState == BT_TWS_STATE_NONE)
	{
		//if((GetBtManager()->twsFlag)&&(GetBtManager()->twsRole == BT_TWS_MASTER))
		if(GetBtManager()->twsFlag) 
		{
			APP_DBG("tws device connect\n");
			//tws_master_connect(GetBtManager()->btTwsDeviceAddr);
			BtTwsConnectApi();
		}
		else
		{
			APP_DBG("no tws paired list\n");
		}
	}
	else
	{
		APP_DBG("tws is connected\n");
	}
}

// 断开TWS连接的函数
void BtTwsDeviceDisconnectExt(void)
{
	if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
	{
		APP_DBG("tws disconnect\n");
		tws_link_disconnect();
	}
}

// 也是断开TWS连接的函数。。。和上面函数的区别就是断开后会自己清除本地的TWS信息，还会发信息让另一台机器也清除TWS信息
void BtTwsDeviceDisconnect(void)
{
	if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
	{
#if (TWS_CONNECTE_WHEN_ACTIVE_DISCONNECTION_SUPPORT == ENABLE)
		tws_active_disconnection();
		BtDdb_ClrTwsDevInfor();
		
		//delay reconnect master device
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_DISCON_TIMEOUT;
		gBtTwsAppCt->btTwsDisconTimeout = 500;
#else
		APP_DBG("tws disconnect\n");
		BtTwsDisconnectApi();
#endif
	}
}

void BtTwsLinkLoss(void){
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if(!btManager.twsSbSlaveDisable)
		tws_slave_simple_pairing_ready();
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
	{
		//delay reconnect master device
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_LINKLOSS;
		gBtTwsAppCt->btTwsLinkLossTimeoutCnt = 25;
	}
#endif
}

void CheckBtTwsPairing(void)
{
	if((gBtTwsAppCt->btTwsPairingTimeroutCnt)&&(gBtTwsAppCt->btTwsPairingStart))
	{
		APP_DBG("again...\n");
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
		
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
#endif
	}
	else
	{
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		BtTwsExitPairingMode();
#endif
	}
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
bool BtTwsPairingState(void)
{
	if(gBtTwsAppCt)
		return gBtTwsAppCt->btTwsPairingStart;
	else
		return 0;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//normal
extern void tws_start_pairing_radom(void);
extern void tws_stop_pairing_radom(void);
void BtTwsEnterPairingMode(void)
{
	//BtDdb_ClearTwsDeviceAddrList();
	//1.退出回连状态(TWS回连和手机回连)
	if(GetBtManager()->btReconnectTimer.timerFlag)
	{
		BtStopReconnect();
	}
	if(GetBtManager()->btTwsReconnectTimer.timerFlag)
	{
		BtStopReconnectTws();
	}

	//2.断开当前已连接的设备
	if(GetBtManager()->twsState > BT_TWS_STATE_NONE)
	{
		//tws_link_disconnect();
		BtTwsDisconnectApi();
	}

	gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
	gBtTwsAppCt->btTwsPairingStart = 1;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = BT_TWS_RECONNECT_TIMEOUT;

	tws_start_pairing_radom();
}

void BtTwsExitPairingMode(void)
{
	tws_stop_pairing_radom();
	if(gBtTwsAppCt->btTwsPairingStart == 0)
		return;
	
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING;
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
	gBtTwsAppCt->btTwsPairingStart = 0;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = 0;
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
#if defined(CFG_APP_CONFIGdefined) && defined(BT_FAST_POWER_ON_OFF_FUNC)
	if(!IsBtAudioMode())
	{
		if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
			BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
		else
			BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
		return;
	}
#endif
	//BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
	if(btManager.btLinkState)
	{
		//蓝牙已连接手机,进入不可被连接不可被搜索状态(手机和SLAVE都已连上)
		BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
	}
	else
	{
		//蓝牙未连接手机,进入可被搜索可被连接状态(只有SLAVE已连上)
		BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
	}
#endif
}

void BtTwsPeerEnterPairingMode(void)
{
	//BtDdb_ClearTwsDeviceAddrList();
	GetBtManager()->twsRole = BT_TWS_UNKNOW;
}

void BtTwsRepairDiscon(void)
{
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_REPAIR_CLEAR)
		gBtTwsAppCt->btTwsRepairTimerout = 1;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//peer
void BtTwsEnterPeerPairingMode(void)
{
	APP_DBG("BtTwsEnterPeerPairingMode\n");
	gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
	gBtTwsAppCt->btTwsPairingStart = 1;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = BT_TWS_RECONNECT_TIMEOUT;

	BtCancelConnect();
}

void BtTwsExitPeerPairingMode(void)
{
	if(gBtTwsAppCt->btTwsPairingStart == 0)
		return;

	APP_DBG("BtTwsExitPeerPairingMode\n");
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING;
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
	gBtTwsAppCt->btTwsPairingStart = 0;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = 0;

	BTInquiryCancel();
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//soundbar
void BtTwsEnterSimplePairingMode(void)
{
	APP_DBG("enter tws pairing mode...\n");
	//清除TWS组网配对记录
	//BtDdb_ClearTwsDeviceAddrList();

	//断开已经连接的TWS组网
	if(GetBtManager()->twsState > BT_TWS_STATE_NONE)
	{
		gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING_READY;
		//tws_link_disconnect();
		BtTwsDisconnectApi();
	}

	gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_SIMPLE_PAIRING;
	gBtTwsAppCt->btTwsPairingStart = 1;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = BT_TWS_RECONNECT_TIMEOUT;

	GetBtManager()->twsRole = BT_TWS_UNKNOW;
	btManager.twsEnterPairingFlag = 1;
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if(((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)==0)&&(!btManager.twsSbSlaveDisable))
		tws_slave_start_simple_pairing();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)==0)
		ble_advertisement_data_update();
#endif
}

void BtTwsExitSimplePairingMode(void)
{
	//停止广播
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_SIMPLE_PAIRING;
	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING_READY;

	gBtTwsAppCt->btTwsPairingStart = 0;
	gBtTwsAppCt->btTwsPairingTimeroutCnt = 0;
	btManager.twsEnterPairingFlag = 0;
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if((GetBtManager()->twsState == BT_TWS_STATE_NONE)&&(GetBtManager()->btLinkState == 0))
		tws_slave_stop_simple_pairing();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if((GetBtManager()->twsState == BT_TWS_STATE_NONE))
		ble_advertisement_data_update();
#endif
}

void BtTwsRunLoop(void)
{
	//20ms
	gBtTwsCount++;
	if(gBtTwsCount<20)
		return;

	if(gBtTwsDelayConnectCnt)
	{
		gBtTwsDelayConnectCnt--;
		if(gBtTwsDelayConnectCnt==0)
		{
			if((gBtTwsAppCt->btTwsPairingStart)&&(gBtTwsAppCt->btTwsPairingTimeroutCnt))
			{
				gBtTwsAppCt->btTwsEvent |= BT_TWS_EVENT_PAIRING;
				printf("pairing again...\n");
			}
		}
	}

	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_DISCON_TIMEOUT)
	{
		if(gBtTwsAppCt->btTwsDisconTimeout)
		{
			gBtTwsAppCt->btTwsDisconTimeout--;

			if(gBtTwsAppCt->btTwsDisconTimeout == 0)
			{
				gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_DISCON_TIMEOUT;
				if(btManager.twsState == BT_TWS_STATE_CONNECTED)
				{
					APP_DBG("BT_TWS_EVENT_DISCON_TIMEOUT: tws disconnect\n");
					BtTwsDisconnectApi();
				}
			}
		}
	}

	gBtTwsCount=0;
	//配对连接相关处理
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING)
	{
		if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_REPAIR_CLEAR)
		{
			if(GetBtManager()->twsState != BT_TWS_STATE_NONE)
			{
				gBtTwsAppCt->btTwsRepairTimerout++;
				if(gBtTwsAppCt->btTwsRepairTimerout >= 20)
				{
					gBtTwsAppCt->btTwsRepairTimerout = 0;
					gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
					//tws_link_disconnect();
					BtTwsDisconnectApi();
				}
			}
			else
			{
				gBtTwsAppCt->btTwsRepairTimerout = 0;
				gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_REPAIR_CLEAR;
			}
		}
		else
		{
			//if((GetBtManager()->twsState == BT_TWS_STATE_NONE)&&(GetBtManager()->btLinkState == 0))
			if(GetBtManager()->twsState == BT_TWS_STATE_NONE)
			{
			#if (TWS_PAIRING_MODE != CFG_TWS_PEER_SLAVE)
				//BtDdb_ClearTwsDeviceAddrList();
			#endif
				APP_DBG("tws pairing start\n");
				GetBtManager()->twsRole = BT_TWS_UNKNOW;
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
				tws_start_pairing(TWS_ROLE_RANDOM);
				if(btManager.btLinkState == 1)
				{
					APP_DBG("NoDisc_NoCon\n");
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
				else
				{
					APP_DBG("Disc_Con\n");
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				}
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
				tws_start_pairing(TWS_ROLE_MASTER);
				if(btManager.btLinkState == 1)
				{
					APP_DBG("NoDisc_NoCon\n");
					BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
				}
				else
				{
					APP_DBG("Disc_Con\n");
					BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
				}
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
				tws_start_pairing(TWS_ROLE_SLAVE);
				BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
#endif
				gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING;
			}
		}
	}
	if(gBtTwsAppCt->btTwsPairingTimeroutCnt)
	{
		gBtTwsAppCt->btTwsPairingTimeroutCnt--;

#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
		if(gBtTwsAppCt->btTwsPairingTimeroutCnt == 0)
			BtTwsExitPeerPairingMode();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		if(gBtTwsAppCt->btTwsPairingTimeroutCnt == 0)
			BtTwsExitPairingMode();
#endif
	}

//only master and slave
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)
	{
		if((GetBtManager()->twsState == BT_TWS_STATE_NONE)&&(GetBtManager()->btLinkState == 0))
		{
			gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_PAIRING_READY;
	#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
			{
				ble_advertisement_data_update();
			}
	#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
			{
				if(!btManager.twsSbSlaveDisable)
					tws_slave_start_simple_pairing();
			}
	#endif
		}
	}
	
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_SIMPLE_PAIRING)
	{
		if((gBtTwsAppCt->btTwsPairingTimeroutCnt == 0)||((GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)&&((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_PAIRING_READY)==0)))
		{
			APP_DBG("auto exit tws simple pairing\n");
			//gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_SIMPLE_PAIRING;

			BtTwsExitSimplePairingMode();
		}
	}

	//linkloss处理
	if(gBtTwsAppCt->btTwsLinkLossTimeoutCnt)
		gBtTwsAppCt->btTwsLinkLossTimeoutCnt--;
	if((gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_LINKLOSS)&&(gBtTwsAppCt->btTwsLinkLossTimeoutCnt == 0))
	{
		gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_LINKLOSS;
		BtStartReconnectTws(BT_TWS_BLR_TRY_COUNTS, BT_TWS_BLR_INTERNAL_TIME);
	}

	//slave开机回连超时后进入可被搜索可被连接状态
	if(gBtTwsAppCt->btTwsEvent & BT_TWS_EVENT_SLAVE_TIMEOUT)
	{
		//30s timeout
		if(gBtTwsAppCt->btTwsSlaveTimeoutCount)
			gBtTwsAppCt->btTwsSlaveTimeoutCount--;
		
		if(gBtTwsAppCt->btTwsSlaveTimeoutCount == 0)
		{
			gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_SLAVE_TIMEOUT;
			//gBtTwsAppCt->btTwsSlaveTimeoutCount = 0;

			if((GetBtManager()->twsState == BT_TWS_STATE_NONE)&&(GetBtManager()->btLinkState == 0))
			{
				//TWS回连超时,进入可被搜索可被连接状态
				BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
			}
		}
	}
}

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
void TwsDoubleKeyPairTimeOut(void)
{
	extern uint32_t doubleKeyCnt;
	extern void tws_stop_pairing_doubleKey(void);
	if(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
	{
		doubleKeyCnt = 0;
	}
	if(doubleKeyCnt==1)
	{
		doubleKeyCnt = 0;
		tws_stop_pairing_doubleKey();
		BtBroadcastNameUpdate();//20230519
		printf("tws doublekey pairing stop!\n");
	}
}
#endif
#endif


