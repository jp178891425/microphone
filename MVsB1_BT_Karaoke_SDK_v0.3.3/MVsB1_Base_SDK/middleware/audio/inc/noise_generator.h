/**
 *************************************************************************************
 * @file	noise_generator.h
 * @brief	Noise generator with mono output.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __NOISE_GENERATOR_H__
#define __NOISE_GENERATOR_H__

#include <stdint.h>

/** error code for Noise Generator */
typedef enum _NOISE_GENERATOR_ERROR_CODE
{
    NOISE_GENERATOR_ERROR_ILLEGAL_NOISE_TYPE = -256,
    NOISE_GENERATOR_ERROR_AMPLITUDE_OUT_OF_RANGE,

    // No Error
    NOISE_GENERATOR_ERROR_OK = 0, /**< no error              */
} NOISE_GENERATOR_ERROR_CODE;

/** Noise Generator Context */
typedef struct _NoiseGeneratorContext
{
    //int32_t sample_rate;
    //int32_t num_channels;
    int32_t noise_type;
    int32_t amplitude;
    int32_t amplitude_scale;
    int32_t b[7];
    
} NoiseGeneratorContext;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/**
 * @brief Initialize the noise generator context.
 * @param ct Pointer to a NoiseGeneratorContext object.
 * @param noise_type Noise type. 0: white noise, 1: pink noise, 2: brown noise
 * @param amplitude Output amplitude in 0.1dB. Range: -900 ~ 0 for -90.0dB ~ 0.0dB
 * @return error code. NOISE_GENERATOR_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported.
 */
int32_t noise_generator_init(NoiseGeneratorContext*ct, int32_t noise_type, int32_t amplitude);

/**
 * @brief Generation of the specified type of noise (16-bit).
 * @param ct Pointer to a NoiseGeneratorContext object.
 * @param pcm_out PCM output buffer.
 * @param n Number of PCM samples to process. 
 * @param amplitude Output amplitude in 0.1dB. Range: -900 ~ 0 for -90.0dB ~ 0.0dB
 * @return error code. NOISE_GENERATOR_ERROR_OK means successful, other codes indicate error.
 */
int32_t noise_generator_apply16(NoiseGeneratorContext*ct, int16_t *pcm_out, int32_t n, int32_t amplitude);

/**
 * @brief Generation of the specified type of noise (24-bit).
 * @param ct Pointer to a NoiseGeneratorContext object.
 * @param pcm_out PCM output buffer.
 * @param n Number of PCM samples to process.
 * @param amplitude Output amplitude in 0.1dB. Range: -900 ~ 0 for -90.0dB ~ 0.0dB
 * @return error code. NOISE_GENERATOR_ERROR_OK means successful, other codes indicate error.
 */
int32_t noise_generator_apply24(NoiseGeneratorContext*ct, int32_t *pcm_out, int32_t n, int32_t amplitude);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__NOISE_GENERATOR_H__
