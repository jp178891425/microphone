/**
 *************************************************************************************
 * @file	filter_butterworth.h
 * @brief	Butterworth filter
 *
 * @author	ZHAO Ying (Alfred)
 * @version	V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef __FILTER_BUTTERWORTH_H__
#define __FILTER_BUTTERWORTH_H__

#include <stdint.h>


/** error code for Butterworth filter */
typedef enum _FILTERBUTTERWORTH_ERROR_CODE
{
	FILTERBUTTERWORTH_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	FILTERBUTTERWORTH_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED,
	FILTERBUTTERWORTH_ERROR_ILLEGAL_FILTER_TYPE,
	FILTERBUTTERWORTH_ERROR_ILLEGAL_FILTER_ORDER,
	FILTERBUTTERWORTH_ERROR_ILLEGAL_CUTOFF_FREQUENCY,

	// No Error
	FILTERBUTTERWORTH_ERROR_OK = 0,					/**< no error              */
} FILTERBUTTERWORTH_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Estimate the memory usage of Butterworth filter before actual initialization.
 * @param[in]  filter order     Order of Butterworth filter. Range: 1 ~ 10
 * @param[out] persistent_size  Persistent memory usage in bytes. The contents of this memory cannot be modified or cleared between frame calls.
 * @return error code. FILTERBUTTERWORTH_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before filter_butterworth_init() to estimate the memory required for Butterworth filter context object.
 * Typical memory usage is as follows:
 * -----------------
 * | Order | Bytes |
 * |   1   |   60  |
 * |   2   |   60  |
 * |   3   |  104  |
 * |   4   |  104  |
 * |   5   |  148  |
 * |   6   |  148  |
 * |   7   |  192  |
 * |   8   |  192  |
 * |   9   |  236  |
 * |  10   |  236  |
 * -----------------
 */
int32_t filter_butterworth_estimate_memory_usage(int32_t filter_order, uint32_t* persistent_size);


/**
 * @brief Initialize Butterworth filter module
 * @param ct Pointer to a Butterworth filter context object.
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param sample_rate Sample rate.
 * @param filter_type Type of filter. 0:Low-pass, 1:High-pass
 * @param filter order Order of Butterworth filter. Range: 1 ~ 10
 * @param fc Cut-off frequency in Hz. This frequency should be less than sample_rate/2.
 * @return error code. FILTERBUTTERWORTH_ERROR_OK means successful, other codes indicate error.
 */
int32_t filter_butterworth_init(uint8_t* ct, int32_t num_channels, int32_t sample_rate, int32_t filter_type, int32_t filter_order, int32_t fc);


/**
 * @brief Apply Butterworth filter to a frame of PCM data (16-bit).
 * @param ct Pointer to a Butterworth filter context object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. FILTERBUTTERWORTH_ERROR_OK means successful, other codes indicate error.
 */
int32_t filter_butterworth_apply16(uint8_t* ct, int16_t* pcm_in, int16_t* pcm_out, int32_t n);


/**
 * @brief Apply Butterworth filter to a frame of PCM data (24-bit).
 * @param ct Pointer to a Butterworth filter context object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. FILTERBUTTERWORTH_ERROR_OK means successful, other codes indicate error.
 */
int32_t filter_butterworth_apply24(uint8_t* ct, int32_t* pcm_in, int32_t* pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __FILTER_BUTTERWORTH_H__
