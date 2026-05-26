/**
 *******************************************************************************
 * @file    bt_a2dp_app.h
 * @author  Halley
 * @version V1.0.1
 * @date    27-Apr-2016
 * @brief   A2dp callback events and actions
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#include "type.h"
#include "debug.h"
#include "bt_manager.h"
#include "bt_app_interface.h"
#include "bt_config.h"
#include "mcu_circular_buf.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#include "main_task.h"
#endif
#include <nds32_intrinsic.h>

#if (BT_A2DP_SUPPORT == ENABLE)
void SetA2dpState(BT_A2DP_STATE state);

uint32_t a2dp_avrcp_connect_flag = 0;//A2DP连接成功后，AVRCP状态更新判断标志//主要区分A2DP未播放,AVRCP反馈播放状态

extern uint32_t gSpecificDevice;

#ifdef BT_HFP_MODE_DISABLE
extern uint32_t gHfpCallNoneWaiting;
#endif


void BtA2dpCallback(BT_A2DP_CALLBACK_EVENT event, BT_A2DP_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_A2DP_CONNECTED:
		{
			TWS_sbc_decoer_init();
#ifdef BT_TWS_SUPPORT			
			if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
			{
				//从机已经组网成功,此时被手机连接上,则断开手机
				BTHciDisconnectCmd(param->params.bd_addr);
				break;
			}

#endif

			if((GetAvrcpState() == BT_AVRCP_STATE_CONNECTED)&&(memcmp(GetBtManager()->remoteAddr, param->params.bd_addr, 6) != 0))
			{
				//avrcp已连接,和a2dp连上的设备不一致,则断开
				APP_DBG("A2dp Connected : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n",
					(param->params.bd_addr)[0],
					(param->params.bd_addr)[1],
					(param->params.bd_addr)[2],
					(param->params.bd_addr)[3],
					(param->params.bd_addr)[4],
					(param->params.bd_addr)[5]);
				BTHciDisconnectCmd(param->params.bd_addr);
				APP_DBG("----------- Disconnect a2dp device\n");
				break;
			}
			
			APP_DBG("A2dp Connected : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n",
					(param->params.bd_addr)[0],
					(param->params.bd_addr)[1],
					(param->params.bd_addr)[2],
					(param->params.bd_addr)[3],
					(param->params.bd_addr)[4],
					(param->params.bd_addr)[5]);

			SetA2dpState(BT_A2DP_STATE_CONNECTED);
			
			if((param->params.bd_addr)[0] || (param->params.bd_addr)[1] || (param->params.bd_addr)[2] 
				|| (param->params.bd_addr)[3] || (param->params.bd_addr)[4] || (param->params.bd_addr)[5])
			{
				memcpy(GetBtManager()->remoteAddr, param->params.bd_addr, 6);
			}

			SetBtConnectedProfile(BT_CONNECTED_A2DP_FLAG);

			if(!btManager.btReconnectTimer.timerFlag)
			{
				//Remote Device主动连接BP10,A2DP连接成功,AVRCP未连接上,主动发起1次avrcp连接
				if(GetAvrcpState() < BT_AVRCP_STATE_CONNECTED)
				{
					btEventListB2Count = 1500;//延时1500ms//时间太短,某些手机音量同步异常
					btCheckEventList |= BT_EVENT_AVRCP_CONNECT;
				}
			}
			
			BtStartReconnectProfile();

			BtLinkStateConnect();
			a2dp_avrcp_connect_flag = 1;
		}

		break;

		case BT_STACK_EVENT_A2DP_DISCONNECTED:
		{
			APP_DBG("A2dp disconnect\n");
			SetA2dpState(BT_A2DP_STATE_NONE);
			SetBtDisconnectProfile(BT_CONNECTED_A2DP_FLAG);
			//重新更新蓝牙decoder相关参数
			if(RefreshSbcDecoder)
				RefreshSbcDecoder();
			
			BtLinkStateDisconnect();
			
			//A2DP断开后，开启检测AVRCP断开机制(5S超时)
			if(IsAvrcpConnected())
			{
				btEventListB0Count = btEventListCount;
				btEventListB0Count += 5000;//延时5s
				btCheckEventList |= BT_EVENT_AVRCP_DISCONNECT;
			}
#ifdef BT_PROFILE_BQB_ENABLE
#include "bt_stack_api.h"
			GetBtManager()->btDdbLastProfile &= ~(BT_PROFILE_SUPPORTED_A2DP);
#endif
		}
		break;

		case BT_STACK_EVENT_A2DP_CONNECT_TIMEOUT:
		{
			APP_DBG("A2dp connect timeout\n");
			if(GetA2dpState()>BT_A2DP_STATE_NONE)
			{
				SetA2dpState(BT_A2DP_STATE_NONE);
				SetBtDisconnectProfile(BT_CONNECTED_A2DP_FLAG);

				//重新更新蓝牙decoder相关参数
				if(RefreshSbcDecoder)
					RefreshSbcDecoder();
				
				BtLinkStateConnect();

				//A2DP断开后，开启检测AVRCP断开机制(5S超时)
				if(IsAvrcpConnected())
				{
					btEventListB0Count = btEventListCount;
					btEventListB0Count += 5000;//延时5s
					btCheckEventList |= BT_EVENT_AVRCP_DISCONNECT;
				}
			}
		}
		break;

		case BT_STACK_EVENT_A2DP_STREAM_START:
		{
			SetA2dpState(BT_A2DP_STATE_STREAMING);
			
			APP_DBG("A2dp streaming...\n");

#ifdef BT_TWS_SUPPORT
			extern uint32_t g_tws_need_init;
			if((GetSystemMode() != AppModeBtHfPlay)&&(g_tws_need_init != 2))
				g_tws_need_init = 1;
#endif
			BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, 1);
			
#ifndef BT_TWS_SUPPORT
			a2dp_avrcp_connect_flag = 0;
#if (BT_HFP_SUPPORT == ENABLE)
			if(gSpecificDevice)
			{
				extern void SpecialDeviceFunc(void);
				SpecialDeviceFunc();
			}
#endif
#endif
			TWS_sbc_decoer_init();
#if (BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
			if(!CheckTimerStart_BtPlayStatus())
			{
				BTCtrlGetPlayStatus();
				TimerStart_BtPlayStatus();
			}
#endif
		}
		break;
		case BT_STACK_EVENT_A2DP_STREAM_SUSPEND:
		{
			APP_DBG("A2dp suspend\n");
			SetA2dpState(BT_A2DP_STATE_CONNECTED);
			if(GetSystemMode() == AppModeBtAudioPlay)
	        {
	            extern void set_a2dp_stream_suspend(void);
	            set_a2dp_stream_suspend();
				BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, 2);
	        }
		}
		break;

		case BT_STACK_EVENT_A2DP_STREAM_DATA_IND:
		{
#ifdef BT_HFP_MODE_DISABLE
			if(gHfpCallNoneWaiting)
				break;
#endif
#ifdef MVA_BT_OBEX_UPDATE_FUNC_SUPPORT
			extern uint32_t volatile obex_start;
			if(obex_start)
			{
				break;
			}
#endif
			//SBC or AAC
			if(SaveA2dpStreamDataToBuffer)
			{
				//SaveA2dpStreamDataToBuffer(param->params.a2dpStreamParams.a2dpStreamData, param->params.a2dpStreamParams.a2dpStreamDataLen);
			
#ifdef BT_SCMS_ENABLE //have content protection george temp add
				if(GetBtManager()->a2dpStreamType == BT_A2DP_STREAM_TYPE_SBC)//sbc support
				{
					if((param->params.a2dpStreamParams.a2dpStreamData[0] != 0x9c) &&(param->params.a2dpStreamParams.a2dpStreamData[1] == 0x9c))
					{
						SaveA2dpStreamDataToBuffer(&param->params.a2dpStreamParams.a2dpStreamData[1], param->params.a2dpStreamParams.a2dpStreamDataLen-1);
					}
					else
					{
						SaveA2dpStreamDataToBuffer(param->params.a2dpStreamParams.a2dpStreamData, param->params.a2dpStreamParams.a2dpStreamDataLen);
					}
				}
				else
				{
					SaveA2dpStreamDataToBuffer(param->params.a2dpStreamParams.a2dpStreamData, param->params.a2dpStreamParams.a2dpStreamDataLen);
				}
#else
				SaveA2dpStreamDataToBuffer(param->params.a2dpStreamParams.a2dpStreamData, param->params.a2dpStreamParams.a2dpStreamDataLen);
#endif
			}
		}
		break;

		case BT_STACK_EVENT_A2DP_STREAM_DATA_TYPE:
		{
			APP_DBG("A2dp stream type: ");
			if(param->params.a2dpStreamDataType)
			{
				APP_DBG("AAC\n");
				GetBtManager()->a2dpStreamType = BT_A2DP_STREAM_TYPE_AAC;
			}
			else
			{
				APP_DBG("SBC\n");
				GetBtManager()->a2dpStreamType = BT_A2DP_STREAM_TYPE_SBC;
			}
			if(RefreshSbcDecoder)
				RefreshSbcDecoder();
			
		}
		break;

		default:
		break;
	}
}

void SetA2dpState(BT_A2DP_STATE state)
{
	GetBtManager()->a2dpState = state;
}
#endif

BT_A2DP_STATE GetA2dpState(void)
{
	return GetBtManager()->a2dpState;
}

void BtA2dpConnect(uint8_t *addr)
{
	if(GetA2dpState() == BT_A2DP_STATE_NONE)
	{
		A2dpConnect(addr);
	}
}

