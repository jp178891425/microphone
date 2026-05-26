/**
 *************************************************************************************
 * @file	reverb_pro.h
 * @brief	Professional stereo reverberation effect
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.4.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __REVERB_PRO_H__
#define __REVERB_PRO_H__

#include <stdint.h>

/** error code for reverb */
typedef enum _REVERB_PRO_ERROR_CODE
{
    REVERB_PRO_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,    
    REVERB_PRO_ERROR_ILLEGAL_PARAMETER_LEVEL,
    REVERB_PRO_ERROR_ILLEGAL_PARAMETER_EARLY_REFLECTION,
    REVERB_PRO_ERROR_ILLEGAL_PARAMETER_LATE_REVERB,
    REVERB_PRO_ERROR_PERMISSION_NOT_GRANTED,
    REVERB_PRO_ERROR_BIT_DEPTH_NOT_SUPPORTED,

    // No Error
    REVERB_PRO_ERROR_OK = 0,					/**< no error              */
} REVERB_PRO_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Estimate the memory usage of stereo reverberation effect module before actual initialization.
 * @param[in]  bit_depth        PCM bit depth. Set either 16 or 24.
 * @param[in]  sample_rate      Sample rate. Only 3 sample rates are supported, i.e. 32000, 44100 & 48000 Hz.
 * @param[in]  erfactor         early reflection factor [50 to 250] %          [Early Reflection]
 * @param[in]  delay            amount of delay         [0 to 100] millisecond [Late Reverb]
 * @param[out] persistent_size  Persistent memory usage in bytes. The contents of this memory cannot be modified or cleared between frame calls.
 * @return error code. REVERB_PRO_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before reverb_pro_init() to estimate the memory required for reverb context object.
 * Typical memory usage is as follows:
 * -------------------------------------------------------------------------------------------
 * |                                        16-bit PCM                                       |
 * -------------------------------------------------------------------------------------------
 * |    default (erfactor=160, delay=20)        |    maximum (erfactor=250, delay=100)       |
 * | Sample Rate:    32000    44100    48000    | Sample Rate:    32000    44100    48000    |
 * | Bytes:         133300   142788   146160    | Bytes:         152836   169712   175468    |
 * -------------------------------------------------------------------------------------------
 * |                                        24-bit PCM                                       |
 * -------------------------------------------------------------------------------------------
 * |    default (erfactor=160, delay=20)        |    maximum (erfactor=250, delay=100)       |
 * | Sample Rate:    32000    44100    48000    | Sample Rate:    32000    44100    48000    |
 * | Bytes:         152424   169128   174828    | Bytes:         191500   222976   233444    |
 * -------------------------------------------------------------------------------------------
 */
int32_t reverb_pro_estimate_memory_usage(int32_t bit_depth, int32_t sample_rate, int32_t erfactor, int32_t delay, uint32_t* persistent_size);


/**
 * @brief Initialize stereo reverberation effect module.
 * @param ct            Pointer to the reverb context object. 
 *        The context object can be allocated as a global instance or to a buffer created from heap or any free space in memory.
 *        The estimated context size is equal to persistent_size obtained by calling reverb_pro_estimate_memory_usage().
 *        The actual context size used can be checked later by calling reverb_pro_context_size() after successful initialization.
 *        The estimated context size should be no less than the actual context size used.
 * @param bit_depth     PCM bit depth. Set either 16 or 24.
 * @param sample_rate   Sample rate. Only 3 sample rates are supported, i.e. 32000, 44100 & 48000 Hz.
 *
 * ---------- Reverb Parameters ----------
 *
 *           [Level]
 * @param dry           final dry mix [-70 to 10] dB
 * @param wet           final wet(late reverb) mix [-70 to 10] dB
 * @param erwet         final wet(early reflection) mix [-70 to 10] dB
 *
 *           [Early Reflection]
 * @param erfactor      early reflection factor [50 to 250] %
 * @param erwidth       early reflection width [-100 to 100] %
 * @param ertolate      early reflection amount [0 to 100] %
 *
 *           [Late Reverb]
 * @param rt60          reverb time decay [100 to 15000] millisecond
 * @param delay         amount of delay [0 to 100] millisecond
 * @param width         width of reverb L/R mix [0 to 100] %
 *
 * @param wander        LFO(low-frequency oscillator) wander amount [10 to 60] %
 * @param spin          LFO(low-frequency oscillator) spin amount [0 to 1000] %
 *
 * @param inputlpf      lowpass cutoff for input [200 to 18000] Hz
 * @param damplpf       lowpass cutoff for dampening [200 to 18000] Hz
 * @param basslpf       lowpass cutoff for bass [50 to 1050] Hz
 * @param bassb         bass boost [0 to 50] %
 * @param outputlpf     lowpass cutoff for output [200 to 18000] Hz
 * @return error code.  REVERB_PRO_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_pro_init(
    uint8_t *ct, int32_t bit_depth, int32_t sample_rate,
    int32_t dry, int32_t wet, int32_t erwet,
    int32_t erfactor, int32_t erwidth, int32_t ertolate,
    int32_t rt60, int32_t delay, int32_t width, int32_t wander, int32_t spin,
    int32_t inputlpf, int32_t damplpf, int32_t basslpf, int32_t bassb, int32_t outputlpf);


/**
 * @brief Apply stereo reverberation effect to a frame of PCM data (16-bit, 2 channels)
 * @param ct        Pointer to the reverb context object.
 * @param pcm_in    PCM input buffer. The PCM layout is like "L,R,L,R,..."
 * @param pcm_out   PCM output buffer. The PCM layout is like "L,R,L,R,..."
 *        pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n         Number of PCM samples to process. Note that one (L,R) pair is counted as 1.
 * @return error code. REVERB_PRO_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_pro_apply16(uint8_t *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply stereo reverberation effect to a frame of PCM data (24-bit, 2 channels)
 * @param ct        Pointer to the reverb context object.
 * @param pcm_in    PCM input buffer. The PCM layout is like "L,R,L,R,..."
 * @param pcm_out   PCM output buffer. The PCM layout is like "L,R,L,R,..."
 *        pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n         Number of PCM samples to process. Note that one (L,R) pair is counted as 1.
 * @return error code. REVERB_PRO_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_pro_apply24(uint8_t* ct, int32_t* pcm_in, int32_t* pcm_out, int32_t n);


/**
 * @brief Obtain the size of reverb context.
 * @param ct Pointer to the reverb context object.
 * @return the size of reverb context in bytes.
 * @note Do not call this function before reverb_pro_init() is called. 
 *       The returned size is only valid after reverb_pro_init() is called.
 */
int32_t reverb_pro_context_size(uint8_t *ct);


/**
 * @brief Configure stereo reverberation effect's level parameters.
 * @param dry   final dry mix [-70 to 10] dB
 * @param wet   final wet(late reverb) mix [-70 to 10] dB
 * @param erwet final wet(early reflection) mix [-70 to 10] dB
 * @return error code. REVERB_PRO_ERROR_OK means successful, other codes indicate error.
 * @note Do not call this function before reverb_pro_init() is called.
 */
int32_t reverb_pro_configure_level(uint8_t *ct, int32_t dry, int32_t wet, int32_t erwet);


/**
 * @brief Configure stereo reverberation effect's rt60 parameter.
 * @param rt60  reverb time decay [100 to 15000] millisecond
 * @return error code. REVERB_PRO_ERROR_OK means successful, other codes indicate error.
 * @note Do not call this function before reverb_pro_init() is called.
 */
int32_t reverb_pro_configure_rt60(uint8_t *ct, int32_t rt60);


/**
 * @brief Configure stereo reverberation effect's width parameter.
 * @param width width of reverb L/R mix [0 to 100] %
 * @return error code. REVERB_PRO_ERROR_OK means successful, other codes indicate error.
 * @note Do not call this function before reverb_pro_init() is called.
 */
int32_t reverb_pro_configure_width(uint8_t *ct, int32_t width);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __REVERB_PRO_H__
