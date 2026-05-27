/*
 * @Author: wujunpeng
 * @Date: 2026-05-26 18:06:11
 * @LastEditors: Do not edit
 * @LastEditTime: 2026-05-27 11:49:41
 * @FilePath: \microphone\MVsB1_BT_Karaoke_SDK_v0.3.3\BT_Audio_APP\bt_audio_app_src\device\code_key.c
 */
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

//CODEKEY 1
#if CODE_KEY_1_OPEN
#define CODE_KEY_1_GPIO_A_BANK		(CFG_CODE_KEY1P_BANK)
#define CODE_KEY_1_GPIO_A_PIN		(CFG_CODE_KEY1P_PIN)
#define CODE_KEY_1_GPIO_B_BANK		(CFG_CODE_KEY1N_BANK)
#define CODE_KEY_1_GPIO_B_PIN		(CFG_CODE_KEY1N_PIN)

#define CODE_KEY_1_GPIO_A_PIN_MASK    (1 << CODE_KEY_1_GPIO_A_PIN)
#define CODE_KEY_1_GPIO_A_IE          ((CODE_KEY_1_GPIO_A_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_1_GPIO_A_OE          ((CODE_KEY_1_GPIO_A_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_1_GPIO_A_PU          ((CODE_KEY_1_GPIO_A_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_1_GPIO_A_PD          ((CODE_KEY_1_GPIO_A_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_1_GPIO_A_IN          ((CODE_KEY_1_GPIO_A_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_1_GPIO_A_INT         ((CODE_KEY_1_GPIO_A_BANK - 'A') *  5 + GPIO_A_INT)
#define CODE_KEY_1_GPIO_B_PIN_MASK    (1 << CODE_KEY_1_GPIO_B_PIN)
#define CODE_KEY_1_GPIO_B_IE          ((CODE_KEY_1_GPIO_B_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_1_GPIO_B_OE          ((CODE_KEY_1_GPIO_B_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_1_GPIO_B_PU          ((CODE_KEY_1_GPIO_B_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_1_GPIO_B_PD          ((CODE_KEY_1_GPIO_B_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_1_GPIO_B_IN          ((CODE_KEY_1_GPIO_B_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_1_GPIO_B_INT         ((CODE_KEY_1_GPIO_B_BANK - 'A') * 5  + GPIO_A_INT)
#endif

//CODEKEY 2
#if CODE_KEY_2_OPEN
#define CODE_KEY_2_GPIO_A_BANK		(CFG_CODE_KEY2P_BANK)
#define CODE_KEY_2_GPIO_A_PIN		(CFG_CODE_KEY2P_PIN )
#define CODE_KEY_2_GPIO_B_BANK		(CFG_CODE_KEY2N_BANK)
#define CODE_KEY_2_GPIO_B_PIN		(CFG_CODE_KEY2N_PIN )

#define CODE_KEY_2_GPIO_A_PIN_MASK    (1 << CODE_KEY_2_GPIO_A_PIN)
#define CODE_KEY_2_GPIO_A_IE          ((CODE_KEY_2_GPIO_A_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_2_GPIO_A_OE          ((CODE_KEY_2_GPIO_A_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_2_GPIO_A_PU          ((CODE_KEY_2_GPIO_A_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_2_GPIO_A_PD          ((CODE_KEY_2_GPIO_A_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_2_GPIO_A_IN          ((CODE_KEY_2_GPIO_A_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_2_GPIO_A_INT         ((CODE_KEY_2_GPIO_A_BANK - 'A') *  5 + GPIO_A_INT)
#define CODE_KEY_2_GPIO_B_PIN_MASK    (1 << CODE_KEY_2_GPIO_B_PIN)
#define CODE_KEY_2_GPIO_B_IE          ((CODE_KEY_2_GPIO_B_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_2_GPIO_B_OE          ((CODE_KEY_2_GPIO_B_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_2_GPIO_B_PU          ((CODE_KEY_2_GPIO_B_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_2_GPIO_B_PD          ((CODE_KEY_2_GPIO_B_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_2_GPIO_B_IN          ((CODE_KEY_2_GPIO_B_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_2_GPIO_B_INT         ((CODE_KEY_2_GPIO_B_BANK - 'A') * 5  + GPIO_A_INT)
#endif

//CODEKEY 3
#if CODE_KEY_3_OPEN
#define CODE_KEY_3_GPIO_A_BANK		(CFG_CODE_KEY3P_BANK)
#define CODE_KEY_3_GPIO_A_PIN		(CFG_CODE_KEY3P_PIN )
#define CODE_KEY_3_GPIO_B_BANK		(CFG_CODE_KEY3N_BANK)
#define CODE_KEY_3_GPIO_B_PIN		(CFG_CODE_KEY3N_PIN )

#define CODE_KEY_3_GPIO_A_PIN_MASK    (1 << CODE_KEY_3_GPIO_A_PIN)
#define CODE_KEY_3_GPIO_A_IE          ((CODE_KEY_3_GPIO_A_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_3_GPIO_A_OE          ((CODE_KEY_3_GPIO_A_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_3_GPIO_A_PU          ((CODE_KEY_3_GPIO_A_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_3_GPIO_A_PD          ((CODE_KEY_3_GPIO_A_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_3_GPIO_A_IN          ((CODE_KEY_3_GPIO_A_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_3_GPIO_A_INT         ((CODE_KEY_3_GPIO_A_BANK - 'A') *  5 + GPIO_A_INT)
#define CODE_KEY_3_GPIO_B_PIN_MASK    (1 << CODE_KEY_3_GPIO_B_PIN)
#define CODE_KEY_3_GPIO_B_IE          ((CODE_KEY_3_GPIO_B_BANK - 'A') * 12 + GPIO_A_IE)
#define CODE_KEY_3_GPIO_B_OE          ((CODE_KEY_3_GPIO_B_BANK - 'A') * 12 + GPIO_A_OE)
#define CODE_KEY_3_GPIO_B_PU          ((CODE_KEY_3_GPIO_B_BANK - 'A') * 12 + GPIO_A_PU)
#define CODE_KEY_3_GPIO_B_PD          ((CODE_KEY_3_GPIO_B_BANK - 'A') * 12 + GPIO_A_PD)
#define CODE_KEY_3_GPIO_B_IN          ((CODE_KEY_3_GPIO_B_BANK - 'A') * 12 + GPIO_A_IN)
#define CODE_KEY_3_GPIO_B_INT         ((CODE_KEY_3_GPIO_B_BANK - 'A') * 5  + GPIO_A_INT)
#endif
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
//CODEKEY 1
#if CODE_KEY_1_OPEN
	//enable pull up resister.
	GPIO_RegOneBitSet(CODE_KEY_1_GPIO_A_IE, CODE_KEY_1_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_1_GPIO_A_OE, CODE_KEY_1_GPIO_A_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_1_GPIO_A_PU, CODE_KEY_1_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_1_GPIO_A_PD, CODE_KEY_1_GPIO_A_PIN_MASK);

	//enable pull up resister.
    GPIO_RegOneBitSet(CODE_KEY_1_GPIO_B_IE, CODE_KEY_1_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_1_GPIO_B_OE, CODE_KEY_1_GPIO_B_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_1_GPIO_B_PU, CODE_KEY_1_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_1_GPIO_B_PD, CODE_KEY_1_GPIO_B_PIN_MASK);
#endif
//CODEKEY 2
#if CODE_KEY_2_OPEN
	//enable pull up resister.
	GPIO_RegOneBitSet(CODE_KEY_2_GPIO_A_IE, CODE_KEY_2_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_2_GPIO_A_OE, CODE_KEY_2_GPIO_A_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_2_GPIO_A_PU, CODE_KEY_2_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_2_GPIO_A_PD, CODE_KEY_2_GPIO_A_PIN_MASK);

	//enable pull up resister.
    GPIO_RegOneBitSet(CODE_KEY_2_GPIO_B_IE, CODE_KEY_2_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_2_GPIO_B_OE, CODE_KEY_2_GPIO_B_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_2_GPIO_B_PU, CODE_KEY_2_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_2_GPIO_B_PD, CODE_KEY_2_GPIO_B_PIN_MASK);
#endif
//CODEKEY 3
#if CODE_KEY_3_OPEN
	//enable pull up resister.
	GPIO_RegOneBitSet(CODE_KEY_3_GPIO_A_IE, CODE_KEY_3_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_3_GPIO_A_OE, CODE_KEY_3_GPIO_A_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_3_GPIO_A_PU, CODE_KEY_3_GPIO_A_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_3_GPIO_A_PD, CODE_KEY_3_GPIO_A_PIN_MASK);

	//enable pull up resister.
    GPIO_RegOneBitSet(CODE_KEY_3_GPIO_B_IE, CODE_KEY_3_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_3_GPIO_B_OE, CODE_KEY_3_GPIO_B_PIN_MASK);
	GPIO_RegOneBitSet(CODE_KEY_3_GPIO_B_PU, CODE_KEY_3_GPIO_B_PIN_MASK);
	GPIO_RegOneBitClear(CODE_KEY_3_GPIO_B_PD, CODE_KEY_3_GPIO_B_PIN_MASK);
#endif
}

// Key process, image key value to key event.
CodeKeyType CodeKeyScan(void)
{
	CodeKeyType CodeKey = CODE_KEY_NONE;
	switch (gCtrlVars.CodeKey)
	{
#if CODE_KEY_1_OPEN
	case CODE_KEY_1_FORWARD:
		CodeKey = CODE_KEY_1_FORWARD;
		break;
	case CODE_KEY_1_BACKWARD:
		CodeKey = CODE_KEY_1_BACKWARD;
		break;
#endif
#if CODE_KEY_2_OPEN
	case CODE_KEY_2_FORWARD:
		CodeKey = CODE_KEY_2_FORWARD;
		break;
	case CODE_KEY_2_BACKWARD:
		CodeKey = CODE_KEY_2_BACKWARD;
		break;
#endif
#if CODE_KEY_3_OPEN
	case CODE_KEY_3_FORWARD:
		CodeKey = CODE_KEY_3_FORWARD;
		break;
	case CODE_KEY_3_BACKWARD:
		CodeKey = CODE_KEY_3_BACKWARD;
		break;
#endif
	default:
		CodeKey = CODE_KEY_NONE;
		break;
	}
	gCtrlVars.CodeKey = CODE_KEY_NONE;
	return CodeKey;
}

#if CODE_KEY_1_OPEN
#define CODE_KEY_1_P_IN     	GPIO_RegOneBitGet(CODE_KEY_1_GPIO_A_IN, CODE_KEY_1_GPIO_A_PIN_MASK)
#define CODE_KEY_1_N_IN     	GPIO_RegOneBitGet(CODE_KEY_1_GPIO_B_IN, CODE_KEY_1_GPIO_B_PIN_MASK)
#endif
#if CODE_KEY_2_OPEN
#define CODE_KEY_2_P_IN     	GPIO_RegOneBitGet(CODE_KEY_2_GPIO_A_IN, CODE_KEY_2_GPIO_A_PIN_MASK)
#define CODE_KEY_2_N_IN     	GPIO_RegOneBitGet(CODE_KEY_2_GPIO_B_IN, CODE_KEY_2_GPIO_B_PIN_MASK)
#endif
#if CODE_KEY_3_OPEN
#define CODE_KEY_3_P_IN     	GPIO_RegOneBitGet(CODE_KEY_3_GPIO_A_IN, CODE_KEY_3_GPIO_A_PIN_MASK)
#define CODE_KEY_3_N_IN     	GPIO_RegOneBitGet(CODE_KEY_3_GPIO_B_IN, CODE_KEY_3_GPIO_B_PIN_MASK)
#endif

/*
	放在定时器中调用,void Timer2Interrupt(void)
*/
void CodeKeyCheckInTimer(void)
{
	
#if CODE_KEY_1_OPEN
	// 定义了两个变量用来储蓄上一次调用此方法是编码开关两引脚的电平
	static uint8_t codekey_1_p_old = 0, codekey_1_n_old = 0;
	// 定义了一个变量用来储蓄以前是否出现了两个引脚都为高电平的状态
	static uint8_t codekey_1_flag = 0;
#endif
#if CODE_KEY_2_OPEN
	static uint8_t codekey_2_p_old = 0, codekey_2_n_old = 0;
	static uint8_t codekey_2_flag = 0;
#endif
#if CODE_KEY_3_OPEN
	static uint8_t codekey_3_p_old = 0, codekey_3_n_old = 0;
	static uint8_t codekey_3_flag = 0; 
#endif
#if CODE_KEY_1_OPEN
    if ((CODE_KEY_1_P_IN && CODE_KEY_1_N_IN)) {
        codekey_1_flag = 1;
    }

    if (codekey_1_flag) {
        if (((!CODE_KEY_1_P_IN) && (!CODE_KEY_1_N_IN))) {
            if (codekey_1_n_old) {   //为高说明编码开关在向加大的方向转
                codekey_1_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_1_FORWARD; 
            }
            if (codekey_1_p_old) {   //为高说明编码开关在向减小的方向转
                codekey_1_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_1_BACKWARD; 
            }
        }
    }
	
    if (CODE_KEY_1_P_IN) codekey_1_p_old = 1; else codekey_1_p_old = 0;
    if (CODE_KEY_1_N_IN) codekey_1_n_old = 1; else codekey_1_n_old = 0;
#endif

#if CODE_KEY_2_OPEN
	if ((CODE_KEY_2_P_IN && CODE_KEY_2_N_IN)) {
        codekey_2_flag = 1;
    }

    if (codekey_2_flag) {
        if (((!CODE_KEY_2_P_IN) && (!CODE_KEY_2_N_IN))) {
            if (codekey_2_n_old) {   //为高说明编码开关在向加大的方向转
                codekey_2_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_2_FORWARD; 
            }
            if (codekey_2_p_old) {   //为高说明编码开关在向减小的方向转
                codekey_2_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_2_BACKWARD; 
            }
        }
    }
	
    if (CODE_KEY_2_P_IN) codekey_2_p_old = 1; else codekey_2_p_old = 0;
    if (CODE_KEY_2_N_IN) codekey_2_n_old = 1; else codekey_2_n_old = 0;
#endif

#if CODE_KEY_3_OPEN
	if ((CODE_KEY_3_P_IN && CODE_KEY_3_N_IN)) {
        codekey_3_flag = 1;
    }

    if (codekey_3_flag) {
        if (((!CODE_KEY_3_P_IN) && (!CODE_KEY_3_N_IN))) {
            if (codekey_3_n_old) {   //为高说明编码开关在向加大的方向转
                codekey_3_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_3_FORWARD; 
            }
            if (codekey_3_p_old) {   //为高说明编码开关在向减小的方向转
                codekey_3_flag = 0;
				gCtrlVars.CodeKey = CODE_KEY_3_BACKWARD; 
            }
        }
    }
	
    if (CODE_KEY_3_P_IN) codekey_3_p_old = 1; else codekey_3_p_old = 0;
    if (CODE_KEY_3_N_IN) codekey_3_n_old = 1; else codekey_3_n_old = 0;
#endif
}

#endif //CFG_RES_CODE_KEY_USE

