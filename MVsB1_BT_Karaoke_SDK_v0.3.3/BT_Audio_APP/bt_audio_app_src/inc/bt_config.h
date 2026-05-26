
///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: bt_config.h
//  maintainer: keke
///////////////////////////////////////////////////////////////////////////////
#ifndef __BT_DEVICE_CFG_H__
#define __BT_DEVICE_CFG_H__
#include "type.h"
#include "app_config.h"

#define ENABLE						TRUE
#define DISABLE						FALSE
/*****************************************************************
 *
 * 在BQB测试时,开启此宏定义
 *
 */
//#define BT_PROFILE_BQB_ENABLE

/*****************************************************************
 *
 * Bluetooth stack common config
 *
 */
//蓝牙名称注意事项:
//1.蓝牙名称支持中文,需要使用URL编码
//2.BLE的名称修改在ble广播数据中体现(ble_app_func.c)
//3.SDK蓝牙名称上电后从flash中读取,如需使用固定的名称,请移步到bt_app_func.c中LoadBtConfigurationParams函数内修改获取方式
#define BT_NAME						"BP10_BT"
//使用中文名称,需要调用bt_name.h中的定义,原因是容易引起乱码
//#include "bt_name.h"

#define BT_NAME_SIZE				40		//最大支持name size,不能修改
#define BT_ADDR_SIZE				6

//#define FLASH_SAVE_REMOTE_BT_NAME		 	//是否需要记忆已连设备的蓝牙名称

#define BT_TRIM_ECO0				0x18 	//trim范围:0x07~0x1d -- 针对于老芯片
#define BT_TRIM						0x12 	//trim范围:0x00~0x1f

#define BT_SIMPLEPAIRING_FLAG		ENABLE	//0:use pin code; 1:simple pairing
#define BT_PINCODE_LEN				4		//最大16bytes
#define BT_PINCODE					"0000"

#define BT_LSTO_DFT					8000 	//连接超时时间 (换算公式: 8000*0.625=5s)

#define BT_SNIFF_REJ_EN
#define	BT_SNIFF_HOSC_CLK 			0
#define	BT_SNIFF_RC_CLK 			1
#define	BT_SNIFF_LOSC_CLK 			2
#define BT_SNIFF_CLK_SEL 			BT_SNIFF_HOSC_CLK

/*
 * Rf Tx Power Range 
 *   level:  [23] [22] [21] [20] [19] [18] [17] [16] [15] [14] [13] [12] [11] [10] -- [0]
 *   dbm  :   8    6    4    2    0    -2   -4   -6   -8  -10  -13  -15  -17  -19  .. -39
 */
#define BT_TX_POWER_LEVEL			21		//defined +4dbm
#define BT_PAGE_TX_POWER_LEVEL		16  	//蓝牙回连发射功率

//inquiry scan params
#define BT_INQUIRYSCAN_INTERVAL		0x100	//default:0x1000
#define BT_INQUIRYSCAN_WINDOW		0x12	//default:0x12

//page scan params
#define BT_PAGESCAN_INTERVAL		0x1000	//default:0x1000
#define BT_PAGESCAN_WINDOW			0x12	//default:0x12

//page timeout(ms)
#define BT_PAGE_TIMEOUT				5120	//default:5.12s

/*
 * 以下宏请勿随意修改，否则会引起编译错误
 * 注：A2DP和AVRCP是标配，必须要ENABLE
 */
#define BT_A2DP_SUPPORT				ENABLE
#define BT_AVRCP_SUPPORT			ENABLE	//a2dp and avrcp must be enable at the same time
#if CFG_RES_MIC_SELECT
#define BT_HFP_SUPPORT				DISABLE
#endif
#define BT_SPP_SUPPORT				DISABLE
#define BT_HID_SUPPORT				DISABLE
#define BT_MFI_SUPPORT				DISABLE
#define BT_OBEX_SUPPORT				DISABLE
#define BT_PBAP_SUPPORT				DISABLE

#ifdef CFG_TWS_SOUNDBAR_APP
	#define BLE_SUPPORT				ENABLE   // BLE config
#else
	#define BLE_SUPPORT				DISABLE
#endif

/*****************************************************************
 *
 * A2DP config
 *
 */
#if BT_A2DP_SUPPORT == ENABLE

#include "bt_a2dp_api.h"

/*
 * SCMS-T function
 */
//#define BT_SCMS_ENABLE

/*
 * Note:开启AAC,需要同步开启解码器类型USE_AAC_DECODER(app_config.h)
 */
//#define BT_AUDIO_AAC_ENABLE


//BQB认证测试A2DP时,需要开启AAC,与AVP测试项相关
#if defined(BT_PROFILE_BQB_ENABLE) && !defined(BT_AUDIO_AAC_ENABLE)
	#define BT_AUDIO_AAC_ENABLE
#endif
#if defined(BT_AUDIO_AAC_ENABLE) && !defined(USE_AAC_DECODER)
	#define USE_AAC_DECODER
#endif
#endif /* BT_A2DP_SUPPORT == ENABLE */

/*****************************************************************
 *
 * AVRCP config
 *
 */
#if BT_AVRCP_SUPPORT == ENABLE

#include "bt_avrcp_api.h"

/*
 * 建议开启高级AVRCP,关闭后可能会导致A2DP播放状态的更新异常
 */
#define BT_AVRCP_ADVANCED				ENABLE

#if (BT_AVRCP_ADVANCED == ENABLE)
/*
 * If it doesn't support Advanced AVRCP, TG side will be ignored
 * 音量同步功能需要开启该宏开关
 */
#define BT_AVRCP_VOLUME_SYNC			DISABLE

#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
	//#define BT_VOLUME_SYNC_TO_PHONE //设备回连手机OK后，设备端主动同步设备的VOL值给到手机端
#endif

/*
 * If it doesn't support Advanced AVRCP, TG side will be ignored
 * 和音量同步功能都用到AVRCP TG
 * player application setting and value和音量同步宏定义开关一致(eg:EQ/repeat mode/shuffle/scan configuration)
 */
#define BT_AVRCP_PLAYER_SETTING			DISABLE

/*
 * If it doesn't support Advanced AVRCP, song play state will be ignored
 * 歌曲播放时间
 */
#define BT_AVRCP_SONG_PLAY_STATE		DISABLE

/*
 * If it doesn't support Advanced AVRCP, song track infor will be ignored
 * 歌曲ID3信息反馈
 * 歌曲信息有依赖播放时间来获取,请和BT_AVRCP_SONG_PLAY_STATE同步开启
 */
#define BT_AVRCP_SONG_TRACK_INFOR		DISABLE

/*
 * AVRCP BROWSER功能使用,必须使用libBtStack_AvrcpBrws.a库文件
 * 重要! BB_EM_SIZE必须要配置为(20*1024)  此功能不可与TWS共用
 */
#define BT_AVRCP_BROWSER_FUNC			DISABLE

#endif

/*
 * AVRCP连接成功后，自动播放歌曲
 */
#define BT_AUTO_PLAY_MUSIC				DISABLE


//在非蓝牙模式下,播放音乐自动切换到播放模式
//#define BT_AUTO_ENTER_PLAY_MODE

//开机蓝牙不可见只回连状态
//#define POWER_ON_BT_ACCESS_MODE_SET

#endif /* BT_AVRCP_SUPPORT == ENABLE */

/*****************************************************************
 *
 * HFP config
 *
 */
#if BT_HFP_SUPPORT == ENABLE
#include "bt_hfp_api.h"

#define RING_NULL                       0   //不支持来电铃声
#define RING_NUMBER_REMIND              1   //来电报号和铃声
#define RING_LOCAL_AND_PHONE            2   //使用手机自带铃声，若没有则用默认本地铃声
#define RING_ONLY_LOCAL                 3   //强制使用本地铃声

#define CFG_BT_RING_MODE            	RING_LOCAL_AND_PHONE
#define CFG_BT_RING_TIME				1000      //设置本地铃声播放间隔时间


//DISABLE: only cvsd
//ENABLE: cvsd + msbc
#define BT_HFP_SUPPORT_WBS				ENABLE

/*
 * If it doesn't support WBS, only PCM format data can be
 * transfered to application.
 */
#define BT_HFP_AUDIO_DATA				HFP_AUDIO_DATA_mSBC

//电池电量同步(开启需要和 CFG_FUNC_POWER_MONITOR_EN 关联)
//#define BT_HFP_BATTERY_SYNC

//开启HFP连接，不使能进入HFP相关模式(K歌模式和通话模式)
//该应用适用于某些需要连接HFP协议，但是不需要HFP相关功能的场合
//#define BT_HFP_MODE_DISABLE

//同步通讯录请求
#define BT_HFP_QUIRY_PHONEBOOK			DISABLE

#ifndef BT_HFP_MODE_DISABLE
//K歌功能宏开关 (前提：使能通话模式相关功能)
//注:该功能为苹果手机开启全名K歌等K歌软件而设定
//#define BT_RECORD_FUNC_ENABLE
#endif

/*
 * 通话相关配置   MIC的参数调节都已经移到调音工具上位机进行调节
 */
#define BT_REMOTE_AEC_DISABLE			//关闭手机端AEC

//安卓手机来电声音会比iphone小，此宏用于调节这个声音匹配
#define BT_HFP_ANDROID_GAIN				600

//来电通话时长配置选项
#define BT_HFP_CALL_DURATION_DISP

//MIC变调处理
//#define BT_HFP_MIC_PITCH_SHIFTER_FUNC
#ifdef BT_HFP_MIC_PITCH_SHIFTER_FUNC
	#define MAX_PITCH_SHIFTER_STEP		25
#endif

#endif /* BT_HFP_SUPPORT == ENABLE */


/*****************************************************************
 *
 * TWS config
 *
 */
#ifdef BT_TWS_SUPPORT
#include "bt_tws_api.h"

//TWS组网条件判断
#define TWS_FILTER_NAME					 			//过滤名称
//#define TWS_FILTER_USER_DEFINED		 			//自定义过滤条件(max:6bytes)
//默认定义为字符串,假如是定义参数,请移步到配置处自行赋值
#define TWS_FILTER_INFOR				 "TWS-MV"	//(length:6bytes)

#define TWS_VOLUME_SYNC_ENABLE			 			//主从之间音量控制同步

#define CFG_TWS_ROLE_RANDOM						0	//适用于双键组网		主从角色在组网前随机
#define CFG_TWS_ROLE_MASTER						1	//Soundbar主
#define CFG_TWS_ROLE_SLAVE						2	//Soundbar从
#define CFG_TWS_PEER_MASTER						3	//对箱主				按键发起配对的机器组网成功后默认就是 MASTER
#define CFG_TWS_PEER_SLAVE						4	//对箱从				按键发起配对的机器组网成功后默认就是 SLAVE

#ifdef CFG_TWS_SOUNDBAR_APP

#define BT_SNIFF_ENABLE                              //SOUNDBAR需要用到sniff

/*
 *  soundbar组网配对方式选择
 * 	使用soundbar配置主从角色固定:		主机选择 CFG_TWS_ROLE_MASTER 编译烧录
 * 									从机选择 CFG_TWS_ROLE_SLAVE 编译烧录
 */
#define TWS_PAIRING_MODE						CFG_TWS_ROLE_MASTER
//#define TWS_PAIRING_MODE						CFG_TWS_ROLE_SLAVE


#define TWS_SIMPLE_PAIRING_SUPPORT				ENABLE	//DISABLE: 需2个设备先后按键组网     ENABLE: 单按键组网
//#define CFG_TWS_ROLE_SLAVE_TEST                		//打开soundbar从机开机进较频偏模式；如果没有较过频偏，则开机可被测试盒连接，已经较过频偏则正常开机
//#define TWS_SOUNDBAR_MASTER_DISCONNECT_ENABLE  		//使能soundbar Master主动断开TWS; 在TWS断开时,清空配对记录

#elif (defined(CFG_TWS_PEER_APP))
/*
 *  对箱组网配对方式选择
 *  选择除 CFG_TWS_ROLE_MASTER, CFG_TWS_ROLE_SLAVE 外配置
 */
#define TWS_PAIRING_MODE						CFG_TWS_PEER_MASTER


#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	#define TWS_SIMPLE_PAIRING_SUPPORT			DISABLE	//双键组网
#else
	#define TWS_SIMPLE_PAIRING_SUPPORT			ENABLE	//单键组网
#endif

/*
 * 打开表示支持TWS主从同步关机；
 * 关闭表示单箱关机，哪个音箱按键关机就关哪个音箱
 */
#define TWS_POWEROFF_MODE_SYNC

#endif

/*
 * 打开表示从机连上TWS后能够切模式 
 * 关闭表示从机连上TWS后不能切模式
 */
//#define TWS_SLAVE_MODE_SWITCH_EN


//slave连接成功后自动切换到tws_slave模式
#if(TWS_PAIRING_MODE != CFG_TWS_PEER_SLAVE)
	#define CFG_AUTO_ENTER_TWS_SLAVE_MODE
#endif

/*
 * ENABLE 表示手机连接时，该音箱可以发起组队
 * DISABLE 表示手机连接时，该音箱无法发起组队
 */
#define TWS_PAIRING_WHEN_PHONE_CONNECTED_SUPPORT				DISABLE

/*
 * ENABLE 表示用户主动断开TWS配对后，下次开机不能自动回连
 * DISABLE 表示用户主动断开TWS配对后，下次开机能再回连
 */
#if (TWS_SIMPLE_PAIRING_SUPPORT == FALSE) && defined(CFG_TWS_SOUNDBAR_APP)
	#define TWS_CONNECTE_WHEN_ACTIVE_DISCONNECTION_SUPPORT		ENABLE  //soundbar双键组网需清除配对信息
#else
	#define TWS_CONNECTE_WHEN_ACTIVE_DISCONNECTION_SUPPORT		DISABLE
#endif
#endif /* #ifdef BT_TWS_SUPPORT */

/*****************************************************************
 *
 * OBEX config
 *
 */
#if (BT_OBEX_SUPPORT == ENABLE)
//mva通过obex进行升级(升级方式:双bank)
//#define MVA_BT_OBEX_UPDATE_FUNC_SUPPORT
#endif

/*****************************************************************
 * BB EM参数配置
 */
#ifdef CFG_APP_BT_MODE_EN
	#define BB_EM_MAP_ADDR			0x80000000
#ifdef BT_TWS_SUPPORT
	#if (BLE_SUPPORT == ENABLE)
	#define BB_EM_SIZE				(20*1024)       //需要BLE和和TWS需要配置20K
	#else
	#define BB_EM_SIZE				(16*1024)       //不需要BLE则配置16K
	#endif
#else
	#if (BT_AVRCP_BROWSER_FUNC == ENABLE)
	#define BB_EM_SIZE				(20*1024)		//BROWSER需要20K  
	#elif (BLE_SUPPORT == ENABLE)
	#define BB_EM_SIZE				(16*1024)		//BLE需要16K 
	#else
	#define BB_EM_SIZE				(12*1024)		//其他配置可为12K
	#endif
#endif

	#define BB_EM_START_PARAMS		((320*1024-BB_EM_SIZE)/1024)
	#define BB_MPU_START_ADDR		(0x20050000 - BB_EM_SIZE)
#else
	#define BB_EM_MAP_ADDR			0x80000000
	#define BB_EM_START_PARAMS		0
	#define BB_EM_SIZE				0
	#define BB_MPU_START_ADDR		(0x20050000 - BB_EM_SIZE)
#endif

/*****************************************************************
 * ram config
 * 如果开机打印!!!ERR: BT_STACK_EVENT_COMMON_STACK_FREE_MEM_SIZE，搜索不到蓝牙，适当增加此内存申请
 */
#ifdef BT_TWS_SUPPORT
#define BT_BASE_MEM_SIZE			(23*1024)
#else
#define BT_BASE_MEM_SIZE			(24*1024)
#endif

#if (BLE_SUPPORT == ENABLE)
#define BT_BLE_MEM_SIZE				(2*1024+700)
#else
#define BT_BLE_MEM_SIZE				0
#endif

#ifdef BT_AUDIO_AAC_ENABLE
#define BT_AUDIO_AAC_MEM_SIZE		400
#else
#define BT_AUDIO_AAC_MEM_SIZE		0
#endif

#if ((BT_AVRCP_VOLUME_SYNC == ENABLE)||(BT_AVRCP_PLAYER_SETTING == ENABLE))
#define BT_AVRCP_TG_MEM_SIZE		(3*1024)
#else
#define BT_AVRCP_TG_MEM_SIZE		0
#endif

#if (BT_AVRCP_BROWSER_FUNC == ENABLE)
#define BT_AVRCP_BRWS_MEM_SIZE		(23*1024)
#else
#define BT_AVRCP_BRWS_MEM_SIZE		0
#endif

#if (BT_HFP_SUPPORT == ENABLE)
#define BT_HFP_MEM_SIZE				(4*1024)
#else
#define BT_HFP_MEM_SIZE				0
#endif

#if (BT_SPP_SUPPORT == ENABLE)
#define BT_SPP_MEM_SIZE				700
#else
#define BT_SPP_MEM_SIZE				0
#endif

#ifdef BT_TWS_SUPPORT
#define BT_TWS_MEM_SIZE				6*1024
#else
#define BT_TWS_MEM_SIZE				0
#endif

#if (BT_HID_SUPPORT == ENABLE)
#define BT_HID_MEM_SIZE				1024
#else
#define BT_HID_MEM_SIZE				0
#endif

#if (BT_MFI_SUPPORT == ENABLE)
#define BT_MFI_MEM_SIZE				700
#else
#define BT_MFI_MEM_SIZE				0
#endif

#if (BT_OBEX_SUPPORT == ENABLE)
#define BT_OBEX_MEM_SIZE			400
#else
#define BT_OBEX_MEM_SIZE			0
#endif

#if (BT_PBAP_SUPPORT == ENABLE)
#define BT_PBAP_MEM_SIZE			800
#else
#define BT_PBAP_MEM_SIZE			0
#endif

#define BT_STACK_MEM_SIZE	(BT_BASE_MEM_SIZE + BT_AUDIO_AAC_MEM_SIZE + BT_AVRCP_TG_MEM_SIZE + BT_AVRCP_BRWS_MEM_SIZE +\
							BT_HFP_MEM_SIZE + BT_SPP_MEM_SIZE + BT_BLE_MEM_SIZE + BT_HID_MEM_SIZE + BT_TWS_MEM_SIZE + \
							BT_MFI_MEM_SIZE + BT_OBEX_MEM_SIZE + BT_PBAP_MEM_SIZE)


/*****************************************************************
 * bt 回连参数配置
 * Note: 如不需要开机回连,则即把TRY_COUNTS设置为0即可
 */
// 蓝牙 开机自动重连
#ifdef BT_PROFILE_BQB_ENABLE
	#define BT_POR_TRY_COUNTS			(0)			// 开机重连尝试次数
	#define BT_POR_INTERNAL_TIME		(2)			// 开机重连每两次间隔时间(in seconds)
#else
	#define BT_POR_TRY_COUNTS			(5)			// 开机重连尝试次数
	#define BT_POR_INTERNAL_TIME		(5)			// 开机重连每两次间隔时间(in seconds)
#endif

#ifdef BT_PROFILE_BQB_ENABLE
// 蓝牙 BB Lost之后自动重连
#define BT_BLR_TRY_COUNTS				(2)			// BB Lost 尝试重连次数
#define BT_BLR_INTERNAL_TIME			(10)		// BB Lost 重连每两次间隔时间(in seconds)
#else
#define BT_BLR_TRY_COUNTS				(90)		// BB Lost 尝试重连次数
#define BT_BLR_INTERNAL_TIME			(10)		// BB Lost 重连每两次间隔时间(in seconds)
#endif

#ifdef BT_TWS_SUPPORT
	// TWS 自动回连功能关闭(开机或者切换模式)
	#define BT_TWS_TRY_COUNTS			(5)			// TWS 尝试重连次数 (开机)
	#define BT_TWS_INTERNAL_TIME		(5)			// TWS 重连每两次间隔时间(in seconds)
	
	#define BT_TWS_BLR_TRY_COUNTS		(3)			// TWS Link Loss 尝试重连次数
	#define BT_TWS_BLR_INTERNAL_TIME	(5)			// TWS Link Loss 重连每两次间隔时间(in seconds)
#endif


#ifdef CFG_BT_BACKGROUND_RUN_EN
//注意：该宏定义的开启，必须要蓝牙保持后台
#define BT_FAST_POWER_ON_OFF_FUNC						// 快速打开/关闭蓝牙功能
#endif

//手机端清除配对记录,回连时,被手机端拒绝连接,则同步的清理配对记录,下次不再回连;
//#define BT_AUTO_CLEAR_LAST_PAIRING_LIST

#define BT_MATCH_CHECK					1

#endif /*__BT_DEVICE_CFG_H__*/

