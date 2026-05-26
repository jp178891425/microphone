/**
 *****************************************************************************
 * @file     gt20l16s1y.c
 * @author   AI Assistant
 * @version  V1.0.0
 * @date     2026-01-08
 * @brief    GT20L16S1Y字库芯片SPI驱动实现
 *****************************************************************************
 * @attention
 *
 * GT20L16S1Y是一款点阵字库芯片,支持GB2312编码的中文字符
 * 包含16x16点阵和12x12点阵的汉字字库,以及8x16和6x12的ASCII字库
 *
 * 硬件连接:
 * - SPI时钟最大20MHz
 * - 工作电压3.3V或5V
 * - CS片选信号低电平有效
 *
 *****************************************************************************
 */

#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif

#include "gt20l16s1y.h"
#include "gpio.h"
#include "spim.h"
#include "delay.h"
#include <string.h>

/* SPI接口组选择: 0-SPIM_PORT0_A5_A6_A7; 1-SPIM_PORT1_A20_A21_A22 */
#define GT20L16S1Y_SPI_IO_GROUP    0

/* GPIO定义 - CS片选引脚 */
#define GT20L16S1Y_CS_PIN          GPIOA4
#define GT20L16S1Y_CS_PORT         GPIO_A_OUT
#define GT20L16S1Y_CS_BIT          (1<<4)

/* CS控制宏 */
#define GT20L16S1Y_CS_HIGH()       GPIO_RegOneBitSet(GT20L16S1Y_CS_PORT, GT20L16S1Y_CS_BIT)
#define GT20L16S1Y_CS_LOW()        GPIO_RegOneBitClear(GT20L16S1Y_CS_PORT, GT20L16S1Y_CS_BIT)

/* SPI读命令 */
#define GT20L16S1Y_CMD_READ        0x03

/**
 * @brief  发送单字节数据
 * @param  data: 要发送的数据
 * @return 无
 */
static inline void GT20L16S1Y_SendByte(uint8_t data)
{
    SPIM_Send(&data, 1);
}

/**
 * @brief  接收单字节数据
 * @param  无
 * @return 接收到的数据
 */
static inline uint8_t GT20L16S1Y_RecvByte(void)
{
    uint8_t ret;
    SPIM_Recv(&ret, 1);
    return ret;
}

/**
 * @brief  初始化GT20L16S1Y字库芯片
 * @param  无
 * @return 无
 */
void GT20L16S1Y_Init(void)
{
    /* 配置CS引脚为输出 */
    GPIO_RegOneBitSet(GPIO_A_OE, GT20L16S1Y_CS_PIN);
    GPIO_RegOneBitClear(GPIO_A_IE, GT20L16S1Y_CS_PIN);
    GT20L16S1Y_CS_HIGH();
    
    /* 初始化SPI接口 
     * Mode 0: CPOL=0, CPHA=0 (GT20L16S1Y支持Mode 0和Mode 3)
     * 时钟分频: SPIM_CLK_DIV_6M (约7.5MHz,安全速度)
     */
    SPIM_Init(0, SPIM_CLK_DIV_6M);
    
    /* 配置SPI引脚 */
    SPIM_IoConfig(GT20L16S1Y_SPI_IO_GROUP);
    
    /* 设置MSB高位先传输 */
    SPIM_SetMSB(1);
    
    DelayUs(100);  // 等待稳定
}

/**
 * @brief  CS片选信号控制
 * @param  level: 1-高电平, 0-低电平
 * @return 无
 */
void GT20L16S1Y_CS_Control(uint8_t level)
{
    if (level) {
        GT20L16S1Y_CS_HIGH();
    } else {
        GT20L16S1Y_CS_LOW();
    }
}

/**
 * @brief  从字库芯片读取指定地址的数据
 * @param  address: 字库芯片内部地址 (0x000000 - 0x0FFFFF)
 * @param  buffer: 存储读取数据的缓冲区
 * @param  length: 读取数据长度
 * @return 0: 成功, -1: 失败
 */
int8_t GT20L16S1Y_ReadData(uint32_t address, uint8_t *buffer, uint16_t length)
{
    if (buffer == NULL || length == 0) {
        return -1;
    }
    
    if (address > 0x0FFFFF) {  // GT20L16S1Y地址空间检查
        return -1;
    }
    
    GT20L16S1Y_CS_LOW();  // 使能芯片
    
    /* 发送读命令 */
    GT20L16S1Y_SendByte(GT20L16S1Y_CMD_READ);
    
    /* 发送24位地址 (A23-A16, A15-A8, A7-A0) */
    GT20L16S1Y_SendByte((uint8_t)((address >> 16) & 0xFF));
    GT20L16S1Y_SendByte((uint8_t)((address >> 8) & 0xFF));
    GT20L16S1Y_SendByte((uint8_t)(address & 0xFF));
    
    /* 读取数据 */
    SPIM_Recv(buffer, length);
    
    GT20L16S1Y_CS_HIGH();  // 禁用芯片
    
    DelayUs(1);  // 短暂延时
    
    return 0;
}

/**
 * @brief  从GT20L16S1Y读取ASCII字符点阵数据
 * @param  ascii_code: ASCII字符编码 (0x20-0x7E)
 * @param  font_size: 字体大小
 * @param  buffer: 存储读取数据的缓冲区
 * @return 读取的字节数
 */
uint16_t GT20L16S1Y_ReadASCII(uint8_t ascii_code, GT20L16S1Y_FontSize font_size, uint8_t *buffer)
{
    uint32_t address = 0;
    uint16_t bytes_per_char = 0;
    
    if (buffer == NULL) {
        return 0;
    }
    
    /* 检查ASCII码范围 */
    if (ascii_code < 0x20 || ascii_code > 0x7E) {
        return 0;
    }
    
    /* 计算字符在字库中的偏移 */
    uint8_t offset = ascii_code - 0x20;  // ASCII可打印字符从0x20开始
    
    switch (font_size) {
        case FONT_SIZE_ASCII_6X12:
            // 5x7实际占用6列(含间距),每个字符8字节
            address = GT20L16S1Y_ADDR_ASCII_5X7 + offset * 8;
            bytes_per_char = 8;
            break;
            
        case FONT_SIZE_ASCII_8X16:
            address = GT20L16S1Y_ADDR_ASCII_8X16 + offset * 16;
            bytes_per_char = 16;  // 8x16需要16字节
            break;
            
        default:
            return 0;
    }
    
    /* 读取字符点阵数据 */
    if (GT20L16S1Y_ReadData(address, buffer, bytes_per_char) == 0) {
        return bytes_per_char;
    }
    
    return 0;
}

/**
 * @brief  从GT20L16S1Y读取GB2312汉字/字符点阵数据
 * @param  msb: GB2312编码高字节
 * @param  lsb: GB2312编码低字节
 * @param  buffer: 存储读取数据的缓冲区
 * @return 读取的字节数 (成功返回32,失败返回0)
 * @note   支持A1-A3区、A9区符号和B0-F7区汉字
 */
uint16_t GT20L16S1Y_ReadGB2312(uint8_t msb, uint8_t lsb, uint8_t *buffer)
{
    uint32_t address = 0;
    
    if (buffer == NULL) {
        return 0;
    }
    
    /* 根据不同的区码计算地址 */
    if (msb == 0xA9 && lsb >= 0xA1) {
        /* A9区符号 */
        address = (282 + (lsb - 0xA1)) * 32 + GT20L16S1Y_ADDR_GB2312_BASE;
    }
    else if (msb >= 0xA1 && msb <= 0xA3 && lsb >= 0xA1) {
        /* A1-A3区符号 */
        address = ((msb - 0xA1) * 94 + (lsb - 0xA1)) * 32 + GT20L16S1Y_ADDR_GB2312_BASE;
    }
    else if (msb >= 0xB0 && msb <= 0xF7 && lsb >= 0xA1) {
        /* B0-F7区汉字 */
        uint32_t offset = (msb - 0xB0) * 94 + (lsb - 0xA1);
        address = (846 + offset) * 32 + GT20L16S1Y_ADDR_GB2312_BASE;
    }
    else {
        /* 无效的GB2312编码 */
        return 0;
    }
    
    /* 读取字符点阵数据 (16x16=32字节) */
    if (GT20L16S1Y_ReadData(address, buffer, 32) == 0) {
        return 32;
    }
    
    return 0;
}

/**
 * @brief  从GT20L16S1Y读取GB2312汉字点阵数据(兼容旧接口)
 * @param  gb_code: GB2312编码 (高字节<<8 | 低字节)
 * @param  font_size: 字体大小 (仅支持FONT_SIZE_GB2312_16X16)
 * @param  buffer: 存储读取数据的缓冲区
 * @return 读取的字节数
 * @note   此函数为兼容接口,内部调用GT20L16S1Y_ReadGB2312
 */
uint16_t GT20L16S1Y_ReadGB2312_Code(uint16_t gb_code, GT20L16S1Y_FontSize font_size, uint8_t *buffer)
{
    uint8_t msb, lsb;
    
    if (buffer == NULL || font_size != FONT_SIZE_GB2312_16X16) {
        return 0;
    }
    
    /* 提取高低字节 */
    msb = (uint8_t)(gb_code >> 8);
    lsb = (uint8_t)(gb_code & 0xFF);
    
    return GT20L16S1Y_ReadGB2312(msb, lsb, buffer);
}
