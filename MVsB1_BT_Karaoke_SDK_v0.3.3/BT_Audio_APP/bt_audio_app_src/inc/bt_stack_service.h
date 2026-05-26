/**
 **************************************************************************************
 * @file    bt_stack_service.h
 * @brief   
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BT_STACK_SERVICE_H__
#define __BT_STACK_SERVICE_H__

#include "type.h"
#include "rtos_api.h"

uint8_t GetBtStackCt(void);

MessageHandle GetBtStackServiceMsgHandle(void);

TaskState GetBtStackServiceState(void);

/**
 * @brief	Start bluetooth stack service.
 * @param	NONE
 * @return  
 */
bool BtStackServiceStart(void);


/**
 * @brief	Kill bluetooth stack service.
 * @param	NONE
 * @return  
 */
bool BtStackServiceKill(void);


void BtBbStart(void);

void BT_IntDisable(void);

void BT_ModuleClose(void);

void BtStackServiceWaitResume(void);

void BtFastPowerOff(void);
void BtFastPowerOn(void);

#ifdef	BT_SNIFF_ENABLE

void BtStartEnterSniffMode(void);
void BtStartEnterSniffStep(void);
void BtExitSniffReconnectPhone(void);

void BtExitSniffReconnectFlagSet(void);


#endif//BT_SNIFF_ENABLE

void BtEnterDutModeFunc(void);

//TWS功能sniff唤醒时ADDA是否准备好的标志
//1bit:Master OK   2bit:Slave OK
void BtSniffADDAReadySet(uint8_t set);
uint8_t BtSniffADDAReadyGet();


void BtLocalDeviceNameUpdate(uint8_t *deviceName);

void BleAdvScanEnable(void);
void BleOnlyAdv(void);
void BleOnlyScan(void);
void BleAdvScanDisable(void);


void BtTwsConnectApi(void);
void BtTwsDisconnectApi(void);

#endif //__AUDIO_CORE_SERVICE_H__

