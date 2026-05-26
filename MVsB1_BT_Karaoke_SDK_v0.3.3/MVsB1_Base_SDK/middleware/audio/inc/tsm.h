/**
 *************************************************************************************
 * @file	tsm.h
 * @brief	Time Scale Modification (TSM)
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.4.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __TSM_H__
#define __TSM_H__

#include <stdint.h>

#define MAX_CH 2
#define TSM_MAX_W_SIZE 1152//1152 // Maximum window size in samples for TSM

/** error code for TSM */
typedef enum _TSM_ERROR_CODE
{
    TSM_ERROR_UNSUPPORTED_NUMBER_OF_CHANNELS = -256,
	TSM_ERROR_FRAME_SIZE_TOO_LARGE,
	TSM_ERROR_FRAME_SIZE_NOT_EVEN,
	TSM_ERROR_SPEED_RATIO_OUT_OF_RANGE,
	// No Error
	TSM_ERROR_OK = 0,					/**< no error              */
} TSM_ERROR_CODE;


/** TSM context */
typedef struct _TSMContext
{
	int32_t num_channels;						// number of channels
	int32_t sample_rate;						// sample rate
	int32_t sa;									// input frame size
	int32_t ss;									// output frame size
	int32_t w;									// frame size for synthesis
	int32_t shift_bits;							// shift bits for wov
	int32_t kmax;								// max. search range for k
	int16_t inbuf[MAX_CH][TSM_MAX_W_SIZE*3];		// input buffer
	int16_t inbufa[TSM_MAX_W_SIZE*2+TSM_MAX_W_SIZE/2];		// input calculation buffer
	int16_t outbuf[MAX_CH][TSM_MAX_W_SIZE];		// output buffer
	int16_t outbufb[TSM_MAX_W_SIZE/2];					// output calculation buffer
	int16_t xfwin[TSM_MAX_W_SIZE/2];				// overlapping-window
} TSMContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize Time Scale Modification (TSM) module
 * @param ct Pointer to a TSMContext object.
 * @param num_channels Number of channels. 
 * @param sample_rate Sample rate.
 * @param speed_ratio speed ratio (new speed / old speed) in Q3.15. e.g. 16384 stands for 0.5 (sound is half slowed down), 65536 stands for 2 (sound is double speeded up). Range of speed_ratio allowed: x0.5(16384) ~ x2(65536)
 * @param w window size for synthesis. Maximum value allowed is defined by MAX_W_SIZE. This value affects not only the quality of output but also the size of input/output frame size.
 * @return error code. TSM_ERROR_OK means successful, other codes indicate error. 
 * @note After initialization, the input and output frame sizes are determined and represented by "sa"(in size) and "ss"(out size) in TSMContext object "ct" respectively.
 */
int32_t tsm_init(TSMContext *ct, int32_t num_channels, int32_t sample_rate, int32_t speed_ratio, int32_t w);


/**
 * @brief Configure Time Scale Modification (TSM) module
 * @param ct Pointer to a TSMContext object.
 * @param speed_ratio speed ratio (new speed / old speed) in Q3.15. e.g. 16384 stands for 0.5 (sound is half slowed down), 65536 stands for 2 (sound is double speeded up). Range of speed_ratio allowed: x0.5(16384) ~ x2(65536)
 * @return error code. TSM_ERROR_OK means successful, other codes indicate error. 
 * @note Once new speed_ratio is set, input frame size "sa" in TSMContext object "ct" will be changed, but output frame size "ss" keeps the same.
 */
int32_t tsm_configure(TSMContext *ct, int32_t speed_ratio);


/**
 * @brief Apply time scale modification to a frame of PCM data (input frame size = ct->sa, output frame size = ct->ss)
 * @param ct Pointer to a TSMContext object.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format, i.e. for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The PCM layout is the same as in Microsoft WAVE format, i.e. for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out cannot be the same as pcm_in, i.e. the PCM signals cannot be changed in-place.
 * @return error code. TSM_ERROR_OK means successful, other codes indicate error.
 */
int32_t tsm_apply(TSMContext *ct, int16_t *pcm_in, int16_t *pcm_out);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__TSM_H__
