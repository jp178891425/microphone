/**
 *******************************************************************************
 * @file    bt_tws_app.c
 * @author  Owen
 * @version V1.0.1
 * @date    10-Oct-2019
 * @brief   tws callback events and actions
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
#include "clk.h"
#include "stdlib.h"
#include "bt_manager.h"
#include "bt_app_interface.h"
#include "bt_tws_api.h"
#include "mcu_circular_buf.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#include "bt_play_mode.h"
#include "main_task.h"
#include "audio_core_api.h"
#include "ctrlvars.h"
#endif
#include "bt_avrcp_api.h"
#include "dac_interface.h"
#include "adc_interface.h"
#include "sbcenc_api.h"
#include "uarts.h"
#include "dma.h"
#include "i2s.h"
#include "audio_adc.h"
#include "i2s_interface.h"
#include "ble_api.h"
#include "irqn.h"
#include "audio_vol.h"
#include "ble_app_func.h"
#include "bt_ddb_flash.h"
#include "remind_sound_service.h"
#include "bt_stack_service.h"

#ifdef BT_TWS_SUPPORT
#include "bt_tws_app_func.h"

//TWS命令收发接口,按照以上内容格式发送
void tws_master_send(uint8_t *buf,uint16_t len);
void tws_slave_send(uint8_t *buf,uint16_t len);
//TWS命令收发接口,用于通话下主从的同步命令发送
void tws_hfp_cmd_send(uint8_t *buf,uint16_t len);

extern void Set_rwip_sleep_enable(bool flag);
extern void TwsSlaveModeEnter(void);
extern void TwsSlaveModeExit(void);
extern void tws_master_active_cmd(void);
extern void send_sniff_msg();
extern void tws_slave_fifo_clear(void);
extern uint8_t tws_reconnect_flag_get(void);

extern uint32_t gBtEnterDeepSleepFlag;
extern uint32_t tws_linked_timeout_flag;

#define TWS_ACL_LINK		0xF1
#define TWS_ACL_UNLINK		0xF2

uint32_t gBtTwsSniffLinkLoss = 0; //1=在sniff下tws linkloss,需要进行回连处理

static char *cmd_reset      = "CMD:TWS_RESET";
static char *cmd_sniff      = "CMD:TWS_SNIFF";
static char *cmd_vol        = "CMD:TWS_VOL";
static char *cmd_mute       = "CMD:MUTE";
static char *cmd_a2dp_state = "CMD:A2DP_STATE";
static char *key_msg        = "CMD:TWS_KEY_MSG";
static char *cmd_audio_init       = "CMD:TWS_AUDIO_INIT";

#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
static char *cmd_eq         = "CMD:TWS_EQ";
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
static char *cmd_music_treb_bass  = "CMD:TWS_MUSIC_TREB_BASS";
#endif
#ifdef TWS_POWEROFF_MODE_SYNC
static char *power_off_sync_msg   = "CMD:TWS_POWER_OFF_SYNC_MSG";
#endif

uint8_t temp[30];
uint32_t tws_first_sync = 0;
uint32_t gBtTwsDelayConnectCnt = 0;//配对组网,主动发起连接异常,延时1s再次发起连接

bool GetTwsConnectState(void)
{
	return (GetBtManager()->twsState == BT_TWS_STATE_CONNECTED);
}

//命令解析按照见函数
//static void BtTws_Master_RecvData(BT_TWS_CALLBACK_PARAMS * param)
//static void BtTws_Slave_RecvData(BT_TWS_CALLBACK_PARAMS * param)
void tws_vol_send(uint16_t Vol,bool MuteFlag)
{
#ifndef TWS_VOLUME_SYNC_ENABLE
	return;
#endif
	if(GetBtManager()->twsState < BT_TWS_STATE_CONNECTED)
		return;

	bool ret = GIE_STATE_GET();
	GIE_DISABLE();
	memcpy(temp,cmd_vol,strlen(cmd_vol));
	temp[strlen(cmd_vol) + 1] = Vol;
	temp[strlen(cmd_vol) + 2] = MuteFlag;
	if(GetBtManager()->twsRole == BT_TWS_MASTER)
	{
		tws_master_send(temp, (strlen(cmd_vol) + 3));
	}
	else if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		tws_slave_send(temp, (strlen(cmd_vol) + 3));
	}
	if(ret)
	{
		GIE_ENABLE();
	}
	
	if(GetBtManager()->twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send cmd_vol_mute:%u %u\n",Vol,MuteFlag);
	}
	else if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send cmd_vol_mute:%u %u\n",Vol,MuteFlag);
	}
}

void tws_slave_send_key_msg(uint16_t key_data)
{
	if((btManager.twsState < BT_TWS_STATE_CONNECTED)||(btManager.twsRole != BT_TWS_SLAVE))
		return;

	bool ret = GIE_STATE_GET();
	GIE_DISABLE();	
	memcpy(temp, key_msg, strlen(key_msg));
	temp[strlen(key_msg) + 1] = (uint8_t)(key_data>>8);
	temp[strlen(key_msg) + 2] = (uint8_t)(key_data&0xff);
	tws_slave_send(temp, (strlen(key_msg) + 3));
	
	if(ret)
	{
		GIE_ENABLE();
	}

	APP_DBG("slave send key msg: 0x%x\n", key_data);
}

#ifdef TWS_POWEROFF_MODE_SYNC
void tws_sync_power_off_msg(uint16_t msg)
{
	if((btManager.twsState < BT_TWS_STATE_CONNECTED)||(btManager.twsRole != BT_TWS_MASTER))
		return;

	bool ret = GIE_STATE_GET();
	GIE_DISABLE();
	memcpy(temp, power_off_sync_msg, strlen(power_off_sync_msg));
	temp[strlen(power_off_sync_msg) + 1] = (uint8_t)(msg>>8);
	temp[strlen(power_off_sync_msg) + 2] = (uint8_t)(msg&0xff);
	tws_master_send(temp, (strlen(power_off_sync_msg) + 3));
	
	if(ret)
	{
		GIE_ENABLE();
	}
	
	APP_DBG("master send power off msg: 0x%x\n", msg);
}
#endif

void tws_master_a2dp_send(void)
{
	if((GetBtManager()->twsState < BT_TWS_STATE_CONNECTED)||(GetBtManager()->twsRole != BT_TWS_MASTER))
		return;

	bool ret = GIE_STATE_GET();
	GIE_DISABLE();
	memcpy(temp, cmd_a2dp_state, strlen(cmd_a2dp_state));
	temp[strlen(cmd_a2dp_state) + 1] = (uint8_t)GetBtPlayState();
	tws_master_send(temp, (strlen(cmd_a2dp_state) + 2));

	if(ret)
	{
		GIE_ENABLE();
	}
	APP_DBG("master send cmd_a2dp_state:%d\n", temp[strlen(cmd_a2dp_state) + 1]);
}

void tws_master_mute_send(bool MuteFlag)
{
	if((GetBtManager()->twsState < BT_TWS_STATE_CONNECTED)||(GetBtManager()->twsRole != BT_TWS_MASTER))
		return;
	
	bool ret = GIE_STATE_GET();
	GIE_DISABLE();
	memcpy(temp,cmd_mute,strlen(cmd_mute));
	temp[strlen(cmd_mute) + 1] = MuteFlag;
	tws_master_send(temp, strlen(cmd_mute) + 2);

	if(ret)
	{
		GIE_ENABLE();
	}
	vTaskDelay(CFG_CMD_DELAY);
	APP_DBG("master send mute: %u\n",MuteFlag);
}


void tws_audio_init_send(void)
{
	if((GetBtManager()->twsState < BT_TWS_STATE_CONNECTED) || (GetBtManager()->twsRole != BT_TWS_MASTER))
		return;
	
	uint8_t i = 0;
	bool ret = GIE_STATE_GET();
	GIE_DISABLE();
	memcpy(temp,cmd_audio_init,strlen(cmd_audio_init));
#ifdef TWS_VOLUME_SYNC_ENABLE
	temp[strlen(cmd_audio_init) + i++] = mainAppCt.MusicVolume;
#endif
	temp[strlen(cmd_audio_init) + i++] = IsAudioPlayerMute();
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN	
	temp[strlen(cmd_audio_init) + i++] = mainAppCt.EqMode;
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	temp[strlen(cmd_audio_init) + i++] = mainAppCt.MusicBassStep;
	temp[strlen(cmd_audio_init) + i++] = mainAppCt.MusicTrebStep;
#endif
	tws_master_send(temp, (strlen(cmd_audio_init) + i++));
	if(ret)
	{
		GIE_ENABLE();
	}

	APP_DBG("master send cmd vol:%u   mute:%u\n",mainAppCt.MusicVolume, IsAudioPlayerMute());
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	APP_DBG("master send cmd EqMode:%u\n", mainAppCt.EqMode);
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	APP_DBG("master send cmd bass:%u   treb:%u\n",mainAppCt.MusicBassStep,mainAppCt.MusicTrebStep);
#endif
}


#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN	 
void tws_eq_mode_send(uint16_t eq_mode)
{
	if(GetBtManager()->twsState < BT_TWS_STATE_CONNECTED)
		return;

	bool ret = GIE_STATE_GET();
	GIE_DISABLE();
	memcpy(temp,cmd_eq,strlen(cmd_eq));
	temp[strlen(cmd_eq) + 1] = eq_mode;
	if(GetBtManager()->twsRole == BT_TWS_MASTER)
	{
		tws_master_send(temp, (strlen(cmd_eq) + 2));
	}
	else if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		tws_slave_send(temp, (strlen(cmd_eq) + 2));
	}
	if(ret)
	{
		GIE_ENABLE();
	}
	
	if(GetBtManager()->twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send eq_mode:%u\n",eq_mode);
	}
	else if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send eq_mode:%u\n",eq_mode);
	}
}
#endif

#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
void tws_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol)
{
	if(GetBtManager()->twsState < BT_TWS_STATE_CONNECTED)
		return;
	
	bool ret = GIE_STATE_GET();
	GIE_DISABLE();
	memcpy(temp,cmd_music_treb_bass,strlen(cmd_music_treb_bass));
	temp[strlen(cmd_music_treb_bass) + 1] = bass_vol;
	temp[strlen(cmd_music_treb_bass) + 2] = treb_vol;
	if(GetBtManager()->twsRole == BT_TWS_MASTER)
	{
		tws_master_send(temp, (strlen(cmd_music_treb_bass) + 3));
	}
	else if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		tws_slave_send(temp, (strlen(cmd_music_treb_bass) + 3));
	}
	if(ret)
	{
		GIE_ENABLE();
	}
	
	if(GetBtManager()->twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send cmd_music_bass_treb:%u %u\n",bass_vol,treb_vol);
	}
	else if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send cmd_music_bass_treb:%u %u\n",bass_vol,treb_vol);
	}
}
#endif


int tws_cmp_paired_mac(uint8_t *addr)
{
	/*uint8_t ls_addr[6];
	ls_addr[5] = addr[0];
	ls_addr[4] = addr[1];
	ls_addr[3] = addr[2];
	ls_addr[2] = addr[3];
	ls_addr[1] = addr[4];
	ls_addr[0] = addr[5];
	return memcmp(ls_addr, btManager.btTwsDeviceAddr, 6);
	*/
	return memcmp(addr, btManager.btTwsDeviceAddr, 6);
}

int tws_cmp_local_mac(uint8_t *addr)
{
	/*uint8_t ls_addr[6];
	ls_addr[5] = addr[0];
	ls_addr[4] = addr[1];
	ls_addr[3] = addr[2];
	ls_addr[2] = addr[3];
	ls_addr[1] = addr[4];
	ls_addr[0] = addr[5];
	return memcmp(ls_addr, btManager.btDevAddr, 6);
	*/
	return memcmp(addr, btManager.btDevAddr, 6);
}

int tws_cmp_name(char* name)
{
	return memcmp(name, BT_NAME, strlen(BT_NAME)-2);
}

int tws_inquiry_cmp_name(char* name, uint8_t len)
{
#ifdef TWS_FILTER_NAME
	extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;
	uint8_t localLen = (strlen((const char *)(btStackConfigParams->bt_LocalDeviceName)));
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	uint8_t gBtNameNull[] = {' '};
	if(len == 1)
	return memcmp(name, gBtNameNull, strlen((const char *)gBtNameNull));
#endif

	if(len != localLen)
		return -1;
	return memcmp(name, &btStackConfigParams->bt_LocalDeviceName[0], localLen);
#else
	return 0;
#endif
}

int tws_filter_user_defined_infor_cmp(uint8_t *infor)
{
#ifdef TWS_FILTER_USER_DEFINED
	return memcmp(infor, &btManager.TwsFilterInfor[0], 6);
#else
	return 0;
#endif
}

BT_TWS_ROLE tws_get_role(void)
{
	return GetBtManager()->twsRole;
}

bool tws_connect_cmp(uint8_t * addr)
{
	return memcmp(&GetBtManager()->btTwsDeviceAddr[0], addr, 6);
}

unsigned char tws_role_match(unsigned char *addr)
{
#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
#include "mode_switch_api.h"
	if(GetSystemMode() == AppModeTwsSlavePlay)
		return BT_TWS_SLAVE;
	else
		return BT_TWS_MASTER;
#endif
	if(btManager.twsEnterPairingFlag)
	{
		return 0xff;
	}
	else if(memcmp(&GetBtManager()->btTwsDeviceAddr[0], addr, 6) == 0)
	{
		return btManager.twsRole;
	}
	else
	{
		return 0xff;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//master callback
static void BtTws_Master_Connected(BT_TWS_CALLBACK_PARAMS * param)
{
#ifdef BT_SNIFF_ENABLE
		//enable bb enter sleep
		Set_rwip_sleep_enable(1);
#endif

	APP_DBG("TWS_MASTER_CONNECTED:\n");
	MainTaskMsgSend(MSG_BT_TWS_MASTER_CONNECTED);

	memcpy(GetBtManager()->btTwsDeviceAddr, param->params.bd_addr, param->paramsLen);
	APP_DBG("addr: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
		GetBtManager()->btTwsDeviceAddr[0],
		GetBtManager()->btTwsDeviceAddr[1],
		GetBtManager()->btTwsDeviceAddr[2],
		GetBtManager()->btTwsDeviceAddr[3],
		GetBtManager()->btTwsDeviceAddr[4],
		GetBtManager()->btTwsDeviceAddr[5]
		);
	GetBtManager()->twsState = BT_TWS_STATE_CONNECTED;
	GetBtManager()->twsRole = BT_TWS_MASTER;

	if(!SoftFlagGet(SoftFlagDecoderRemind))
	{
		AudioPlayerSinkMuteRemind(1);
	}
	
	BtDdb_UpgradeTwsInfor(GetBtManager()->btTwsDeviceAddr);
	GetBtManager()->twsFlag = 1;
	btManager.btReconnectedFlag = 1;
	GetBtManager()->btReconnectDeviceFlag &= ~(RECONNECT_TWS);
	btManager.btReconDelayFlag = 0;

	if(btManager.btConStateProtectCnt)
		btManager.btConStateProtectCnt=0;

	if(btManager.btTwsReconnectTimer.timerFlag)
		BtStopReconnectTws();

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtTwsExitPairingMode();

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
#elif ((TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER))
	BtTwsExitSimplePairingMode();
#else //CFG_TWS_PEER_SLAVE/CFG_TWS_PEER_MASTER
	BtTwsExitPeerPairingMode();

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

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if(Bt_sniff_sniff_start_state_get())
	{
		//再次使从进入sniff
		//printf("send sniff cmd to slave again\n");
		BTSetRemDevIntoSniffMode(GetBtManager()->btTwsDeviceAddr);
		return;
	}
	else
	{
		tws_master_active_cmd();
	}
#endif
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	DisableAdvertising();
#endif


	gBtTwsSniffLinkLoss = 0;

	if(tws_linked_timeout_flag)
	{
		MessageContext		msgSend;
		MessageHandle 		msgHandle;
		tws_linked_timeout_flag = 0;

		printf("msg:MSG_BTSTACK_TWS_SYNC\n");
		msgHandle = (MessageHandle)GetBtStackServiceMsgHandle();
		msgSend.msgId = MSG_BTSTACK_TWS_SYNC;
		MessageSend(msgHandle, &msgSend);
	}
}

static void BtTws_Master_Disconnected(BT_TWS_CALLBACK_PARAMS * param)
{
	bool twsState = FALSE;
	if (GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
		twsState = TRUE;

	GetBtManager()->twsMode=0;
	APP_DBG("TWS_MASTER_DISCONNECT:\n");
	GetBtManager()->twsState = BT_TWS_STATE_NONE;
	AudioPlayerSinkMuteRemind(1500);
	if(btManager.btLinkState)
	{
		//蓝牙已连接手机,进入可被连接不可被搜索状态(只有手机已连上)
		BtSetAccessModeMsg(BtAccessModeConnectableOnly, 50);
	}
	else
	{
		//蓝牙未连接手机,进入可被搜索可被连接状态(手机和SLAVE都未连上)
		BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
	}
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if(Bt_sniff_sniff_start_state_get() == 0)
	{
		ble_advertisement_data_update();
	}
	else
	{
		gBtTwsSniffLinkLoss = 1;
	}
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
	if(tws_reconnect_flag_get())
	{
		//extern void BtTwsLinkLoss(void);
		//APP_DBG("tws link loss, reconnect...\n");
		//BtTwsLinkLoss();
	}
	else if(btManager.btTwsReconnectTimer.timerFlag)
	{
		BtReconnectTwsCB();
	}
#endif

	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_DISCON_TIMEOUT;
	gBtTwsAppCt->btTwsDisconTimeout = 0;
	if (twsState)
	{
		MainTaskMsgSend(MSG_BT_TWS_DISCONNECT_REMIND);
	}
}

static void BtTws_Master_RecvData(BT_TWS_CALLBACK_PARAMS * param)
{
	if(memcmp(param->params.twsData, key_msg, strlen(key_msg)) == 0)
	{
		MessageContext		msgSend;
		bool ret = GIE_STATE_GET();
		
		GIE_DISABLE();
		uint8_t *temp = param->params.twsData;
		msgSend.msgId = (uint16_t)(temp[strlen(key_msg) + 1] << 8) + temp[strlen(key_msg) + 2];
		if(ret)
		{
			GIE_ENABLE();
		}		

		if(GetSystemMode() == AppModeBtHfPlay)
		{
			if((msgSend.msgId != MSG_POWER)\
				&&(msgSend.msgId != MSG_DEEPSLEEP)\
				&&(msgSend.msgId != MSG_POWERDOWN)\
				&&(msgSend.msgId != MSG_BT_SNIFF))
			{
				msgSend.msgId = MSG_NONE;
			}
		}		
		
		DBG("master rev key msg: 0x%x\n", msgSend.msgId);	
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	else if(memcmp(param->params.twsData,cmd_reset,strlen(cmd_reset)) == 0)
	{
		APP_DBG("cmd_reset\n\n");
		extern uint32_t g_tws_need_init;
		g_tws_need_init = 1;
	}
	else if(memcmp(param->params.twsData,cmd_sniff,strlen(cmd_sniff)) == 0)
	{
		#ifdef BT_SNIFF_ENABLE
		APP_DBG("cmd_gotodeepsleep\n\n");
		send_sniff_msg();
		#endif//BT_SNIFF_ENABLE
	}
	else if(memcmp(param->params.twsData,cmd_vol,strlen(cmd_vol)) == 0)
	{
		bool ret = GIE_STATE_GET();
		GIE_DISABLE();
		uint8_t *temp = param->params.twsData;
		mainAppCt.MusicVolume = temp[strlen(cmd_vol) + 1];
		mainAppCt.gSysVol.MuteFlag = temp[strlen(cmd_vol) + 2];
		if(ret)
		{
			GIE_ENABLE();
		}
		APP_DBG("master rcv cmd_vol_mute:%u %u\n",mainAppCt.MusicVolume,mainAppCt.gSysVol.MuteFlag);
		int i;
		if(mainAppCt.gSysVol.MuteFlag)
		{
			for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM ;i++)
			{
				AudioCoreSinkMute(i, TRUE, TRUE);
			}
		}
		else
		{
#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
			//add volume sync(bluetooth play mode)
			if(GetSystemMode() == AppModeBtAudioPlay)
			{
				MessageContext		msgSend;

				SetBtSyncVolume(mainAppCt.MusicVolume);

				msgSend.msgId		= MSG_BT_PLAY_VOLUME_SET;
				MessageSend(GetBtPlayMessageHandle(), &msgSend);
			}
#endif
			for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM ;i++)
			{
				AudioCoreSinkUnmute(i, TRUE, TRUE);
			}
			AudioMusicVolSet(mainAppCt.MusicVolume);
			SystemVolSet();
		}
	}
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	else if(memcmp(param->params.twsData,cmd_eq,strlen(cmd_eq)) == 0)
	{
		bool ret = GIE_STATE_GET();
		GIE_DISABLE();
		uint8_t *temp = param->params.twsData;
		mainAppCt.EqMode = temp[strlen(cmd_eq) + 1];
		if(ret)
		{
			GIE_ENABLE();
		}
		APP_DBG("master rcv cmd_eq_mode = %u\n", mainAppCt.EqMode);
		#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
		EqModeSet(mainAppCt.EqMode);
		#endif		
	}
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	else if(memcmp(param->params.twsData,cmd_music_treb_bass,strlen(cmd_music_treb_bass)) == 0)
	{
		bool ret = GIE_STATE_GET();
		GIE_DISABLE();
		uint8_t *temp = param->params.twsData;
		mainAppCt.MusicBassStep = temp[strlen(cmd_music_treb_bass) + 1];
		mainAppCt.MusicTrebStep = temp[strlen(cmd_music_treb_bass) + 2];
		if(ret)
		{
			GIE_ENABLE();
		}
		APP_DBG("master rcv cmd_music_bass_treb = %u %u\n", mainAppCt.MusicBassStep,mainAppCt.MusicTrebStep);
		MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);		
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////
//slave callback
static void BtTws_Slave_Connected(BT_TWS_CALLBACK_PARAMS * param)
{
#ifdef BT_SNIFF_ENABLE
	//enable bb enter sleep
	Set_rwip_sleep_enable(1);
#endif

	APP_DBG("TWS_SLAVE_CONNECTED:\n");
	MainTaskMsgSend(MSG_BT_TWS_SLAVE_CONNECTED);

	memcpy(GetBtManager()->btTwsDeviceAddr, param->params.bd_addr, param->paramsLen);
	APP_DBG("addr: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
		GetBtManager()->btTwsDeviceAddr[0],
		GetBtManager()->btTwsDeviceAddr[1],
		GetBtManager()->btTwsDeviceAddr[2],
		GetBtManager()->btTwsDeviceAddr[3],
		GetBtManager()->btTwsDeviceAddr[4],
		GetBtManager()->btTwsDeviceAddr[5]
		);
	GetBtManager()->twsState = BT_TWS_STATE_CONNECTED;
	GetBtManager()->twsRole = BT_TWS_SLAVE;

	if(!SoftFlagGet(SoftFlagDecoderRemind))
	{
		AudioPlayerSinkMuteRemind(1);
	}
	
	BtDdb_UpgradeTwsInfor(GetBtManager()->btTwsDeviceAddr);
	GetBtManager()->twsFlag = 1;
	btManager.btReconnectedFlag = 1;
	btManager.btReconDelayFlag = 0;
	
	if(btManager.btReconnectTimer.timerFlag)
	{
		BtStopReconnect();
	}
	
	if(btManager.btTwsReconnectTimer.timerFlag)
		BtStopReconnectTws();
	
	if(btManager.btConStateProtectCnt)
		btManager.btConStateProtectCnt=0;
	
	//slave在tws组网成功后，进入不可被搜索不可被连接状态
	BtSetAccessModeMsg(BtAccessModeNotAccessible, 50);
	GetBtManager()->btReconnectDeviceFlag &= ~((RECONNECT_TWS));
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	tws_slave_simple_pairing_end();
	if(Bt_sniff_sniff_start_state_get() && (gBtTwsSniffLinkLoss==0))
		Bt_sniff_sniff_stop();
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtTwsExitPairingMode();
#elif ((TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER))
	BtTwsExitSimplePairingMode();
#else //CFG_TWS_PEER_SLAVE/CFG_TWS_PEER_MASTER
	BtTwsExitPeerPairingMode();
#endif

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	TwsSlaveModeEnter();
#endif

	gBtTwsSniffLinkLoss = 0;
	tws_linked_timeout_flag = 0;
}

extern uint32_t deepsleep_count;
static void BtTws_Slave_Disconnected(BT_TWS_CALLBACK_PARAMS * param)
{
	bool twsState = FALSE;
	if (GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
		twsState = TRUE;

	GetBtManager()->twsMode=0;
	APP_DBG("TWS_SLAVE_DISCONNECT:\n");
	GetBtManager()->twsState = BT_TWS_STATE_NONE;
	AudioPlayerSinkMuteRemind(1500);
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if(Bt_sniff_sniff_start_state_get() || deepsleep_count)
	{
		BleScanParamConfig_Sniff();
		//tws_master_connect(&btManager.btTwsDeviceAddr[0]);
		BtTwsConnectApi();
		gBtTwsSniffLinkLoss = 1;
	}
	else if(!btManager.twsSbSlaveDisable)
	{
		tws_slave_simple_pairing_ready();
	}
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))//||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(tws_reconnect_flag_get())
	{
		extern void BtTwsLinkLoss(void);
		APP_DBG("tws link loss, reconnect...\n");
		BtTwsLinkLoss();
	}
	else if(btManager.btTwsReconnectTimer.timerFlag)
	{
		BtReconnectTwsCB();
	}
#endif

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	if(gBtTwsSniffLinkLoss != 1)
		TwsSlaveModeExit();
#endif

#if (TWS_PAIRING_MODE != CFG_TWS_ROLE_SLAVE)
	BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
#endif

	gBtTwsAppCt->btTwsEvent &= ~BT_TWS_EVENT_DISCON_TIMEOUT;
	gBtTwsAppCt->btTwsDisconTimeout = 0;

	if (twsState)
	{
		MainTaskMsgSend(MSG_BT_TWS_DISCONNECT_REMIND);
	}
}

static void BtTws_Slave_RecvData(BT_TWS_CALLBACK_PARAMS * param)
{
	if((param->params.twsData)[0] != 'd')
	{
		if(memcmp(param->params.twsData,cmd_mute,strlen(cmd_mute)) == 0)
		{
			if((param->params.twsData)[strlen(cmd_mute) + 1])
			{
				if(!IsAudioPlayerMute())
				{
					AudioPlayerMute();
				}
				TwsSlaveFifoMuteTimeSet();
			}
			else
			{
				TwsSlaveFifoUnmuteSet();
			}
		}
		else if(memcmp(param->params.twsData,cmd_vol,strlen(cmd_vol)) == 0)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t *temp = param->params.twsData;

			mainAppCt.MusicVolume = temp[strlen(cmd_vol) + 1];
			uint8_t MuteFlag = temp[strlen(cmd_vol) + 2];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("slave rcv cmd_vol_mute:%u %u\n",mainAppCt.MusicVolume,MuteFlag);

			if(MuteFlag)
			{
				if(!IsAudioPlayerMute())
				{
					AudioPlayerMute();
				}
				TwsSlaveFifoMuteTimeSet();
			}
			else 
			{
				TwsSlaveFifoUnmuteSet();
				AudioMusicVol(mainAppCt.MusicVolume);
				SystemVolSet();
			}
		}
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
		else if(memcmp(param->params.twsData,cmd_eq,strlen(cmd_eq)) == 0)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t *temp = param->params.twsData;
			mainAppCt.EqMode = temp[strlen(cmd_eq) + 1];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("slave rcv cmd_eq_mode = %u\n", mainAppCt.EqMode);
#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
			EqModeSet(mainAppCt.EqMode);
#endif		
		}
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
		else if(memcmp(param->params.twsData,cmd_music_treb_bass,strlen(cmd_music_treb_bass)) == 0)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t *temp = param->params.twsData;
			mainAppCt.MusicBassStep = temp[strlen(cmd_music_treb_bass) + 1];
			mainAppCt.MusicTrebStep = temp[strlen(cmd_music_treb_bass) + 2];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("slave rcv cmd_music_bass_treb = %u %u\n", mainAppCt.MusicBassStep,mainAppCt.MusicTrebStep);
			MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);		
		}
#endif
		else if(memcmp(param->params.twsData, cmd_a2dp_state, strlen(cmd_a2dp_state)) == 0)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t *temp = param->params.twsData;
			uint8_t val;
			val = temp[strlen(cmd_a2dp_state) + 1];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("slave rcv cmd_a2dp_state = %d\n", val);
		}
		else if(memcmp(param->params.twsData, cmd_reset, strlen(cmd_reset)) == 0)
		{
			APP_DBG("slave rcv init\n");
			tws_first_sync = 1;
			extern uint32_t g_tws_need_init;
			g_tws_need_init = 1;
		}
#ifdef TWS_POWEROFF_MODE_SYNC
		else if(memcmp(param->params.twsData, power_off_sync_msg, strlen(power_off_sync_msg)) == 0)
		{
			MessageContext		msgSend;
			bool ret = GIE_STATE_GET();

			GIE_DISABLE();
			uint8_t *temp = param->params.twsData;
			msgSend.msgId = (uint16_t)(temp[strlen(power_off_sync_msg) + 1] << 8) + temp[strlen(power_off_sync_msg) + 2];
			if(ret)
			{
				GIE_ENABLE();
			}		

			DBG("slave rev power off msg: 0x%x\n", msgSend.msgId);	
			MessageSend(GetMainMessageHandle(), &msgSend);
		}
#endif
		else if(memcmp(param->params.twsData, cmd_audio_init, strlen(cmd_audio_init)) == 0)
		{
			uint8_t i = 0;
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t *temp = param->params.twsData;	
			mainAppCt.MusicVolume = temp[strlen(cmd_audio_init) + i++];
			uint8_t MuteFlag = temp[strlen(cmd_audio_init) + i++];
			#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
			mainAppCt.EqMode = temp[strlen(cmd_audio_init) + i++];
			#endif
			#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
			mainAppCt.MusicBassStep = temp[strlen(cmd_audio_init) + i++];
			mainAppCt.MusicTrebStep = temp[strlen(cmd_audio_init) + i++];
			#endif
			if(ret)
			{
				GIE_ENABLE();
			}

			APP_DBG("slave rcv cmd vol:%u   mute:%u\n",mainAppCt.MusicVolume, MuteFlag);
			if(MuteFlag)
			{
				if(!IsAudioPlayerMute())
				{
					AudioPlayerMute();
				}
				TwsSlaveFifoMuteTimeSet();
			}
			else 
			{
				TwsSlaveFifoUnmuteSet();
				AudioMusicVol(mainAppCt.MusicVolume);
				SystemVolSet();
			}

			#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
			APP_DBG("slave rcv cmd_eq_mode:%u\n", mainAppCt.EqMode);
			#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
			EqModeSet(mainAppCt.EqMode);
			#endif		
			#endif
			
			#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
			APP_DBG("slave rcv cmd bass:%u   treb:%u\n", mainAppCt.MusicBassStep,mainAppCt.MusicTrebStep);
			MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);	
			#endif
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//callback
void BtTwsCallback(BT_TWS_CALLBACK_EVENT event, BT_TWS_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		//master
		case BT_STACK_EVENT_TWS_CONNECTED:
#if defined(CFG_APP_CONFIG) && (defined(CFG_FUNC_REMIND_SOUND_EN) || defined (CFG_FUNC_REMIND_MIX_MODE))
			SoftFlagRegister(SoftFlagTwsRemind);
#endif
			if(param->role == BT_TWS_SLAVE)
			{
				BtTws_Slave_Connected(param);
			}
			else
			{
				BtTws_Master_Connected(param);
				#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)//tws_auto_pair
				BtBroadcastNameUpdate();
				#endif
			}
			break;
			
		case BT_STACK_EVENT_TWS_DISCONNECT:
#if defined(CFG_APP_CONFIG) && (defined(CFG_FUNC_REMIND_SOUND_EN) || defined (CFG_FUNC_REMIND_MIX_MODE))
			SoftFlagDeregister(SoftFlagTwsRemind);
#endif
			if(param->role == BT_TWS_SLAVE)
			{
				printf("slave dis\n");
				BtTws_Slave_Disconnected(param);
			}
			else
			{
				BtTws_Master_Disconnected(param);
			}
	#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
			#include "main_task.h"
			if(GetSystemMode() == AppModeTwsSlavePlay)
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_BT_TWS_DISCONNECT;
				MessageSend(GetTwsSlaveMessageHandle(), &msgSend);
			}
	#endif
			break;
			
		case BT_STACK_EVENT_TWS_DATA_IND:
			if(param->role == BT_TWS_SLAVE)
			{
				BtTws_Slave_RecvData(param);
			}
			else
			{
				BtTws_Master_RecvData(param);
			}
			break;

			
		case BT_STACK_EVENT_TWS_SLAVE_STREAM_START:
#ifdef CFG_APP_CONFIG
			tws_slave_fifo_clear();
#endif
			break;

		case BT_STACK_EVENT_TWS_SLAVE_STREAM_STOP:
			break;
			
		//当前TWS链路存在,则延时再次发起连接
		case BT_STACK_EVENT_TWS_CONNECT_DELAY:
			if(gBtTwsAppCt->btTwsPairingStart)
			{
				gBtTwsDelayConnectCnt = 50;//延时1s //20ms*50=1000ms=1s
			}
			break;
			
		default:
			break;
	}
}


//slave 接收到cmd: master当前处于active模式,需要退出sniff和deepsleep状态
void tws_master_active_mode(void)
{
	if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
#ifdef BT_SNIFF_ENABLE
		if(Bt_sniff_sniff_start_state_get())
		{
			Bt_sniff_sniff_stop();
		}
#endif//BT_SNIFF_ENABLE
	}
}


#include "gpio.h"
#include "irqn.h"
uint32_t tws_init_done = 0;
TaskHandle_t audio_core_handle;
char sample_buf[10];
uint8_t sample_count = 0;
uint32_t error_count = 0;
uint32_t dac_sample_count = 0;
uint32_t tws_delay;
extern uint32_t adc_data_count;
void tws_set_delay(uint32_t delay,uint32_t sample);
extern uint32_t tws_get_delay(void);

uint32_t tws_audio_init(uint32_t cmd)
{
	if(cmd == 1)
	{
		AudioPlayerSinkMuteRemind(1);
		//AudioDAC_DigitalMute(DAC0, TRUE, TRUE);
		tws_init_done = 0;
		tws_audio_init_send();
#ifdef CFG_FUNC_MIC_KARAOKE_EN
		*(uint32_t*)0x40032008 = 0x001E0000;
		sample_count = 0;
		error_count = 0;
		memset(sample_buf,0,sizeof(sample_buf));
#else
		*(uint32_t*)0x40032008 = 0x1c3958;
		dac_sample_count = 0;
		adc_data_count = 0;
		tws_first_sync = 1;
#endif
	}
	else if(cmd == 3)
	{
		uint32_t delay= tws_get_delay();
		tws_set_delay(delay,mainAppCt.SamplesPreFrame);
		uint32_t ret = GIE_STATE_GET();
		GIE_DISABLE();
		Clock_Module1Disable(AUDIO_ADC1_CLK_EN);
		DMA_ChannelDisable(PERIPHERAL_ID_AUDIO_ADC0_RX);
		DMA_CircularFIFOClear(PERIPHERAL_ID_AUDIO_ADC1_RX);
		DMA_ChannelEnable(PERIPHERAL_ID_AUDIO_ADC0_RX);
		Clock_Module1Enable(AUDIO_ADC1_CLK_EN);
		tws_init_done = 1;
		dac_sample_count = 0;
		tws_first_sync = 1;
		vTaskPrioritySet(audio_core_handle, 6);
		if(ret)
		{
			GIE_ENABLE();
		}
	}
	else if(cmd == 8)
	{
		return dac_sample_count + AudioADC1DataLenGet();
	}
	return 0;
}

uint32_t tws_audio_adjust(uint32_t m_len,uint32_t s_len)
{
	int tt = m_len-s_len;
#ifdef CFG_FUNC_MIC_KARAOKE_EN
	if(abs(tt) > 44)
	{
		printf("%lu  %lu %d>>",m_len,s_len,tt);
		error_count++;
		if(error_count > 5)
		{
			return 0;
		}
		return 1;
	}
	else
	{
		error_count = 0;;
	}

	//return 1;
	sample_buf[sample_count] = tt;
	sample_count++;
	if(sample_count >= 10)
	{
		sample_count = 0;
		int min = 127;
		int max = -127;
		int i;
		int sum=0;
		int min_index = 0;
		int max_index = 0;
		for(i=0;i<10;i++)
		{
			if(sample_buf[i] < min)
			{
				min = sample_buf[i];
				min_index = i;
			}
			if(sample_buf[i] > max)
			{
				max = sample_buf[i];
				max_index = i;
			}
		}
		if(min_index == 9)
		{
			sample_buf[9] = sample_buf[8];
		}
		else if(min_index == 0)
		{
			sample_buf[0] = sample_buf[1];
		}
		else
		{
			sample_buf[min_index] = (sample_buf[min_index-1] + sample_buf[min_index+1])/2;
		}

		if(max_index == 9)
		{
			sample_buf[9] = sample_buf[8];
		}
		else if(max_index == 0)
		{
			sample_buf[0] = sample_buf[1];
		}
		else
		{
			sample_buf[max_index] = (sample_buf[max_index-1] + sample_buf[max_index+1])/2;
		}
		for(i=0;i<10;i++)
		{
			//printf("%d\n",sample_buf[i]);
			sum += sample_buf[i];
		}
		sum = sum/10;
		if(sum > 0)
		{
			*(uint32_t*)0x40032008 = 0x001E0000 + abs(sum)*3;
		}
		else
		{
			*(uint32_t*)0x40032008 = 0x001E0000 - abs(sum)*3;
		}
	}
	return 1;
#else
	if(abs(tt) > 44)
	{
		printf("%lu  %lu %d>>",m_len,s_len,tt);
		error_count++;
		if(error_count > 5)
		{
			extern void tws_device_enable(void);
			tws_device_enable();
			return 0;
		}
		return 1;
	}
	else
	{
		error_count = 0;;
	}

	//return 1;
	sample_buf[sample_count] = tt;
	sample_count++;
	if(sample_count >= 10)
	{
		sample_count = 0;
		int min = 127;
		int max = -127;
		int i;
		int sum=0;
		int min_index = 0;
		int max_index = 0;
		for(i=0;i<10;i++)
		{
			if(sample_buf[i] < min)
			{
				min = sample_buf[i];
				min_index = i;
			}
			if(sample_buf[i] > max)
			{
				max = sample_buf[i];
				max_index = i;
			}
		}
		if(min_index == 9)
		{
			sample_buf[9] = sample_buf[8];
		}
		else if(min_index == 0)
		{
			sample_buf[0] = sample_buf[1];
		}
		else
		{
			sample_buf[min_index] = (sample_buf[min_index-1] + sample_buf[min_index+1])/2;
		}

		if(max_index == 9)
		{
			sample_buf[9] = sample_buf[8];
		}
		else if(max_index == 0)
		{
			sample_buf[0] = sample_buf[1];
		}
		else
		{
			sample_buf[max_index] = (sample_buf[max_index-1] + sample_buf[max_index+1])/2;
		}
		for(i=0;i<10;i++)
		{
			printf("%d\n",sample_buf[i]);
			sum += sample_buf[i];
		}
		sum = sum/10;
		if(sum > 0)
		{
			*(uint32_t*)0x40032008 = 0x1c3958 + abs(sum)*3;
		}
		else
		{
			*(uint32_t*)0x40032008 = 0x1c3958 - abs(sum)*3;
		}
	}
	return 1;
#endif
}

bool tws_slave_switch_mode_enable(void)
{
#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	return 1;
#else
	return 0;
#endif
}

bool is_tws_slave(void)
{
	if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
#include "sbcenc_api.h"
#include "sbc_frame_decoder.h"

typedef struct {
	uint8_t nrof_blocks;
	uint8_t nrof_subbands;
	uint8_t mode;
	uint8_t bitpool;
}SBC_FRAME_INFO;
static const uint16_t freq_values[] =    { 16000, 32000, 44100, 48000 };
static const uint8_t block_values[] =    { 4, 8, 12, 16 };
static const uint8_t band_values[] =     { 4, 8 };
uint16_t CalculateFramelen(SBC_FRAME_INFO *frame)
{
	uint16_t nbits = frame->nrof_blocks * frame->bitpool;
	uint16_t nrof_subbands = frame->nrof_subbands;
	uint16_t result = nbits;

    if (frame->mode == 3) {
        result += nrof_subbands + (8 * nrof_subbands);
    } else {
        if (frame->mode == 1) { result += nbits; }
        if (frame->mode == 4) { result += 4*nrof_subbands; } else { result += 8*nrof_subbands; }
    }
    return 4 + (result + 7) / 8;
}

int sbc_get_fram_infor(uint8_t*data,uint32_t *fram_size,uint32_t *frequency)
{
	uint8_t d1;
	SBC_FRAME_INFO frame;
	memset(&frame,0,sizeof(SBC_FRAME_INFO));
	if(data[0] != 0x9C)
	{
		return 1;
	}
	d1 = data[1];
    frame.bitpool = data[2];///////////////////////////////
    *frequency = freq_values[((d1 & (0x80 | 0x40)) >> 6)];
	frame.nrof_blocks = block_values[((d1 & (0x20 | 0x10)) >> 4)];////////
	frame.mode = (d1 & (0x08 | 0x04)) >> 2;///////////////
	frame.nrof_subbands = band_values[(d1 & 0x01)];/////////////
	*fram_size = CalculateFramelen(&frame);
	return 0;
}


int32_t sbc_decoder_init(SBCFrameDecoderContext *ct)
{
	return sbc_frame_decoder_initialize(ct);
}

int32_t sbc_decoder_init_api(void *ct)
{
	return sbc_frame_decoder_initialize((SBCFrameDecoderContext *)ct);
}


int32_t sbc_decoder_apply(SBCFrameDecoderContext *ct,uint8_t *sbc_buf,uint8_t sbc_size,int16_t *pcm_buf)
{
	int32_t ret = sbc_frame_decoder_decode(ct, sbc_buf, sbc_size);
	if(ret == 0)
	{
		if(ct->num_channels == 1)
		{
			int i;
			int16_t *in,*out;
			in = ct->pcm;
			out = pcm_buf;
			for(i=0;i<ct->pcm_length;i++)
			{
				out[2*i+0] = in[i];
				out[2*i+1] = in[i];
			}
		}
		else
		{
			memcpy(pcm_buf,ct->pcm,ct->num_channels*ct->pcm_length*2);
		}
	}
	else
	{

	}
	return ret;
}

int32_t sbc_decoder_apply_api(void *ct,uint8_t *sbc_buf,uint8_t sbc_size,int16_t *pcm_buf)
{
	return sbc_decoder_apply((SBCFrameDecoderContext *)ct, sbc_buf, sbc_size, pcm_buf);
}


int32_t sbc_encoder_init(SBCEncoderContext *ct,int32_t num_channels,SBC_ENC_QUALITY quality)
{
	int32_t samples_per_frame;
	return sbc_encoder_initialize(ct,num_channels,44100,16,quality,&samples_per_frame);
}
int32_t sbc_encoder_aplly(SBCEncoderContext *ct,int16_t *in_pcm,uint8_t *out_sbc,uint32_t *length)
{
	return sbc_encoder_encode(ct,in_pcm,out_sbc,length);
}

int32_t sbc_encoder_aplly_api(void *ct,int16_t *in_pcm,uint8_t *out_sbc,uint32_t *length)
{
	return sbc_encoder_encode((SBCEncoderContext *)ct,in_pcm,out_sbc,length);
}
#endif

