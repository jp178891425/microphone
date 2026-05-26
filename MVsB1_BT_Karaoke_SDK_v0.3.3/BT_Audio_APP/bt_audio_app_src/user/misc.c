/*
 **************************************************************************************
 * @file    misc.c
 * @brief    
 * 
 * @author  
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <stdint.h>
#include <string.h>
#include <nds32_intrinsic.h>
#include "remap.h"
#include "spi_flash.h"
#include "gpio.h"
#include "uarts.h"
#include "irqn.h"
#include "uarts_interface.h"
#include "timeout.h"
#include "debug.h"
#include "watchdog.h"
#include "clk.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc.h"
#include "delay.h"
#include "ctrlvars.h"
#include "app_config.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "audio_effect.h"
#include "audio_vol.h"
#include "chip_info.h"
#include "otg_device_stor.h"
#include "otg_device_standard_request.h"
#include "audio_adc.h"
#include "hw_interface.h"
#include "misc.h"
#include "audio_encryption.h"
#include "i2s.h"
#include "timer.h"
#include "main_task.h"

void MicVolSmoothProcess(void);
#if (defined(CFG_FUNC_DETECT_MIC_EN) || defined(CFG_FUNC_DETECT_PHONE_EN))
static TIMER MuteOffTimer;
#endif
/*
****************************************************************
*常用应用处理函数列表，用户可在此扩展应用
*
*
****************************************************************
*/
void (*HWdeviceList[4])(void)={
     //LedDisplay,
     DetectEarPhone,
     DetectMic,
     DetectMic3Or4Line,
     //DetectLineIn,
     //PowerMonitor,
     //PowerProcess,
     //DACVolumeSmoothProcess,
     MicVolSmoothProcess, 
};
/*
****************************************************************
*
*
*
****************************************************************
*/
void HWDeviceDected_Init(void)
{
	DetectEarPhone();
	
	DetectMic();
	
	DetectMic3Or4Line();
	
	#if CFG_DMA_GPIO_SIMULATE_PWM_EN
	DmaTimerCircleMode_GPIO_Init();
	DmaTimerCircleMode_GPIO_Config(50, 70, 90);
	#endif

	#if CFG_HW_PWM_EN
	HwPwmInit();
	HwPwmConfig(50);
	#endif
}
/*
****************************************************************
*常用应用处理主循环
*
*
****************************************************************
*/
void HWDeviceDected(void)
{
   ///////此函数仅调用对时间不作要求的功能////要求偶数次定时器//
    #define  HW_DECTED_DLY	 2
  	static TIMER HWDeviceTimer;
	static uint8_t HWDeviceInit=0;
	static uint8_t HWDeviceConut;
    
	if(HWDeviceInit==0)
	{
		HWDeviceInit = 1;
		HWDeviceConut = 0;
		TimeOutSet(&HWDeviceTimer, 0); 
		return ;
	}
	
    ShunningModeProcess();//闪避功能处理
    
	if(!IsTimeOut(&HWDeviceTimer)) return;

    TimeOutSet(&HWDeviceTimer, HW_DECTED_DLY); 
	
    HWDeviceConut++;
    if(HWDeviceConut > 3)  //HWDeviceConut &= 0x8;/////8 *2ms = 16ms
	{
		HWDeviceConut = 0;
	}
    (*HWdeviceList[HWDeviceConut])();
}
/*
****************************************************************
* MIC插拔检测处理demo
*
*
****************************************************************
*/
void DetectMic(void)
{
#ifdef CFG_FUNC_DETECT_MIC_EN
    #define MIC2_EN  1///for 3or4 line jack detecd.....
    //---------mic1 var--通用三节-----------------//
	static uint8_t MicInit=0;
	static uint8_t MicCnt = 0;
	//--------mic2 var-------------------///
	#if MIC2_EN
	static uint8_t Mic2Cnt = 0;
	#endif	
	uint8_t val;
	uint32_t msg;

	if(!MicInit)
	{
		MicInit = 1;
		MicCnt = 0;

		gCtrlVars.MicOnlin = 0;////default offline
		GPIO_RegOneBitClear(MIC1_DETECT_OE, MIC1_DETECT_PIN);
		GPIO_RegOneBitSet(MIC1_DETECT_IE, MIC1_DETECT_PIN);
		///PULL enable
		GPIO_RegOneBitSet(MIC1_DETECT_PU, MIC1_DETECT_PIN);
		GPIO_RegOneBitClear(MIC1_DETECT_PD, MIC1_DETECT_PIN);	
		DelayUs(50);
		if(GPIO_RegOneBitGet(MIC1_DETECT_IN,MIC1_DETECT_PIN))
		{
			gCtrlVars.MicOnlin = 1;
			APP_DBG("MIC 1 IN\n");
			TimeOutSet(&MuteOffTimer, 1000); 
		}
		////////mic2 detected/////////////////////
        #if MIC2_EN
		Mic2Cnt = 0;
		gCtrlVars.Mic2Onlin = 0;////default offline
		GPIO_RegOneBitClear(MIC2_DETECT_OE, MIC2_DETECT_PIN);
		GPIO_RegOneBitSet(MIC2_DETECT_IE, MIC2_DETECT_PIN);
		///PULL enable
		GPIO_RegOneBitSet(MIC2_DETECT_PU, MIC2_DETECT_PIN);
		GPIO_RegOneBitClear(MIC2_DETECT_PD, MIC2_DETECT_PIN); 
		DelayUs(50);
		if(!GPIO_RegOneBitGet(MIC2_DETECT_IN,MIC2_DETECT_PIN))
		{
			gCtrlVars.Mic2Onlin = 1;	 
			APP_DBG("MIC 2 IN\n");
			TimeOutSet(&MuteOffTimer, 1000); 
		}
        #endif
	}
	else
	{
		//------mic1检测处理--------------------------------------------------//
		if(GPIO_RegOneBitGet(MIC1_DETECT_IN,MIC1_DETECT_PIN))
		{
			if(++MicCnt > 50)
			{
				MicCnt = 0;
				if(gCtrlVars.MicOnlin ==0  )
				{
					gCtrlVars.MicOnlin = 1;	
					APP_DBG("MIC 1 IN\n");
					MUTE_ON();
					TimeOutSet(&MuteOffTimer, 1000); 
				} 			
			}
		}
		else
		{
			MicCnt = 0;
			if(gCtrlVars.MicOnlin )
			{
				gCtrlVars.MicOnlin = 0; 
				APP_DBG("MIC 1 OUT\n");
				MUTE_ON();
				TimeOutSet(&MuteOffTimer, 1000); 
			}
		}
		//------mic2 检测处理---------------------------------------------------//
        #if MIC2_EN
		if(!GPIO_RegOneBitGet(MIC2_DETECT_IN,MIC2_DETECT_PIN))
		{
			if(++Mic2Cnt > 50)
			{
				if(gCtrlVars.Mic2Onlin ==0  )
				{
					gCtrlVars.Mic2Onlin = 1;  
					APP_DBG("MIC 2 IN\n");
					MUTE_ON();
					TimeOutSet(&MuteOffTimer, 1000); 
				}
			}
		}
		else
		{
			Mic2Cnt = 0;
			if(gCtrlVars.Mic2Onlin )
			{
				gCtrlVars.Mic2Onlin = 0;	 
				APP_DBG("MIC 2 Out\n");
				MUTE_ON();
				TimeOutSet(&MuteOffTimer, 1000); 
			}
		}
        #endif
		if(IsTimeOut(&MuteOffTimer)) 
		{
			if(gCtrlVars.MicOnlin
			#ifdef CFG_FUNC_DETECT_PHONE_EN	
				|| gCtrlVars.EarPhoneOnlin
			#endif
			    || gCtrlVars.Mic2Onlin)
			{
				MUTE_OFF();
			}
		}
	}	
#endif	
}

/*************************************************
*    闪避处理函数
*
*
***************************************************/
void ShunningModeProcess(void)
{
	////20ms调用一次此函数//增益值(dyn_gain)闪避功能专用///0~4096 分16级，总为0~16 256为一级  
#ifdef CFG_FUNC_SHUNNING_EN
	static uint16_t shnning_recover_dly = 0;
	static uint16_t shnning_up_dly      = 0;
	static uint16_t shnning_down_dly    = 0;
	static uint16_t cnt = 0;
	 
	if( (gCtrlVars.ShunningMode == 0 )|| (gCtrlVars.MicAudioSdct_unit.enable==0) )
	{
	    if(shnning_up_dly)
		{
			shnning_up_dly--;
			return; 
		}
		shnning_up_dly = SHNNIN_UP_DLY;
	    if(gCtrlVars.aux_out_dyn_gain !=  gCtrlVars.aux_gain_control_unit.gain)//////阀值
		{
			if(gCtrlVars.aux_out_dyn_gain < gCtrlVars.aux_gain_control_unit.gain)
			{
				gCtrlVars.aux_out_dyn_gain += SHNNIN_STEP;
			}
			else if(gCtrlVars.aux_out_dyn_gain > gCtrlVars.aux_gain_control_unit.gain)
			{
			    if(gCtrlVars.aux_out_dyn_gain >= SHNNIN_STEP)
		    	{
					gCtrlVars.aux_out_dyn_gain -= SHNNIN_STEP;
		    	}
			}
			if(gCtrlVars.aux_out_dyn_gain > gCtrlVars.aux_gain_control_unit.gain)
			{
				gCtrlVars.aux_out_dyn_gain = gCtrlVars.aux_gain_control_unit.gain;
			}
		}
			#if defined(CFG_FUNC_REMIND_MIX_MODE) || defined(CFG_FUNC_REMIND_SOUND_EN)
		if(gCtrlVars.remind_out_dyn_gain !=  gCtrlVars.remind_effect_gain_control_unit.gain)//////阀值
		{
			if(gCtrlVars.remind_out_dyn_gain < gCtrlVars.remind_effect_gain_control_unit.gain)
			{
				gCtrlVars.remind_out_dyn_gain += SHNNIN_STEP;
			}
			else if(gCtrlVars.remind_out_dyn_gain > gCtrlVars.remind_effect_gain_control_unit.gain)
			{
			    if(gCtrlVars.remind_out_dyn_gain >= SHNNIN_STEP)
		    	{
					gCtrlVars.remind_out_dyn_gain -= SHNNIN_STEP;
		    	}
			}
			if(gCtrlVars.remind_out_dyn_gain > gCtrlVars.remind_effect_gain_control_unit.gain)
			{
				gCtrlVars.remind_out_dyn_gain = gCtrlVars.remind_effect_gain_control_unit.gain;
			}
		}
		#endif
#ifdef CFG_FUNC_USB_MIX_MODE		
#ifdef CFG_RES_AUDIO_USB_IN_EN
		if(gCtrlVars.usb_in_dyn_gain !=  gCtrlVars.usb_gain_control_unit.gain)//////阀值
		{
			if(gCtrlVars.usb_in_dyn_gain < gCtrlVars.usb_gain_control_unit.gain)
			{
				gCtrlVars.usb_in_dyn_gain += SHNNIN_STEP;
			}
			else if(gCtrlVars.usb_in_dyn_gain > gCtrlVars.usb_gain_control_unit.gain)
			{
				if(gCtrlVars.usb_in_dyn_gain >= SHNNIN_STEP)
				{
					gCtrlVars.usb_in_dyn_gain -= SHNNIN_STEP;
				}
			}
			if(gCtrlVars.usb_in_dyn_gain > gCtrlVars.usb_gain_control_unit.gain)
			{
				gCtrlVars.usb_in_dyn_gain = gCtrlVars.usb_gain_control_unit.gain;
			}
		}
#endif
#endif
		return;
	}
    cnt++;
	cnt &= 0x07;
	//if(cnt==0) APP_DBG("Sdct level:%ld\n",gCtrlVars.AudioSdct_unit.level);
	 	 
	if(gCtrlVars.MicAudioSdct_unit.level > SHNNIN_VALID_DATA)///vol----
	{
		shnning_recover_dly = SHNNIN_VOL_RECOVER_TIME;
        if(shnning_down_dly)
		{
			shnning_down_dly--;
			return; 
		}
        shnning_down_dly = SHNNIN_DOWN_DLY;

		if(gCtrlVars.aux_gain_control_unit.gain > SHNNIN_THRESHOLD)
		{
			if(gCtrlVars.aux_out_dyn_gain > SHNNIN_THRESHOLD)//////阀值 	     
			{
				if(gCtrlVars.aux_out_dyn_gain >= SHNNIN_STEP)
				{
					gCtrlVars.aux_out_dyn_gain -= SHNNIN_STEP;
				}
				APP_DBG("Aux Shunning start\n");
			}
		}
		else
		{
			gCtrlVars.aux_out_dyn_gain = gCtrlVars.aux_gain_control_unit.gain;
		}
			#if defined(CFG_FUNC_REMIND_MIX_MODE) || defined(CFG_FUNC_REMIND_SOUND_EN)
		if(gCtrlVars.remind_effect_gain_control_unit.gain > SHNNIN_THRESHOLD)
		{
			if(gCtrlVars.remind_out_dyn_gain > SHNNIN_THRESHOLD)//////阀值 	     
			{
				if(gCtrlVars.remind_out_dyn_gain >= SHNNIN_STEP)
		    	{
					gCtrlVars.remind_out_dyn_gain -= SHNNIN_STEP;
		    	}

				APP_DBG("Remind Shunning start\n");
			}
		}
		else
		{
			gCtrlVars.remind_out_dyn_gain = gCtrlVars.remind_effect_gain_control_unit.gain;
		}
		#endif
#ifdef CFG_FUNC_USB_MIX_MODE		
#ifdef CFG_RES_AUDIO_USB_IN_EN
		if(gCtrlVars.usb_gain_control_unit.gain > SHNNIN_THRESHOLD)
		{
			if(gCtrlVars.usb_in_dyn_gain > SHNNIN_THRESHOLD)//////阀值		 
			{
				if(gCtrlVars.usb_in_dyn_gain >= SHNNIN_STEP)
				{
					gCtrlVars.usb_in_dyn_gain -= SHNNIN_STEP;
				}

				DBG("Usb In Shunning start\n");
			}
		}
		else
		{
			gCtrlVars.usb_in_dyn_gain = gCtrlVars.usb_gain_control_unit.gain;
		}
#endif
#endif		
	}
	else/////vol+++
	{
		if(shnning_up_dly)
		{
			shnning_up_dly--;
			return; 
		}

		if(shnning_recover_dly)
		{
			shnning_recover_dly--;
			return;
		}
		shnning_up_dly = SHNNIN_UP_DLY;

		if(gCtrlVars.aux_out_dyn_gain !=  gCtrlVars.aux_gain_control_unit.gain)//////阀值
		{
			if(gCtrlVars.aux_out_dyn_gain < gCtrlVars.aux_gain_control_unit.gain)
			{
				gCtrlVars.aux_out_dyn_gain += SHNNIN_STEP;
			}
			else if(gCtrlVars.aux_out_dyn_gain > gCtrlVars.aux_gain_control_unit.gain)
			{
				if(gCtrlVars.aux_out_dyn_gain >= SHNNIN_STEP)
				{
					gCtrlVars.aux_out_dyn_gain -= SHNNIN_STEP;
				}
			}
			if(gCtrlVars.aux_out_dyn_gain > gCtrlVars.aux_gain_control_unit.gain)
			{
				gCtrlVars.aux_out_dyn_gain  = gCtrlVars.aux_gain_control_unit.gain;
			}
		}
			#if defined(CFG_FUNC_REMIND_MIX_MODE) || defined(CFG_FUNC_REMIND_SOUND_EN)
		if(gCtrlVars.remind_out_dyn_gain !=  gCtrlVars.remind_effect_gain_control_unit.gain)//////阀值
		{
			if(gCtrlVars.remind_out_dyn_gain < gCtrlVars.remind_effect_gain_control_unit.gain)
			{
				gCtrlVars.remind_out_dyn_gain += SHNNIN_STEP;
			}
			else if(gCtrlVars.remind_out_dyn_gain > gCtrlVars.remind_effect_gain_control_unit.gain)
			{
				if(gCtrlVars.remind_out_dyn_gain >= SHNNIN_STEP)
		    	{
					gCtrlVars.remind_out_dyn_gain -= SHNNIN_STEP;
		    	}
			}
			if(gCtrlVars.remind_out_dyn_gain > gCtrlVars.remind_effect_gain_control_unit.gain)
			{
				gCtrlVars.remind_out_dyn_gain  = gCtrlVars.remind_effect_gain_control_unit.gain;
			}
		}
		#endif
#ifdef CFG_FUNC_USB_MIX_MODE		
#ifdef CFG_RES_AUDIO_USB_IN_EN
		if(gCtrlVars.usb_in_dyn_gain !=  gCtrlVars.usb_gain_control_unit.gain)//////阀值
		{
			APP_DBG("Usb Shunning restore\n");
		#if 0//def USE_WT2018_I2S_MASTER_MODE			
			gCtrlVars.usb_in_dyn_gain = gCtrlVars.usb_gain_control_unit.gain;
		#else
			if(gCtrlVars.usb_in_dyn_gain < gCtrlVars.usb_gain_control_unit.gain)
			{
				gCtrlVars.usb_in_dyn_gain += SHNNIN_STEP;
			}
			else if(gCtrlVars.usb_in_dyn_gain > gCtrlVars.usb_gain_control_unit.gain)
			{
				if(gCtrlVars.usb_in_dyn_gain >= SHNNIN_STEP)
				{
					gCtrlVars.usb_in_dyn_gain -= SHNNIN_STEP;
				}
			}
			if(gCtrlVars.usb_in_dyn_gain > gCtrlVars.usb_gain_control_unit.gain)
			{
				gCtrlVars.usb_in_dyn_gain  = gCtrlVars.usb_gain_control_unit.gain;
			}
		#endif
		}
#endif	
#endif
	}
#endif
}

/*
****************************************************************
* 耳机插拔检测处理demo
*
*
****************************************************************
*/
void DetectEarPhone(void)
{
#ifdef CFG_FUNC_DETECT_PHONE_EN
	static uint8_t PhoneTimeInit=0;
	static uint8_t PhoneCnt = 0;
	uint32_t msg;

	if(!PhoneTimeInit)
	{
		PhoneTimeInit = 1;
		PhoneCnt = 0;
		gCtrlVars.EarPhoneOnlin = 0;
		GPIO_RegOneBitClear(PHONE_DETECT_OE, PHONE_DETECT_PIN);
		GPIO_RegOneBitSet(PHONE_DETECT_IE, PHONE_DETECT_PIN);
		///PULL enable
		GPIO_RegOneBitSet(PHONE_DETECT_PU, PHONE_DETECT_PIN);
		GPIO_RegOneBitClear(PHONE_DETECT_PD, PHONE_DETECT_PIN);
		DelayUs(50);
		if(!GPIO_RegOneBitGet(PHONE_DETECT_IN,PHONE_DETECT_PIN))
		//if(GPIO_RegOneBitGet(PHONE_DETECT_IN,PHONE_DETECT_PIN))
		{
			gCtrlVars.EarPhoneOnlin = 1;			
			TimeOutSet(&MuteOffTimer, 1000); 
			APP_DBG("Ear Phone In\n");
		}
	}
    else
	{
		if(!GPIO_RegOneBitGet(PHONE_DETECT_IN,PHONE_DETECT_PIN))
		//if(GPIO_RegOneBitGet(PHONE_DETECT_IN,PHONE_DETECT_PIN))
		{
			if(++PhoneCnt > 50)//消抖处理
			{
				PhoneCnt = 0;
				if(gCtrlVars.EarPhoneOnlin == 0)
				{
					gCtrlVars.EarPhoneOnlin = 1;	
					TimeOutSet(&MuteOffTimer, 1000); 
					//msg = FUNC_ID_EARPHONE_IN;
					//SendMessage(&msg,NULL);
					APP_DBG("Ear Phone In\n");
				}		
			}
		}
		else
		{
			PhoneCnt = 0;
			if(gCtrlVars.EarPhoneOnlin)
			{
				//msg =	FUNC_ID_EARPHONE_OUT;
				gCtrlVars.EarPhoneOnlin = 0;
				MUTE_ON();
				APP_DBG("Ear Phone Out\n");
				TimeOutSet(&MuteOffTimer, 1000); 
			}
		}
		if(IsTimeOut(&MuteOffTimer)) 
		{
			if(gCtrlVars.EarPhoneOnlin
			#ifdef CFG_FUNC_DETECT_MIC_EN	
				|| gCtrlVars.MicOnlin
			#endif
			)
			{
				MUTE_OFF();
			}
		}
	}
#endif	
}

/*
****************************************************************
* 3线，4线耳机类型检测处理demo
*
*
****************************************************************
*/
void DetectMic3Or4Line(void)
{
#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
    //---------mic1 var-------------------//
	static uint8_t MicSegmentInit=0;
	static uint8_t MicSegCnt = 0;
	uint8_t val;
	uint32_t msg;

	if(!MicSegmentInit)
	{
		MicSegmentInit = 1;
		MicSegCnt = 0;
		gCtrlVars.MicSegment = 0;////default 0 line
		GPIO_RegOneBitClear(MIC_SEGMENT_PU, MIC_SEGMENT_PIN);
		GPIO_RegOneBitSet(MIC_SEGMENT_PD, MIC_SEGMENT_PIN);			   
		GPIO_RegOneBitClear(MIC_SEGMENT_OE, MIC_SEGMENT_PIN);
		GPIO_RegOneBitSet(MIC_SEGMENT_IE, MIC_SEGMENT_PIN);
		DelayUs(50);
		if(GPIO_RegOneBitGet(MIC_SEGMENT_IN,MIC_SEGMENT_PIN))
		{
			gCtrlVars.MicSegment = 4;
			APP_DBG("Mic segmen is 4 line\n");
		}	   
	}
	else
	{
		#ifdef CFG_FUNC_DETECT_MIC_EN	
		if(!gCtrlVars.MicOnlin)
	    {
			MIC_MUTE_ON();
		}
		else
		#endif
		{
			if(gCtrlVars.MicSegment == 4 )
			{
				MIC_MUTE_OFF();
			}
			else
			{
				MIC_MUTE_ON();
			}
		}
		if(GPIO_RegOneBitGet(MIC_SEGMENT_IN,MIC_SEGMENT_PIN))
		{
			MicSegCnt = 0;
			if(gCtrlVars.MicSegment != 4  )
			{
				gCtrlVars.MicSegment = 4;
				//msg =	FUNC_ID_MIC_3Or4_LINE;
				APP_DBG("Mic segmen is 4 line\n");
			}
		}
		else
		{
			if(++MicSegCnt > 50)
			{
				MicSegCnt = 0;
				if(gCtrlVars.MicSegment != 3 )//////H=4line l=3line
				{
					gCtrlVars.MicSegment = 3;	 
					//msg = FUNC_ID_MIC_3Or4_LINE;
					//SendMessage(&msg,NULL);
					APP_DBG("Mic segmen is 3 line\n");
				}	 
			}
		}
	}
#endif		
}

 /*
 ****************************************************************
 *电位器调节中的参数值的平滑处理
 *提供的电位器demo的调节步长是:0~32
 *
 ****************************************************************
 */
void MicVolSmoothProcess(void)
{
#ifdef CFG_ADC_LEVEL_KEY_EN
	//---------mic vol 电位器渐变调节-------------------//
	if(mainAppCt.MicVolume!= mainAppCt.MicVolumeBak)
	{
		if(mainAppCt.MicVolume > mainAppCt.MicVolumeBak)
		{
			mainAppCt.MicVolume--;
		}
		else if(mainAppCt.MicVolume < mainAppCt.MicVolumeBak)
		{
			mainAppCt.MicVolume++;
		}
		AudioMicVolSet(mainAppCt.MicVolume);
		APP_DBG("MicVolume = %d\n",mainAppCt.MicVolume);
	}
#ifdef CFG_FUNC_MIC_TREB_BASS_EN
    //-------------bass 电位器渐变调节-----------------------------------------//
	if(mainAppCt.MicBassStep !=  mainAppCt.MicBassStepBak)
	{
		if(mainAppCt.MicBassStep >  mainAppCt.MicBassStepBak)
		{
			mainAppCt.MicBassStep -= BASS_TREB_GAIN_STEP;
    					 
		}
		else if(mainAppCt.MicBassStep <  mainAppCt.MicBassStepBak)
		{
			mainAppCt.MicBassStep += BASS_TREB_GAIN_STEP;
    					 
		}		
		MicBassTrebAjust(mainAppCt.MicBassStep, mainAppCt.MicTrebStep);
		APP_DBG("bass = %d\n",mainAppCt.MicBassStep);
	}
    //-------------treb 电位器渐变调节-----------------------------------------//
	if(mainAppCt.MicTrebStep !=  mainAppCt.MicTrebStepBak)
	{
		if(mainAppCt.MicTrebStep >  mainAppCt.MicTrebStepBak)
		{
			mainAppCt.MicTrebStep -= BASS_TREB_GAIN_STEP;
    					 
		}
		else if(mainAppCt.MicTrebStep <  mainAppCt.MicTrebStepBak)
		{
			mainAppCt.MicTrebStep += BASS_TREB_GAIN_STEP;
    					 
		}		
		MicBassTrebAjust(mainAppCt.MicBassStep, mainAppCt.MicTrebStep);
		APP_DBG("treb = %d\n",mainAppCt.MicTrebStep);
	}	
#endif
	//-------------reverb-gain 电位器渐变调节---------------------------------------//
	if(mainAppCt.ReverbStep !=  mainAppCt.ReverbStepBak)
	{
		if(mainAppCt.ReverbStep >  mainAppCt.ReverbStepBak)
		{
			mainAppCt.ReverbStep -= 1;
						 
		}
		else if(mainAppCt.ReverbStep <  mainAppCt.ReverbStepBak)
		{
			mainAppCt.ReverbStep += 1;
						 
		}	
		ReverbStepSet(mainAppCt.ReverbStep);
		APP_DBG("ReverbStep  = %d\n",mainAppCt.ReverbStep);
	}	
	#if 0
    //-------------mic delay 电位器渐变调节-----------------------------------------//
	if(gCtrlVars.mic_echo_delay !=  gCtrlVars.mic_echo_delay_bak)
	{
		if(gCtrlVars.mic_echo_delay >  gCtrlVars.mic_echo_delay_bak)
		{
			gCtrlVars.mic_echo_delay -= BASS_TREB_GAIN_STEP;
    					 
		}
		else if(gCtrlVars.mic_echo_delay <  gCtrlVars.mic_echo_delay_bak)
		{
			gCtrlVars.mic_echo_delay += BASS_TREB_GAIN_STEP;
    					 
		}
		
		gCtrlVars.echo_unit.delay_samples = EchoDlyTab_16[gCtrlVars.mic_echo_delay];
	}
    //------------aux gain 电位器渐变调节-----------------------------------------//
	if(gCtrlVars.aux_gain !=  gCtrlVars.aux_gain_bak)
	{
		if(gCtrlVars.aux_gain >  gCtrlVars.aux_gain_bak)
		{
			gCtrlVars.aux_gain -= BASS_TREB_GAIN_STEP;
    					 
		}
		else if(gCtrlVars.aux_gain <  gCtrlVars.aux_gain_bak)
		{
			gCtrlVars.aux_gain += BASS_TREB_GAIN_STEP;
    					 
		}
		
		gCtrlVars.aux_gain_control_unit.gain = DigVolTab_16[gCtrlVars.aux_gain];
	}
    //-------------rec gain 电位器渐变调节-----------------------------------------//
	if(gCtrlVars.rec_gain !=  gCtrlVars.rec_gain_bak)
	{
		if(gCtrlVars.rec_gain >  gCtrlVars.rec_gain_bak)
		{
			gCtrlVars.rec_gain -= BASS_TREB_GAIN_STEP;
    					 
		}
		else if(gCtrlVars.rec_gain <  gCtrlVars.rec_gain_bak)
		{
			gCtrlVars.rec_gain += BASS_TREB_GAIN_STEP;
    					 
		}
		
		gCtrlVars.rec_out_gain_control_unit.gain = DigVolTab_16[gCtrlVars.rec_gain];
	}	
    #endif
#endif	  
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
#ifdef CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
uint8_t Audio_mode_buff[4096];////可动态分配
uint8_t eff_save_en = 0;
uint16_t eff_slen;
uint8_t flash_effect_total;
uint8_t  flash_effect_name[16];
//---------------------------------------//
extern uint8_t  effect_sum;
extern uint16_t effect_list[EFFECT_LIST_LEN];
extern uint32_t effect_addr[EFFECT_LIST_LEN];
extern uint32_t eff_addr_compare[EFFECT_LIST_LEN];
extern uint16_t effect_list_addr[EFFECT_LIST_LEN];
extern uint8_t  tx_buf[256];
extern uint8_t  communic_buf[512];
void Communication_Effect_Config(uint8_t Control, uint8_t *buf, uint32_t len);
void PrintAudioEffectParamaesList(uint16_t  len,uint8_t *buff);
void WriteEffectParamas(uint32_t mode,uint8_t *buff);
uint16_t ReadEffectParamas(uint32_t mode, uint8_t *buff,uint16_t rLen);
uint16_t GetEffectParamaesLen(uint16_t  len,uint8_t *buff);
uint8_t AECGetMode(uint8_t mode);
//---------------------------------------//
const uint8_t HardWareControlTab[]={
1,3,4,6,7,8,9,10,11,12,13,
};
/*************************************************************************
 *
 *
 *
 *
 **************************************************************************/
void EffectSendToRam(uint8_t *buf, uint32_t len)
{
	 if(eff_save_en<2) return;

	 memcpy(&Audio_mode_buff[eff_slen],buf,len);
	 eff_slen += len;
}
/*************************************************************************
 *
 *
 *
 *
 **************************************************************************/
void AudioEffSaveToFlash(void)
{
	 uint16_t i;
	 if(eff_save_en != 2) return;
	 DBG("[FLASH EFF]: Start Saving Effect\n");

	 memset(&Audio_mode_buff[0],0,4096);

	 eff_slen = 0;
	 //--HW-------------------------------------------------------------------------------//
     for(i = 0; i < sizeof(HardWareControlTab); i++)
	 {
       memset(&communic_buf[0],0,256);//a5 5a 00 00 16
       communic_buf[0]= 0xA5,communic_buf[1]= 0x5A;
       communic_buf[2]= HardWareControlTab[i];///CTL
       communic_buf[3]= 0x00;
       communic_buf[4]= 0x16;
       Communication_Effect_Config(communic_buf[2], &communic_buf[4], communic_buf[3]);
	 }

     //----EFF-----------------------------------------------------------------------------//
  	for(i = 0; i < effect_sum; i++)
  	{
        memset(&communic_buf[0],0,256);//a5 5a 00 00 16
        communic_buf[0]= 0xA5,communic_buf[1]= 0x5A;
        communic_buf[2]= effect_list_addr[i];///CTL
        communic_buf[3]= 0x00;
        communic_buf[4]= 0x16;
        Communication_Effect_Config(communic_buf[2], &communic_buf[4], communic_buf[3]);
  	}
  	//-----END--------------------------------//
  	//PrintAudioEffectParamaesList(eff_slen,Audio_mode_buff);
	if( (mainAppCt.EffectMode != EFFECT_MODE_HFP_AEC) && (mainAppCt.EffectMode != EFFECT_MODE_USB_AEC))
	{
		if(mainAppCt.EffectMode > (flash_effect_total+1))
		{
			DBG("[FLASH EFF]: Saving Effect Fail, mode=%d\n",(uint8_t)(mainAppCt.EffectMode &0xff));
			eff_save_en = 0;
			return;
		}
	}

  	WriteEffectParamas(mainAppCt.EffectMode,Audio_mode_buff);
  	eff_save_en = 0;
}
/*
****************************************************************
* 写调音参数到flash中，在线调音下载用
*
*
****************************************************************
*/
void WriteEffectParamas(uint32_t mode,uint8_t *buff)
{
	//SPI_FLASH_INFO *gSpiFlashInfo;

	//uint32_t sector_total;
    mode = AECGetMode(mode);

	DBG("[FLASH EFF]: Saving Effect Mode to flash, mode=%d\n",(uint8_t)(mode &0xff));

   // gSpiFlashInfo = SpiFlashInfoGet();

	//sector_total = gSpiFlashInfo->Capacity/4096;

    //PrintAudioEffectParamaesList(4096,buff);

	//if(gCtrlVars.crypto_en) AudioEffectEncryption(buff,4096,gCtrlVars.password);

    if(mode>9) mode = 9;///0~~~~9

    SpiFlashErase(SECTOR_ERASE, (AUDIO_EFFECT_ADDR / 4096) + mode, 1);

    mode *= 4096;

    SpiFlashWrite(AUDIO_EFFECT_ADDR + mode, buff, 4096, 1);
}
/*
****************************************************************
* 获取flash中记忆的调音参数
*
*
****************************************************************
*/
uint16_t ReadEffectParamas(uint32_t mode, uint8_t *buff,uint16_t rLen)
{
	//SPI_FLASH_INFO *gSpiFlashInfo;
	uint16_t effect_len;

    mode = AECGetMode(mode);

    DBG("[FLASH EFF]: Read Effect Mode from flash mode=%d\n",(uint8_t)(mode &0xff));

    if(mode>9) mode = 9;///0~~~~9

	mode *= 4096;

	SpiFlashRead(AUDIO_EFFECT_ADDR +  mode, buff, rLen, 100);

	effect_len = GetEffectParamaesLen(rLen,buff);

	//if(gCtrlVars.crypto_en)
	//AudioEffectDecryption(buff,4096,gCtrlVars.password);

	//PrintAudioEffectParamaesList(rLen,buff);
   //---------------------------------------------//
	return effect_len;
}
/*
****************************************************************
* 获取flash中记忆的调音参数长度
*
*
****************************************************************
*/
uint16_t GetEffectParamaesLen(uint16_t  len,uint8_t *buff)
{
	uint16_t cmd_len,packet_len,k,slen;
	uint8_t end_code,ctl_code;//,start;
	uint16_t position;

	position = 0;
	slen = 0;

	while(position < len)
	{
	    if ((buff[position] ==0xa5) && (buff[position+1] ==0x5a))
		{
			packet_len = buff[position + 3];
			end_code   = buff[position + 4 + packet_len];//0x16

			if(end_code == 0x16)////end code ok
			{
				cmd_len = packet_len + 5;
				//sync
				//start=buff[position++];
				position++;
				//DBG("0x%02X, ",start);
				cmd_len--;
				//start=buff[position++];
				position++;
				//DBG("0x%02X,\n",start);
				cmd_len--;

                //control
				//start=buff[position++];
				position++;
				//DBG("0x%02X,\n",start);
				cmd_len--;

                //len
				//start=buff[position++];
				position++;
				//DBG("0x%02X,\n",start);
				cmd_len--;

                //all parameter
				//start=buff[position++];
				position++;
				//DBG("0x%02X,\n",start);
				cmd_len--;

				for(k = 1; k < cmd_len; k++)//get signal COMMAND
				{
					//ctl_code=buff[position++];
					position++;
					slen += 1;
					//DBG("0x%02X, ",ctl_code);
					//if(k % 2 == 0)
					{
						//DBG("\n");
					}
				}

				///end
				//start=buff[position++];
				position++;
				//DBG("0x%02X,\n ",start);
				cmd_len--;
				//DBG("\n");
				slen += 6;

                #if CFG_WATCH_DOG_EN
                WDG_Feed();
                #endif
			}
			else  // end code error
			{
				position++;
				slen += 1;
			}
		}
		else // serch start code....
		{
			position++;
		}
	}
	return slen;
}
/*
****************************************************************
*  对上位机下载的，从SPI读取的调音参数打印显示
*  显示固定存在Audio_mode_buff[]
*
****************************************************************
*/
void PrintAudioEffectParamaesList(uint16_t  len,uint8_t *buff)
{
#if 1
	uint16_t cmd_len,packet_len,k;
	uint8_t end_code,ctl_code,start;
	uint16_t position;

	DBG("start printf effect paramase\n ");
	DBG("\n");

	position = 0;

	while(position < len)
	{
	    if ((buff[position] ==0xa5) && (buff[position+1] ==0x5a))
		{
			packet_len = buff[position + 3];
			end_code   = buff[position + 4 + packet_len];//0x16

			if(end_code == 0x16)////end code ok
			{
				cmd_len = packet_len + 5;

				//sync
				start=buff[position++];
				DBG("0x%02X, ",start);
				cmd_len--;
				start=buff[position++];
				DBG("0x%02X,\n",start);
				cmd_len--;

                //control
				start=buff[position++];
				DBG("0x%02X,\n",start);
				cmd_len--;

                //len
				start=buff[position++];
				DBG("0x%02X,\n",start);
				cmd_len--;

                //all parameter
				start=buff[position++];
				DBG("0x%02X,\n",start);
				cmd_len--;

				for(k = 1; k < cmd_len; k++)//get signal COMMAND
				{
					ctl_code=buff[position++];
					DBG("0x%02X, ",ctl_code);
					if(k % 2 == 0)
					{
						DBG("\n");
					}
				}

				///end
				start=buff[position++];
				DBG("0x%02X,\n ",start);
				cmd_len--;
				DBG("\n");
                #if CFG_WATCH_DOG_EN
                WDG_Feed();
                #endif
			}
			else  // end code error
			{
				position++;
			}
		}
		else // serch start code....
		{
			position++;
		}
	}
#endif
}


#endif //end of CFG_FUNC_DOWNDLOAD_EFF_TO_FLASH
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
/*
****************************************************************
*  对上位机下载的，从SPI读取的调音参数打印显示
*  显示固定存在Audio_mode_buff[]
*
****************************************************************
*/
void PrintAudioEffectParamaesRowList(uint16_t  len,uint8_t *buff,uint8_t IsTx)
{
#if 1
	uint16_t cmd_len,packet_len,k;
	uint8_t end_code,ctl_code,start;
	uint16_t position;

	position = 0;
    if(IsTx)
    {
    	DBG("[TX->] ");
    }
    else
    {
    	DBG("[RX->] ");
    }
	while(position < len)
	{
	    if ((buff[position] ==0xa5) && (buff[position+1] ==0x5a))
		{
			packet_len = buff[position + 3];
			end_code   = buff[position + 4 + packet_len];//0x16

			if(end_code == 0x16)////end code ok
			{
				cmd_len = packet_len + 5;

				//sync
				start=buff[position++];
				DBG("0x%02X, ",start);
				cmd_len--;
				start=buff[position++];
				DBG("0x%02X,",start);
				cmd_len--;

                //control
				start=buff[position++];
				DBG("0x%02X,",start);
				cmd_len--;

                //len
				start=buff[position++];
				DBG("0x%02X,",start);
				cmd_len--;

                //all parameter
				start=buff[position++];
				DBG("0x%02X,",start);
				cmd_len--;

				for(k = 1; k < cmd_len; k++)//get signal COMMAND
				{
					ctl_code=buff[position++];
					DBG("0x%02X, ",ctl_code);
					if(k % 2 == 0)
					{
						//DBG("\n");
					}
				}

				///end
				start=buff[position++];
				if(packet_len)
				{
					DBG("0x%02X, ",start);
				}
				cmd_len--;
				DBG("\n");
			    if(IsTx)
			    {
			    	DBG("\n");
			    }
		        #if CFG_WATCH_DOG_EN
                WDG_Feed();
                #endif
                return;
			}
			else  // end code error
			{
				position++;
			}
		}
		else // serch start code....
		{
			position++;
		}
	}
#endif
}
