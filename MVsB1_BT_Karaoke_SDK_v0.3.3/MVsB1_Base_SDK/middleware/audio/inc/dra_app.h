/**
 *************************************************************************************
 * @file	dra_app.h
 * @brief	dra postprocessing audio effect (stereo widen & CTC).
 *
 * @author	Digirise
 * @version	v1.0.0
 *
 * &copy; Guangzhou digirise Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __DRA_APP_H__
#define __DRA_APP_H__

#include <stdint.h>

#define DRA_PP_PERSISTENT_MEMORY_SIZE			(37888)
#define DRA_PP_SCRATCH_MEMORY_SIZE				(24600)


typedef enum _DRA_APP_ERROR_CODE
{
    DRA_APP_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	DRA_APP_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED,
	DRA_APP_ERROR_ILLEGAL_CUTOFF_FREQ,
	DRA_APP_ERROR_ILLEGAL_INTENSITY_VALUES,
	// No Error
	DRA_APP_ERROR_OK = 0,					/**< no error              */
} DRA_APP_ERROR_CODE;

typedef struct __DraPostProcessingParam
{
//	int32_t nEffectActive;
//	int32_t nFreq1;
//	int32_t nFreq2;
//	int32_t nFreq3;
//	int32_t nFreq4;
//	int32_t nWidenCenter;
//	int32_t nWidenGain;
//	int32_t nTotalGain;		// 10x
//	int32_t bCTCActive;
//	int32_t nCTCMode;
//
//	int32_t bUpmixActive;
//
//	int32_t fUpmixLRGainCoef;	// 10x
//	int32_t fUpmixTlTrGainCoef;	// 10x
//	int32_t fUpmixGainCoef;		// 10x
    int32_t nEffectActive;
    int32_t nFreq1;
    int32_t nFreq2;
    int32_t nFreq3;
    int32_t nFreq4;
    int32_t nWidenCenter;
    int32_t nWidenGain;
    int32_t nTotalGain;
    int32_t bCTCActive;
    int32_t nCTCMode;

    int32_t bUpmixActive;
    int32_t fUpmixLRGainCoef;			// 10x
	int32_t fUpmixTlTrGainCoef;			// 10x
	int32_t fUpmixGainCoef;				// 10x

	int32_t fSpeechEnhancementGain; 	// 10x

	int32_t bVirtualBassActive;
	int32_t nVBCutOffFreq;
    int32_t fVBGain;

	int32_t nLowFreq1;
	int32_t nLowFreq2;
	int32_t nHarmCutOff;
	int32_t fScale; 						// 10x
	int32_t fGain_f1; 					// 10x

     int32_t nVolume;
     int32_t nDistance;
     int32_t nRoomMode;
} DraPostProcessingParam;

typedef struct _DraPostProcessingContext
{
	void *pDraAudioPostProcessingObj;

	uint32_t draAudioPostProcessingPersistentMemory[DRA_PP_PERSISTENT_MEMORY_SIZE / 4];
	uint32_t draAudioPostProcessingScratchMemory[DRA_PP_SCRATCH_MEMORY_SIZE / 4];

	int32_t nReserverd1;
	int32_t nReserverd2;
} DraPostProcessingContext;


#ifdef __cplusplus
extern "C" {
#endif

int32_t DRA_APP_init(DraPostProcessingContext *pContext, int32_t num_channels, int32_t sample_rate, const DraPostProcessingParam *pParamInput);

int32_t DRA_APP_reset(DraPostProcessingContext *pContext, const DraPostProcessingParam *pParamInput);

int32_t DRA_APP_apply(DraPostProcessingContext *pContext, int16_t *pcm_in, int16_t *pcm_out, int32_t nFrameSize, int32_t nReserverd1, int32_t nReserverd2);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__DRA_APP_H__
