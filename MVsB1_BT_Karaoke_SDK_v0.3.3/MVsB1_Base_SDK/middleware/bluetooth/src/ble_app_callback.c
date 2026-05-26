////////////////////////////////////////////////
//
//
#include "debug.h"

#include "ble_api.h"
#include "ble_app_func.h"

#ifdef CFG_APP_CONFIG
#include "app_config.h"
#include "bt_play_mode.h"
#endif
#include "rtos_api.h"
#include "bt_manager.h"

#if (BLE_SUPPORT == ENABLE)

uint8_t BleConnectFlag=0;


uint8_t return_ble_conn_state()
{
	return BleConnectFlag;
}

extern void BleAdvScanEnable(void);
extern void BleAdvScanDisable(void);
extern void BleOnlyAdv(void);
extern bool GetBtLinkState(void);

void BLEStackCallBackFunc(BLE_CALLBACK_EVENT event, BLE_CALLBACK_PARAMS *params)
{
	switch(event)
	{
		case BLE_STACK_INIT_OK:
			APP_DBG("BLE_STACK_INIT_OK\n");
			BleConnectFlag = 0;

#ifdef BT_TWS_SUPPORT
			BleOnlyAdv();
#endif
			break;
		case BLE_STACK_CONNECTED:
			APP_DBG("BLE_STACK_CONNECTED\n");
			BleConnectFlag = 1;
			break;
			
		case BLE_STACK_DISCONNECTED:
			APP_DBG("BLE_STACK_DISCONNECTED\n");
			BleConnectFlag = 0;

#ifdef BT_TWS_SUPPORT
			BleOnlyAdv();
#endif
			break;

		case GATT_SERVER_INDICATION_TIMEOUT:
			APP_DBG("GATT_SERVER_INDICATION_TIMEOUT\n");
			break;

		case GATT_SERVER_INDICATION_COMPLETE:
			APP_DBG("GATT_SERVER_INDICATION_COMPLETE\n");
			break;

		case BLE_CONN_UPDATE_COMPLETE:
			APP_DBG("BLE_CONN_UPDATE_COMPLETE: \n");
			APP_DBG("conn_intreval:0x%04x, ", params->params.conn_update_params.conn_interval);
			APP_DBG("conn_latency:0x%04x, ", params->params.conn_update_params.conn_latency);
			APP_DBG("supervision_timeout:0x%04x\n", params->params.conn_update_params.supervision_timeout);
			break;

		case BLE_ATT_EXCHANGE_MTU_RESPONSE:
			APP_DBG("BLE_ATT_EXCHANGE_MTU_RESPONSE rx_mtu=%d\n", params->params.rx_mtu);
			break;

		default:
			break;

	}
}

#endif

