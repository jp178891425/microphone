/**
 *****************************************************************************
 * @file     gt20l16s1y.h
 * @author   AI Assistant
 * @version  V1.0.0
 * @date     2026-01-08
 * @brief    GT20L16S1Y字库芯片SPI驱动头文件
 *****************************************************************************
 * @attention
 *
 * GT20L16S1Y是一款点阵字库芯片,支持GB2312编码的中文字符
 * 包含16x16点阵和12x12点阵的汉字字库,以及8x16和6x12的ASCII字库
 *
 *****************************************************************************
 */

#ifndef __GT20L16S1Y_H__
#define __GT20L16S1Y_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * 字体大小定义
 */
typedef enum {
    FONT_SIZE_ASCII_6X12   = 0,    // ASCII 6x12点阵
    FONT_SIZE_ASCII_8X16   = 1,    // ASCII 8x16点阵
    FONT_SIZE_GB2312_16X16 = 2,    // GB2312 16x16点阵(中文)
} GT20L16S1Y_FontSize;

/**
 * GT20L16S1Y字库地址定义(根据GT20L16S1Y芯片手册和参考驱动)
 * 
 * GB2312字符集布局:
 * 0x00000-0x02CFF: A1-A3区符号(282个字符,每个32字节)
 * 0x02D00-0x0372F: A9区符号(94个字符,每个32字节)
 * 0x03730-0x0695F: 扩展字符(470个字符,每个32字节)
 * 0x06960-开始:    B0-F7区汉字(6763个汉字,每个32字节)
 */
#define GT20L16S1Y_ADDR_GB2312_BASE    0x00000    // GB2312字库起始地址
#define GT20L16S1Y_ADDR_ASCII_5X7      0x3BFC0    // ASCII 5x7点阵起始地址(参考驱动)
#define GT20L16S1Y_ADDR_ASCII_8X16     0x3CF80    // ASCII 8x16点阵起始地址(参考驱动)

/**
 * @brief  初始化GT20L16S1Y字库芯片
 * @param  无
 * @return 无
 * @note   初始化SPI接口和CS引脚
 */
void GT20L16S1Y_Init(void);

/**
 * @brief  从GT20L16S1Y读取ASCII字符点阵数据
 * @param  ascii_code: ASCII字符编码 (0x20-0x7E)
 * @param  font_size: 字体大小 (FONT_SIZE_ASCII_6X12 或 FONT_SIZE_ASCII_8X16)
 * @param  buffer: 存储读取数据的缓冲区
 * @return 读取的字节数
 * @note   6x12字体返回12字节, 8x16字体返回16字节
 */
uint16_t GT20L16S1Y_ReadASCII(uint8_t ascii_code, GT20L16S1Y_FontSize font_size, uint8_t *buffer);

/**
 * @brief  从GT20L16S1Y读取GB2312汉字/字符点阵数据
 * @param  msb: GB2312编码高字节
 * @param  lsb: GB2312编码低字节
 * @param  buffer: 存储读取数据的缓冲区
 * @return 读取的字节数 (成功返回32,失败返回0)
 * @note   支持A1-A3区、A9区符号和B0-F7区汉字,每个字符32字节(16x16点阵)
 */
uint16_t GT20L16S1Y_ReadGB2312(uint8_t msb, uint8_t lsb, uint8_t *buffer);

/**
 * @brief  从GT20L16S1Y读取GB2312汉字点阵数据(兼容旧接口)
 * @param  gb_code: GB2312编码 (高字节<<8 | 低字节)
 * @param  font_size: 字体大小 (仅支持FONT_SIZE_GB2312_16X16)
 * @param  buffer: 存储读取数据的缓冲区
 * @return 读取的字节数
 * @note   此函数为兼容接口,内部调用GT20L16S1Y_ReadGB2312
 */
uint16_t GT20L16S1Y_ReadGB2312_Code(uint16_t gb_code, GT20L16S1Y_FontSize font_size, uint8_t *buffer);

/**
 * @brief  从字库芯片读取指定地址的数据
 * @param  address: 字库芯片内部地址 (0x000000 - 0x0FFFFF)
 * @param  buffer: 存储读取数据的缓冲区
 * @param  length: 读取数据长度
 * @return 0: 成功, -1: 失败
 */
int8_t GT20L16S1Y_ReadData(uint32_t address, uint8_t *buffer, uint16_t length);

/**
 * @brief  CS片选信号控制
 * @param  level: 1-高电平, 0-低电平
 * @return 无
 */
void GT20L16S1Y_CS_Control(uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* __GT20L16S1Y_H__ */
