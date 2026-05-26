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
#include "irqn.h"
#include "uarts_interface.h"
#include "bt_manager.h"

#ifdef BT_PROFILE_BQB_ENABLE
uint8_t bt_bqb_pts_addr[6];

enum
{
	BT_BQB_CMD_SET_PTS_ADDR = 0,
	BT_BQB_CMD_A2DP_CONNECT,
	BT_BQB_CMD_AVRCP_CONNECT,
	BT_BQB_CMD_HFP_CONNECT,
	BT_BQB_CMD_SCO_CONNECT,
	BT_BQB_CMD_HF_HANGUP,
	BT_BQB_CMD_CONNECT_CTRL,
	BT_BQB_CMD_CLEAR_PAIRED,
	BT_BQB_CMD_PLAY_PAUSE,
	BT_BQB_CMD_STOP,
	BT_BQB_CMD_VOL_DOWN,
	BT_BQB_CMD_VOL_UP,

	BT_BQB_CMD_AVDTP_SMG_BI_38_C,
	BT_BQB_CMD_AVDTP_SMG_BI_33_C,
	BT_BQB_CMD_AVDTP_SMG_BI_05_C,

	BT_BQB_CMD_MAX,
};

const char *bqb_cmd_list[BT_BQB_CMD_MAX] =
{
	"SET_PTS_ADDR",
	"A2DP_CONNECT",
	"AVRCP_CONNECT",
	"HFP_CONNECT",
	"SCO_CONNECT",
	"HANGUP",
	"CONNECT_CTRL",
	"CLEAR_PAIRED",
	"PLAY_PAUSE",
	"STOP",
	"VOL_DOWN",
	"VOL_UP",

	"SMG_BI_38_C",
	"SMG_BI_33_C",
	"SMG_BI_05_C",
};

//void bt_bqb_uart_init(void)
//{
//	GPIO_PortAModeSet(GPIOA5, 1);// UART0 RX
//}
//
//void bt_bqb_uart_rev(void)
//{
//	uint8_t rev_bff[50];
//	memset(rev_bff,0,50);
//	//if(rev_bff[] == )
//}

void bt_bqb_cmd_process(uint8_t *data)
{
	MessageContext		msgSend;
	uint8_t i = 0;
//	APP_DBG("bt_bqb_cmd_process\n");
	for(i = 0;i<BT_BQB_CMD_MAX;i++)
	{
		if(!memcmp( data,bqb_cmd_list[i],strlen(bqb_cmd_list[i])))
		{
			APP_DBG("##R = %s\n",data);
			break;
		}
	}

	if(i >= BT_BQB_CMD_MAX)
	{
		APP_DBG("error %d,%s\n",i,data);
		return;
	}

	switch(i)
	{
		case BT_BQB_CMD_SET_PTS_ADDR:
			APP_DBG("BQB: BT_BQB_CMD_SET_PTS_ADDR\n");
			msgSend.msgId = 0;

			uint8_t len = strlen(bqb_cmd_list[BT_BQB_CMD_SET_PTS_ADDR])+1;
			for(i = 0;i<6;i++)
			{
				bt_bqb_pts_addr[5-i] = 0;

				if(data[len+i*2] >= 'a')
				{
					bt_bqb_pts_addr[5-i] |= (data[len+i*2] - 'a' + 10) << 4;
				}
				else if(data[len+i*2] >= 'A')
				{
					bt_bqb_pts_addr[5-i] |= (data[len+i*2] - 'A' + 10) << 4;
				}
				else
				{
					bt_bqb_pts_addr[5-i] |= (data[len+i*2] - '0') << 4;
				}

				if(data[len+i*2+1] >= 'a')
				{
					bt_bqb_pts_addr[5-i] |= (data[len+i*2+1] - 'a' + 10) & 0x0f;
				}
				else if(data[len+i*2+1] >= 'A')
				{
					bt_bqb_pts_addr[5-i] |= (data[len+i*2+1] - 'A' + 10) & 0x0f;
				}
				else
				{
					bt_bqb_pts_addr[5-i] |= (data[len+i*2+1] - '0') & 0x0f;;
				}
			}

			APP_DBG("BQB: bt_bqb_pts_addr = [%x][%x][%x][%x][%x][%x]\n",bt_bqb_pts_addr[0],bt_bqb_pts_addr[1],bt_bqb_pts_addr[2],bt_bqb_pts_addr[3],bt_bqb_pts_addr[4],bt_bqb_pts_addr[5]);

			memcpy(&btManager.btDdbLastAddr,&bt_bqb_pts_addr,6);
		break;

		case BT_BQB_CMD_A2DP_CONNECT:
			APP_DBG("BQB: BT_BQB_CMD_A2DP_CONNECT\n");
			msgSend.msgId = MSG_A2DP_CONNECT;
		break;

		case BT_BQB_CMD_AVRCP_CONNECT:
			APP_DBG("BQB: BT_BQB_CMD_AVRCP_CONNECT\n");
			msgSend.msgId = MSG_AVRCP_CONNECT;
		break;

		case BT_BQB_CMD_HFP_CONNECT:
			APP_DBG("BQB: BT_BQB_CMD_HFP_CONNECT\n");
			msgSend.msgId = MSG_HFP_CONNECT;
		break;

		case BT_BQB_CMD_SCO_CONNECT:
			APP_DBG("BQB: BT_BQB_CMD_SCO_CONNECT\n");
			msgSend.msgId = MSG_SCO_CONNECT;
		break;

		case BT_BQB_CMD_HF_HANGUP:
			APP_DBG("call hangup\n");
			HfpHangup();
			break;

		case BT_BQB_CMD_CONNECT_CTRL:
			APP_DBG("BQB: BT_BQB_CMD_CONNECT_CTRL\n");
			msgSend.msgId = MSG_BT_CONNECT_CTRL;
		break;

		case BT_BQB_CMD_CLEAR_PAIRED:
			APP_DBG("BQB: BT_BQB_CMD_CLEAR_PAIRED\n");
			msgSend.msgId = MSG_CLR_PAIRED_LIST;
		break;

		case BT_BQB_CMD_PLAY_PAUSE:
			APP_DBG("BQB: BT_BQB_CMD_PLAY_PAUSE\n");
			msgSend.msgId = MSG_PLAY_PAUSE;
		break;

		case BT_BQB_CMD_STOP:
			APP_DBG("BQB: BT_BQB_CMD_STOP\n");
			msgSend.msgId = MSG_AVRCP_STOP;
		break;

		case BT_BQB_CMD_VOL_DOWN:
			APP_DBG("BQB: BT_BQB_CMD_VOL_DOWN\n");
			msgSend.msgId = MSG_MUSIC_VOLDOWN;
		break;

		case BT_BQB_CMD_VOL_UP:
			APP_DBG("BQB: BT_BQB_CMD_VOL_UP\n");
			msgSend.msgId = MSG_MUSIC_VOLUP;
		break;


		case BT_BQB_CMD_AVDTP_SMG_BI_38_C:
			APP_DBG("BQB: BT_BQB_CMD_AVDTP_SMG_BI_38_C\n");
			msgSend.msgId = MSG_BT_BQB_AVDTP_SMG_BI38C;
		break;

		case BT_BQB_CMD_AVDTP_SMG_BI_33_C:
			APP_DBG("BQB: BT_BQB_CMD_AVDTP_SMG_BI_33_C\n");
			msgSend.msgId = MSG_BT_BQB_AVDTP_SMG;
		break;

		case BT_BQB_CMD_AVDTP_SMG_BI_05_C:
			APP_DBG("BQB: BT_BQB_CMD_AVDTP_SMG_BI_33_C\n");
			msgSend.msgId = MSG_BT_BQB_AVDTP_SMG;
		break;
	}
	MessageSend(GetMainMessageHandle(), &msgSend);
}

#endif
