/**
 **************************************************************************************
 * @file    remind_sound_service.h
 * @brief   
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2017-2-26 13:06:47$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __REMIND_SOUND_SERVICE_H__
#define __REMIND_SOUND_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#include "decoder_service.h"

#include "remind_sound_item.h"



#define REMIND_SOUND_SERVICE_AUDIO_DECODER_IN_BUF_SIZE	1024 * 19

MessageHandle GetRemindSoundServiceMessageHandle(void);

int32_t RemindSoundServiceCreate(MessageHandle parentMsgHandle);

void RemindSoundServiceStart(void);

void RemindSoundServicePause(void);

void RemindSoundServiceResume(void);

void RemindSoundServiceStop(void);

void RemindSoundServiceKill(void);


//#define REMIND_ATTR_MIX				0x80
//#define REMIND_ATTR_HOLD				0x40	//用于BlockPlay需要保留未完成的可被打断提示音。
//#define REMIND_ATTR_CLEAR				0x20	//清空可被打断提示音
//#define REMIND_ATTR_MUTE_APP			0x10

#define REMIND_ATTR_SYS					0x04	//抢先播，顺序，期间不能切出模式，不能清空。
#define REMIND_ATTR_ORDER				0x02	//顺序播放 切模式可打断和清空

#define REMIND_ATTR_NORMAL				0x01	//可被新提示音打断或覆盖

#define REMIND_ATTR_NONE				0

//SoundItem:提示音请求条目字符串。
//Attr: 播放条目 的配置，参见REMIND_ATTR_ 属性
bool RemindSoundServiceItemRequest(char *SoundItem, uint32_t Attr);

#define NO_REMIND			(char*)""//空提示音 。
//TRUE 提示音正播放 FALSE：未在播放提示音
bool RemindSoundServiceIsPlaying(void);

bool RemindSoundEmptySys(void);

void RemindSoundServicePlay(void);

void RemindSoundServiceReset(void);

void RemindSoundServiceEnd(void);

void RemindSoundServicePlayEnd(void);


bool sound_clips_all_crc(void);


void RemindMixStop(void);

bool RemindMixServiceRequestPlayStatus(void);

bool RemindMixServiceStatus(void);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif /* __REMIND_SOUND_SERVICE_H__ */

