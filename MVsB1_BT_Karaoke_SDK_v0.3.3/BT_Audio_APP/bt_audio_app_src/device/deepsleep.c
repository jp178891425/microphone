#include "app_config.h"
#include "powercontroller.h"
#include "deepsleep.h"
#include "gpio.h"
#include "timeout.h"
#include "audio_adc.h"
#include "dac.h"
#include "clk.h"
#include "chip_info.h"
#include "otg_device_hcd.h"
#include "rtc.h"
#include "irqn.h"
#include "debug.h"
#include "adc_key.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc_key.h"
#include "uarts.h"
//#include "OrioleReg.h"//for test
#include "hdmi_in_api.h"
#include "sys.h"
#include "sadc_interface.h"
#include "watchdog.h"
#include "backup.h"
#include "ir_key.h"
#include "app_message.h"
#include "reset.h"
#include "bt_stack_service.h"
#include "key.h"
#include "main_task.h"
#include "efuse.h"
#include "bt_common_api.h"
#include "bt_manager.h"
#include "ble_app_func.h"
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
TIMER   waitCECTime;
uint8_t	waitCECTimeFlag = 0;
#endif
#define DPLL_QUICK_START_SLOPE	(*(volatile unsigned long *) 0x40026028)

//DPLL 快速启动参数获取。
void Clock_GetDPll(uint32_t* NDAC, uint32_t* OS, uint32_t* K1, uint32_t* FC);

void UartClkChange(CLK_MODE clk_change);


void SysTickInitSet(void)
{
#ifndef CFG_FUNC_IDLE_TASK_LOW_POWER
	Timer_Config(0/*TIMER1*/,500,0);
	Timer_Start(0/*TIMER1*/);
//	SREG_TIMER1_CTRL.TIMER1_CEN        = 0;
//	SREG_TIMER1_CTRL.TIMER1_OPM		   = 0;
//	SREG_TIMER1_CTRL.TIMER1_HALT_EN	   = 1;
//	SREG_TIMER1_CTRL.TIMER1_INT_EN     = 1;
//
//	REG_TIMER1_ARR = 20*1000-1;
//	REG_TIMER1_PSC = 0;
//
//	SREG_TIMER1_CTRL.TIMER1_CEN        = TRUE;
//	SREG_TIMER1_CTRL.TIMER1_UG		   = 1;

	GIE_ENABLE();
	NVIC_EnableIRQ(0/*TMR1_IRQn*/);
#else
	SysTickInit();
#endif
}

#if	defined(CFG_FUNC_DEEPSLEEP_EN) || defined(CFG_FUNC_MAIN_DEEPSLEEP_EN)
HDMIInfo  			 *gHdmiCt;

void GIE_ENABLE(void);
void SystemOscConfig(void);
void SleepMainAppTask(void);
void SleepAgainConfig(void);

void WakeupMain(void);

void SleepMain(void);
void LogUartConfig(bool InitBandRate);

#define CHECK_SCAN_TIME				5000		//醒来确认有效唤醒源 扫描限时ms。

static uint32_t sources;
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
uint32_t alarm = 0;
#endif

__attribute__((section(".driver.isr")))void WakeupInterrupt(void)
{
	sources |= Power_WakeupSourceGet();

	if(Power_WakeupSourceGet() == SYSWAKEUP_SOURCE13_BT)
	{
		Power_WakeupDisable(SYSWAKEUP_SOURCE13_BT);
		NVIC_DisableIRQ(Wakeup_IRQn);
	}

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	if(sources & CFG_PARA_WAKEUP_SOURCE_CEC)
	{
		if(waitCECTimeFlag == 0)
		{
			SysTickInitSet();
			TimeOutSet(&waitCECTime, CHECK_SCAN_TIME);
			HDMI_HPD_CHECK_IO_INIT();
			waitCECTimeFlag =  1;
		}
	}
#endif
	Power_WakeupSourceClear();
}


void SystermGPIOWakeupConfig(PWR_SYSWAKEUP_SOURCE_SEL source,PWR_WAKEUP_GPIO_SEL gpio,PWR_SYSWAKEUP_SOURCE_EDGE_SEL edge)
{
	if(gpio < 32)
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   (1 << gpio));
		GPIO_RegOneBitClear(GPIO_A_OE, (1 << gpio));
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			GPIO_RegOneBitSet(GPIO_A_PU, (1 << gpio));//因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
			GPIO_RegOneBitClear(GPIO_A_PD, (1 << gpio));
		}
		else if(edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			GPIO_RegOneBitClear(GPIO_A_PU, (1 << gpio));//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			GPIO_RegOneBitSet(GPIO_A_PD, (1 << gpio));
		}
	}
	else if(gpio < 41)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   (1 << (gpio - 32)));
		GPIO_RegOneBitClear(GPIO_B_OE, (1 << (gpio - 32)));
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			GPIO_RegOneBitSet(GPIO_B_PU, (1 << (gpio - 32)));//因为芯片的GPIO有内部上下拉电阻,选择下降沿触发时要将指定的GPIO唤醒管脚配置为上拉
			GPIO_RegOneBitClear(GPIO_B_PD, (1 << (gpio - 32)));
		}
		else if( edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			GPIO_RegOneBitClear(GPIO_B_PU, (1 << (gpio - 32)));//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			GPIO_RegOneBitSet(GPIO_B_PD, (1 << (gpio - 32)));
		}
	}
	else if(gpio == 41)
	{

		BACKUP_WriteEnable();

		BACKUP_C0RegSet(BKUP_GPIO_C0_REG_IE_OFF, TRUE);
		BACKUP_C0RegSet(BKUP_GPIO_C0_REG_OE_OFF, FALSE);
		if( edge == SYSWAKEUP_SOURCE_NEGE_TRIG )
		{
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PU_OFF, TRUE);
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PD_OFF, FALSE);
		}
		else if( edge == SYSWAKEUP_SOURCE_POSE_TRIG )
		{
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PU_OFF, FALSE);//因为芯片的GPIO有内部上下拉电阻，所以选择上升沿触发时要将指定的GPIO唤醒管脚配置为下拉
			BACKUP_C0RegSet(BKUP_GPIO_C0_REG_PD_OFF, TRUE);
		}
		BACKUP_WriteDisable();
	}

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(source, gpio, edge);
	Power_WakeupEnable(source);

	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	GIE_ENABLE();
}

void SystermIRWakeupConfig(IR_MODE_SEL ModeSel, IR_IO_SEL GpioSel, IR_CMD_LEN_SEL CMDLenSel)
{

	Clock_BTDMClkSelect(RC_CLK32_MODE);//sniff开启时使用影响蓝牙功能呢
	Reset_FunctionReset(IR_FUNC_SEPA);
#ifdef CFG_RES_IR_KEY_USE
	IRKeyInit();
#endif
	IR_WakeupEnable();


	if(GpioSel == IR_GPIOB6)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX6);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX6);
	}
	else if(GpioSel == IR_GPIOB7)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX7);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX7);
	}
	else
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX29);
		GPIO_RegOneBitSet(GPIO_A_IN,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX29);
	}
	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	GIE_ENABLE();

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE9_IR, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE9_IR);
}

void SystermIRWakeupConfig_sniff(IR_MODE_SEL ModeSel, IR_IO_SEL GpioSel, IR_CMD_LEN_SEL CMDLenSel)
{

	Reset_FunctionReset(IR_FUNC_SEPA);
#ifdef CFG_RES_IR_KEY_USE
	IRKeyInit();
#endif
	IR_WakeupEnable();


	if(GpioSel == IR_GPIOB6)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX6);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX6);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX6);
	}
	else if(GpioSel == IR_GPIOB7)
	{
		GPIO_RegOneBitSet(GPIO_B_IE,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OE, GPIO_INDEX7);
		GPIO_RegOneBitSet(GPIO_B_IN,   GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_OUT, GPIO_INDEX7);
		GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX7);
	}
	else
	{
		GPIO_RegOneBitSet(GPIO_A_IE,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OE, GPIO_INDEX29);
		GPIO_RegOneBitSet(GPIO_A_IN,   GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX29);
		GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX29);
	}
	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	GIE_ENABLE();

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE9_IR, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE9_IR);
}



#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
//RTC唤醒 并不会进入RTC中断
void SystermRTCWakeupConfig(uint32_t SleepSecond)
{
	//RTC_REG_TIME_UNIT start;
	#ifdef CFG_PARA_RTC_SRC_OSC32K
	RTC_ClockSrcSel(OSC_32K);//此函数的参数选择必须和上面系统初始化选择的晶振（“Clock_Config()”）保持一致
	#else
	RTC_ClockSrcSel(OSC_24M);
	#endif
	RTC_IntDisable();
	RTC_IntFlagClear();
	RTC_WakeupDisable();

	alarm = RTC_SecGet() + SleepSecond;
	RTC_SecAlarmSet(alarm);
	RTC_WakeupEnable();
	RTC_IntEnable();

	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	NVIC_EnableIRQ(Rtc_IRQn);
	NVIC_SetPriority(Rtc_IRQn, 1);
	GIE_ENABLE();

	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE7_RTC, 0, 0);
	Power_WakeupEnable(SYSWAKEUP_SOURCE7_RTC);
}
#endif //CFG_PARA_WAKEUP_RTC

void DeepSleepIOConfig()
{
	//cansle all IO AF
	{
		int IO_cnt = 0;
		for(IO_cnt = 0;IO_cnt < 32;IO_cnt++)
		{
			#if	defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC) && defined(CFG_APP_HDMIIN_MODE_EN)
			if(IO_cnt == CFG_PARA_WAKEUP_GPIO_CEC)
				continue;
			#endif

			GPIO_PortAModeSet(GPIOA0 << IO_cnt,0);
		}
		for(IO_cnt = 0;IO_cnt < 8;IO_cnt++)
		{
			#ifdef CFG_FUNC_SW_DEBUG_EN
			if(IO_cnt == 0)
				continue;//B1是sw口
			if(IO_cnt == 1)
				continue;//B0是sw口
        	#endif
			GPIO_PortBModeSet(GPIOB0 << IO_cnt,0);
		}
	}

	SleepAgainConfig();//配置相同
}

void SystermDeepSleepConfig(void)
{
	//GPIOA0~A31、B0~B7的复用功能，可自行选择关闭
#if (CFG_PARA_WAKEUP_GPIO_CEC != WAKEUP_GPIOA0)
	GPIO_PortAModeSet(GPIOA0, 0x0000);
#endif
	GPIO_PortAModeSet(GPIOA1, 0x0000);
	GPIO_PortAModeSet(GPIOA2, 0x0000);
	GPIO_PortAModeSet(GPIOA3, 0x0000);
	GPIO_PortAModeSet(GPIOA4, 0x0000);
	GPIO_PortAModeSet(GPIOA5, 0x0000);
	GPIO_PortAModeSet(GPIOA6, 0x0000);//A6可配UART0 Tx
	GPIO_PortAModeSet(GPIOA7, 0x0000);
#if (CFG_PARA_WAKEUP_GPIO_CEC != WAKEUP_GPIOA8)
	GPIO_PortAModeSet(GPIOA8, 0x0000);
#endif
	GPIO_PortAModeSet(GPIOA9, 0x0000);
	GPIO_PortAModeSet(GPIOA10, 0x0000);//A10可配UART1 Tx
	GPIO_PortAModeSet(GPIOA11, 0x0000);
	GPIO_PortAModeSet(GPIOA12, 0x0000);
	GPIO_PortAModeSet(GPIOA13, 0x0000);
	GPIO_PortAModeSet(GPIOA14, 0x0000);
	GPIO_PortAModeSet(GPIOA15, 0x0000);
	GPIO_PortAModeSet(GPIOA16, 0x0000);
 	GPIO_PortAModeSet(GPIOA17, 0x0000);
 	GPIO_PortAModeSet(GPIOA18, 0x0000);
 	GPIO_PortAModeSet(GPIOA19, 0x0000);
 	GPIO_PortAModeSet(GPIOA20, 0x0000);
 	GPIO_PortAModeSet(GPIOA21, 0x0000);
#if (CFG_PARA_WAKEUP_GPIO_CEC != WAKEUP_GPIOA22)
 	GPIO_PortAModeSet(GPIOA22, 0x0000);
#endif
	GPIO_PortAModeSet(GPIOA23, 0x0000);
	GPIO_PortAModeSet(GPIOA24, 0x0000);
	GPIO_PortAModeSet(GPIOA25, 0x0000);
	GPIO_PortAModeSet(GPIOA26, 0x0000);
#if (CFG_PARA_WAKEUP_GPIO_CEC != WAKEUP_GPIOA27)
	GPIO_PortAModeSet(GPIOA27, 0x0000);
#endif
	GPIO_PortAModeSet(GPIOA28, 0x0000);
	GPIO_PortAModeSet(GPIOA29, 0x0000);
	GPIO_PortAModeSet(GPIOA30, 0x0000);
	GPIO_PortAModeSet(GPIOA31, 0x0000);
#ifndef CFG_FUNC_SW_DEBUG_EN
	GPIO_PortBModeSet(GPIOB0, 0x000); 
	GPIO_PortBModeSet(GPIOB1, 0x000);
#endif
	GPIO_PortBModeSet(GPIOB2, 0x000);
	GPIO_PortBModeSet(GPIOB3, 0x000);
	GPIO_PortBModeSet(GPIOB4, 0x000);
	GPIO_PortBModeSet(GPIOB5, 0x000);
	GPIO_PortBModeSet(GPIOB6, 0x000);
	GPIO_PortBModeSet(GPIOB7, 0x000);

	SleepAgainConfig();//配置相同

	SleepMain();

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	if(gHdmiCt == NULL)
	{
		HDMI_CEC_DDC_Init();
		gHdmiCt->hdmiReportStatus = 0;
	}
//	osTaskDelay(10);
	while(!HDMI_CEC_IsReadytoDeepSleep(10));//拉低10ms，保障后续18ms没有通信和中断响应。
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	alarm = 0;
#else

	//Clock_LOSCDisable(); //若有RTC应用则不关闭32K晶振 BKD mark sleep
//	BACKUP_32KDisable(OSC32K_SOURCE);// bkd add

#endif
}

//启用多个唤醒源时，source通道配置灵活，但不可重复。
void WakeupSourceSet(void)
{
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	SystermRTCWakeupConfig(CFG_PARA_WAKEUP_TIME_RTC);
#endif	
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)&& defined(CFG_PARA_WAKEUP_GPIO_ADCKEY)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_ADCKEY, CFG_PARA_WAKEUP_GPIO_ADCKEY, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
	SystermGPIOWakeupConfig(SYSWAKEUP_SOURCE6_POWERKEY, 42, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY1) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY1)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY1, CFG_PARA_WAKEUP_GPIO_IOKEY1, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY2) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY2)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY2, CFG_PARA_WAKEUP_GPIO_IOKEY2, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IR) && defined(CFG_RES_IR_KEY_USE)
	SystermIRWakeupConfig(CFG_PARA_IR_SEL, CFG_RES_IR_PIN, CFG_PARA_IR_BIT);
#endif	
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_CEC, CFG_PARA_WAKEUP_GPIO_CEC, SYSWAKEUP_SOURCE_BOTH_EDGES_TRIG);
#endif	
}
void DeepSleeping(void)
{

	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;

	WDG_Disable();

#ifdef CFG_APP_BT_MODE_EN
	BT_ModuleClose();
#endif
	
	GpioAPU_Back = GPIO_RegGet(GPIO_A_PU);
	GpioAPD_Back = GPIO_RegGet(GPIO_A_PD);
	GpioBPU_Back = GPIO_RegGet(GPIO_B_PU);
	GpioBPD_Back = GPIO_RegGet(GPIO_B_PD);
	SystermDeepSleepConfig();
	WakeupSourceSet();
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	waitCECTimeFlag = 0;
#endif
	Power_GotoDeepSleep();
	while(!SystermWackupSourceCheck())
	{
		SleepAgainConfig();
		Power_WakeupDisable(0xff);
		WakeupSourceSet();
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
		waitCECTimeFlag = 0;
#endif
		Power_GotoDeepSleep();
	}
	Power_WakeupDisable(0xff);
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	HDMI_CEC_DDC_DeInit();
#endif
	//GPIO恢复上下拉
	GPIO_RegSet(GPIO_A_PU, GpioAPU_Back);
	GPIO_RegSet(GPIO_A_PD, GpioAPD_Back);
	GPIO_RegSet(GPIO_B_PU, GpioBPU_Back);
	GPIO_RegSet(GPIO_B_PD, GpioBPD_Back);
	GPIO_PortAModeSet(GPIOA30, 0x0005);//调试口 SW恢复 方便下载
	GPIO_PortAModeSet(GPIOA31, 0x0004);

	WDG_Feed();
	WakeupMain();
	SysTickInitSet();
	WDG_Feed();

#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
    extern uint32_t  Silence_Power_Off_Time;
	Silence_Power_Off_Time = SILENCE_POWER_OFF_DELAY_TIME;
#endif	

#ifdef CFG_FUNC_LED_REFRESH
	//默认优先级为0，旨在提高刷新速率，特别是断点记忆等写flash操作有影响刷屏，必须严格遵守所有timer6中断调用都是TCM代码，含调用的driver库代码
	//已确认GPIO_RegOneBitSet、GPIO_RegOneBitClear在TCM区，其他api请先确认。
	NVIC_SetPriority(Timer6_IRQn, 0);
 	Timer_Config(TIMER6,1000,0);
 	Timer_Start(TIMER6);
 	NVIC_EnableIRQ(Timer6_IRQn);
#endif

#ifdef CFG_FUNC_POWER_MONITOR_EN
	extern void PowerMonitorInit(void);
	PowerMonitorInit();
#endif

#if defined(CFG_FUNC_DISPLAY_EN)
    DispInit(0);
#endif
}


bool SystermWackupSourceCheck(void)
{
#ifdef CFG_RES_ADC_KEY_SCAN
	AdcKeyMsg AdcKeyVal;
#endif

#ifdef CFG_PARA_WAKEUP_SOURCE_IR
	IRKeyMsg IRKeyMsg;

#endif
	TIMER WaitScan;
	GPIO_RegSet(GPIO_A_PD,0x0);//去除下拉，否则adckey电平区间变动
	GPIO_RegSet(GPIO_B_PD,0x00);
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY) && defined(CFG_RES_ADC_KEY_USE)
	SarADC_Init();
	AdcKeyInit();
#endif

//********************
	//串口IO设置
	LogUartConfig(FALSE);//此处如果重配clk波特率，较为耗时，不重配。
	SysTickInitSet();
	//APP_DBG("Scan:%x", (int)sources);
#ifdef CFG_PARA_WAKEUP_SOURCE_RTC
	if(sources & CFG_PARA_WAKEUP_SOURCE_RTC)
	{
		sources = 0;//唤醒源清零
		//APP_DBG("Alarm!%d", RTC_SecGet());
		return TRUE;
	}
	else if(alarm)//避免RTC唤醒事件错失
	{
		uint32_t NowTime;
		NowTime = RTC_SecGet();
		if(NowTime + 2 + CHECK_SCAN_TIME / 1000 > alarm)//如果存在多个唤醒源，rtc可以提前唤醒()，以避免丢失
		{
			//APP_DBG("Timer");
			sources = 0;//唤醒源清零
			alarm = 0;
			return TRUE;
		}
	}
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	bool CecRest = FALSE;//cec空闲状态(18ms消抖)。以免影响后续解析和唤醒。
	HDMI_HPD_CHECK_IO_INIT();
#endif

	TimeOutSet(&WaitScan, CHECK_SCAN_TIME);
	while(!IsTimeOut(&WaitScan)
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
			|| !CecRest
#endif
		)
	{
#if defined(CFG_PARA_WAKEUP_SOURCE_IR) && defined(CFG_RES_IR_KEY_USE)
		if(sources & CFG_PARA_WAKEUP_SOURCE_IR)
		{
			IRKeyMsg = IRKeyScan();			
			if(IRKeyMsg.index != IR_KEY_NONE && IRKeyMsg.type != IR_KEY_UNKOWN_TYPE)
			{
				//APP_DBG("IRID:%d,type:%d\n", IRKeyMsg.index, IRKeyMsg.type);
				SetIrKeyValue(IRKeyMsg.type,IRKeyMsg.index);
#ifdef CFG_APP_REST_MODE_EN
				if(GetGlobalKeyValue() == MSG_POWER)
#else
				if((GetGlobalKeyValue() == MSG_DEEPSLEEP)||(GetGlobalKeyValue() == MSG_BT_SNIFF))
#endif
				{
					sources = 0;
					ClrGlobalKeyValue();
					return TRUE;
				}
				ClrGlobalKeyValue();
			}
		}
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
		if(sources & SYSWAKEUP_SOURCE6_POWERKEY)
		{
			sources = 0;
			return TRUE;
		}
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)  && defined(CFG_RES_ADC_KEY_USE)
		if(sources & (CFG_PARA_WAKEUP_SOURCE_ADCKEY))
		{
			AdcKeyVal = AdcKeyScan();
			if(AdcKeyVal.index != ADC_CHANNEL_EMPTY && AdcKeyVal.type != ADC_KEY_UNKOWN_TYPE)
			{
				//APP_DBG("KeyID:%d,type:%d\n", AdcKeyVal.index, AdcKeyVal.type);
				SetAdcKeyValue(AdcKeyVal.type,AdcKeyVal.index);
#ifdef CFG_APP_REST_MODE_EN
				if(GetGlobalKeyValue() == MSG_POWER)
#else
				if((GetGlobalKeyValue() == MSG_DEEPSLEEP)||(GetGlobalKeyValue() == MSG_BT_SNIFF))
#endif
				{
					sources = 0;
					ClrGlobalKeyValue();
					return TRUE;
				}
				ClrGlobalKeyValue();
			}
		}
#endif
#ifdef CFG_RES_IO_KEY_SCAN
#ifdef CFG_PARA_WAKEUP_SOURCE_IOKEY1
		if(sources & CFG_PARA_WAKEUP_SOURCE_IOKEY1)
		{
			return TRUE;
		}
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_IOKEY2
		if(sources & CFG_PARA_WAKEUP_SOURCE_IOKEY2)
		{
			return TRUE;
		}
#endif
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
		HDMI_CEC_Scan(0);
		if(gHdmiCt->hdmi_poweron_flag == 1)
		{
			SoftFlagRegister(SoftFlagWakeUpSouceIsCEC);
			//APP_DBG("CEC PowerOn\n");
			return TRUE;
		}
		if(IsTimeOut(&WaitScan))//超时之后等待下拉电平，同时保持scan。
		{
			CecRest = HDMI_CEC_IsReadytoDeepSleep(6);
		}
#endif

	}
	sources = 0;
//	SysTickDeInit();
	return FALSE;
}


void SleepAgainConfig(void)
{
	GPIO_RegSet(GPIO_A_IE,0x00000000
	#if defined(CFG_RES_ADC_KEY_CH1_ANA_MASK)
			| CFG_RES_ADC_KEY_CH1_ANA_MASK
	#endif
	#if defined(CFG_PARA_WAKEUP_GPIO_CEC) && defined(CFG_PARA_WAKEUP_SOURCE_CEC)
			| BIT(CFG_PARA_WAKEUP_GPIO_CEC)
	#endif
			);

	GPIO_RegSet(GPIO_A_OE,0x00000000);
	GPIO_RegSet(GPIO_A_OUTDS,0x00000000);//bkd GPIO_A_REG_OUTDS
	
	GPIO_RegSet(GPIO_A_PD,0xffffffff
	#if defined(CFG_RES_ADC_KEY_CH1_ANA_MASK)
			& (~ CFG_RES_ADC_KEY_CH1_ANA_MASK)
	#endif
	#if defined(CFG_PARA_WAKEUP_GPIO_CEC) && defined(CFG_PARA_WAKEUP_SOURCE_CEC)//cec端口不做上下拉配置，需要cec状态保障。
			& ~ BIT(CFG_PARA_WAKEUP_GPIO_CEC)
	#endif
			);
	
	GPIO_RegSet(GPIO_A_PU,0x00000000   //此时的flash的CS必须拉高0x00400000
	#if defined(CFG_RES_ADC_KEY_CH1_ANA_MASK)
			| CFG_RES_ADC_KEY_CH1_ANA_MASK
	#endif
	#if defined(CFG_PARA_WAKEUP_GPIO_CEC) && defined(CFG_PARA_WAKEUP_SOURCE_CEC)
			| BIT(CFG_PARA_WAKEUP_GPIO_CEC)
	#endif
			);
	
	GPIO_RegSet(GPIO_A_ANA_EN,0x00000000);
	GPIO_RegSet(GPIO_A_PULLDOWN0,0x00000000);//bkd
	GPIO_RegSet(GPIO_A_PULLDOWN1,0x00000000);//bkd

	GPIO_RegSet(GPIO_B_IE,0x00);
	GPIO_RegSet(GPIO_B_OE,0x00); 
	GPIO_RegSet(GPIO_B_OUTDS,0x00); // bkd mark GPIO_B_REG_OUTDS
	GPIO_RegSet(GPIO_B_PD,0xff);//B2、B3下拉，B4,B5高阻 0x1cc
	GPIO_RegSet(GPIO_B_PU,0x00);//B0、B1上拉 0x03
	GPIO_RegSet(GPIO_B_ANA_EN,0x00);
	GPIO_RegSet(GPIO_B_PULLDOWN,0x00);//bkd mark GPIO_B_REG_PULLDOWN
}

#endif//CFG_FUNC_DEEPSLEEP_EN

#ifdef BT_TWS_SUPPORT
void tws_stop_callback()//进入sniff时库里会调用
{
#ifdef BT_SNIFF_ENABLE
	SysDeepsleepStandbyStatus();
	printf("SysDeepsleepStandbyStatus\n");
#endif//BT_SNIFF_ENABLE
}
#endif

#ifdef BT_SNIFF_ENABLE

void BtWakeupConfigUsr()//蓝牙唤醒配置入口
{
	//打开默认串口
//	Clock_UARTClkSelect(PLL_CLK_MODE);//先切换log clk。避免后续慢速处理
//	LogUartConfig(TRUE);
}
BTSNIFF_GET_DEFAULT_CLKCONFIG_t sniff_get_clkconfig;

void BtDeepSleepForUsr(void)//蓝牙休眠配置入口，目前没做处理
{
#ifdef BT_TWS_SUPPORT
	//快速启动pll有点问题
//	uint32_t SLOPE, NDAC, OS, K1, FC;
//	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;
//
//	Clock_GetDPll(&NDAC, &OS, &K1, &FC);
//	SLOPE = DPLL_QUICK_START_SLOPE;//获取快速启动，参数。

//	GPIO_PortAModeSet(GPIOA30,0);		  //去掉Sw复用。调试口
//	GPIO_PortAModeSet(GPIOA31,0);

	DelayMs(1);//蓝牙硬件delay时间

	UartClkChange(RC_CLK_MODE);
	GIE_DISABLE();
	DeepSleepIOConfig();
	Power_DeepSleepLDO12ConfigTest(0,5,0);//进入deepsleep时电压降为1V0
	SysTickDeInit();

	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE13_BT, 0, 0);//设置蓝牙为唤醒源，无需IO所以随便填了0
	Power_WakeupEnable(SYSWAKEUP_SOURCE13_BT);

#if defined(CFG_PARA_WAKEUP_SOURCE_IR)
	SystermIRWakeupConfig_sniff(CFG_PARA_IR_SEL, CFG_RES_IR_PIN, CFG_PARA_IR_BIT);
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_APP_HDMIIN_MODE_EN)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_CEC, CFG_PARA_WAKEUP_GPIO_CEC, SYSWAKEUP_SOURCE_BOTH_EDGES_TRIG);
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_ADCKEY, CFG_PARA_WAKEUP_GPIO_ADCKEY, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
	SystermGPIOWakeupConfig(SYSWAKEUP_SOURCE6_POWERKEY, 42, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY1) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY1)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY1, CFG_PARA_WAKEUP_GPIO_IOKEY1, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY2) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY2)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY2, CFG_PARA_WAKEUP_GPIO_IOKEY2, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

	NVIC_DisableForDeepsleep();//关闭所有中断，只开唤醒中断

	Clock_DeepSleepSysClkSelect(RC_CLK_MODE, FSHC_RC_CLK_MODE, TRUE);

	Clock_PllClose();
	Clock_LOSCDisable();
//	Clock_HOSCDisable();

	sources = 0;
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	waitCECTimeFlag = 0;
#endif
//	GIE_ENABLE();//Power_GotoDeepSleep里面有开时钟动作

	Power_GotoDeepSleep();

	GIE_DISABLE();
//	Clock_PllQuicklock(288000, K1, OS, NDAC, FC, SLOPE);
#ifdef BT_TWS_SUPPORT
#ifdef CFG_FUNC_MIC_KARAOKE_EN
    Clock_PllLock(320000);
#else
    Clock_PllLock(300000);
#endif
#else
    Clock_PllLock(288000);
#endif
	Clock_DeepSleepSysClkSelect(PLL_CLK_MODE,FSHC_PLL_CLK_MODE,FALSE);

	NVIC_EnableForDeepsleep();//中断恢复

//	GIE_ENABLE();

	SysTickInitSet();//开始OS 打开全局时钟

#if (BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK)
	Clock_RcFreqCntOneTimeStart();
#endif//BT_SNIFF_RC_CLK

	UartClkChange(PLL_CLK_MODE);

//	GPIO_PortAModeSet(GPIOA30, 0x0005);//调试口 SW恢复 方便下载
//	GPIO_PortAModeSet(GPIOA31, 0x0004);

//	extern uint8_t OtgPortLinkState;
//	Timer_Config(TIMER2,1000,0);
//	Timer_Start(TIMER2);
//	NVIC_EnableIRQ(Timer2_IRQn);
//	OtgPortLinkState = 0;

#else
	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;

#ifdef BT_SNIFF_DEBUG
	{
		extern uint32_t rccnt_new;
		APP_DBG("rc:%ld\r\n",rccnt_new);
	}
#endif

	if(!((*(volatile unsigned long*)0x40021024) & 0x2))//系统在PLL下，需要切到apll保证更低的功耗
	{
		Clock_DeepSleepSysClkSelect(APLL_CLK_MODE,FSHC_APLL_CLK_MODE,FALSE);
	}
	DelayMs(1);//给蓝牙硬件点时间处理(蓝牙最后一个tim和evt大概1m)
	GpioAPU_Back = GPIO_RegGet(GPIO_A_PU);//保存上下拉
	GpioAPD_Back = GPIO_RegGet(GPIO_A_PD);
	GpioBPU_Back = GPIO_RegGet(GPIO_B_PU);
	GpioBPD_Back = GPIO_RegGet(GPIO_B_PD);

//	GPIO_PortAModeSet(GPIOA30,0);		  //去掉Sw复用。调试口
//	GPIO_PortAModeSet(GPIOA31,0);
	GPIO_PortBModeSet(GPIOB0,0);
	GPIO_PortBModeSet(GPIOB1,0);

	GIE_DISABLE();
	DeepSleepIOConfig();
	Power_DeepSleepLDO12ConfigTest(0,5,0);//进入deepsleep时电压降为1V0
	SysTickDeInit();
	NVIC_EnableIRQ(Wakeup_IRQn);
	NVIC_SetPriority(Wakeup_IRQn, 0);
	Power_WakeupSourceClear();
	Power_WakeupSourceSet(SYSWAKEUP_SOURCE13_BT, 0, 0);//设置蓝牙为唤醒源，无需IO所以随便填了0
	Power_WakeupEnable(SYSWAKEUP_SOURCE13_BT);
	{
		int div = (int)floor((double)sniff_get_clkconfig.sys_clk/48000000);//向下取整保证sys>48M 防BT异常

		Clock_SysClkDivSet(div);
		Clock_CoreClkDivSet(1);
	}
	Clock_PllClose();
	Clock_DeepSleepSysClkSelect(RC_CLK_MODE, FSHC_RC_CLK_MODE, TRUE);
	Power_LDO12Config(1000);
	Efuse_ReadDataDisable();
	Clock_AUPllClose();


#if (BT_SNIFF_CLK_SEL != BT_SNIFF_LOSC_CLK)
	Clock_LOSCDisable();
#endif

#if (BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK)
	//RC
	Power_HRCCtrlByHwDuringDeepSleep(1);
	Clock_HOSCDisable();
#endif

	Power_GotoDeepSleep();

	GIE_DISABLE();
#if (BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK)
	Clock_Config(1, 24000000);
#endif
//	Clock_PllLock(120000);		//用DPLL
//	Clock_DeepSleepSysClkSelect(PLL_CLK_MODE,FSHC_PLL_CLK_MODE,FALSE);
	Clock_APllLock(144000);		//用APLL
	Clock_DeepSleepSysClkSelect(APLL_CLK_MODE,FSHC_APLL_CLK_MODE,FALSE);

	Clock_UARTClkSelect(APLL_CLK_MODE);//先切换log clk。避免后续慢速处理
	LogUartConfig(TRUE); //scan不打印时 可屏蔽
	GIE_ENABLE();

#if (BT_SNIFF_CLK_SEL == BT_SNIFF_RC_CLK)
	Clock_RcFreqCntOneTimeStart();
#endif

	GPIO_RegSet(GPIO_A_PU, GpioAPU_Back);
	GPIO_RegSet(GPIO_A_PD, GpioAPD_Back);
	GPIO_RegSet(GPIO_B_PU, GpioBPU_Back);
	GPIO_RegSet(GPIO_B_PD, GpioBPD_Back);
//	GPIO_PortAModeSet(GPIOA30, 0x0005);//调试口 SW恢复 方便下载
//	GPIO_PortAModeSet(GPIOA31, 0x0004);
	GPIO_PortBModeSet(GPIOB0,1);
	GPIO_PortBModeSet(GPIOB1,1);

	SysTickInitSet();
#endif


}

//IR退出sniff轮询
void IrWakeupProcess(void)
{
	uint32_t Cmd = 0;
	uint8_t val = 0;

	//printf("IrWakeupProcess\n");
	if(IR_CommandFlagGet())
	{
		Cmd = IR_CommandDataGet();
		val = IRKeyIndexGet_BT(Cmd);

		SetIrKeyValue((uint8_t)2,(uint16_t)val);
		//APP_DBG("cmd:0x%lx,0x%d,%x\n",Cmd,GetIrKeyValue(),val);
		if(GetIrKeyValue() == MSG_BT_SNIFF)
		{
			extern void BtSniffExit_process(void);
			sources = 0;
			ClrGlobalKeyValue();
			IR_Disable();
			BtSniffExit_process();
		}
		IR_IntFlagClear();
		IR_CommandFlagClear();
	}
}

#ifdef CFG_APP_HDMIIN_MODE_EN
//cec退出sniff
void CecWakeupProcess(void)
{
	HDMI_CEC_Scan(0);
	if(gHdmiCt->hdmi_poweron_flag == 1)
	{
		SoftFlagRegister(SoftFlagWakeUpSouceIsCEC);
		extern void BtSniffExit_process(void);
		sources = 0;
		BtSniffExit_process();
	}
}
#endif
uint8_t GetDebugPrintPort(void);
void UartClkChange(CLK_MODE clk_change)//蓝牙唤醒配置入口
{
	//串口时钟切换
#ifdef FUNC_OS_EN
	if(GetDebugPrintPort())
	{
		osMutexLock(UART1Mutex);
	}
	else
	{
		osMutexLock(UART0Mutex);
	}
#endif
	Clock_UARTClkSelect(clk_change);//先切换log clk。避免后续慢速处理
	LogUartConfig(TRUE);
#ifdef FUNC_OS_EN
	if(GetDebugPrintPort())
	{
		osMutexUnlock(UART1Mutex);;
	}
	else
	{
		osMutexUnlock(UART0Mutex);;
	}

#endif
}

uint8_t sniffiocnt = 0;
uint8_t sniff_wakeup_check()
{
	bool CecRest = FALSE;

//	if(!GPIO_RegOneBitGet(GPIO_A_IN,GPIOA23))
//	{//唤醒流程，按键大于两个sniff周期就唤醒。如果用时间判断可能导致sniff功能不正常
//
//		if(sniffiocnt > 2)//按键超过两个周期就退出。
//		{//退出sniff。
//			sniffiocnt = 0;
//			extern void BtSniffExit_process(void);
//			BtSniffExit_process();
//
//			return 1;
//		}
//		else
//		{
//			sniffiocnt++;
//
//			return 1;
//		}
//
//	}

#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY) && defined(CFG_RES_ADC_KEY_SCAN)
		if(sources & (CFG_PARA_WAKEUP_SOURCE_ADCKEY))
		{
			AdcKeyMsg AdcKeyVal;

			SarADC_Init();
			AdcKeyInit();

			sources = 0;
			//如果没出现RELEASED就在while里面
			while((AdcKeyVal.type != ADC_KEY_RELEASED) && (AdcKeyVal.type != ADC_KEY_LONG_RELEASED))
			{
				AdcKeyVal = AdcKeyScan();

				if(AdcKeyVal.index != ADC_CHANNEL_EMPTY && AdcKeyVal.type != ADC_KEY_UNKOWN_TYPE)
				{
//					APP_DBG("KeyID:%d,type:%d\n", AdcKeyVal.index, AdcKeyVal.type);
					SetAdcKeyValue(AdcKeyVal.type,AdcKeyVal.index);
					if((GetGlobalKeyValue() == MSG_DEEPSLEEP)||(GetGlobalKeyValue() == MSG_BT_SNIFF))
					{
						ClrGlobalKeyValue();
						extern void BtSniffExit_process(void);
						BtSniffExit_process();
						return 1;
					}
					ClrGlobalKeyValue();
				}
			}
			return 1;
		}
#endif

#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
		if(sources & SYSWAKEUP_SOURCE6_POWERKEY)
		{
			APP_DBG("POWER_Key!\n");
			sources = 0;
			extern void BtSniffExit_process(void);
			BtSniffExit_process();
			return 1;
		}
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_IR) && defined(CFG_RES_IR_KEY_SCAN)
		if(sources & (CFG_PARA_WAKEUP_SOURCE_IR))
		{
			IRKeyMsg IRKeyVal;
			
			//如果没出现RELEASED就在while里面
			while((IRKeyVal.type != ADC_KEY_RELEASED) && (IRKeyVal.type != ADC_KEY_LONG_RELEASED))
			{
				IRKeyVal = IRKeyScan();

				if(IRKeyVal.index != ADC_CHANNEL_EMPTY && IRKeyVal.type != ADC_KEY_UNKOWN_TYPE)
				{
					//APP_DBG("IRID:%d,type:%d\n", IRKeyVal.index, IRKeyVal.type);
					SetIrKeyValue(IRKeyVal.type,IRKeyVal.index);
#ifdef CFG_APP_REST_MODE_EN
					if(GetGlobalKeyValue() == MSG_POWER)
#else
					if((GetGlobalKeyValue() == MSG_DEEPSLEEP)||(GetGlobalKeyValue() == MSG_BT_SNIFF))
#endif
					{
						sources = 0;
						ClrGlobalKeyValue();
						extern void BtSniffExit_process(void);
						BtSniffExit_process();
						return 1;
					}
					ClrGlobalKeyValue();
				}
			}
			return 1;
		}
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_APP_HDMIIN_MODE_EN)
	if(sources & (CFG_PARA_WAKEUP_SOURCE_CEC))
	{
#if defined(CFG_PARA_WAKEUP_SOURCE_IR)
		IrWakeupProcess();
#endif
		CecWakeupProcess();
		if(IsTimeOut(&waitCECTime))//超时之后等待下拉电平，同时保持scan。
			CecRest = HDMI_CEC_IsReadytoDeepSleep(6);
		else
			return 1;

		if(!CecRest)
			return 1;
		sources = 0;
		return 1;
	}
#endif
	sources = 0;
	return 0;

}


uint8_t sniff_wakeup_flag = 0;//本次休眠唤醒是否触发sniff标志，为保系统稳定，此标志可确保系统不受stack影响
							  //0sniff流程没触发   1sniff已经触发
void sniff_wakeup_set(uint8_t set)
{
	sniff_wakeup_flag = set;
}
uint8_t sniff_wakeup_get()
{
	return sniff_wakeup_flag;
}

uint8_t sniff_lmp_sync_flag = 0;//sniff的lmp同步命令已经发送过标志，防止用于UI多次发送lmp导致异常,此标志可确保协议栈不受系统影响
								//0表示未发送可接受，1表示已经有命令请等待。
void sniff_lmpsend_set(uint8_t set)
{
	sniff_lmp_sync_flag = set;
}
uint8_t sniff_lmpsend_get()
{
	return sniff_lmp_sync_flag;
}


void tws_sniff_check_adda_process()
{
//	if(GetBtManager()->twsRole == BT_TWS_SLAVE)
	{

//		if(BtSniffADDAReadyGet() == 3)
//		{
//			printf("wakeup all ready\n");
//			if(sniff_lmpsend_get() == 1)
//			{
//				//##__退出sniff，标志位恢复__##
//				sniff_lmpsend_set(0);
//				printf("sniff_lmpsend_set(0)\n");
//				//##____________________##
//			}
//			BtSniffADDAReadySet(0);//清空ADDA准备标志
//			tws_link_status_set(1);
//			printf("tws_link_status_set(1)\n");
//		}

		//链路层断开后，发现sniff有标志没清。
		if((sniff_wakeup_get() /*|| sniff_lmpsend_get()*/)
#ifdef BT_TWS_SUPPORT
				&& (GetBtManager()->twsState != BT_TWS_STATE_CONNECTED)
#endif
				)
		{
			//断开后的sniff状态恢复
			sniff_wakeup_set(0);
			//sniff_lmpsend_set(0);

		}
	}
}

TIMER   sniffrerequsettimer;
#define RESEND_SCAN_TIME				2000		//醒来确认有效唤醒源 扫描限时ms。
uint32_t deepsleep_count = 0;
extern uint8_t sniff_clk_get(void);
void DeepSleeping_BT(void)
{
	uint32_t GpioAPU_Back,GpioAPD_Back,GpioBPU_Back,GpioBPD_Back;

	//Efuse_ReadDataDisable();
//	SysDeepsleepStart();
	deepsleep_count = 1;
	
	BtStartEnterSniffMode();
	TimeOutSet(&sniffrerequsettimer, RESEND_SCAN_TIME);

	while((Bt_sniff_sniff_start_state_get() == 0) ||
			(Bt_sniff_sleep_state_get() == 0))
	{

		if(IsTimeOut(&sniffrerequsettimer))
		{
			APP_DBG("LMP sniff state ERR!!!\r\n");
			BtDisconnectCtrl();//避免部分手机未真正断开,再次发起断开请求
			BtStartEnterSniffMode();
#ifdef BT_TWS_SUPPORT
			//断开连接后，跳出等待siff req，然后进入低功耗扫描
			if(GetBtManager()->twsState != BT_TWS_STATE_CONNECTED)
			{
				Bt_sniff_sniff_start();
				break;
			}
			deepsleep_count++;
			if(deepsleep_count > 3)
			{
				Bt_sniff_sniff_start();
				break;
			}
#endif
			TimeOutSet(&sniffrerequsettimer, RESEND_SCAN_TIME);
		}

		vTaskDelay(2);
	}
	deepsleep_count = 0;

	GpioAPU_Back = GPIO_RegGet(GPIO_A_PU);//保存上下拉
	GpioAPD_Back = GPIO_RegGet(GPIO_A_PD);
	GpioBPU_Back = GPIO_RegGet(GPIO_B_PU);
	GpioBPD_Back = GPIO_RegGet(GPIO_B_PD);

#ifdef BT_TWS_SUPPORT
	if(tws_get_role() == BT_TWS_MASTER)
		BtSetAccessModeMsg(BtAccessModeNotAccessible, 0);
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
		DisableAdvertising();
#endif
#endif

	//cec config
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC) && defined(CFG_APP_HDMIIN_MODE_EN)
	if(gHdmiCt == NULL)
	{
		HDMI_CEC_DDC_Init();
		gHdmiCt->hdmiReportStatus = 0;
	}
	while(!HDMI_CEC_IsReadytoDeepSleep(10));//拉低10ms，保障后续18ms没有通信和中断响应。
#endif

#ifdef CFG_PARA_WAKEUP_SOURCE_POWERKEY
	SystermGPIOWakeupConfig(SYSWAKEUP_SOURCE6_POWERKEY, 42, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_ADCKEY, CFG_PARA_WAKEUP_GPIO_ADCKEY, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY1) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY1)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY1, CFG_PARA_WAKEUP_GPIO_IOKEY1, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif
#if defined(CFG_PARA_WAKEUP_SOURCE_IOKEY2) && defined(CFG_PARA_WAKEUP_GPIO_IOKEY2)
	SystermGPIOWakeupConfig(CFG_PARA_WAKEUP_SOURCE_IOKEY2, CFG_PARA_WAKEUP_GPIO_IOKEY2, SYSWAKEUP_SOURCE_NEGE_TRIG);
#endif

	sources = 0;//休眠前清除所有唤醒中断。
#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC)
	waitCECTimeFlag = 0;
#endif	
	printf("==== sniff clk:%d\n", sniff_clk_get());
	
#ifdef BT_TWS_SUPPORT
	BTSniffSet();//准备进入sniff
#endif

	while(Bt_sniff_sniff_start_state_get())//没退出sniff消息，进入sniff休眠轮询。
	{
		vTaskDelay(1);
		if(Bt_sniff_sleep_state_get())
		{
			Bt_sniff_sleep_exit();

			#ifdef BT_TWS_SUPPORT
			if(GetBtManager()->twsRole == BT_TWS_MASTER)
			{
				//此项目从机无UI，所以注释掉了从机唤醒的逻辑
				if(sniff_wakeup_check())// 如果出现唤醒标志次周期不睡，并且函数内部可以跳出sniff
				{
					Bt_sniff_sleep_exit();
					continue;
				}
			}
			#endif

			BtDeepSleepForUsr();
			
			#ifdef BT_TWS_SUPPORT
			if((sources & CFG_PARA_WAKEUP_SOURCE_IR) || (sources & CFG_PARA_WAKEUP_SOURCE_ADCKEY))
			{
				if(sniff_wakeup_check())// 如果出现唤醒标志次周期不睡，并且函数内部可以跳出sniff
				{
					Bt_sniff_sleep_exit();
				}
			}
			#endif
		}
	}

#ifdef BT_TWS_SUPPORT
	if(tws_get_role() == BT_TWS_MASTER)
	{
	#ifdef BT_FAST_POWER_ON_OFF_FUNC
		if(IsBtAudioMode())
		{
			BtExitSniffReconnectFlagSet();
		}
	#else
		BtExitSniffReconnectFlagSet();
	#endif
		BtSetAccessModeMsg(BtAccessModeGeneralAccessible, 200);
	}
#endif

#if defined(CFG_PARA_WAKEUP_SOURCE_CEC) && defined(CFG_PARA_WAKEUP_GPIO_CEC) && defined(CFG_APP_HDMIIN_MODE_EN)
	HDMI_CEC_DDC_DeInit();
#endif

	GPIO_RegSet(GPIO_A_PU, GpioAPU_Back);
	GPIO_RegSet(GPIO_A_PD, GpioAPD_Back);
	GPIO_RegSet(GPIO_B_PU, GpioBPU_Back);
	GPIO_RegSet(GPIO_B_PD, GpioBPD_Back);

	Efuse_ReadDataEnable();

#ifdef BT_TWS_SUPPORT
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	ble_advertisement_data_update();
#elif (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	BleScanParamConfig_Default();
#endif
#endif
	UartClkChange(APLL_CLK_MODE);

	WDG_Feed();
	WakeupMain();
	SysTickInitSet();
	WDG_Feed();

#ifdef CFG_FUNC_LED_REFRESH
	//默认优先级为0，旨在提高刷新速率，特别是断点记忆等写flash操作有影响刷屏，必须严格遵守所有timer6中断调用都是TCM代码，含调用的driver库代码
	//已确认GPIO_RegOneBitSet、GPIO_RegOneBitClear在TCM区，其他api请先确认。
	NVIC_SetPriority(Timer6_IRQn, 0);
 	Timer_Config(TIMER6,1000,0);
 	Timer_Start(TIMER6);
 	NVIC_EnableIRQ(Timer6_IRQn);

 	//此行代码仅仅用于延时，配合Timer中断处理函数，客户一定要做修改调整
 	//GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX2);//only test，user must modify
#endif

#if defined(CFG_FUNC_DISPLAY_EN)
    DispInit(0);
#endif
}


#endif //BT_SNIFF_ENABLE

