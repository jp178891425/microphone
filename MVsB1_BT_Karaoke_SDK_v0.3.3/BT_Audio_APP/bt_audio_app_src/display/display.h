/*
 * @Author: wujunpeng
 * @Date: 2026-05-26 18:06:11
 * @LastEditors: Do not edit
 * @LastEditTime: 2026-05-26 18:17:26
 * @FilePath: \microphone\MVsB1_BT_Karaoke_SDK_v0.3.3\BT_Audio_APP\bt_audio_app_src\display\display.h
 */
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "type.h"
#include "timeout.h"

void DispInit(bool IsStandBy);

// 主函数调用.
void Display(uint16_t msgRecv);


#endif
