/**
 *****************************************************************************
 * @file     gt20l16s1y_example.c
 * @author   AI Assistant
 * @version  V1.0.0
 * @date     2026-01-08
 * @brief    GT20L16S1Y字库芯片使用示例
 *****************************************************************************
 * 
 * 本文件演示如何使用GT20L16S1Y字库芯片驱动配合OLED12864显示器
 * 
 *****************************************************************************
 */

#include "gt20l16s1y.h"
#include <stdio.h>

/* OLED12864显示器函数声明(需要用户自行实现或使用现有驱动) */
extern void OLED_Init(void);
extern void OLED_Clear(void);
extern void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t *font_data, uint8_t width, uint8_t height);

/**
 * @brief  在OLED上显示8x16的ASCII字符
 * @param  x: X坐标 (0-127)
 * @param  y: Y坐标,以页为单位 (0-7)
 * @param  ch: 要显示的ASCII字符
 * @return 无
 */
void OLED_ShowASCII_8x16(uint8_t x, uint8_t y, char ch)
{
    uint8_t font_buffer[16];  // 8x16需要16字节
    uint16_t bytes_read;
    
    // 从字库芯片读取字符点阵数据
    bytes_read = GT20L16S1Y_ReadASCII(ch, FONT_SIZE_ASCII_8X16, font_buffer);
    
    if (bytes_read == 16) {
        // 显示到OLED (需要根据你的OLED驱动实现)
        OLED_ShowChar(x, y, font_buffer, 8, 16);
    }
}

/**
 * @brief  在OLED上显示16x16的中文字符
 * @param  x: X坐标 (0-127)
 * @param  y: Y坐标,以页为单位 (0-7)
 * @param  msb: GB2312编码高字节
 * @param  lsb: GB2312编码低字节
 * @return 无
 */
void OLED_ShowChinese_16x16(uint8_t x, uint8_t y, uint8_t msb, uint8_t lsb)
{
    uint8_t font_buffer[32];  // 16x16需要32字节
    uint16_t bytes_read;
    
    // 从字库芯片读取汉字点阵数据
    bytes_read = GT20L16S1Y_ReadGB2312(msb, lsb, font_buffer);
    
    if (bytes_read == 32) {
        // 显示到OLED (需要根据你的OLED驱动实现)
        OLED_ShowChar(x, y, font_buffer, 16, 16);
    }
}

/**
 * @brief  示例:初始化并显示测试文本
 */
void GT20L16S1Y_Example(void)
{
    // 初始化字库芯片
    GT20L16S1Y_Init();
    
    // 初始化OLED显示器
    OLED_Init();
    OLED_Clear();
    
    // 显示示例文本 - GT20L16S1Y支持16x16中文点阵
    // 显示"温度"两字 (GB2312: 0xCE 0xC2, 0xB6 0xC8)
    OLED_ShowChinese_16x16(0, 0, 0xCE, 0xC2);  // 温
    OLED_ShowChinese_16x16(16, 0, 0xB6, 0xC8); // 度
    
    // 显示英文字符
    OLED_ShowASCII_8x16(40, 0, ':');
    OLED_ShowASCII_8x16(48, 0, '2');
    OLED_ShowASCII_8x16(56, 0, '5');
    OLED_ShowASCII_8x16(64, 0, 'C');
    
    // 显示"音量"两字 (GB2312: 0xD2 0xF4, 0xC1 0xBF)
    OLED_ShowChinese_16x16(0, 2, 0xD2, 0xF4);  // 音
    OLED_ShowChinese_16x16(16, 2, 0xC1, 0xBF); // 量
}

/**
 * @brief  获取常见汉字的GB2312编码
 * @note   这些是常用汉字的GB2312编码,可以直接使用
 */
/*
常用汉字GB2312编码表:
温度  0xCEC2 0xB6C8
电池  0xB5E7 0xB3D8
音量  0xD2F4 0xC1BF
播放  0xB2A5 0xB7C5
暂停  0xD4DD 0xCDA3
上一曲 0xC9CF 0xD2BB 0xC7FA
下一曲 0xCFC2 0xD2BB 0xC7FA
设置  0xC9E8 0xD6C3
确定  0xC8B7 0xB6A8
取消  0xC8A1 0xCFB
*/
