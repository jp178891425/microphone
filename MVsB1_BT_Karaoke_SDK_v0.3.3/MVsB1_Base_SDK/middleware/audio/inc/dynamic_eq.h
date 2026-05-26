/**
 *************************************************************************************
 * @file	dynamic_eq.h
 * @brief	Dynamic EQ
 *
 * @author	ZHAO Ying (Alfred)
 * @version	V1.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef __DYNAMIC_EQ_H__
#define __DYNAMIC_EQ_H__

#include <stdint.h>
#include "eq.h"


/** error code for Dynamic EQ */
typedef enum _DYNAMICEQ_ERROR_CODE
{
	DYNAMICEQ_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	DYNAMICEQ_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED,
	DYNAMICEQ_ERROR_ILLEGAL_LOW_ENERGY_THRESHOLD,
	DYNAMICEQ_ERROR_ILLEGAL_NORMAL_ENERGY_THRESHOLD,
	DYNAMICEQ_ERROR_ILLEGAL_HIGH_ENERGY_THRESHOLD,
	DYNAMICEQ_ERROR_LOW_ENERGY_GREATER_THAN_NORMAL_ENERGY_THRESHOLD,
	DYNAMICEQ_ERROR_NORMAL_ENERGY_GREATER_THAN_HIGH_ENERGY_THRESHOLD,
	DYNAMICEQ_ERROR_LOW_ENERGY_NOT_LESS_THAN_HIGH_ENERGY_THRESHOLD,
	DYNAMICEQ_ERROR_PCM_OUTPUT_ADDRESS_SAME_AS_INPUT,	

	// No Error
	DYNAMICEQ_ERROR_OK = 0,					/**< no error              */
} DYNAMICEQ_ERROR_CODE;

/** Dynamic EQ Context */
typedef struct _DynamicEQContext
{
	int32_t sample_rate;
	int32_t num_channels;
	int32_t low_energy_threshold;
	int32_t normal_energy_threshold;
	int32_t high_energy_threshold;
	int32_t alpha_attack;
	int32_t alpha_release;
	int32_t state;
	int32_t dt_state;
	int32_t dt_alpha_release;
	EQContext* eq_low;
	EQContext* eq_high;
} DynamicEQContext;

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize dynamic EQ module
 * @param ct Pointer to a dynamic EQ context object.
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param sample_rate Sample rate.
 * @param low_energy_threshold The threshold below which EQ for low energy signals will apply.
 *        Set the threshold in step of 0.01dB. Allowable range: -9000 ~ 0 to cover -90.00dB ~ 0.00dB. For example, -4150 stands for -41.50dB threshold.
 * @param normal_energy_threshold The threshold around which no processing will apply.
 *        Set the threshold in step of 0.01dB. Allowable range: -9000 ~ 0 to cover -90.00dB ~ 0.00dB. For example, -2222 stands for -22.22dB threshold.
 * @param high_energy_threshold The threshold above which EQ for high energy signals will apply.
 *        Set the threshold in step of 0.01dB. Allowable range: -9000 ~ 0 to cover -90.00dB ~ 0.00dB. For example, -618 stands for -6.18dB threshold. 
 * @param attack_time Amount of time (in millisecond) the module will go from no processing to full EQ for low or high energy signals.
 * @param release_time Amount of time (in millisecond) the module will go from full EQ for low or high energy signals back to no processing.
 * @param eq_low Pointer to an EQ context object for low energy signals.
 * @param eq_high Pointer to an EQ context object for high energy signals.
 * @return error code. DYNAMICEQ_ERROR_OK means successful, other codes indicate error.
 * @note The following conditions should be satisfied :
 *		      low_energy_threshold <= normal_energy_threshold
 *		   normal_energy_threshold <= high_energy_threshold
 *            low_energy_threshold <  high_energy_threshold
 *       In case that low_energy_threshold == normal_energy_threshold or normal_energy_threshold==high_energy_threshold, the direct
 *       transition from low energy EQ to high energy EQ is performed by skipping the intermediate "no-processing" part.
 *       The illustration shows the concept:
 *       -------------------------Case 1 ------------------------------------
 *       [low energy EQ]    -->    [no processing]    -->    [high energy EQ]
 *               low th.             normal th.              high th.
 *
 *       -------------------------Case 2 ------------------------------------
 *       [low energy EQ]    -->    [high energy EQ]
 *               low th.           normal th.==high th.               
 *
 *       -------------------------Case 3 ------------------------------------
 *                                [low energy EQ]    -->    [high energy EQ]
 *                            low th.==normal th.           high th.
 *       
 *       low th.    : low_energy_threshold
 *       normal th. : normal_energy_threshold
 *       high th.   : highs_energy_threshold
 * 
 *       The module checks the energy level of the signals in pcm_watch and applies dynamic EQ accordingly to the input signals 
 *       in pcm_in and makes the output in pcm_out. The dynamic EQ means the output may contain one of the following cases:
 *       - EQ for low energy
 *       - original input
 *       - EQ for high energy
 *       - mixture of EQ for low energy + original input
 *       - mixture of original input    + EQ for high energy
 *       - mixture of EQ for low energy + EQ for high energy
 */
int32_t dynamic_eq_init(DynamicEQContext* ct, int32_t num_channels, int32_t sample_rate,
	int32_t low_energy_threshold, int32_t normal_energy_threshold, int32_t high_energy_threshold,
	int32_t attack_time, int32_t release_time, EQContext* eq_low, EQContext* eq_high);


/**
 * @brief Apply dynamic EQ to a frame of PCM data (16-bit).
 * @param ct Pointer to a dynamic EQ context object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_watch Address of the PCM to watch. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in.
 * @param n Number of PCM samples to process.
 * @return error code. DYNAMICEQ_ERROR_OK means successful, other codes indicate error.
 */
int32_t dynamic_eq_apply16(DynamicEQContext* ct, int16_t* pcm_in, int16_t* pcm_watch, int16_t* pcm_out, int32_t n);


/**
 * @brief Apply dynamic EQ to a frame of PCM data (24-bit).
 * @param ct Pointer to a dynamic EQ context object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_watch Address of the PCM to watch. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in.
 * @param n Number of PCM samples to process.
 * @return error code. DYNAMICEQ_ERROR_OK means successful, other codes indicate error.
 */
int32_t dynamic_eq_apply24(DynamicEQContext* ct, int32_t* pcm_in, int32_t* pcm_watch, int32_t* pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __DYNAMIC_EQ_H__
