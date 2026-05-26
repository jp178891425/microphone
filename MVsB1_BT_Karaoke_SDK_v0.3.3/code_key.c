/**
 **************************************************************************************
 * @file    code_key.c
 * @brief   
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-01-18 15:30:47$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <type.h>
#include "app_config.h"
#include "gpio.h"
#include "irqn.h"
#include "code_key.h"
#include <stdio.h>
#include "ctrlvars.h"

#ifdef CFG_RES_CODE_KEY_USE
//MIC
#define CODE_KEY_MIC_GPIO_A_BANK		(CFG_CODE_MIC_P_BANK)
#define CODE_KEY_MIC_GPIO_A_PIN			(CFG_CODE_MIC_P_PIN )
#define CODE_KEY_MIC_GPIO_B_BANK		(CFG_CODE_MIC_N_BANK)
#define CODE_KEY_MIC_GPIO_B_PIN			(CFG_CODE_MIC_N_PIN )

#define CODE_KEY_MIC_GPIO_A_PIN_MASK    (1 << CODE_KEY_MIC_GPIO_A_PIN)
#define CODE_KEY_MIC_GPIO_A_IE          ((CODE_KEY_MIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_MIC_GPIO_A_OE          ((CODE_KEY_MIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_MIC_GPIO_A_PU          ((CODE_KEY_MIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_MIC_GPIO_A_PD          ((CODE_KEY_MIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_MIC_GPIO_A_IN          ((CODE_KEY_MIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_MIC_GPIO_A_INT         ((CODE_KEY_MIC_GPIO_A_BANK - 'A') *  5 + GPIO_A_INT)
#define CODE_KEY_MIC_GPIO_B_PIN_MASK    (1 << CODE_KEY_MIC_GPIO_B_PIN)
#define CODE_KEY_MIC_GPIO_B_IE          ((CODE_KEY_MIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_MIC_GPIO_B_OE          ((CODE_KEY_MIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_MIC_GPIO_B_PU          ((CODE_KEY_MIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_MIC_GPIO_B_PD          ((CODE_KEY_MIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_MIC_GPIO_B_IN          ((CODE_KEY_MIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_MIC_GPIO_B_INT         ((CODE_KEY_MIC_GPIO_B_BANK - 'A') * 5  + GPIO_A_INT)


//EFFECT
#define CODE_KEY_EFFECT_GPIO_A_BANK		(CFG_CODE_EFFECT_P_BANK)
#define CODE_KEY_EFFECT_GPIO_A_PIN		(CFG_CODE_EFFECT_P_PIN )
#define CODE_KEY_EFFECT_GPIO_B_BANK		(CFG_CODE_EFFECT_N_BANK)
#define CODE_KEY_EFFECT_GPIO_B_PIN		(CFG_CODE_EFFECT_N_PIN )

#define CODE_KEY_EFFECT_GPIO_A_PIN_MASK    (1 << CODE_KEY_EFFECT_GPIO_A_PIN)
#define CODE_KEY_EFFECT_GPIO_A_IE          ((CODE_KEY_EFFECT_GPIO_A_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_EFFECT_GPIO_A_OE          ((CODE_KEY_EFFECT_GPIO_A_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_EFFECT_GPIO_A_PU          ((CODE_KEY_EFFECT_GPIO_A_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_EFFECT_GPIO_A_PD          ((CODE_KEY_EFFECT_GPIO_A_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_EFFECT_GPIO_A_IN          ((CODE_KEY_EFFECT_GPIO_A_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_EFFECT_GPIO_A_INT         ((CODE_KEY_EFFECT_GPIO_A_BANK - 'A') *  5 + GPIO_A_INT)
#define CODE_KEY_EFFECT_GPIO_B_PIN_MASK    (1 << CODE_KEY_EFFECT_GPIO_B_PIN)
#define CODE_KEY_EFFECT_GPIO_B_IE          ((CODE_KEY_EFFECT_GPIO_B_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_EFFECT_GPIO_B_OE          ((CODE_KEY_EFFECT_GPIO_B_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_EFFECT_GPIO_B_PU          ((CODE_KEY_EFFECT_GPIO_B_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_EFFECT_GPIO_B_PD          ((CODE_KEY_EFFECT_GPIO_B_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_EFFECT_GPIO_B_IN          ((CODE_KEY_EFFECT_GPIO_B_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_EFFECT_GPIO_B_INT         ((CODE_KEY_EFFECT_GPIO_B_BANK - 'A') * 5  + GPIO_A_INT)


//music
#define CODE_KEY_MUSIC_GPIO_A_BANK		(CFG_CODE_MUSIC_P_BANK)
#define CODE_KEY_MUSIC_GPIO_A_PIN		(CFG_CODE_MUSIC_P_PIN )
#define CODE_KEY_MUSIC_GPIO_B_BANK		(CFG_CODE_MUSIC_N_BANK)
#define CODE_KEY_MUSIC_GPIO_B_PIN		(CFG_CODE_MUSIC_N_PIN )

#define CODE_KEY_MUSIC_GPIO_A_PIN_MASK    (1 << CODE_KEY_MUSIC_GPIO_A_PIN)
#define CODE_KEY_MUSIC_GPIO_A_IE          ((CODE_KEY_MUSIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_MUSIC_GPIO_A_OE          ((CODE_KEY_MUSIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_MUSIC_GPIO_A_PU          ((CODE_KEY_MUSIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_MUSIC_GPIO_A_PD          ((CODE_KEY_MUSIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_MUSIC_GPIO_A_IN          ((CODE_KEY_MUSIC_GPIO_A_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_MUSIC_GPIO_A_INT         ((CODE_KEY_MUSIC_GPIO_A_BANK - 'A') *  5 + GPIO_A_INT)
#define CODE_KEY_MUSIC_GPIO_B_PIN_MASK    (1 << CODE_KEY_MUSIC_GPIO_B_PIN)
#define CODE_KEY_MUSIC_GPIO_B_IE          ((CODE_KEY_MUSIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_MUSIC_GPIO_B_OE          ((CODE_KEY_MUSIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_MUSIC_GPIO_B_PU          ((CODE_KEY_MUSIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_MUSIC_GPIO_B_PD          ((CODE_KEY_MUSIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_MUSIC_GPIO_B_IN          ((CODE_KEY_MUSIC_GPIO_B_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_MUSIC_GPIO_B_INT         ((CODE_KEY_MUSIC_GPIO_B_BANK - 'A') * 5  + GPIO_A_INT)

//clockwise rotation
//    ----|		   |--------|		 |-------------------
//A       |        |        |        |
//        |--------|        |--------|
//     -------|        |--------|		 |---------------
//B           |        |        |		 |
//            |--------|        |--------|
//counterclockwise rotation
//    --------|		   |--------|		 |---------------
//A           |		   |        |		 |
//            |--------|        |--------|
//     ---|        |--------|		 |-------------------
//B       |        |        |		 |
//        |--------|        |--------|

// Initialize coding key scan (GPIO) operation.
// Config interrupt at negative edge of signal-A
void CodeKeyInit(void)
{	

	//mic
	//enable pull up resister.
	GPIO_RegOneBitSet(CODE_KEY_MIC_GPIO_A_IE, CODE_KEY_MIC_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MIC_GPIO_A_OE, CODE_KEY_MIC_GPIO_A_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_MIC_GPIO_A_PU, CODE_KEY_MIC_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MIC_GPIO_A_PD, CODE_KEY_MIC_GPIO_A_PIN_MASK);

	//enable pull up resister.
    GPIO_RegOneBitSet(CODE_KEY_MIC_GPIO_B_IE, CODE_KEY_MIC_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MIC_GPIO_B_OE, CODE_KEY_MIC_GPIO_B_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_MIC_GPIO_B_PU, CODE_KEY_MIC_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MIC_GPIO_B_PD, CODE_KEY_MIC_GPIO_B_PIN_MASK);


	//effect
	//enable pull up resister.
	GPIO_RegOneBitSet(CODE_KEY_EFFECT_GPIO_A_IE, CODE_KEY_EFFECT_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_EFFECT_GPIO_A_OE, CODE_KEY_EFFECT_GPIO_A_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_EFFECT_GPIO_A_PU, CODE_KEY_EFFECT_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_EFFECT_GPIO_A_PD, CODE_KEY_EFFECT_GPIO_A_PIN_MASK);

	//enable pull up resister.
    GPIO_RegOneBitSet(CODE_KEY_EFFECT_GPIO_B_IE, CODE_KEY_EFFECT_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_EFFECT_GPIO_B_OE, CODE_KEY_EFFECT_GPIO_B_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_EFFECT_GPIO_B_PU, CODE_KEY_EFFECT_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_EFFECT_GPIO_B_PD, CODE_KEY_EFFECT_GPIO_B_PIN_MASK);


	//music
	//enable pull up resister.
	GPIO_RegOneBitSet(CODE_KEY_MUSIC_GPIO_A_IE, CODE_KEY_MUSIC_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MUSIC_GPIO_A_OE, CODE_KEY_MUSIC_GPIO_A_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_MUSIC_GPIO_A_PU, CODE_KEY_MUSIC_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MUSIC_GPIO_A_PD, CODE_KEY_MUSIC_GPIO_A_PIN_MASK);

	//enable pull up resister.
    GPIO_RegOneBitSet(CODE_KEY_MUSIC_GPIO_B_IE, CODE_KEY_MUSIC_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MUSIC_GPIO_B_OE, CODE_KEY_MUSIC_GPIO_B_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_MUSIC_GPIO_B_PU, CODE_KEY_MUSIC_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_MUSIC_GPIO_B_PD, CODE_KEY_MUSIC_GPIO_B_PIN_MASK);
}

// Key process, image key value to key event.
CodeKeyType CodeKeyScan(void)
{
	CodeKeyType CodeKey = CODE_KEY_NONE;
	switch (gCtrlVars.CodeKey)
	{
	case CODE_KEY_MIC_FORWARD:
		CodeKey = CODE_KEY_MIC_FORWARD;
		break;
	case CODE_KEY_MIC_BACKWARD:
		CodeKey = CODE_KEY_MIC_BACKWARD;
		break;
	case CODE_KEY_EFFECT_FORWARD:
		CodeKey = CODE_KEY_EFFECT_FORWARD;
		break;
	case CODE_KEY_EFFECT_BACKWARD:
		CodeKey = CODE_KEY_EFFECT_BACKWARD;
		break;
	case CODE_KEY_MUSIC_FORWARD:
		CodeKey = CODE_KEY_MUSIC_FORWARD;
		break;
	case CODE_KEY_MUSIC_BACKWARD:
		CodeKey = CODE_KEY_MUSIC_BACKWARD;
		break;
	default:
		CodeKey = CODE_KEY_NONE;
		break;
	}
	// if(gCtrlVars.CodeKey == CODE_KEY_FORWARD)
	// {
	// 	CodeKey = CODE_KEY_FORWARD;
	// }
	// else if(gCtrlVars.CodeKey == CODE_KEY_BACKWARD)
	// {
	// 	CodeKey = CODE_KEY_BACKWARD;
	// }
	gCtrlVars.CodeKey = CODE_KEY_NONE;
	return CodeKey;
}

//输入寄存器
// #define REG_GPIO_A_IN   (0x40010000)
// #define REG_GPIO_B_IN   (0x40010030)

// #define CODE_KEY_GPIO_A_IN_REG_ADDR          ((CODE_KEY_GPIO_A_BANK - 'A') * 0x30 + REG_GPIO_A_IN)
// #define CODE_KEY_GPIO_B_IN_REG_ADDR          ((CODE_KEY_GPIO_B_BANK - 'A') * 0x30 + REG_GPIO_A_IN)

#define A_MIC_ENCODER_IN     	GPIO_RegOneBitGet(CODE_KEY_MIC_GPIO_A_IN, CODE_KEY_MIC_GPIO_A_PIN_MASK)//((*(volatile uint32_t *)(CODE_KEY_GPIO_A_IN_REG_ADDR)) & CODE_KEY_GPIO_A_PIN_MASK)//GPIO_RegOneBitGet(CODE_KEY_GPIO_A_IN, CODE_KEY_GPIO_A_PIN_MASK)
#define B_MIC_ENCODER_IN     	GPIO_RegOneBitGet(CODE_KEY_MIC_GPIO_B_IN, CODE_KEY_MIC_GPIO_B_PIN_MASK)//((*(volatile uint32_t *)(CODE_KEY_GPIO_B_IN_REG_ADDR)) & CODE_KEY_GPIO_B_PIN_MASK)//(CODE_KEY_GPIO_B_IN, CODE_KEY_GPIO_B_PIN_MASK)
#define A_EFFECT_ENCODER_IN     GPIO_RegOneBitGet(CODE_KEY_EFFECT_GPIO_A_IN, CODE_KEY_EFFECT_GPIO_A_PIN_MASK)
#define B_EFFECT_ENCODER_IN     GPIO_RegOneBitGet(CODE_KEY_EFFECT_GPIO_B_IN, CODE_KEY_EFFECT_GPIO_B_PIN_MASK)
#define A_MUSIC_ENCODER_IN     	GPIO_RegOneBitGet(CODE_KEY_MUSIC_GPIO_A_IN, CODE_KEY_MUSIC_GPIO_A_PIN_MASK)
#define B_MUSIC_ENCODER_IN     	GPIO_RegOneBitGet(CODE_KEY_MUSIC_GPIO_B_IN, CODE_KEY_MUSIC_GPIO_B_PIN_MASK)
void CodeKeyCheckInTimer(void)
{
    static uint8_t mic_A_old = 0, mic_B_old = 0, effect_A_old = 0, effect_B_old = 0, music_A_old = 0, music_B_old = 0; //定义了两个变量用来储蓄上一次调用此方法是编码开关两引脚的电平
    static uint8_t mic_flag = 0, effect_flag = 0, music_flag = 0; //定义了一个变量用来储蓄以前是否出现了两个引脚都为高电平的状态

	//mic
    if ((A_MIC_ENCODER_IN && B_MIC_ENCODER_IN)) {
        mic_flag = 1;
    }

    if (mic_flag) {
        if (((!A_MIC_ENCODER_IN) && (!B_MIC_ENCODER_IN))) {
            if (mic_B_old) {   //为高说明编码开关在向加大的方向转
                mic_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_MIC_FORWARD; 
            }
            if (mic_A_old) {   //为高说明编码开关在向减小的方向转
                mic_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_MIC_BACKWARD; 
            }
        }
    }
	
    if (A_MIC_ENCODER_IN) mic_A_old = 1; else mic_A_old = 0;
    if (B_MIC_ENCODER_IN) mic_B_old = 1; else mic_B_old = 0;

	//effect
	if ((A_EFFECT_ENCODER_IN && B_EFFECT_ENCODER_IN)) {
        effect_flag = 1;
    }

    if (effect_flag) {
        if (((!A_EFFECT_ENCODER_IN) && (!B_EFFECT_ENCODER_IN))) {
            if (effect_B_old) {   //为高说明编码开关在向加大的方向转
                effect_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_EFFECT_FORWARD; 
            }
            if (effect_A_old) {   //为高说明编码开关在向减小的方向转
                effect_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_EFFECT_BACKWARD; 
            }
        }
    }
	
    if (A_EFFECT_ENCODER_IN) effect_A_old = 1; else effect_A_old = 0;
    if (B_EFFECT_ENCODER_IN) effect_B_old = 1; else effect_B_old = 0;

	//music
	if ((A_MUSIC_ENCODER_IN && B_MUSIC_ENCODER_IN)) {
        music_flag = 1;
    }

    if (music_flag) {
        if (((!A_MUSIC_ENCODER_IN) && (!B_MUSIC_ENCODER_IN))) {
            if (music_B_old) {   //为高说明编码开关在向加大的方向转
                music_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_MUSIC_FORWARD; 
            }
            if (music_A_old) {   //为高说明编码开关在向减小的方向转
                music_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_MUSIC_BACKWARD; 
            }
        }
    }
	
    if (A_MUSIC_ENCODER_IN) music_A_old = 1; else music_A_old = 0;
    if (B_MUSIC_ENCODER_IN) music_B_old = 1; else music_B_old = 0;
	
}

#endif //CFG_RES_CODE_KEY_USE

