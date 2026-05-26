/**
 *************************************************************************************
 * @file	adc_levels.h
 * @author	ken bu/bkd
 * @version	v0.0.1
 * @date    2019/04/24
 * @brief	 for  Sliding rheostat
 * @ maintainer: 
 * Copyright (C) Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************
 */
#include "app_config.h"
#include <stdint.h>

#ifdef CFG_ADC_LEVEL_KEY_EN

#define how_many_times_have_effect	4		//值越大，受毛刺影响越少，但是速度越慢，推荐值3-5
#define RANGER_FILTER_LEVEL			25		//限幅滤波的值	无蓝牙：12，有蓝牙：25 

void ADCLevelsKeyInit(void);
uint16_t  AdcLevelKeyOneChanScan(uint8_t ADCLevelsScanNum);
uint16_t AdcLevelKeyProcess(void);

#endif//__POTENTIOMETER_KEY_H__
