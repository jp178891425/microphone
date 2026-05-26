/**
 *************************************************************************************
 * @file	adc_levels.c
 * @author	ken bu/bkd
 * @version	v0.0.1
 * @date    2019/04/24
 * @brief	 for  Sliding rheostat
 * @ maintainer: 
 * Copyright (C) Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#include "app_config.h"
#include "app_message.h"

#ifdef CFG_ADC_LEVEL_KEY_EN
#include "adc_levels.h"
#include "adc.h"
//#include "config.h"
#include "timeout.h"
#include "gpio.h"
#include "debug.h"
#include "key.h"
#include "seqlist.h"

/*
Lv000:[   0,   47]
Lv001:[  47,   94]
Lv002:[  94,  141]
Lv003:[ 141,  188]
Lv004:[ 188,  235]
Lv005:[ 235,  282]
Lv006:[ 282,  329]
Lv007:[ 329,  376]
Lv008:[ 376,  423]
Lv009:[ 423,  470]
Lv010:[ 470,  517]
Lv011:[ 517,  564]
Lv012:[ 564,  611]
Lv013:[ 611,  658]
Lv014:[ 658,  705]
Lv015:[ 705,  752]
Lv016:[ 752,  799]
Lv017:[ 799,  846]
Lv018:[ 846,  893]
Lv019:[ 893,  940]
Lv020:[ 940,  987]
Lv021:[ 987, 1034]
Lv022:[1034, 1081]
Lv023:[1081, 1128]
Lv024:[1128, 1175]
Lv025:[1175, 1222]
Lv026:[1222, 1269]
Lv027:[1269, 1316]
Lv028:[1316, 1363]
Lv029:[1363, 1410]
Lv030:[1410, 1457]
Lv031:[1457, 1504]
Lv032:[1504, 1551]
Lv033:[1551, 1598]
Lv034:[1598, 1645]
Lv035:[1645, 1692]
Lv036:[1692, 1739]
Lv037:[1739, 1786]
Lv038:[1786, 1833]
Lv039:[1833, 1900]
Lv040:[1900, 2150]	//中间
Lv041:[2150, 2198]
Lv042:[2198, 2246]
Lv043:[2246, 2294]
Lv044:[2294, 2342]
Lv045:[2342, 2390]
Lv046:[2390, 2438]
Lv047:[2438, 2486]
Lv048:[2486, 2534]
Lv049:[2534, 2582]
Lv050:[2582, 2630]
Lv051:[2630, 2678]
Lv052:[2678, 2726]
Lv053:[2726, 2774]
Lv054:[2774, 2822]
Lv055:[2822, 2870]
Lv056:[2870, 2918]
Lv057:[2918, 2966]
Lv058:[2966, 3014]
Lv059:[3014, 3062]
Lv060:[3062, 3110]
Lv061:[3110, 3158]
Lv062:[3158, 3206]
Lv063:[3206, 3254]
Lv064:[3254, 3302]
Lv065:[3302, 3350]
Lv066:[3350, 3398]
Lv067:[3398, 3446]
Lv068:[3446, 3494]
Lv069:[3494, 3542]
Lv070:[3542, 3590]
Lv071:[3590, 3638]
Lv072:[3638, 3686]
Lv073:[3686, 3734]
Lv074:[3734, 3782]
Lv075:[3782, 3830]
Lv076:[3830, 3878]
Lv077:[3878, 3926]
Lv078:[3926, 3974]
Lv079:[3974, 4022]
Lv080:[4022, 4096]
*/
static const uint16_t MyMin[] = {
       0, 
      47,   94,  141,  188,  235,  282,  329,  376,  423,  470,  517,  564,  611,  658,  705,  752,  799,  846,  893,  940, 
     987, 1034, 1081, 1128, 1175, 1222, 1269, 1316, 1363, 1410, 1457, 1504, 1551, 1598, 1645, 1692, 1739, 1786, 1833, 1900,
    2150, 2198, 2246, 2294, 2342, 2390, 2438, 2486, 2534, 2582, 2630, 2678, 2726, 2774, 2822, 2870, 2918, 2966, 3014, 3062,
    3110, 3158, 3206, 3254, 3302, 3350, 3398, 3446, 3494, 3542, 3590, 3638, 3686, 3734, 3782, 3830, 3878, 3926, 3974, 4022,
};
static const uint16_t MyMax[] = {
      47,
      94,  141,  188,  235,  282,  329,  376,  423,  470,  517,  564,  611,  658,  705,  752,  799,  846,  893,  940,  987,
    1034, 1081, 1128, 1175, 1222, 1269, 1316, 1363, 1410, 1457, 1504, 1551, 1598, 1645, 1692, 1739, 1786, 1833, 1900, 2150,
    2198, 2246, 2294, 2342, 2390, 2438, 2486, 2534, 2582, 2630, 2678, 2726, 2774, 2822, 2870, 2918, 2966, 3014, 3062, 3110,
    3158, 3206, 3254, 3302, 3350, 3398, 3446, 3494, 3542, 3590, 3638, 3686, 3734, 3782, 3830, 3878, 3926, 3974, 4022, 4096,
};

static uint8_t ADCLevelsScanCount        = 0;
static uint8_t ADCLevelChannelTotal      = 0;
static uint16_t ADCLevelsChannel[14];
static uint8_t repeat_count[14]          = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static uint16_t ADCLevels_STEP_Store[14]  = {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff};
static uint16_t ADCLevels_Msg_Ch[14]  = {MSG_ADC_LEVEL_CH1,MSG_ADC_LEVEL_CH2,MSG_ADC_LEVEL_CH3,MSG_ADC_LEVEL_CH4,MSG_ADC_LEVEL_CH5,MSG_ADC_LEVEL_CH6,MSG_ADC_LEVEL_CH7,MSG_ADC_LEVEL_CH8,MSG_ADC_LEVEL_CH9,MSG_ADC_LEVEL_CH10,MSG_ADC_LEVEL_CH11};

static volatile uint16_t Val_Store[14];
static SeqList ADCValSeqList[14];
void AdcChannelOpen_n_Ignore(uint32_t adc_ch);
uint16_t MyGetAdcVal(uint32_t adc_ch);
void AdcChannelClose(uint32_t adc_ch);
uint8_t RangeFilter(uint16_t Val, uint8_t Channel);

/*
****************************************************************
* 全部ADC通道初始化列表
*
*
****************************************************************
*/
const uint32_t ADC_CHANNEL_Init_Tab[14*2]=
{

	GPIO_A_ANA_EN, GPIO_INDEX20,/**channel 0*/

	GPIO_A_ANA_EN, GPIO_INDEX21,/**channel 1*/

	GPIO_A_ANA_EN, GPIO_INDEX22,/**channel 2*/

	GPIO_A_ANA_EN, GPIO_INDEX23,/**channel 3*/

	GPIO_A_ANA_EN, GPIO_INDEX24,/**channel 4*/

	GPIO_A_ANA_EN, GPIO_INDEX25,/**channel 5*/

	GPIO_A_ANA_EN, GPIO_INDEX26,/**channel 6*/

	GPIO_A_ANA_EN, GPIO_INDEX27,/**channel 7*/

	GPIO_A_ANA_EN, GPIO_INDEX28,/**channel 8*/

	GPIO_A_ANA_EN, GPIO_INDEX29,/**channel 9*/

	GPIO_A_ANA_EN, GPIO_INDEX30,/**channel 10*/

	GPIO_A_ANA_EN, GPIO_INDEX31,/**channel 11*/

	GPIO_B_ANA_EN, GPIO_INDEX0,	/**channel 12*/

	GPIO_B_ANA_EN, GPIO_INDEX1,	/**channel 13*/

};
	
/*
****************************************************************
*  ADC通道选择列表
*
*
****************************************************************
*/
const uint32_t ADC_CHANNEL_Select_Tab[14]=
{
	ADC_GPIOA20,
	ADC_GPIOA21,
	ADC_GPIOA22,
	ADC_GPIOA23,
	ADC_GPIOA24,
	ADC_GPIOA25,
	ADC_GPIOA26,
	ADC_GPIOA27,
	ADC_GPIOA28,
	ADC_GPIOA29,
	ADC_GPIOA30,
	ADC_GPIOA31,
	ADC_GPIOB0,
	ADC_GPIOB1,
};

/*
****************************************************************
* adc电位器初始化函数
*
*
****************************************************************
*/
void ADCLevelsKeyInit(void)
{
    uint8_t k;

    uint32_t adc_ch;

	ADCLevelChannelTotal = 0;

    adc_ch = ADCLEVL_CHANNEL_MAP;
	
	for(k = 0; k < (sizeof(ADC_CHANNEL_Select_Tab)/sizeof(ADC_CHANNEL_Select_Tab[0])); k++)
	{
		switch(adc_ch & ADC_CHANNEL_Select_Tab[k])
		{
		    case ADC_GPIOA20:
            case ADC_GPIOA21:
            case ADC_GPIOA22:
            case ADC_GPIOA23:
            case ADC_GPIOA24:
    			ADCLevelsChannel[ADCLevelChannelTotal] = ADC_CHANNEL_Select_Tab[k];
    			ADCLevelChannelTotal++;
                break;
            case ADC_GPIOA25:
            case ADC_GPIOA26:
            case ADC_GPIOA27:
            case ADC_GPIOA28:
            case ADC_GPIOA29:
            case ADC_GPIOA30:
            case ADC_GPIOA31:
            case ADC_GPIOB0:
            case ADC_GPIOB1:
    			GPIO_RegOneBitSet(ADC_CHANNEL_Init_Tab[k*2], ADC_CHANNEL_Init_Tab[k*2 + 1]);
    			ADCLevelsChannel[ADCLevelChannelTotal] = ADC_CHANNEL_Select_Tab[k];
    			ADCLevelChannelTotal++;
                break;
		    default:
                break;
		}
	}

	for(k=0; k<ADCLevelChannelTotal; k++)
	{
		SeqListInit(&ADCValSeqList[k]);
	}

    /* 可以防止开机时触发一次AD显示 */
    extern void AdcLevelParamSync(void);
    AdcLevelParamSync();
}
/*
****************************************************************
* 扫描一路adc电位器处理
*
*
****************************************************************
*/
uint16_t  AdcLevelKeyOneChanScan(uint8_t ADCLevelsScanNum)
{
    uint32_t    TotalVal;
    uint8_t     i_count;
    uint16_t    Val;

	if(ADCLevelChannelTotal == 0) return MSG_NONE;
	
    /* 打开ADC复用通道，并采样2次舍弃掉 */
    AdcChannelOpen_n_Ignore(ADCLevelsChannel[ADCLevelsScanNum]);
    
    /* 如果顺序表不满，则填满 */
    while(ADCValSeqList[ADCLevelsScanNum].size < ADCValSeqList[ADCLevelsScanNum].capacity)
    {
        Val = MyGetAdcVal(ADCLevelsChannel[ADCLevelsScanNum]);    
        SeqListPushBack(&ADCValSeqList[ADCLevelsScanNum], (SeqListDataType)Val);
    }

    /* 关闭ADC复用通道 */
    AdcChannelClose(ADCLevelsChannel[ADCLevelsScanNum]);
    
    /* 数量足够，排序 */
    SeqListSort(&ADCValSeqList[ADCLevelsScanNum]);

    /* 删除几个最大值 */
    ADCValSeqList[ADCLevelsScanNum].size -= MAX_IGNORE_NUM;
    
    /* 删除几个最小值 */
    for(i_count=0; i_count<MIN_IGNORE_NUM; i_count++)
    {
        SeqListPopFront(&ADCValSeqList[ADCLevelsScanNum]);
    }

    /* 排序后，对有效部分取平均值 */
    TotalVal = 0;
    for(i_count=0; i_count<ADCValSeqList[ADCLevelsScanNum].size; i_count++)
    {
        TotalVal += ADCValSeqList[ADCLevelsScanNum].base[i_count];//求和
    }
    Val = TotalVal / ADCValSeqList[ADCLevelsScanNum].size;//取平均

#if 1//[XH]原SDK
	for(i_count=0;i_count < MAX_ADCLEVL_STEP_NUMBER;i_count++)
	{
		uint32_t min = MAX_ADCLEVL_LEVEL_VAL/MAX_ADCLEVL_STEP_NUMBER*i_count - DISTANCE_BETWEEN_STEP;
		uint32_t max = MAX_ADCLEVL_LEVEL_VAL/MAX_ADCLEVL_STEP_NUMBER*(i_count+1) + DISTANCE_BETWEEN_STEP;
		if(i_count == 0)
		{
			min = 0;
		}
		if(i_count == (MAX_ADCLEVL_STEP_NUMBER-1))
		{
			max = MAX_ADCLEVL_LEVEL_VAL;
		}
		if(Val >= min && Val <= max)
		{
			break;
		}
	}
#else//提前算好数组
    uint32_t min, max;
	for(i_count=0; i_count<MAX_ADCLEVL_STEP_NUMBER; i_count++)
    {
        min = MyMin[i_count];
        max = MyMax[i_count];
        
        if(Val >= min && Val <= max)
        {
            break;
        }
    }   
#endif

	ADCLevels_STEP_Store[ADCLevelsScanNum] =  i_count;
    Val_Store[ADCLevelsScanNum] = Val;
	return i_count;
}
/*
****************************************************************
* adc电位器扫描处理
*
*
****************************************************************
*/
uint16_t  AdcLevelKeyProcess(void)
{
    uint32_t    TotalVal;
    uint8_t     i_count;
    uint16_t    Val;

    if(ADCLevelChannelTotal == 0) return MSG_NONE;

	ADCLevelsScanCount++;

	if(ADCLevelsScanCount >= ADCLevelChannelTotal)
	{
		ADCLevelsScanCount = 0;
	}

    /* 打开ADC复用通道，并采样2次舍弃掉 */
    AdcChannelOpen_n_Ignore(ADCLevelsChannel[ADCLevelsScanCount]);
    
    /* 如果顺序表不满，则填满 */
    while(ADCValSeqList[ADCLevelsScanCount].size < ADCValSeqList[ADCLevelsScanCount].capacity)
    {
        Val = MyGetAdcVal(ADCLevelsChannel[ADCLevelsScanCount]);    
        SeqListPushBack(&ADCValSeqList[ADCLevelsScanCount], (SeqListDataType)Val);
    }

    /* 关闭ADC复用通道 */
    AdcChannelClose(ADCLevelsChannel[ADCLevelsScanCount]);

    /* 数量足够，排序 */
    SeqListSort(&ADCValSeqList[ADCLevelsScanCount]);

    /* 删除几个最大值 */
    ADCValSeqList[ADCLevelsScanCount].size -= MAX_IGNORE_NUM;

    /* 删除几个最小值 */
    for(i_count=0; i_count<MIN_IGNORE_NUM; i_count++)
    {
        SeqListPopFront(&ADCValSeqList[ADCLevelsScanCount]);
    }
    
    /* 排序后，对有效部分取平均值 */
    TotalVal = 0;
    for(i_count=0; i_count<ADCValSeqList[ADCLevelsScanCount].size; i_count++)
    {
        TotalVal += ADCValSeqList[ADCLevelsScanCount].base[i_count];//求和
    }
    Val = TotalVal / ADCValSeqList[ADCLevelsScanCount].size;//取平均
    
    /* 限幅滤波 */
    if(RangeFilter(Val, ADCLevelsScanCount) == 0)
    {
        repeat_count[ADCLevelsScanCount] = 0;
        //DBG("限幅+1\n");
        
        return MSG_NONE;
    }
    else
    {
        repeat_count[ADCLevelsScanCount]++;

#if 1
        if(repeat_count[ADCLevelsScanCount] > how_many_times_have_effect)
        {
            Val_Store[ADCLevelsScanCount] = Val;//[XH]
            repeat_count[ADCLevelsScanCount] = 0;
            //DBG("累计:%u\n", repeat_count[ADCLevelsScanCount]);
        }
        else
        {
            //DBG("误差+1:%u\n", repeat_count[ADCLevelsScanCount]);
            return MSG_NONE;
        }
#else//[XH]开蓝牙会跳动
        Val_Store[ADCLevelsScanCount] = Val;//[XH]
#endif
    }

#if 1//[XH]原SDK
	for(i_count=0;i_count < MAX_ADCLEVL_STEP_NUMBER;i_count++)
	{
		uint32_t min = MAX_ADCLEVL_LEVEL_VAL/MAX_ADCLEVL_STEP_NUMBER*i_count - DISTANCE_BETWEEN_STEP;
		uint32_t max = MAX_ADCLEVL_LEVEL_VAL/MAX_ADCLEVL_STEP_NUMBER*(i_count+1) + DISTANCE_BETWEEN_STEP;
		if(i_count == 0)
		{
			min = 0;
		}
		if(i_count == (MAX_ADCLEVL_STEP_NUMBER-1))
		{
			max = MAX_ADCLEVL_LEVEL_VAL;
		}
		if(Val >= min && Val <= max)
		{
			break;
		}
	}
#else//提前算好数组
    uint32_t min, max;
    for(i_count=0; i_count<MAX_ADCLEVL_STEP_NUMBER; i_count++)
    {
        min = MyMin[i_count];
        max = MyMax[i_count];
        
        if(Val >= min && Val <= max)
        {
            break;
        }
    }   
#endif

	if(i_count != MAX_ADCLEVL_STEP_NUMBER)
	{
		if(i_count != ADCLevels_STEP_Store[ADCLevelsScanCount] )
		{
            ADCLevels_STEP_Store[ADCLevelsScanCount] =  i_count;
            return (ADCLevels_Msg_Ch[ADCLevelsScanCount]+i_count);
		}
	}
	return MSG_NONE;
}
#endif


/* 整体思路
   - 打开ADC复用通道，并采样2次舍弃掉
   - 多次调用MyGetAdcVal()填充顺序表
   - 关闭ADC复用通道
*/

/* 适用于ADC复用的情况- 修改GetAdcVal() */
void AdcChannelOpen_n_Ignore(uint32_t adc_ch)
{
    switch(adc_ch)
    {
        case ADC_GPIOA20:
            GPIO_RegOneBitSet(GPIO_A_ANA_EN, GPIO_INDEX20);//若是用到复用的ADC口，例如A20和A23口做ADC，需要打开此代码
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA20_A23);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA20_A23);
            break;
        case ADC_GPIOA21:
            GPIO_RegOneBitSet(GPIO_A_ANA_EN, GPIO_INDEX21);//若是用到复用的ADC口，例如A21和A24口做ADC，需要打开此代码
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA21_A24);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA21_A24);
            break;
        case ADC_GPIOA22:
            GPIO_RegOneBitSet(GPIO_A_ANA_EN, GPIO_INDEX22);//若是用到复用的ADC口，例如A22和A25口做ADC，需要打开此代码
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA22_A25);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA22_A25);
            break;
        case ADC_GPIOA23:
            GPIO_RegOneBitSet(GPIO_A_ANA_EN, GPIO_INDEX23);//若是用到复用的ADC口，例如A20和A23口做ADC，需要打开此代码
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA20_A23);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA20_A23);
            break;
        case ADC_GPIOA24:
            GPIO_RegOneBitSet(GPIO_A_ANA_EN, GPIO_INDEX24);//若是用到复用的ADC口，例如A21和A24口做ADC，需要打开此代码
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA21_A24);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA21_A24);
            break;
        case ADC_GPIOA25:
            GPIO_RegOneBitSet(GPIO_A_ANA_EN, GPIO_INDEX25);//若是用到复用的ADC口，例如A22和A25口做ADC，需要打开此代码
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA22_A25);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA22_A25);
            break;
        case ADC_GPIOA26:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA26);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA26);
            break;
        case ADC_GPIOA27:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA27);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA27);
            break;
        case ADC_GPIOA28:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA28);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA28);
            break;
        case ADC_GPIOA29:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA29);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA29);
            break;
        case ADC_GPIOA30:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA30);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA30);
            break;
        case ADC_GPIOA31:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA31);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA31);
            break;
        case ADC_GPIOB0:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB0);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB0);
            break;
        case ADC_GPIOB1:
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB1);
            ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB1);
            break;
        default:
            break;
    }
}

/* 适用于ADC复用的情况- 修改GetAdcVal() */
uint16_t MyGetAdcVal(uint32_t adc_ch)
{
    uint16_t Val = 0x00;

    switch(adc_ch)
    {
        case ADC_GPIOA20:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA20_A23);
            break;
        case ADC_GPIOA21:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA21_A24);
            break;
        case ADC_GPIOA22:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA22_A25);
            break;
        case ADC_GPIOA23:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA20_A23);
            break;
        case ADC_GPIOA24:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA21_A24);
            break;
        case ADC_GPIOA25:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA22_A25);
            break;
        case ADC_GPIOA26:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA26);
            break;
        case ADC_GPIOA27:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA27);
            break;
        case ADC_GPIOA28:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA28);
            break;
        case ADC_GPIOA29:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA29);
            break;
        case ADC_GPIOA30:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA30);
            break;
        case ADC_GPIOA31:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA31);
            break;
        case ADC_GPIOB0:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB0);
            break;
        case ADC_GPIOB1:
            Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB1);
            break;
        default:
            Val = 0x00;
            break;
    }

    return Val;
}

/* 适用于ADC复用的情况- 修改GetAdcVal() */
void AdcChannelClose(uint32_t adc_ch)
{
    switch(adc_ch)
    {
        case ADC_GPIOA20:
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX20);//若是用到复用的ADC口，例如A20和A23口做ADC，需要打开此代码
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX23);//若是用到复用的ADC口，例如A20和A23口做ADC，需要打开此代码
            break;
        case ADC_GPIOA21:
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX21);//若是用到复用的ADC口，例如A21和A24口做ADC，需要打开此代码
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX24);//若是用到复用的ADC口，例如A21和A24口做ADC，需要打开此代码
            break;
        case ADC_GPIOA22:
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX22);//若是用到复用的ADC口，例如A22和A25口做ADC，需要打开此代码
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX25);//若是用到复用的ADC口，例如A22和A25口做ADC，需要打开此代码
            break;
        case ADC_GPIOA23:
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX20);//若是用到复用的ADC口，例如A20和A23口做ADC，需要打开此代码
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX23);//若是用到复用的ADC口，例如A20和A23口做ADC，需要打开此代码
            break;
        case ADC_GPIOA24:
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX21);//若是用到复用的ADC口，例如A21和A24口做ADC，需要打开此代码
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX24);//若是用到复用的ADC口，例如A21和A24口做ADC，需要打开此代码
            break;
        case ADC_GPIOA25:
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX22);//若是用到复用的ADC口，例如A22和A25口做ADC，需要打开此代码
            GPIO_RegOneBitClear(GPIO_A_ANA_EN, GPIO_INDEX25);//若是用到复用的ADC口，例如A22和A25口做ADC，需要打开此代码
            break;
        case ADC_GPIOA26:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA26);
            break;
        case ADC_GPIOA27:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA27);
            break;
        case ADC_GPIOA28:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA28);
            break;
        case ADC_GPIOA29:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA29);
            break;
        case ADC_GPIOA30:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA30);
            break;
        case ADC_GPIOA31:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOA31);
            break;
        case ADC_GPIOB0:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB0);
            break;
        case ADC_GPIOB1:
            //Val = ADC_SingleModeDataGet(ADC_CHANNEL_GPIOB1);
            break;
        default:
            //Val = 0x00
            break;
    }
}

//0：马上退出
//1：继续执行
uint8_t RangeFilter(uint16_t Val, uint8_t Channel)
{
	int32_t i = Val - Val_Store[Channel];

	if(i < 0) i = -i;

	if(i <= 25)//无蓝牙：12，有蓝牙：25 
	{
		return 0;
	}
	else
	{
		return 1;
	}
}



