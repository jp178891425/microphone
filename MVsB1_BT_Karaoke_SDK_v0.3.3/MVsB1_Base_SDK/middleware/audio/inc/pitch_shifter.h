/**
 *************************************************************************************
 * @file	pitch_shifter.h
 * @brief	Pitch Shifter
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.8.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __PITCH_SHIFTER_H__
#define __PITCH_SHIFTER_H__

#include <stdint.h>

#define MAX_W_SIZE 576	// Maximum w size which is related to sample rate
//#define MAX_W_SIZE 1152	// Maximum w size which is related to sample rate

#define UPPER_SEMITONE_LIMIT 120
#define LOWER_SEMITONE_LIMIT -120
#define MAX_SA_SIZE MAX_W_SIZE // Maximum sa size (down to -12 semitones @ 48kHz)
//#define MAX_SA_SIZE 1152 // Maximum sa size (down to -12 semitones @ 48kHz)
//#define LOWER_SEMITONE_LIMIT -180
//#define MAX_SA_SIZE 1632 // Maximum sa size (down to -18 semitones @ 48kHz)


/** error code for Pitch Shifter */
typedef enum _PITCHSHIFTER_ERROR_CODE
{
    PITCHSHIFTER_ERROR_UNSUPPORTED_NUMBER_OF_CHANNELS = -256,
	PITCHSHIFTER_ERROR_FRAME_SIZE_TOO_LARGE,
	PITCHSHIFTER_ERROR_FRAME_SIZE_NOT_EVEN,
	PITCHSHIFTER_ERROR_STEPS_OUT_OF_RANGE,
	// No Error
	PITCHSHIFTER_ERROR_OK = 0,					/**< no error              */
} PITCHSHIFTER_ERROR_CODE;


/** Pitch Shifter context for 16-bit */
typedef struct _PitchShifterContext16
{
	int32_t num_channels;							// number of channels
	int32_t sample_rate;							// sample rate
	int32_t sa;										// input frame distance
	int32_t ss;										// output frame distance
	int32_t w;										// size for analysis/synthesis
	int32_t wov;									// overlap size
	int32_t shift_bits;								// shift bits
	int32_t kmax;									// max. search range for k
	int32_t xdecim;									// decimation factor for input
	int32_t ni[MAX_SA_SIZE];						// new position	
	int16_t xfwin[MAX_W_SIZE];						// overlapping-window	
	// ----- 16-bit buffers -----
	int16_t inbuf[MAX_W_SIZE * 4 * 2];				// input buffer	
	int16_t inbufc[MAX_W_SIZE * 3];					// input calculation buffer
	int16_t outbuf[MAX_W_SIZE * 2 * 2];				// output buffer	
	int16_t outbufc[MAX_W_SIZE];					// output calculation buffer	
	int16_t lastPCM[2];								// last PCM samples
} PitchShifterContext16;


/** Pitch Shifter context for 24-bit */
typedef struct _PitchShifterContext24
{
	int32_t num_channels;							// number of channels
	int32_t sample_rate;							// sample rate
	int32_t sa;										// input frame distance
	int32_t ss;										// output frame distance
	int32_t w;										// size for analysis/synthesis
	int32_t wov;									// overlap size
	int32_t shift_bits;								// shift bits
	int32_t kmax;									// max. search range for k
	int32_t xdecim;									// decimation factor for input
	int32_t ni[MAX_SA_SIZE];						// new position
	int16_t xfwin[MAX_W_SIZE];						// overlapping-window	
	// ----- 24-bit buffers -----
	int32_t inbuf[MAX_W_SIZE * 4 * 2];				// input buffer	
	int32_t inbufc[MAX_W_SIZE * 3];					// input calculation buffer
	int32_t outbuf[MAX_W_SIZE * 2 * 2];				// output buffer	
	int32_t outbufc[MAX_W_SIZE];					// output calculation buffer	
	int32_t lastPCM[2];								// last PCM samples
} PitchShifterContext24;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize Pitch Shifter module for 16-bit PCM
 * @param ct Pointer to a PitchShifterContext16 object.
 * @param num_channels Number of channels. 
 * @param sample_rate Sample rate.
 * @param semitone_steps Number of steps to change in unit of 0.1 semitone. Range: LOWER_SEMITONE_LIMIT ~ UPPER_SEMITONE_LIMIT
 * @param w size for analysis/synthesis. Maximum value allowed is defined by MAX_W_SIZE. w must be an even number.
 * @return error code. PITCHSHIFTER_ERROR_OK means successful, other codes indicate error. 
 * @note Upon successful initialization, the length of input/output PCM samples per frame = w/2.
 */
int32_t pitch_shifter_init16(PitchShifterContext16 *ct, int32_t num_channels, int32_t sample_rate, int32_t semitone_steps, int32_t w);

/**
 * @brief Initialize Pitch Shifter module for 24-bit PCM
 * @param ct Pointer to a PitchShifterContext24 object.
 * @param num_channels Number of channels.
 * @param sample_rate Sample rate.
 * @param semitone_steps Number of steps to change in unit of 0.1 semitone. Range: LOWER_SEMITONE_LIMIT ~ UPPER_SEMITONE_LIMIT
 * @param w size for analysis/synthesis. Maximum value allowed is defined by MAX_W_SIZE. w must be an even number.
 * @return error code. PITCHSHIFTER_ERROR_OK means successful, other codes indicate error.
 * @note Upon successful initialization, the length of input/output PCM samples per frame = w/2.
 */
int32_t pitch_shifter_init24(PitchShifterContext24 *ct, int32_t num_channels, int32_t sample_rate, int32_t semitone_steps, int32_t w);

/**
 * @brief Configure Pitch Shifter module for 16-bit PCM
 * @param ct Pointer to a PitchShifterContext16 object.
 * @param semitone_steps Number of steps to change in unit of 0.1 semitone. Range: LOWER_SEMITONE_LIMIT ~ UPPER_SEMITONE_LIMIT
 */
int32_t pitch_shifter_configure16(PitchShifterContext16 *ct, int32_t semitone_steps);

/**
 * @brief Configure Pitch Shifter module for 24-bit PCM
 * @param ct Pointer to a PitchShifterContext24 object.
 * @param semitone_steps Number of steps to change in unit of 0.1 semitone. Range: LOWER_SEMITONE_LIMIT ~ UPPER_SEMITONE_LIMIT
 */
int32_t pitch_shifter_configure24(PitchShifterContext24 *ct, int32_t semitone_steps);

/**
 * @brief Apply pitch shifting to a frame of 16-bit PCM (length of input/output PCM samples per frame = w/2)
 * @param ct Pointer to a PitchShifterContext16 object.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format, i.e. for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The PCM layout is the same as in Microsoft WAVE format, i.e. for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @return error code. PITCHSHIFTER_ERROR_OK means successful, other codes indicate error.
 */
int32_t pitch_shifter_apply16(PitchShifterContext16 *ct, int16_t *pcm_in, int16_t *pcm_out);

/**
 * @brief Apply pitch shifting to a frame of 24-bit PCM (length of input/output PCM samples per frame = w/2)
 * @param ct Pointer to a PitchShifterContext24 object.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format, i.e. for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The PCM layout is the same as in Microsoft WAVE format, i.e. for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @return error code. PITCHSHIFTER_ERROR_OK means successful, other codes indicate error.
 */
int32_t pitch_shifter_apply24(PitchShifterContext24 *ct, int32_t *pcm_in, int32_t *pcm_out);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__PITCH_SHIFTER_H__
