#ifndef _HT1621B_H_
#define _HT1621B_H_

/*################################### include ###################################*/
#include "gpio.h"
#include "sys_gpio.h"

/*################################### define/enum ###################################*/
#define COM_MODE_1  0x52  //4COM,1/3bias
#define COM_MODE_2  0x50  //4COM,1/2bias
#define RC_OSC      0x30  //内部RC振荡器(上电默认)
#define LCD_ON      0x06  //打开LCD 偏压发生器
#define LCD_OFF     0x04  //关闭LCD显示
#define SYS_EN      0x02  //系统振荡器开
#define CTRL_CMD    0x80  //写控制命令
#define DATA_CMD    0xA0  //写数据命令 

/*************** [XH]GPIO定义 ***************/
#define HT1621B_IC_MAP(XX) \
    XX(HT1621B_IC1, A, GPIO_INDEX17) \
    XX(HT1621B_IC2, A, GPIO_INDEX16) \
    XX(HT1621B_IC3, A, GPIO_INDEX15) \
    XX(HT1621B_IC4, A, GPIO_INDEX5)  \

#define HT1621B_CTRL_MAP(XX) \
    XX(HT1621B_WR,   A, GPIO_INDEX6) \
    XX(HT1621B_DATA, A, GPIO_INDEX7) \
/*************** [XH]GPIO定义 ***************/

enum {
    #define XX_ENUM(name, port, pin) name,
    HT1621B_IC_MAP(XX_ENUM)
    #undef XX_ENUM
    HT1621B_IC_NUM,

    HT1621B_WR = HT1621B_IC_NUM,
    HT1621B_DATA,
    HT1621B_GPIO_NUM,
};

/*################################### typedef ###################################*/

/*################################### 全局变量 ###################################*/

/*################################### 全局函数 ###################################*/
void HT1621B_Init(void);
void HT1621B_Write_4Bit(uint8_t ic, uint8_t addr, uint8_t data);
void HT1621B_Write_8Bit(uint8_t ic, uint8_t addr, uint8_t data);
void HT1621B_Write_All(uint8_t ic, uint8_t addr, uint8_t *p, uint8_t len);
void HT1621B_All_Off(uint8_t ic);
void HT1621B_All_On(uint8_t ic);

#endif