/**
 *************************************************************************************
 * @file	robot_tone.h
 * @brief	Robot tone-like effect for mono signals at high sample rate.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __ROBOT_TONE_H__
#define __ROBOT_TONE_H__

#include <stdint.h>

/** error code for robot tone */
typedef enum _ROBOT_TONE_ERROR_CODE
{
    ROBOT_TONE_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
    ROBOT_TONE_ERROR_ILLEGAL_STEP_SIZE,

    // No Error
    ROBOT_TONE_ERROR_OK = 0, /**< no error              */
} ROBOT_TONE_ERROR_CODE;

/** Robot Tone Context */
typedef struct _RobotToneContext
{
    int32_t sample_rate;
    // int32_t num_channels;
    int32_t n1;
    int32_t lx;

    int32_t x[1024];
    int32_t xlast; // must be put here
    int32_t pcm_x[1][1024];
    int32_t pcm_y[1][1024];
    //int32_t r[512];
} RobotToneContext;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/**
 * @brief Initialize the robot tone context.
 * @param ct Pointer to a RobotToneContext object.
 * @param sample_rate Sample rate. Range: >= 32000 Hz.
 * @param step_size Step size in samples. Range: 128 ~ 512. This is the number of samples to process in each loop.
 * The choice of step_size not only affects the CPU usage, but also the effect itself, i.e. the tone.
 * Usually 1/100 of the sample rate is a good choice for this effect. For example, if sample_rate equals 44100,
 * setting step_size=441 will generate a robot tone around 44100/441=100 Hz.
 * @return error code. ROBOT_TONE_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported.
 */
int32_t robot_tone_init(RobotToneContext *ct, int32_t sample_rate, int32_t step_size);

/**
 * @brief Apply the robot tone effect (16-bit).
 * @param ct Pointer to a RobotToneContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @return error code. ROBOT_TONE_ERROR_OK means successful, other codes indicate error.
 * @note Note that the number of PCM samples per frame is equal to the step_size set in robot_tone_init(...).
 */
int32_t robot_tone_apply16(RobotToneContext *ct, int16_t *pcm_in, int16_t *pcm_out);

/**
 * @brief Apply the robot tone effect (24-bit).
 * @param ct Pointer to a RobotToneContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @return error code. ROBOT_TONE_ERROR_OK means successful, other codes indicate error.
 * @note Note that the number of PCM samples per frame is equal to the step_size set in robot_tone_init(...).
 */
int32_t robot_tone_apply24(RobotToneContext *ct, int32_t *pcm_in, int32_t *pcm_out);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__ROBOT_TONE_H__
