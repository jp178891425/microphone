/**
 **************************************************************************************
 * @file    code_key.h
 * @brief   code key 
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-1-17 16:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */


#ifndef __CODE_KEY_H__
#define __CODE_KEY_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <type.h>

#define CODE_KEY_1_OPEN     1
#if CODE_KEY_1_OPEN
    #define CFG_CODE_KEY1P_BANK				'A'
    #define CFG_CODE_KEY1P_PIN				(29)
    #define CFG_CODE_KEY1N_BANK				'A'
    #define CFG_CODE_KEY1N_PIN				(30)
#endif
#define CODE_KEY_2_OPEN     0
#if CODE_KEY_2_OPEN
    #define CFG_CODE_KEY2P_BANK				'A'
    #define CFG_CODE_KEY2P_PIN				(29)
    #define CFG_CODE_KEY2N_BANK				'A'
    #define CFG_CODE_KEY2N_PIN				(30)
#endif
#define CODE_KEY_3_OPEN     0
#if CODE_KEY_3_OPEN
    #define CFG_CODE_KEY3P_BANK				'A'
    #define CFG_CODE_KEY3P_PIN				(29)
    #define CFG_CODE_KEY3N_BANK				'A'
    #define CFG_CODE_KEY3N_PIN				(30)
#endif


typedef enum _CodeKeyType
{
    CODE_KEY_NONE = 0,
    CODE_KEY_1_FORWARD,
    CODE_KEY_1_BACKWARD,
    CODE_KEY_2_FORWARD,
    CODE_KEY_2_BACKWARD,
    CODE_KEY_3_FORWARD,
    CODE_KEY_3_BACKWARD,

}CodeKeyType;

// Initialize coding key scan (GPIO) operation.
// Config interrupt at negative edge of signal-A
void CodeKeyInit(void);

// Key process, image key value to key event.
CodeKeyType CodeKeyScan(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__CODE_KEY_H__
