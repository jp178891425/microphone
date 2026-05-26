/**
 *************************************************************************************
 * @file	howling_suppressor_specified.h
 * @brief	Howling detection & suppression with specified suppression filters.
 *
 * @author	Zhao Ying (Alfred)
 * @version	V4.1.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef _HOWLING_SUPPRESSOR_SPECIFIED_H
#define _HOWLING_SUPPRESSOR_SPECIFIED_H

#include <stdint.h>


#define MAX_PAST_FRAMES 12
#define MAX_ACTIVE_FILTERS 4
#define MAX_SPECIFIED_FILTERS 6

 /** error code for howling suppressor with specified suppression filters  */
typedef enum _HOWLING_SUPPRESSOR_SPECIFIED_ERROR_CODE
{
	HOWLING_SUPPRESSOR_SPECIFIED_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	HOWLING_SUPPRESSOR_SPECIFIED_ERROR_ILLEGAL_FILTER_Q,
	HOWLING_SUPPRESSOR_SPECIFIED_ERROR_ILLEGAL_FILTER_CENTER_FREQUENCY,
	HOWLING_SUPPRESSOR_SPECIFIED_ERROR_ILLEGAL_FILTER_DEPTH,
	HOWLING_SUPPRESSOR_SPECIFIED_ERROR_NUM_SPECIFIED_FILTERS_OUT_OF_RANGE,
	HOWLING_SUPPRESSOR_SPECIFIED_ERROR_FILTER_BANDWIDTHS_OVERLAP,

	// No Error
	HOWLING_SUPPRESSOR_SPECIFIED_ERROR_OK = 0,	/**< no error */
} HOWLING_SUPPRESSOR_SPECIFIED_ERROR_CODE;

typedef struct _HowlingSpecifiedContext
{	
	int32_t sample_rate;
	int32_t lowfreq_limit;
	int32_t highfreq_limit;
	int32_t ibuf;
	int32_t howling_detected;
	int32_t lpd[4];
	//int16_t pcm_win[512];
	int32_t pcm_win[512];
	int16_t howling_candidates[MAX_PAST_FRAMES *MAX_ACTIVE_FILTERS];
	int32_t fc[MAX_ACTIVE_FILTERS];
	int32_t fc_new[MAX_ACTIVE_FILTERS];
	float   fchz[MAX_ACTIVE_FILTERS];
	int32_t fba[MAX_ACTIVE_FILTERS][4];
	int32_t d[MAX_ACTIVE_FILTERS][4];
	int32_t dp[MAX_ACTIVE_FILTERS][4];
	int16_t pcm_buf[512 * 2];
	int32_t x[512 * 2];
	uint8_t peaks[512];
	uint8_t covered[512];
	int32_t coef[MAX_SPECIFIED_FILTERS][4];
} HowlingSpecifiedContext;



#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Initialize howling suppressor module with specified suppression filters.
 * @param ct Pointer to a HowlingSpecifiedContext object. 
 * @param sample_rate Sample rate. Range: 8000 ~ 48000 Hz
 * @param num_specified_filters Number of specified filters for howling suppression. This number should not exceed MAX_SPECIFIED_FILTERS.
 *        If 0 is set, no howling suppression can be done.
 * @param center_freq An array of center frequencies in Hz of the suppression filters. Range: 2 ~ sample_rate/2-2 Hz.
 * @param Q An array of Q factors of the suppression filters in Q10.6 format. Range: 1 ~ 32767
 *        For example, 181 stands for 2.828125, 640 stands for 10.0, 16385 stands for 256.015625
 *        Q is defined as the ratio of center frequency over bandwidth of the filter.
 *        The bandwidth is defined as the half suppression range around the center frequency, i.e. at both edges of the bandwidth, 
 *        the suppression is only half that at the center frequency.
 * @param depth An array of suppression depths in dB of the suppression filters. Range: -100 ~ 0 dB.
 * @return error code. HOWLING_SUPPRESSOR_SPECIFIED_ERROR_OK means successful, other codes indicate error.
 */
int32_t howling_suppressor_specified_init(HowlingSpecifiedContext* ct, int32_t sample_rate, int32_t num_specified_filters, int32_t *center_freq, int32_t *Q, int32_t *depth);


/**
 * @brief Apply howling detection & suppression with specified suppression filters to a frame of PCM data (16-bit).
 * @param ct Pointer to a HowlingSpecifiedContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n Number of PCM samples to process. It should be an even number between 128 ~ 512 (both inclusive). 
 *        It is recommended to keep n constant between frames although it can be varied. Usually, the smaller the value of n, 
 *        the greater the amount of MCPS consumed.
 * @return error code. HOWLING_SUPPRESSOR_SPECIFIED_ERROR_OK means successful, other codes indicate error.
 * @note Input signals should be mono.  
 */
int32_t howling_suppressor_specified_apply(HowlingSpecifiedContext*ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply howling detection & suppression with specified suppression filters to a frame of PCM data (24-bit).
 * @param ct Pointer to a HowlingSpecifiedContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n Number of PCM samples to process. It should be an even number between 128 ~ 512 (both inclusive).
 *        It is recommended to keep n constant between frames although it can be varied. Usually, the smaller the value of n,
 *        the greater the amount of MCPS consumed.
 * @return error code. HOWLING_SUPPRESSOR_SPECIFIED_ERROR_OK means successful, other codes indicate error.
 * @note Input signals should be mono.
 */
int32_t howling_suppressor_specified_apply24(HowlingSpecifiedContext*ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _HOWLING_SUPPRESSOR_SPECIFIED_H
