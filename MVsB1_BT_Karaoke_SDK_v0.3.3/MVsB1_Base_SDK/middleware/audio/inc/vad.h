/**
 *************************************************************************************
 * @file	vad.h
 * @brief	Voice Activity Detector (VAD) for mono input.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __VAD_H__
#define __VAD_H__

#include <stdint.h>


/** error code for VAD */
typedef enum _VAD_ERROR_CODE
{
    VAD_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
    VAD_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED,
    VAD_ERROR_SCRATCH_NOT_AVAILABLE,
    VAD_ERROR_INVALID_FRAME_LENGTH,
    VAD_ERROR_CONTEXT_NOT_INITIALIZED,

    // No Error
    VAD_ERROR_OK = 0,					/**< no error              */
} VAD_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Estimate the memory usage of Voice Activity Detector(VAD) before actual initialization.
 * @param[in] sample_rate sample rate. Supported sample rates: 8000, 16000, 22050, 32000, 44100, 48000 Hz.
 * @param[out] persistent_size  size of persistent memory in bytes. The contents of this memory cannot be modified or cleared between calls to vad_apply().
 * @param[out] scratch_size  size of scratch memory in bytes. The contents of this memory can be modified or cleared for other use between calls to vad_apply().
 * @return error code. VAD_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before vad_init() to estimate the memory required for VAD context object.
 */
int32_t vad_estimate_memory_usage(int32_t sample_rate, uint32_t* persistent_size, uint32_t* scratch_size);


/**
 * @brief Initialize Voice Activity Detector(VAD) module
 * @param ct Pointer to a VAD context object (persistent_size in bytes).
 * @param scratch Pointer to the scratch area (scratch_size in bytes).
 * @param sample_rate sample rate. Supported sample rates: 8000, 16000, 22050, 32000, 44100, 48000 Hz.
 * @param post_processing Whether post-processing is enabled after frame-level VAD. 0:disabled, 1:enabled.
 * @return error code. VAD_ERROR_OK means successful, other codes indicate error.
 * @note Post-processing will make a comprehensive judgement based on the VAD processing results of the current 
 *       frame as well as past frames to improve the accuracy of the VAD output. If post-processing is disabled, 
 *       the frame-level raw VAD processing results will be given directly, and users can do their own post-processing 
 *       algorithm based on that.
 */
int32_t vad_init(uint8_t* ct, uint8_t* scratch, int32_t sample_rate, int32_t post_processing);


/**
 * @brief Apply voice activity detection (VAD) to a frame of PCM data (mono only)
 * @param ct Pointer to a VAD context object (persistent_size in bytes)
 * @param pcm_in Address of the PCM input.
 * @param n Number of PCM samples to process. n = sample_rate/100, i.e. 10 ms in time.
 *        for sample_rate = 48000, n = 480 samples.
 *        for sample_rate = 44100, n = 440 samples. <<< Here n is 440, NOT 441!
 *        for sample_rate = 32000, n = 320 samples.
 *        for sample_rate = 22050, n = 220 samples. <<< Here n is 220, NOT 221!
 *        for sample_rate = 16000, n = 160 samples.
 *        for sample_rate =  8000, n =  80 samples.
 * @return whether current frame is consider voiced (1) or unvoiced (0). If the returned value is less than 0, error occurs.
 */
int32_t vad_apply(uint8_t* ct, int16_t *pcm_in, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__VAD_H__
