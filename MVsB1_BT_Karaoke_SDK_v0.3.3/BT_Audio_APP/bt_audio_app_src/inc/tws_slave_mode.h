/**
 **************************************************************************************
 * @file    tws slave mode.h
 * @brief    
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2020-3-6 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __TWS_SLAVE_MODE_H__
#define __TWS_SLAVE_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 


MessageHandle GetTwsSlaveMessageHandle(void);

bool TwsSlavePlayCreate(MessageHandle parentMsgHandle);

bool TwsSlavePlayKill(void);

bool TwsSlavePlayStart(void);

bool TwsSlavePlayPause(void);

bool TwsSlavePlayResume(void);

bool TwsSlavePlayStop(void);

void TwsSlaveModeEnter(void);

void TwsSlaveModeExit(void);

//#define	DEBUG_TWS_PACKET 打开时有效
extern uint8_t DebugTwsState;
extern uint32_t	DebugTwsDataCount;
extern uint32_t	DebugTwsDataMax;
extern uint32_t	DebugTwsDataMin;
extern uint32_t	DebugTwsDataTotal;
extern uint16_t	DebugTwsTime;
extern bool		DebugTwsLog;

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __TWS_SLAVE_MODE_H__
