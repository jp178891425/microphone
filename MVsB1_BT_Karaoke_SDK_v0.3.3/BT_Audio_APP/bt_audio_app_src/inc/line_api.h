/*
 * line_api.h
 *
 *  Created on: Apr 13, 2020
 *      Author: szsj-1
 */

#ifndef BT_AUDIO_APP_SRC_INC_LINE_API_H_
#define BT_AUDIO_APP_SRC_INC_LINE_API_H_
uint16_t AudioLine_DataLenGet(void);
uint16_t AudioLine_DataGet(void* Buf, uint16_t Len);
void AudioLine_HWInit(void);
void AudioLine_ConfigInit(void);
void AudioLine_Release(void);
void AudioLine_ResMalloc(uint16_t SampleLen);

void AudioADC_DigitalInit(ADC_MODULE Module, uint32_t SampleRate, void* Buf, uint16_t Len);
uint16_t AudioADC0DataLenGet(void);
uint16_t AudioADC0DataGet(void* Buf, uint16_t Len);

#endif /* BT_AUDIO_APP_SRC_INC_LINE_API_H_ */
