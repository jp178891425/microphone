/**
 *************************************************************************************
 * @file	blue_ns.h
 * @brief	Noise Suppression for Mono Signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v3.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __BLUE_NS_H__
#define __BLUE_NS_H__

#include <stdint.h>


 /** error code for noise suppression */
typedef enum _BLUENS_ERROR_CODE
{
	BLUENS_ERROR_ILLEGAL_NS_LEVEL = -256,
	BLUENS_ERROR_ILLEGAL_BLOCK_LENGTH,
	BLUENS_ERROR_ILLEGAL_BIT_DEPTH,

	// No Error
	BLUENS_ERROR_OK = 0,					/**< no error              */
} BLUENS_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * 
 * @brief Estimate the memory usage of noise suppression module before actual initialization.
 * @param[in]  blk_len Block length in samples. 3 supported values: 128 / 256 / 512.
 * @param[in]  bit_depth Bit depth of the PCM samples. Choose either 16 or 24 bits.
 * @param[out] persistent_size  size of persistent memory in bytes. The contents of this memory cannot be modified or cleared between calls to blue_ns_run16() or blue_ns_run24().
 * @param[out] scratch_size  size of scratch memory in bytes. The contents of this memory can be modified or cleared for other use between calls to blue_ns_run16() or blue_ns_run24().
 * @return error code. BLUENS_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before module initialization to estimate the memory required for the context object.
 * Typical memory usage is as follows:
 * ----------------------------------------------------
 * | block_length | bit_depth | persistent |  scratch |
 * |              |           |   (bytes)  |  (bytes) |
 * ----------------------------------------------------
 * |     128      |    16     |    3104    |   1024   |
 * |     256      |    16     |    6176    |   2048   |
 * |     512      |    16     |   12320    |   4096   |
 * |---------------------------------------------------
 * |     128      |    24     |    3616    |   1024   |
 * |     256      |    24     |    7200    |   2048   |
 * |     512      |    24     |   14368    |   4096   |
 * ----------------------------------------------------
 * The values above are for reference only. The actual values returned by the function should prevail in case of any difference.
 */
int32_t blue_ns_estimate_memory_usage(int32_t blk_len, int32_t bit_depth, uint32_t* persistent_size, uint32_t* scratch_size);


/**
 * @brief Initialize the noise suppression module.
 * @param ct Pointer to the noise suppression context object (persistent_size in bytes).
 * @param scratch Pointer to the scratch area (scratch_size in bytes).
 * @param blk_len Block length in samples. 3 supported values: 128 / 256 / 512.
 * @param bit_depth Bit depth of the PCM samples. Choose either 16 or 24 bits.
 * @return error code. BLUENS_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported. 
 *       The blk_len selected not only affects the CPU usage but also the effect. Longer block
 *       usually has better effect but with longer delay.
 */
int32_t blue_ns_init(uint8_t* ct, uint8_t* scratch, int32_t blk_len, int32_t bit_depth);


/**
 * @brief Run noise suppression to a block of PCM data (16-bit).
 * @param ct Pointer to the noise suppression context object.
 * @param xin Input PCM data. The size of xin is equal to blk_len set in blue_ns_init().
 * @param xout Output PCM data. The size of xout is equal to blk_len set in blue_ns_init().
 *        xout can be the same as xin. In this case, the PCM is changed in-place.
 * @param ns_level Noise suppression level. Valid range: 0 ~ 9. Use 0 to disable noise suppression while 9 to apply maximum suppression.
 * @return error code. BLUENS_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported.
 */
int32_t blue_ns_run16(uint8_t* ct, int16_t* xin, int16_t* xout, int32_t ns_level);


/**
 * @brief Run noise suppression to a block of PCM data (24-bit).
 * @param ct Pointer to the noise suppression context object.
 * @param xin Input PCM data. The size of xin is equal to blk_len set in blue_ns_init().
 * @param xout Output PCM data. The size of xout is equal to blk_len set in blue_ns_init().
 *        xout can be the same as xin. In this case, the PCM is changed in-place.
 * @param ns_level Noise suppression level. Valid range: 0 ~ 9. Use 0 to disable noise suppression while 9 to apply maximum suppression.
 * @return error code. BLUENS_ERROR_OK means successful, other codes indicate error.
 * @note Only mono signals are supported.
 */
int32_t blue_ns_run24(uint8_t* ct, int32_t* xin, int32_t* xout, int32_t ns_level);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__BLUE_NS_H__
