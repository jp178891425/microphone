/**
 *************************************************************************************
 * @file	tremolo.h
 * @brief	Tremolo effect for mono signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __TREMOLO_H__
#define __TREMOLO_H__

#include <stdint.h>


/** error code for tremolo */
typedef enum _TREMOLO_ERROR_CODE
{
	TREMOLO_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	TREMOLO_ERROR_ILLEGAL_WAVE_TYPE,
	TREMOLO_ERROR_ILLEGAL_MODULATION_DEPTH,
	TREMOLO_ERROR_ILLEGAL_MODULATION_RATE,

	// No Error
	TREMOLO_ERROR_OK = 0,					/**< no error              */
} TREMOLO_ERROR_CODE;

/** tremolo context */
typedef struct _TremoloContext
{
	int32_t sample_rate;
	//int32_t num_channels;
	int32_t wave_type;
	int32_t mod_count;
	int32_t mod_depth;
	int32_t mod_depth_q15;
	int32_t mod_rate;
	int32_t mod_rate_step;	
} TremoloContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Initialize the tremolo module.
 * @param ct Pointer to a TremoloContext object.
 * @param sample_rate Sample rate.
 * @param wave_type Wave type used in modulation. 0:Sine, 1:Triangular, 2:Square
 * @param mod_depth Modulation depth in percentage. Range: 0 ~ 100 %
 * @param mod_rate Modulation rate in 0.01Hz. For example, 66 for 0.66Hz, 1000 for 10.00Hz. Range: 50~2000 for 0.50~20.00Hz in step of 0.01Hz
 * @return error code. TREMOLO_ERROR_OK means successful, other codes indicate error.
 */
int32_t tremolo_init(TremoloContext *ct, int32_t sample_rate, int32_t wave_type, int32_t mod_depth, int32_t mod_rate);


/**
 * @brief Apply tremolo effect to a frame of 16-bit PCM data (mono only)
 * @param ct Pointer to a TremoloContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer.
 *        pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n Number of PCM samples to process.
 * @param mod_depth Modulation depth in percentage. Range: 0 ~ 100 %
 * @param mod_rate Modulation rate in 0.01Hz. For example, 66 for 0.66Hz, 1000 for 10.00Hz. Range: 50~2000 for 0.50~20.00Hz in step of 0.01Hz
 * @return error code. TREMOLO_ERROR_OK means successful, other codes indicate error.
 * @note Note that only mono signals are accepted.
 */
int32_t tremolo_apply(TremoloContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t mod_depth, int32_t mod_rate);


/**
 * @brief Apply tremolo effect to a frame of 24-bit PCM data (mono only)
 * @param ct Pointer to a TremoloContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer.
 *        pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n Number of PCM samples to process.
 * @param mod_depth Modulation depth in percentage. Range: 0 ~ 100 %
 * @param mod_rate Modulation rate in 0.01Hz. For example, 66 for 0.66Hz, 1000 for 10.00Hz. Range: 50~2000 for 0.50~20.00Hz in step of 0.01Hz
 * @return error code. TREMOLO_ERROR_OK means successful, other codes indicate error.
 * @note Note that only mono signals are accepted.
 */
int32_t tremolo_apply24(TremoloContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int32_t mod_depth, int32_t mod_rate);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __TREMOLO_H__
