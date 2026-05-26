/*
 * spdif_mix_api.h
 *
 *  Created on: Mar 1, 2022
 *      Author: szsj-1
 */

#ifndef BT_AUDIO_APP_SRC_INC_SPDIF_MIX_API_H_
#define BT_AUDIO_APP_SRC_INC_SPDIF_MIX_API_H_

uint16_t AudioSpdif_DataGetX(void* Buf, uint16_t Len);
uint16_t AudioSpdif_DataLenGetX(void);
int32_t Get_Resampler_Polyphase(uint32_t resampler);
void SpdifDataCarry(void);
void AudioSpdif_SRAInit(void);
uint16_t AudioSpdif_SRAProcess(int16_t *InBuf, uint16_t InLen);
void AudioSpdif_SRAStepCnt(void);
void AudioSpdif_DataInProcess(void);
void AudioSpdif_Release(void);
void AudioSpdif_ResMalloc(uint16_t SampleLen);
void AudioSpdif_ConfigInit(void);
#endif /* BT_AUDIO_APP_SRC_INC_SPDIF_MIX_API_H_ */
