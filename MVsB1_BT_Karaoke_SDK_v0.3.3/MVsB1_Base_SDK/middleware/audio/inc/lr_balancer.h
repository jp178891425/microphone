/**
 *************************************************************************************
 * @file	lr_balancer.h
 * @brief	Balancer for Left & Right Channel
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __LR_BALANCER_H__
#define __LR_BALANCER_H__

#include <stdint.h>


/** error code for LR Balancer */
typedef enum _LR_BALANCER_ERROR_CODE
{
	LR_BALANCER_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	LR_BALANCER_ERROR_NON_POSITIVE_NUMBER_OF_SAMPLES,
	LR_BALANCER_ERROR_ILLEGAL_BALANCE_VALUE,

	// No Error
	LR_BALANCER_ERROR_OK = 0,					/**< no error              */
} LR_BALANCER_ERROR_CODE;


/** LR Balancer Context */
typedef struct _LRBalancerContext
{
	int32_t num_channels;
	
} LRBalancerContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize Balancer for Left & Right Channel.
 * @param ct Pointer to a LRBalancerContext object.
 * @param num_channels number of channels. Choose either 1 or 2. For 1 channel (mono), the output is the same as the input.
 * @return error code. LR_BALANCER_ERROR_OK means successful, other codes indicate error.
 */
int32_t lr_balancer_init(LRBalancerContext* ct, int32_t num_channels);


/**
 * @brief Apply left & right balancing to a frame of PCM data.
 * @param ct Pointer to a LRBalancerContext object.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 * @param pcm_out Address of the PCM output. The PCM layout is the same as in Microsoft WAVE format for both mono and stereo cases.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @param balance Value to control the balancing between left and right channel. range: -100 ~ 100.
 *        Typical outputs are as follows,
 *        -----------------------------
 *        | balance |  Left Output |  Right Output|
 *        -----------------------------
 *        |  -100   |  100% Left   |    0%        | 
 *        |   -50   |  100% Left   |   50% Right  |
 *        |   -25   |  100% Left   |   75% Right  |
 *        |     0   |  100% Left   |  100% Right  |
 *        |    25   |   75% Left   |  100% Right  |
 *        |    50   |   50% Left   |  100% Right  |
 *        |   100   |    0% Left   |  100% Right  |
 *        -----------------------------------------
 * @return error code. LR_BALANCER_ERROR_OK means successful, other codes indicate error.
 */
int32_t lr_balancer_apply16(LRBalancerContext* ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t balance);


/**
 * @brief Apply left & right balancing to a frame of PCM data (24-bit).
 * @param ct Pointer to a LRBalancerContext object.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 * @param pcm_out Address of the PCM output. The PCM layout is the same as in Microsoft WAVE format for both mono and stereo cases.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @param balance Value to control the balancing between left and right channel. range: -100 ~ 100.
 *        Typical outputs are as follows,
 *        -----------------------------------------
 *        | balance |  Left Output |  Right Output|
 *        -----------------------------------------
 *        |  -100   |  100% Left   |    0%        |
 *        |   -50   |  100% Left   |   50% Right  |
 *        |   -25   |  100% Left   |   75% Right  |
 *        |     0   |  100% Left   |  100% Right  |
 *        |    25   |   75% Left   |  100% Right  |
 *        |    50   |   50% Left   |  100% Right  |
 *        |   100   |    0% Left   |  100% Right  |
 *        -----------------------------------------
 * @return error code. LR_BALANCER_ERROR_OK means successful, other codes indicate error.
 */
int32_t lr_balancer_apply24(LRBalancerContext* ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int32_t balance);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__LR_BALANCER_H__
