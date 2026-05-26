/*
 * audio_effect_class.c
 *
 *  Created on: Jun 8, 2023
 *      Author: szsj-1
 */
#include <string.h>
#include <stdint.h>
#include "debug.h"
#include "app_config.h"
#include "audio_effect.h"
#include "audio_effect_library.h"
#include "ctrlvars.h"
#include "timeout.h"
#include "delay.h"
#include "nds32_intrinsic.h"
#include "communication.h"
#include "main_task.h"
//-------------//
extern uint8_t  tx_buf[256];
extern bool IsEffectChange;
extern uint16_t effect_list[EFFECT_LIST_LEN];
extern uint32_t effect_addr[EFFECT_LIST_LEN];
void Communication_Effect_Send(uint8_t *buf, uint32_t len);
uint8_t AudioEffectClassDescParser(uint8_t Control,const void *pdata);
//--------------//
/*
 * 具体协议在调音工具目录下：固件与用户应用程序通信协议V2.39.3.pdf----> 5.20.46 用户自定义音效格式
|----------------------------------------------------------------------------------------|
|                |  parameter total |
|----------------------------------------------------------------------------------------|
|   parameter n  |  data type       | data .......     | default value    | name,unit
|----------------------------------------------------------------------------------------|
|   parameter n  |  data type       | data .......     | default value    | name,unit
|----------------------------------------------------------------------------------------|
|   parameter n  |  data type       | data .......     | default value    | name,uni
|----------------------------------------------------------------------------------------|
|   parameter n  |  data type       | data .......     | default value    | name,unit
|----------------------------------------------------------------------------------------|
|   parameter n  |  data type       | data .......     | default value    | name,unit
|----------------------------------------------------------------------------------------|
*/
/*data type
  boolType
  0x00: 逻辑类型
                 2 Bytes: 默认值
  enumType
  0x01: 枚举类型
                 1 Byte：参数内容长度
                 N Bytes: 参数内容， 字符串格式上传，并用分号（；）间隔开不同的枚举值
                 2 Bytes: 默认值

  NumberType
  0x02: 连续类型    2 Bytes: 最小值
                 2 Bytes: 最大值
                 2 Bytes: 步进值
                 2 Bytes： 显示与传输值关系, 范围 1~65536 1： 显示值=传输值2： 显示值=传输值/2…1024： 显示值 = 传输值/1024
                 2 Bytes： 保留小数位                 0： 整数1： 1 位小数 2： 2 位小数
                 2 Bytes: 默认值
                 1 Byte:  参数的单位长度
                 N Bytes: 参数的单位内容
  DispType
  0x03: 显示类型（只读）
                2 Bytes: 最小值
                2 Bytes: 最大值
                2 Bytes：显示值与传输值关系           1： 显示值=传输值           2： 显示值=传输值/2   1024： 显示值 = 传输值/1024
                2 Bytes：保留小数位 0： 整数 1： 1 位小数 2： 2 位小数
                2 Bytes: 默认值
                1 Byte:  参数的单位长度
                N Bytes: 参数的单位内容
*/
/*********************************************
 *
 *
 * dra post audio class,describe tab
 *
 *
 *********************************************/
const DraPostDescribe1 DraPostTabPage1=
{
  {8},
#if CFG_AUDIO_EFFECT_DRAPOST_EN
  //----------------page 1-------
  {boolType,     0x0000,"EffSw",},
  {continueType, 0x0000,0x03e8,0x0001,0x0001,0x0000,0x0032,"Freq1",         "HZ"},
  {continueType, 0x0000,0x03e8,0x0001,0x0001,0x0000,0x0032,"Freq2",         "HZ"},
  {continueType, 0x03e8,0x4E20,0x0001,0x0001,0x0000,0x03e8,"Freq3",         "HZ"},
  {continueType, 0x03e8,0x4E20,0x0001,0x0001,0x0000,0x03e8,"Freq4",         "HZ"},
  {continueType, 0x0000,0x005A,0x0001,0x0001,0x0000,0x0000,"WideCen",       "°C"},
  {continueType, 0x0001,0x0013,0x0001,0x0001,0x0000,0x0001,"WideGain",       ""},
  {continueType, 0x000A,0x0014,0x0001,0x0001,0x0000,0x000A,"TotalGain",      ""},
#endif
};
const DraPostDescribe2 DraPostTabPage2=
{
  {2},
#if CFG_AUDIO_EFFECT_DRAPOST_EN
  //----------------page 1-------
  {boolType,     0x0000,"CTCSw",},
  {continueType, 0x0001,0x00C,0x0001,0x0001,0x0000,0x0001,"CTCMode",        ""},
#endif
};
const DraPostDescribe3 DraPostTabPage3=
{
  {5},
#if CFG_AUDIO_EFFECT_DRAPOST_EN
 //---------------page 2-------------10----------------------------------------//
  {boolType,     0x0000,"UPMixSw",},
  {continueType, 0x0000,0x0014,0x0001,0x0001,0x0000,0x0001,"LRGain",       ""},
  {continueType, 0x0000,0x0014,0x0001,0x0001,0x0000,0x0001,"T1Gain",      ""},
  {continueType, 0x0000,0x0014,0x0001,0x0001,0x0000,0x0001,"MixGain",       ""},
  {continueType, 0x000A,0x0014,0x0001,0x0001,0x0000,0x000A,"SpeechGain",      ""},
#endif
};
const DraPostDescribe4 DraPostTabPage4=
{
  {5},
#if CFG_AUDIO_EFFECT_DRAPOST_EN
 //---------------page 2-------------10----------------------------------------//
  {boolType,     0x0000,"VBSw",},
  {continueType, 0x003c,0x00B4,0x0014,0x0001,0x0000,0x003c,"VBCut",      "HZ"},
  {continueType, 0x0001,0x000A,0x0001,0x0001,0x0000,0x0001,"VBGain",      ""},
  {enumType,     "30HZ;40HZ;55HZ;70HZ",0x0000,"LowFreq1"},
  {continueType, 0x003c,0x00B4,0x0014,0x0001,0x0000,0x003c,"LowFreq2",      "HZ"},
#endif
};

const DraPostDescribe5 DraPostTabPage5=
{
  {6},
#if CFG_AUDIO_EFFECT_DRAPOST_EN
  {enumType,     "240HZ;300HZ;400HZ;500HZ;600HZ;700HZ;800HZ",0x0000,"HarmCut"},
  {continueType, 0x0001,0x000A,0x0001,0x0001,0x0000,0x0001,"FScale",       ""},
  {continueType, 0x0001,0x000A,0x0001,0x0001,0x0000,0x0001,"F1Gain",       ""},
  {continueType, 0x0001,0x0020,0x0001,0x0001,0x0000,0x0001,"Volume",       ""},
  {continueType, 0x0032,0x01f4,0x0001,0x0001,0x0000,0x0032,"Distance",       ""},
  {enumType,     "LOW;MID;HIG",0x0000,"RoomMode"},
#endif
};
void Communication_Effect_DraPost_page1(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN
	DraPostUnit *p = (DraPostUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)DraPostTabPage1.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2);

		memcpy(&tx_buf[7], &p->nEffectActive, 2);//1
		memcpy(&tx_buf[9], &p->nFreq1, 2);//2
		memcpy(&tx_buf[11], &p->nFreq2, 2);//3
		memcpy(&tx_buf[13], &p->nFreq3, 2);//4
		memcpy(&tx_buf[15], &p->nFreq4, 2);//5
		memcpy(&tx_buf[17], &p->nWidenCenter, 2);//6
		memcpy(&tx_buf[19], &p->nWidenGain, 2);//7
		memcpy(&tx_buf[21], &p->nTotalGain, 2);//8

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					}
					IsEffectChange = 1;
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				TmpData16 &= 0x01;
				p->nEffectActive = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 1000)
				{
					TmpData16 = 1000;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->nFreq1 = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 1000)
				{
					TmpData16 = 1000;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->nFreq2 = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 4:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 20000)
				{
					TmpData16 = 20000;
				}
				if(TmpData16 < 1000)
				{
					TmpData16 = 1000;
				}
				p->nFreq3 = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 5:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 20000)
				{
					TmpData16 = 20000;
				}
				if(TmpData16 < 1000)
				{
					TmpData16 = 1000;
				}
				p->nFreq4 = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 6:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 90)
				{
					TmpData16 = 90;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->nWidenCenter = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 7:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 19)
				{
					TmpData16 = 19;
				}
				if(TmpData16 < 1)
				{
					TmpData16 = 1;
				}
				p->nWidenGain = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 8:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 20)
				{
					TmpData16 = 20;
				}
				if(TmpData16 < 10)
				{
					TmpData16 = 10;
				}
				p->nTotalGain = TmpData16;
				AudioEffectDraPostReset(p);
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);

				memcpy(&p->nEffectActive, &buf[3], 2);//1
				memcpy(&p->nFreq1, &buf[5], 2 );//2
				memcpy(&p->nFreq2, &buf[7], 2);//3
				memcpy(&p->nFreq3, &buf[9], 2);//4
				memcpy(&p->nFreq4, &buf[11], 2);//5
				memcpy(&p->nWidenCenter, &buf[13], 2);//6
				memcpy(&p->nWidenGain, &buf[15], 2);//7
				memcpy(&p->nTotalGain, &buf[17], 2);//8

				if(p->enable)
				{
					AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&DraPostTabPage1);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_DRAPOST_EN
}
//
void Communication_Effect_DraPost_page2(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN
	DraPostUnit *p = (DraPostUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)DraPostTabPage2.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2);

		memcpy(&tx_buf[7], &p->bCTCActive, 2);//9
		memcpy(&tx_buf[9], &p->nCTCMode, 2);//10

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					}
					IsEffectChange = 1;
				}
				break;

			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				TmpData16 &= 1;
				p->bCTCActive = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 12)
				{
					TmpData16 = 12;
				}
				if(TmpData16 < 1)
				{
					TmpData16 = 1;
				}
				p->nCTCMode = TmpData16;
				AudioEffectDraPostReset(p);
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);

				memcpy(&p->bCTCActive, &buf[3], 2);//1
				memcpy(&p->nCTCMode, &buf[5], 2 );//2

				if(p->enable)
				{
					AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&DraPostTabPage2);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_DRAPOST_EN
}
//-------------------//
void Communication_Effect_DraPost_page3(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN
	DraPostUnit *p = (DraPostUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)DraPostTabPage3.numbers.paramete_totals+1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;


		memcpy(&tx_buf[5], &p->enable, 2);
		//------------------------------------//
		memcpy(&tx_buf[7], &p->bUpmixActive, 2);//1
		memcpy(&tx_buf[9], &p->fUpmixLRGainCoef, 2);//2
		memcpy(&tx_buf[11], &p->fUpmixTlTrGainCoef, 2);//3
		memcpy(&tx_buf[13], &p->fUpmixGainCoef, 2);//4
		memcpy(&tx_buf[15], &p->fSpeechEnhancementGain, 2);//5

		//---------------------------------//
		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
						if(p->enable)
						{
							IsEffectChange = 1;
						}
					}
					else
					{
						IsEffectChange = 1;
					}
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				TmpData16 &= 0x01;
				p->bUpmixActive = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 20)
				{
					TmpData16 = 20;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->fUpmixLRGainCoef = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 20)
				{
					TmpData16 = 20;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->fUpmixTlTrGainCoef = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 4:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 20)
				{
					TmpData16 = 20;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->fUpmixGainCoef = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 5:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 20)
				{
					TmpData16 = 20;
				}
				if(TmpData16 < 10)
				{
					TmpData16 = 10;
				}
				p->fSpeechEnhancementGain = TmpData16;
				AudioEffectDraPostReset(p);
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);
				memcpy(&p->bUpmixActive, &buf[3], 2);//1
				memcpy(&p->fUpmixLRGainCoef, &buf[5], 2 );//2
				memcpy(&p->fUpmixTlTrGainCoef, &buf[7], 2);//3
				memcpy(&p->fUpmixGainCoef, &buf[9], 2);//4
				memcpy(&p->fSpeechEnhancementGain, &buf[11], 2);//5

				if(p->enable)
				{
					AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&DraPostTabPage3);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_DRAPOST_EN
}
//------------------//
void Communication_Effect_DraPost_page4(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN
	DraPostUnit *p = (DraPostUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)DraPostTabPage4.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2);

		memcpy(&tx_buf[7], &p->bVirtualBassActive, 2);//6
		memcpy(&tx_buf[9], &p->nVBCutOffFreq, 2);//7
		memcpy(&tx_buf[11], &p->fVBGain, 2);//8
		memcpy(&tx_buf[13], &p->nLowFreq1, 2);//9
		memcpy(&tx_buf[15], &p->nLowFreq2, 2);//10
       //------------------------------------------------//
		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					}

					IsEffectChange = 1;
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 1)
				{
					TmpData16 = 1;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->bVirtualBassActive = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 180)
				{
					TmpData16 = 180;
				}
				if(TmpData16 < 60)
				{
					TmpData16 = 60;
				}

				p->nVBCutOffFreq = TmpData16;
				AudioEffectDraPostReset(p);
				break;

			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 10)
				{
					TmpData16 = 10;
				}
				if(TmpData16 < 1)
				{
					TmpData16 = 1;
				}
				p->fVBGain = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 4:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 3)
				{
					TmpData16 = 3;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}

				p->nLowFreq1 = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 5:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 180)
				{
					TmpData16 = 180;
				}
				if(TmpData16 < 60)
				{
					TmpData16 = 60;
				}
				p->nLowFreq2 = TmpData16;
				AudioEffectDraPostReset(p);
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);

				memcpy(&p->bVirtualBassActive, &buf[3], 2);//1
				memcpy(&p->nVBCutOffFreq, &buf[5], 2 );//2
				memcpy(&p->fVBGain, &buf[7], 2);//3
				memcpy(&p->nLowFreq1, &buf[9], 2);//4
				memcpy(&p->nLowFreq2, &buf[11], 2);//5

				if(p->enable)
				{
					AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&DraPostTabPage4);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_DRAPOST_EN
}
//------------------------//
void Communication_Effect_DraPost_page5(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_DRAPOST_EN
	DraPostUnit *p = (DraPostUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)DraPostTabPage5.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2);

		//------------------------------------//
		memcpy(&tx_buf[7], &p->nHarmCutOff, 2);//1
		memcpy(&tx_buf[9], &p->fScale, 2);//2
		memcpy(&tx_buf[11], &p->fGain_f1, 2);//3
		memcpy(&tx_buf[13], &p->nVolume, 2);//4
		memcpy(&tx_buf[15], &p->nDistance, 2);//5
		memcpy(&tx_buf[17], &p->nRoomMode, 2);//6
       //------------------------------------------------//
		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					}
					IsEffectChange = 1;
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 6)
				{
					TmpData16 = 6;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->nHarmCutOff = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 10)
				{
					TmpData16 = 10;
				}
				if(TmpData16 < 1)
				{
					TmpData16 = 1;
				}
				p->fScale = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 10)
				{
					TmpData16 = 10;
				}
				if(TmpData16 < 1)
				{
					TmpData16 = 1;
				}
				p->fGain_f1 = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 4:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 32)
				{
					TmpData16 = 32;
				}
				if(TmpData16 < 1)
				{
					TmpData16 = 1;
				}
				p->nVolume = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 5:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 500)
				{
					TmpData16 = 500;
				}
				if(TmpData16 < 50)
				{
					TmpData16 = 50;
				}
				p->nDistance = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 6:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 2)
				{
					TmpData16 = 2;
				}
				if(TmpData16 < 0)
				{
					TmpData16 = 0;
				}
				p->nRoomMode = TmpData16;
				AudioEffectDraPostReset(p);
				break;
			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);

				memcpy(&p->nHarmCutOff, &buf[3], 2);//1
				memcpy(&p->fScale, &buf[5], 2 );//2
				memcpy(&p->fGain_f1, &buf[7], 2);//3
				memcpy(&p->nVolume, &buf[9], 2);//4
				memcpy(&p->nFreq4, &buf[11], 2);//5
				memcpy(&p->nDistance, &buf[13], 2);//6
				memcpy(&p->nRoomMode, &buf[15], 2);//7

				if(p->enable)
				{
					AudioEffectDraPostInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&DraPostTabPage5);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_DRAPOST_EN
}
  const PhaseShifterDescribe PhaseShifterTab=
  {
    {1},
  #if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
    {continueType, 0xfe98,0x0168,0x0001,0x0001,0x0000,0x0000,"Phase",   "℃"},
  #endif
  };

void Communication_Effect_PhaseShifter(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
  #if CFG_AUDIO_EFFECT_PHASE_SHIFTER_EN
	  PhaseShifterUnit *p = (PhaseShifterUnit *)addr;
  	int16_t TmpData16;
  	int16_t parameter_len = (int16_t)PhaseShifterTab.numbers.paramete_totals + 1;//+ 1 = enable

  	memset(tx_buf, 0, sizeof(tx_buf));

  	if(len == 0)//ask
  	{
  		tx_buf[0] = 0xa5;
  		tx_buf[1] = 0x5a;
  		tx_buf[2] = Control;
  		tx_buf[3] = 2 * parameter_len + 1;
  		tx_buf[4] = 0xff;

  		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

  		tx_buf[5 + 2 * parameter_len] = 0x16;
  		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
  	}
  	else
  	{
  		switch(buf[0])//
  		{
  			case 0:
  				memcpy(&TmpData16, &buf[1], 2);
  				if(p->enable != TmpData16)
  				{
  					p->enable = TmpData16;
  					if(p->enable)
  					{
  						AudioEffectPhaseShifterdInit(p,gCtrlVars.adc_mic_channel_num,mainAppCt.SamplesPreFrame,gCtrlVars.sample_rate);
  						if(p->enable)
  						{
  							IsEffectChange = 1;
  						}
  					}
  					else
  					{
  						IsEffectChange = 1;
  					}
  				}
  				break;
  			case 1:
  				memcpy(&TmpData16, &buf[1], 2);
  				p->phase_shift = TmpData16;
  				break;
  			case 0xff:
  				memcpy((uint8_t *)&p->enable, &buf[1], 2*parameter_len);

  				if(p->enable)
  				{
  					AudioEffectPhaseShifterdInit(p,gCtrlVars.adc_mic_channel_num,mainAppCt.SamplesPreFrame,gCtrlVars.sample_rate);
  					IsEffectChange = 1;
  				}
  				break;

  			case 0xf0:
  				AudioEffectClassDescParser(Control,&PhaseShifterTab);
  				break;
  			default:
  				break;
  		}
  	}
  #endif //CFG_AUDIO_EFFECT_CHORUS_EN
}
/*********************************************
 *
 *
 * Howling guard audio class,describe tab
 *
 *
 *********************************************/
const HowlingGuardDescribe HowlingGuardTab=
{
  {7},
#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
  {continueType, 0xfda8,0x0000,0x0001,0x0064,0x0002,0xffce,"Saturation",   "db"},
  {continueType, 0x03e8,0x0fa0,0x0001,0x0001,0x0000,0x0032,"high_freq",     "Hz"},
  {continueType, 0x0001,0x0063,0x0001,0x0001,0x0000,0x0063,"high_freq_ratio",   "%"},
  {continueType, 0x0001,0x0bb8,0x0001,0x0001,0x0000,0x0001,"duration",       "ms"},
  {continueType, 0x0001,0x0bb8,0x0001,0x0001,0x0000,0x0001,"max_duration",  "ms"},
  {continueType, 0x0002,0x0bb8,0x0001,0x0001,0x0000,0x0002,"mute_period",   "ms"},
  {continueType, 0xffa6,0x0000,0x0001,0x0001,0x0000,0x0000,"gate_threshold",  "db"},
#endif
};

void Communication_Effect_HowlingGuard(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_HOWLING_GUARD_EN
	HowlingGuardUnit *p = (HowlingGuardUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)HowlingGuardTab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectHowlingGuardInit(p,gCtrlVars.sample_rate);
						if(p->enable)
						{
							IsEffectChange = 1;
						}
					}
					else
					{
						IsEffectChange = 1;
					}
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				p->saturation_threshold = TmpData16;
				AudioEffectHowlingGuardConfigure(p, gCtrlVars.sample_rate);
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				p->high_freq_threshold = TmpData16;
				AudioEffectHowlingGuardConfigure(p, gCtrlVars.sample_rate);
				break;
			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				p->high_freq_energy_ratio_threshold = TmpData16;
				AudioEffectHowlingGuardConfigure(p, gCtrlVars.sample_rate);
				break;
			case 4:
				memcpy(&TmpData16, &buf[1], 2);
				p->max_saturated_high_freq_duration = TmpData16;
				AudioEffectHowlingGuardConfigure(p, gCtrlVars.sample_rate);
				break;
			case 5:
				memcpy(&TmpData16, &buf[1], 2);
				p->max_saturated_duration = TmpData16;
				AudioEffectHowlingGuardConfigure(p, gCtrlVars.sample_rate);
				break;
			case 6:
				memcpy(&TmpData16, &buf[1], 2);
				p->mute_period = TmpData16;
				AudioEffectHowlingGuardConfigure(p, gCtrlVars.sample_rate);
				break;
			case 7:
				memcpy(&TmpData16, &buf[1], 2);
				p->noise_gate_threshold = TmpData16;
				AudioEffectHowlingGuardConfigure(p, gCtrlVars.sample_rate);
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2*parameter_len);

				if(p->enable)
				{
					AudioEffectHowlingGuardInit(p, gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&HowlingGuardTab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_CHORUS_EN
}
/*********************************************
 *
 *
 * chorus2 audio class,describe tab
 *
 *
 *********************************************/
const Chorus2Describe Chorus2Tab=
{
  {8},
#if CFG_AUDIO_EFFECT_CHORUS2_EN
  {continueType, 0x0000,0x001E,0x0001,0x0001,0x0000,0x0010,"delay_len",   "ms"},
  {continueType, 0x0000,0x0064,0x0001,0x0001,0x0000,0x0032,"dry",         "%"},
  {continueType, 0x0000,0x0064,0x0001,0x0001,0x0000,0x0032,"wet_1",       "%"},
  {continueType, 0x0000,0x0064,0x0001,0x0001,0x0000,0x0032,"wet_2",       "%"},
  {continueType, 0x0000,0x0064,0x0001,0x0001,0x0000,0x0032,"mod1_depth",  "ms"},
  {continueType, 0x0000,0x03E8,0x0001,0x0064,0x0002,0x0001,"mod1_rate",   "Hz"},
  {continueType, 0x0000,0x0064,0x0001,0x0001,0x0000,0x0032,"mod2_depth",  "ms"},
  {continueType, 0x0000,0x03E8,0x0001,0x0064,0x0002,0x0001,"mod2_rate",   "Hz"},
#endif
};

void Communication_Effect_Chorus2(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_CHORUS2_EN
	Chorus2Unit *p = (Chorus2Unit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)Chorus2Tab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectChorus2Init(p,gCtrlVars.sample_rate,p->bit_width);
						if(p->enable)
						{
							IsEffectChange = 1;
						}
					}
					else
					{
						IsEffectChange = 1;
					}
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 30)
				{
					TmpData16 = 30;
				}
				p->delay_length = TmpData16;
				if(p->mod1_depth > p->delay_length)
				{
					p->mod1_depth = p->delay_length;
				}
				if(p->mod2_depth > p->delay_length)
				{
					p->mod2_depth = p->delay_length;
				}
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 100)
				{
					TmpData16 = 90;
				}
				p->dry = TmpData16;
				break;
			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 100)
				{
					TmpData16 = 60;
				}
				p->wet_1 = TmpData16;
				break;
			case 4:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 100)
				{
					TmpData16 = 60;
				}
				p->wet_2 = TmpData16;
				break;
			case 5:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 100)
				{
					TmpData16 = 100;
				}
				p->mod1_depth = TmpData16;
				break;
			case 6:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 1000)
				{
					TmpData16 = 1000;
				}
				p->mod1_rate = TmpData16;
				break;
			case 7:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > p->delay_length)
				{
					TmpData16 = p->delay_length;
				}
				p->mod2_depth = TmpData16;
				break;
			case 8:
				memcpy(&TmpData16, &buf[1], 2);
				if(TmpData16 > 1000)
				{
					TmpData16 = 1000;
				}
				p->mod2_rate = TmpData16;
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2*9);

				if(p->delay_length > 30)
				{
					p->delay_length = 30;
				}

				if(p->mod1_depth > p->delay_length)
				{
					p->mod1_depth = p->delay_length;
				}

				if(p->mod1_rate > 1000)
				{
					p->mod1_rate = 1000;
				}
				if(p->mod2_depth > p->delay_length)
				{
					p->mod2_depth = p->delay_length;
				}

				if(p->mod2_rate > 1000)
				{
					p->mod2_rate = 1000;
				}

				memcpy(&TmpData16, &buf[11], 2);
				if(p->dry > 100)
				{
					p->dry = 100;
				}

				if(p->wet_1 > 100)
				{
					p->wet_1 = 100;
				}
				if(p->wet_2 > 100)
				{
					p->wet_2 = 100;
				}

				if(p->enable)
				{
					AudioEffectChorus2Init(p, gCtrlVars.sample_rate,p->bit_width);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&Chorus2Tab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_CHORUS_EN
}

/*********************************************
 *
 *
 * DCBlocker audio class,describe tab
 *
 *
 *********************************************/
const DCBlockerDescribe DCBlockerTab=
{
  {1},
#if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
  {continueType, 0x0000,0x001E,0x0001,0x0001,0x0000,0x0010,"reserve",   "xx"},
#endif
};

void Communication_Effect_DCBlocker(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_DC_BLOCKER_EN
	DCBlockerUnit *p = (DCBlockerUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)DCBlockerTab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectDCBlockerInit(p,p->channel);
						if(p->enable)
						{
							IsEffectChange = 1;
						}
					}
					else
					{
						IsEffectChange = 1;
					}
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2*2);
				if(p->enable)
				{
					AudioEffectDCBlockerInit(p,p->channel);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&DCBlockerTab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_DC_BLOCKER_EN
}

/*********************************************
 *
 *
 * VBSurround audio class,describe tab
 *
 *
 *********************************************/
const VirtualSurroundDescribe VBSurroundTab=
{
  {1},
#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
  {continueType, 0x0000,0x001E,0x0001,0x0001,0x0000,0x0010,"reserve",   "xx"},
#endif
};

void Communication_Effect_VBSurround(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_VIRTUAL_SURROUND_EN
	VirtualSurroundUnit *p = (VirtualSurroundUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)VBSurroundTab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectVirtualSurroundInit(p,p->channel,p->SampleRate);
						if(p->enable)
						{
							IsEffectChange = 1;
						}
					}
					else
					{
						IsEffectChange = 1;
					}
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2*2);
				if(p->enable)
				{
					AudioEffectVirtualSurroundInit(p,p->channel,p->SampleRate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&VBSurroundTab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_VB_SURROUND_EN
}
/*********************************************
 *
 *
 * ButterWorth audio class,describe tab
 *
 *
 *********************************************/
const ButterWorthDescribe ButterWorthTab=
{
  {3},
#if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
  {enumType,     "Low Pass;Hight Pass",0x0000,             "filter_type"         },//0:Low-pass, 1:High-pass
  {continueType, 0x0001,0x000A,0x0001,0x0001,0x0000,0x0001,"filter_order",   " "},//1~10
  {continueType, 0x0014,0x5DC0,0x0001,0x0001,0x0000,0x0032,"Cut Freq",       "HZ"},//
#endif
};

void Communication_Effect_ButterWorth(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_BUTTERWORTH_EN
	ButterWorthUnit *p = (ButterWorthUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)ButterWorthTab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectButterWorthInit(p,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
					}
				   IsEffectChange = 1;
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->filter_type == TmpData16)
				{
					break;
				}
				p->filter_type = TmpData16 &0x01;

				if(p->enable)
				{
					AudioEffectButterWorthInit(p,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->filter_order == TmpData16)//ButterWorthTab
				{
					break;
				}
				if(TmpData16 < 1)  TmpData16 = 1;
				if(TmpData16 > 10)  TmpData16 = 10;

				p->filter_order = TmpData16;


				if(p->enable)
				{
					AudioEffectButterWorthInit(p,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;
			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->fc == TmpData16)
				{
					break;
				}
				if(TmpData16 < 20)   TmpData16 = 20;//ButterWorthTab
				if(TmpData16 > 24000)  TmpData16 = 24000;
				p->fc = TmpData16;
				if(p->enable)
				{
					AudioEffectButterWorthInit(p,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);

				memcpy((uint8_t *)&TmpData16,  &buf[3], 2);
				p->filter_type =      TmpData16 &0x01;

				memcpy((uint8_t *)&TmpData16,  &buf[5], 2);
				if(TmpData16 < 1)      TmpData16 = 1;
				if(TmpData16 > 10)     TmpData16 = 10;
				p->filter_order = TmpData16;

				memcpy((uint8_t *)&TmpData16,  &buf[7], 2);
				if(TmpData16 < 20)     TmpData16 = 20;//ButterWorthTab
				if(TmpData16 > 24000)  TmpData16 = 24000;//hz
				p->fc = TmpData16;

				if(p->enable)
				{
					AudioEffectButterWorthInit(p,gCtrlVars.adc_line_channel_num,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&ButterWorthTab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_BUTTERWORTH_EN
}

/*********************************************
 *
 *
 * Dynamic Eq  audio class,describe tab
 *
 *
 *********************************************/
const DynamicEqDescribe DynamicEqDescribeTab=
{
  {5},
#if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
  // ID           Min     Max   Step   ratio fraction    default   name                  unit
  {continueType, 0xDCD8,0x0000,0x0001,0x0064,0x0002,     0x0000,   "Low_threshold",      "DB"},// -9000 ~ 0 to cover -90.00dB ~ 0.00dB
  {continueType, 0xDCD8,0x0000,0x0001,0x0064,0x0002,     0x0000,   "Normal_threshold",   "DB"},// -9000 ~ 0 to cover -90.00dB ~ 0.00dB
  {continueType, 0xDCD8,0x0000,0x0001,0x0064,0x0002,     0x0000,   "Hight_threshold",    "DB"},// -9000 ~ 0 to cover -90.00dB ~ 0.00dB
  {continueType, 0x0000,0x07D0,0x0001,0x0001,0x0000,     0x0064,   "Attack",             "ms"},//1~10
  {continueType, 0x0000,0x07D0,0x0001,0x0001,0x0000,     0x03E8,   "Release",            "ms"},//
#endif
};

void Communication_Effect_DynamicEQ(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
	DynamicEqUnit *p = (DynamicEqUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)DynamicEqDescribeTab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectDynamicEqInit(p,p->channel,gCtrlVars.sample_rate);
					}
				   IsEffectChange = 1;
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->low_energy_threshold == TmpData16)
				{
					break;
				}
				if(TmpData16 > p->normal_energy_threshold)
				{
					break;
				}
				p->low_energy_threshold = TmpData16;

				if(p->enable)
				{
					AudioEffectDynamicEqInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;
			case 2:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->normal_energy_threshold == TmpData16)
				{
					break;
				}
				if(TmpData16 > p->high_energy_threshold)
				{
					break;
				}
				if(TmpData16 < p->low_energy_threshold)
				{
					break;
				}

				p->normal_energy_threshold = TmpData16;

				if(p->enable)
				{
					AudioEffectDynamicEqInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;
			case 3:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->high_energy_threshold == TmpData16)
				{
					break;
				}
				if(TmpData16 < p->normal_energy_threshold)
				{
					break;
				}
				p->high_energy_threshold = TmpData16;

				if(p->enable)
				{
					AudioEffectDynamicEqInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;
			case 4:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->attack_time == TmpData16)
				{
					break;
				}
				p->attack_time = TmpData16;

				if(p->enable)
				{
					AudioEffectDynamicEqInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;
			case 5:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->release_time == TmpData16)
				{
					break;
				}
				p->release_time = TmpData16;

				if(p->enable)
				{
					AudioEffectDynamicEqInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;
			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);

				memcpy(&TmpData16, &buf[7], 2);
				p->high_energy_threshold = TmpData16;

				memcpy(&TmpData16, &buf[5], 2);
				p->normal_energy_threshold = TmpData16;

				if(p->normal_energy_threshold > p->high_energy_threshold)
				{
					p->normal_energy_threshold--;
				}

				memcpy(&TmpData16, &buf[3], 2);
				p->low_energy_threshold = TmpData16;

				if(p->low_energy_threshold > p->normal_energy_threshold)
				{
					p->low_energy_threshold--;
				}

				memcpy(&TmpData16, &buf[9], 2);
				p->attack_time = TmpData16;

				memcpy(&TmpData16, &buf[11], 2);
				p->release_time = TmpData16;

				if(p->enable)
				{
					AudioEffectDynamicEqInit(p,p->channel,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&DynamicEqDescribeTab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_DYNAMIC_EQ_EN
}
/*********************************************
 *
 *
 * lr balancer audio class,describe tab
 *
 *
 *********************************************/
const LRBalancerDescribe LRBalancerTab=
{
  {1},
#if CFG_AUDIO_EFFECT_LRBALANCER_EN
  {continueType, 0xff9c,0x0064,0x0000,0x0001,0x0000,0x0000,"LR %", " "},//
#endif
};

void Communication_Effect_LRBalancer(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_LRBALANCER_EN
	LRBalancerUnit *p = (LRBalancerUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)LRBalancerTab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{
		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectLRBalancerInit(p,p->channel);
					}
					IsEffectChange = 1;
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
				p->balance = TmpData16;
				break;

			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2);

				memcpy((uint8_t *)&TmpData16,  &buf[3], 2);
				p->balance                     =  TmpData16;
				if(p->balance>100)  p->balance =  100;
				if(p->balance<-100) p->balance = -100;

				if(p->enable)
				{
					AudioEffectLRBalancerInit(p,p->channel);
					IsEffectChange = 1;
				}
				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&LRBalancerTab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_LRBALANCER_EN
}
/*********************************************
 *
 *
 * HowlingSpecified audio class,describe tab
 *
 *
 *********************************************/

const HowlingSpecifiedDescribe HolingSpecifiedTab=
{
  {13},
#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
  {continueType, 0x0000,0x0006,0x0001,0x0001,0x0000,0x0000,"FilterNums", " "},//num_specified_filters 0~6 = MAX_SPECIFIED_FILTERS

  {continueType, 0x0002,0x4E20,0x0001,0x0001,0x0000,0x03E8,"f1", "HZ"},//center_freq1;Range: 2 ~ sample_rate/2-2 Hz.
  {continueType, 0x0002,0x4E20,0x0001,0x0001,0x0000,0x03E8,"f2", "HZ"},//
  {continueType, 0x0002,0x4E20,0x0001,0x0001,0x0000,0x03E8,"f3", "HZ"},//
  {continueType, 0x0002,0x4E20,0x0001,0x0001,0x0000,0x03E8,"f4", "HZ"},//
  {continueType, 0x0002,0x4E20,0x0001,0x0001,0x0000,0x03E8,"f5", "HZ"},//
  {continueType, 0x0002,0x4E20,0x0001,0x0001,0x0000,0x03E8,"f6", "HZ"},//

  {continueType, 0x0001,0x7FFF,0x0001,0x0040,0x0002,0x002D,"Q1", " "},//center_freq1;Range: 2 ~ sample_rate/2-2 Hz.
  {continueType, 0x0001,0x7FFF,0x0001,0x0040,0x0002,0x002D,"Q2", " "},//
  {continueType, 0x0001,0x7FFF,0x0001,0x0040,0x0002,0x002D,"Q3", " "},//
  {continueType, 0x0001,0x7FFF,0x0001,0x0040,0x0002,0x002D,"Q4", " "},//
  {continueType, 0x0001,0x7FFF,0x0001,0x0040,0x0002,0x002D,"Q5", " "},//
  {continueType, 0x0001,0x7FFF,0x0001,0x0040,0x0002,0x002D,"Q6", " "},//

//  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"D1", "DB"},////Range: -100 ~ 0 dB.
//  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"D2", "DB"},////Range: -100 ~ 0 dB.
//  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"D3", "DB"},////Range: -100 ~ 0 dB.
//  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"D4", "DB"},////Range: -100 ~ 0 dB.
//  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"D5", "DB"},////Range: -100 ~ 0 dB.
//  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"D6", "DB"},////Range: -100 ~ 0 dB.
#endif
};

void Communication_Effect_HowlingSpecified(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
	uint8_t cnt = 0;
	HowlingSpecifiedUnit *p = (HowlingSpecifiedUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)HolingSpecifiedTab.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2*parameter_len);

		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{

		switch(buf[0])//
		{
			case 0:
				memcpy(&TmpData16, &buf[1], 2);
				if(p->enable != TmpData16)
				{
					p->enable = TmpData16;
					if(p->enable)
					{
						AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
					}
					IsEffectChange = 1;
				}
				break;
			case 1:
				memcpy(&TmpData16, &buf[1], 2);
                if(TmpData16 != p->num_specified_filters)
                {
                	if(TmpData16 < 7)
                	{
                		p->num_specified_filters = TmpData16;
    					if(p->enable)
    					{
    						AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
    					}
                	}
                }
				break;
				///
			case 2://center_freq1
			case 3:
			case 4:
			case 5:
			case 6:
			case 7://center_freq
				cnt = buf[0] -2;
				memcpy(&TmpData16, &buf[1], 2);
                if(TmpData16 != p->center_freq[cnt])
                {
                	p->center_freq[cnt] = TmpData16;
					if(p->enable)
					{
						AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
					}
                }
				break;
			case 8://q1
			case 9://q2
			case 10://q3
			case 11://q4
			case 12://q5
			case 13://q6
				cnt = buf[0] -8;
				memcpy(&TmpData16, &buf[1], 2);
                if(TmpData16 != p->q[cnt])
                {
                	p->q[cnt]= TmpData16;
					if(p->enable)
					{
						AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
					}
                }
				break;
			case 14://q1
			case 15://q2
			case 16://q3
			case 17://q4
			case 18://q5
			case 19://q6
				cnt = buf[0] -14;
				memcpy(&TmpData16, &buf[1], 2);
                if(TmpData16 != p->depth[cnt])
                {
                	p->depth[cnt]= TmpData16;
					if(p->enable)
					{
						AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
					}
                }
				break;
			case 0xff:
				memcpy((uint8_t *)&p->enable, &buf[1], 2*parameter_len);

				if(p->enable)
				{
					AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}

				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&HolingSpecifiedTab);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
}
const HowlingSpecifiedDescribe HolingSpecifiedTab_1=
{
  {6},
#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"Depth1", "DB"},////Range: -100 ~ 0 dB.
  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"Depth2", "DB"},////Range: -100 ~ 0 dB.
  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"Depth3", "DB"},////Range: -100 ~ 0 dB.
  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"Depth4", "DB"},////Range: -100 ~ 0 dB.
  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"Depth5", "DB"},////Range: -100 ~ 0 dB.
  {continueType, 0xff9c,0x0000,0x0001,0x0001,0x0000,0x0000,"Depth6", "DB"},////Range: -100 ~ 0 dB.
#endif
};
void Communication_Effect_HowlingSpecified_1(uint8_t Control, uint32_t addr, uint8_t *buf, uint32_t len)
{
#if CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
	uint8_t cnt = 0;
	HowlingSpecifiedUnit *p = (HowlingSpecifiedUnit *)addr;
	int16_t TmpData16;
	int16_t parameter_len = (int16_t)HolingSpecifiedTab_1.numbers.paramete_totals + 1;//+ 1 = enable

	memset(tx_buf, 0, sizeof(tx_buf));

	if(len == 0)//ask
	{
		tx_buf[0] = 0xa5;
		tx_buf[1] = 0x5a;
		tx_buf[2] = Control;
		tx_buf[3] = 2 * parameter_len + 1;
		tx_buf[4] = 0xff;

		memcpy(&tx_buf[5], &p->enable, 2);

		parameter_len -=1;
		memcpy(&tx_buf[7], &p->depth, 2*parameter_len);


		parameter_len +=1;
		tx_buf[5 + 2 * parameter_len] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 2 * parameter_len);
	}
	else
	{

		switch(buf[0])//
		{
			case 0:
				break;
			case 1://q1
			case 2://q2
			case 3://q3
			case 4://q4
			case 5://q5
			case 6://q6
				cnt = buf[0] -1;
				memcpy(&TmpData16, &buf[1], 2);
                if(TmpData16 != p->depth[cnt])
                {
                	p->depth[cnt]= TmpData16;
					if(p->enable)
					{
						AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
					}
                }
				break;
			case 0xff:
				//memcpy((uint8_t *)&p->enable, &buf[1], 2*parameter_len);

				parameter_len -=1;
				memcpy(&p->depth, &buf[3],2*parameter_len);
				if(p->enable)
				{
					AudioEffectHowlingSuppressorSpecifieInit(p,gCtrlVars.sample_rate);
					IsEffectChange = 1;
				}

				break;

			case 0xf0:
				AudioEffectClassDescParser(Control,&HolingSpecifiedTab_1);
				break;
			default:
				break;
		}
	}
#endif //CFG_AUDIO_EFFECT_MIC_HOWLING_SPECIFIED_EN
}
/*
 *  AudioEffectTypeDescParser
 *  buf = tx_buf, pdata=audio effect describe
 *  return data_len
 */
uint8_t AudioEffectClassDescParser(uint8_t Control,const void *pdata)
{
	    uint8_t len ,slen,i;
	    uint8_t pos = 0,m;
	    int32_t *dptr,data_type;
	    TypeBool      *p1;
	    TypeEnum      *p2;
	    TypeContinue  *p3;
	    TypeDisp      *p4;

	    uint8_t *buf = tx_buf;

	    buf[pos++] = 0xa5;
	    buf[pos++] = 0x5a;
	    buf[pos++] = Control;
	    buf[pos++] = 0;//len  //0x03
	    buf[pos++] = 0xf0;

	    dptr =  (int32_t *)pdata;
	    data_type = *dptr;
        dptr += ParameterTotasSize;

        len = data_type&0xff;
        buf[pos++] = len;//parameter numbers;
        //-------------------------------//
        for(i = 0; i < len;i++)
        {
			//----------------------------------------//
			p1 = (TypeBool *)dptr;
			p2 = (TypeEnum *)dptr;
			p3 = (TypeContinue *)dptr;
			p4 = (TypeDisp *)dptr;
			data_type = p1->data_type;
			//----------------------------------------//
			if(data_type==boolType)//bool
			{
			   dptr += LogicSize;
			   slen=0;
			   m = pos;
			   pos++;
			   strcpy((char *)&buf[pos],(char *)p1->name);
			   slen = strlen(p1->name);
			   buf[m] = slen;
			   pos += slen;
			   memcpy(&buf[pos],&p1->data_type,1);
			   pos += 1;

			   memcpy(&buf[pos],&p1->defualt,2);
			   pos += 2;
			   //DBG("P1:%d->%08x  %08x\n",slen,p1->data_type,p1->enable);

			}

			if(data_type==enumType)//enum
			{
			   dptr += EnumSize;
			   //-----name--------------------//
			   slen=0;
			   m = pos;
			   pos++;
			   strcpy((char *)&buf[pos],(char *)p2->name);
			   slen = strlen(p2->name);
			   buf[m] = slen;
			   pos += slen;
			   memcpy(&buf[pos],&p2->data_type,1);
			   pos += 1;
			   //-----select name-----------------//
			   slen=0;
			   m = pos;
			   pos++;
			   strcpy((char *)&buf[pos],(char *)p2->enum_name);
			   slen = strlen(p2->enum_name);
			   buf[m] = slen;
			   pos += slen;
			   //------------------------------//
			   memcpy(&buf[pos],&p2->defualt,2);
			   pos += 2;
			   //DBG("P1:%d->%08x  %08x  %08x  %08x  %s  %s\n",slen,p2->data_type,p2->defualt,p2->name,p2->str);
			}

			if(data_type==continueType)//continue
			{
				 dptr += ContinueSize;
				 slen=0;
				 m = pos;
				 pos++;
				 strcpy((char *)&buf[pos],(char *)p3->name);
				 slen = strlen(p3->name);
				 buf[m] = slen;
				 pos += slen;
				 memcpy(&buf[pos],&p3->data_type,1);
				 pos += 1;

				 memcpy(&buf[pos],&p3->min,2);
				 pos += 2;

				 memcpy(&buf[pos],&p3->max,2);
				 pos += 2;

				 memcpy(&buf[pos],&p3->step,2);
				 pos += 2;

				 memcpy(&buf[pos],&p3->ratio,2);
				 pos += 2;

				 memcpy(&buf[pos],&p3->fraction,2);
				 pos += 2;

				 memcpy(&buf[pos],&p3->defualt,2);
				 pos += 2;
				 //--unit-------------//
				 if(p3->unit==0)
				 {
					 buf[pos++] = 0;//len = 0
				 }
				 else if(*p3->unit==0x0)//unit len = 0
				 {
					 buf[pos++] = 0;//len = 0
				 }
				 else if(*p3->unit==' ')//unit len = 0
				 {
					 buf[pos++] = 0;//len = 0
				 }
				 else
				 {
				   m = pos;
				   pos++;
				   strcpy((char *)&buf[pos],(char *)p3->unit);
				   slen = strlen(p3->unit);
				   buf[m] = slen;
				   pos += slen;
				 }

				//DBG("P1:%d->%08x  %08x  %08x  %08x  %s\n",slen,p3->data_type,p3->min,p3->max,p3->defualt,p3->name);
			}

			if(data_type==dispType)//disp
			{
			   dptr += DispSize;
			   slen=0;
			   m = pos;
			   pos++;

			   strcpy((char *)&buf[pos],(char *)p4->name);
			   slen = strlen(p4->name);
			   buf[m] = slen;
			   pos += slen;
			   memcpy(&buf[pos],&p4->data_type,1);
			   pos += 1;

			   memcpy(&buf[pos],&p4->min,2);
			   pos += 2;

			   memcpy(&buf[pos],&p4->max,2);
			   pos += 2;

			   memcpy(&buf[pos],&p4->ratio,2);
			   pos += 2;

			   memcpy(&buf[pos],&p4->fraction,2);
			   pos += 2;

			   memcpy(&buf[pos],&p4->defualt,2);
			   pos += 2;
			   //--unit-------------//
			   m = pos;
			   pos++;
			   strcpy((char *)&buf[pos],(char *)p4->unit);
			   slen = strlen(p4->unit);
			   buf[m] = slen;
			   pos += slen;
			   //DBG("P1:%d->%08x  %08x\n",slen,p4->data_type,p4->defualt);
			}
			//----------------------------------//
			if(pos>254)
			{
				pos = 4;
			}
        }
        buf[pos] = 0x16;//end
        pos -= 4;
        buf[3] = pos;//len

    	if(pos>250) {
    		DBG("HolingSpecified >250\n");
    	}
        Communication_Effect_Send(buf, pos+1);
	    return pos;
}
//----------------//
