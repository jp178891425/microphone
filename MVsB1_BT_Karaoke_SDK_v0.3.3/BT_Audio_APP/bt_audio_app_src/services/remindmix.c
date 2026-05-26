/*
 * remindmix.c
 *
 *  Created on: Apr 13, 2020
 *      Author: szsj-1
 */

#include <string.h>
#include "app_config.h"

#ifdef CFG_FUNC_REMIND_MIX_MODE

#include "rtos_api.h"
#include "app_message.h"
#include "type.h"
#include "spi_flash.h"
#include "debug.h"
#include "audio_utility.h"
#include "remind_sound_service.h"
#include "audio_core_api.h"
#include "decoder_service.h"
#include "device_service.h"
#include "audio_core_service.h"
#include "mcu_circular_buf.h"
#include "mp2.h"
#include "mp2dec.h"
#include "main_task.h"
#include "dac_interface.h"
#include "watchdog.h"
#include "timeout.h"

#define SBC_DEC			1
#define MP2_DEC			2
#define DECODER_TPYE	MP2_DEC//SBC_DEC
int16_t             Source2Buf_MixRemind[512*2];

/**************************************************************************************
 *
 *
 *
 **************************************************************************************
 */
#define MIX_ONE_SAMPLE	                                1152
#define FIFO_SAMPLE_COUNT	                            3
#define MIX_REMIND_SOUND_SERVICE_NUM_MESSAGE_QUEUE		8
#define	MIX_REMIND_SOUND_ID_LEN			                sizeof(((MIX_SongClipsEntry *)0)->id)

#define MIX_REMIND_SOUND_SERVICE_TASK_STACK_SIZE		384//1024
#define MIX_REMIND_SOUND_SERVICE_TASK_PRIO				3

#define MIX_REMIND_FLASH_MAX_NUM		                255 //flash提示音区配置决定
#define MIX_REMIND_FLASH_HDR_SIZE		                0x1000 //提示音条目信息区大小

#define MIX_REMIND_FLASH_READ_TIMEOUT 	                100
#define MIX_REMIND_FLASH_ADDR(n) 		               (REMIND_FLASH_STORE_BASE + sizeof(MIX_SongClipsEntry) * n + sizeof(MIX_SongClipsHdr))//flash提示音区配置决定

#define MIX_REMIND_FLASH_FLAG_STR		                ("MVUB")
#define MIX_REMIND_BLOCK_PLAYING_BIT					0x01	//正在播放的提示音阻塞登记位
#define MIX_REMIND_BLOCK_REQUEST_BIT					0x02	//请求播放的提示音阻塞登记位

#define MIX_REMIND_SOUND_SERVICE_AUDIO_DECODER_IN_BUF_SIZE	1024 * 19

#pragma pack(1)
typedef struct __SongClipsHdr
{
	char sync[4];
	uint32_t crc;
	uint8_t cnt;
} MIX_SongClipsHdr;
#pragma pack()

#pragma pack(1)
typedef struct __SongClipsEntry
{
	uint8_t id[8];
	uint32_t offset;
	uint32_t size;
} MIX_SongClipsEntry;
#pragma pack()

typedef enum __REMIND_SOUND_STATE
{
	REMIND_STANDBY,
	REMIND_WAIT_DECODER,
	REMIND_PLAY,
	REMIND_WAIT_RENEW,
	REMIND_STOPPING,
	REMIND_GIVE_DECODER,
} MIX_REMINDSOUNDSTATE;

/*
 * ************************************************************************************
 *
 *
 **************************************************************************************
 */
typedef struct __RemindSoundServiceContext
{
	xTaskHandle 		taskHandle;
	MessageHandle		msgHandle;
	MessageHandle		parentMsgHandle;
	TaskState			RemindSoundServiceState;

	MemHandle			RemindMemHandle;

	uint32_t 			ConstDataAddr;
	uint32_t			ConstDataSize;
	uint32_t 			ConstDataOffset;

	uint8_t				RequestRemind[MIX_REMIND_SOUND_ID_LEN];
	int16_t             Source2Buf_MixRemind[512*2];
	MIX_REMINDSOUNDSTATE	RemindState;
//	uint32_t			PlayDataCount;
	uint8_t			    IsBlock; //1： 提示音独占播放; 0：提示音混合其它音源一起播放
	//---------------------------------------------//

	uint32_t            MP2FramSize;
	uint32_t            MP2FrameLen;
	uint32_t            Remindaddr;
	uint32_t            RemindFileSize;
	uint16_t            MP2DecoderSta;

}RemindSoundServiceContextX;


static const unsigned short CrcCCITTTableX[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
//-----------------------------------
#if (DECODER_TPYE == MP2_DEC)
uint8_t Mix_Remind_decoder_buf[MIX_ONE_SAMPLE*2*MPA_MAX_CHANNELS*FIFO_SAMPLE_COUNT];
MPADecodeContext  mp3_cnt;
uint8_t Mix_Remind_temp_buf[MIX_ONE_SAMPLE*2*MPA_MAX_CHANNELS];
uint8_t decoder_mem[626];
#endif

#if (DECODER_TPYE == SBC_DEC)
uint8_t Mix_Remind_decoder_buf[16*120];//16*128sample
uint8_t sbc_cnt[1624];
uint8_t file_buf[120];
uint8_t decoder_mem[120];
uint32_t frame_len = 0;
uint32_t channel = 0;
#endif

MCU_CIRCULAR_CONTEXT RemindMixCircularBuf;
RemindSoundServiceContextX		RemindMixCt;
//osMutexId RemindMixMutex = NULL;


const char RemindMixServiceName[] = "RemindSoundService";/** remind sound task name*/

void RemindMixDecoder(void);
uint16_t AudioRemindMixDataGet(void* Buf, uint16_t Len);
uint16_t AudioRemindMixDataLenGet(void);
void MP2_decode_init(void* MemAddr);
int32_t RemindMixServiceInit(MessageHandle parentMsgHandle);
uint8_t MixRemindErrorProcess(void);
//----------------------------------
unsigned short CRC16X(unsigned char *Buf, unsigned int BufLen, unsigned short CRC)
{
	unsigned int i;
	for(i = 0 ; i < BufLen; i++)
	{
		CRC = (CRC << 8) ^ CrcCCITTTableX[((CRC >> 8) ^ *Buf++) & 0x00FF];
	}
	return CRC;
}

//提示音条目和数据区完整性校验，影响开机速度。
bool sound_clips_all_crcX(void)
{
	MIX_SongClipsHdr *hdr;
	MIX_SongClipsEntry *ptr;
	uint16_t crc=0, i, j, CrcRead;
	uint32_t FlashAddr, all_len = 0;
	uint8_t *data_ptr = NULL;
	//uint8_t *data_ptr1 = NULL;// bkd add for test 2019.4.22

	bool ret = TRUE;
	FlashAddr = REMIND_FLASH_STORE_BASE;

	if(FlashAddr > CHIP_FLASH_SIZE)
	{
		DBG("remind addr > chip flash size\n");
		return FALSE;
	}
	data_ptr = (uint8_t *)osPortMalloc(MIX_REMIND_FLASH_HDR_SIZE);
	//data_ptr1=data_ptr;
	if(data_ptr == NULL)
	{
		return FALSE;
	}
	if(SpiFlashRead(FlashAddr,
					data_ptr,
					MIX_REMIND_FLASH_HDR_SIZE,
					MIX_REMIND_FLASH_READ_TIMEOUT) != FLASH_NONE_ERR)
	{
		DBG("mix read const data error!\r\n");
		ret = FALSE;
	}
	else
	{
		ptr = (MIX_SongClipsEntry*)(data_ptr + sizeof(MIX_SongClipsHdr));
		hdr = (MIX_SongClipsHdr *)(data_ptr);
		if(strncmp(hdr->sync, "MVUB", 4) || !hdr->cnt)
		{
			DBG("mix sync not found or no Item\n");
			ret = FALSE;
		}
		else
		{
			for(i = 0; i < hdr->cnt; i++)
			{
				all_len += ptr[i].size;
				for(j = 0; j < MIX_REMIND_SOUND_ID_LEN; j++)
				{
					DBG("%c", ((uint8_t *)&ptr[i].id)[j]);
				}
				DBG("\n");
			}
			DBG("\n mix ALL clips size = %d\n", (int)all_len);
			if(REMIND_FLASH_STORE_BASE + MIX_REMIND_FLASH_HDR_SIZE + all_len >= REMIND_FLASH_STORE_OVERFLOW)
			{
				DBG("mix Remind flash const data overflow.\n");
				ret = FALSE;
			}
			CrcRead = hdr->crc;
			crc = CRC16X(data_ptr, 4, crc);
			crc = CRC16X(data_ptr + 8, MIX_REMIND_FLASH_HDR_SIZE - 8, crc);
			FlashAddr += MIX_REMIND_FLASH_HDR_SIZE;
			while(all_len && ret)
			{
				if(all_len > MIX_REMIND_FLASH_HDR_SIZE)
				{
					i = MIX_REMIND_FLASH_HDR_SIZE;
				}
				else
				{
					i = all_len;
				}
				if(SpiFlashRead(FlashAddr,
								data_ptr,
								i,
								MIX_REMIND_FLASH_READ_TIMEOUT) != FLASH_NONE_ERR)
				{
					DBG("mix read const data error!\r\n");
					ret = FALSE;
				}
				else
				{
					crc = CRC16X(data_ptr, i, crc);
					FlashAddr += i;
					all_len -= i;
				}
			}
			if(crc == CrcRead)
			{
				DBG("Crc = %04X\n", crc);
			}
			else
			{
				DBG("Crc error: %04X != %04X\n", crc, CrcRead);
				ret = FALSE;
			}
		}
	}
	osPortFree(data_ptr);
	return ret;
}

//根据flash驱动设计，最大支持255条提示音。
bool RemindMixServiceReadItemInfo(uint8_t *RemindItem)
{
	uint16_t j;
	MIX_SongClipsEntry SongClips;

	//查找对应的ConstDataId
	for(j = 0; j < SOUND_REMIND_TOTAL; j++)
	{
		if(FLASH_NONE_ERR != SpiFlashRead(MIX_REMIND_FLASH_ADDR(j), (uint8_t *)&SongClips, sizeof(MIX_SongClipsEntry), MIX_REMIND_FLASH_READ_TIMEOUT))
		{
			return FALSE;
		}
		if(memcmp(&SongClips.id,RemindItem, sizeof(SongClips.id)) == 0)//找到
		{
			RemindMixCt.ConstDataOffset = 0;
			RemindMixCt.ConstDataAddr = SongClips.offset + REMIND_FLASH_STORE_BASE; //工具制作提示音bin 使用相对地址
			RemindMixCt.ConstDataSize = SongClips.size;
			DBG("play remind sound : ");
			for(j=0;j<sizeof(SongClips.id);j++)
				DBG("%c",RemindItem[j]);
			DBG("\n");
			return TRUE;
		}
	}

	return FALSE;
}
////-------------------------------------------//
void RemindMixTaskInit(void)
{
	DBG("%s %u\n",__FILE__,__LINE__);
	RemindMixServiceInit(mainAppCt.msgHandle);
}
/*
 * ************************************************************************************
 *
 *
 **************************************************************************************
 */
void RemindMixCtServiceEntrance(void * param)
{
	MessageContext msg;
	while(1)
	{
		WDG_Feed();
		RemindMixDecoder();
		if(MixRemindErrorProcess())
		{
			MessageRecv(RemindMixCt.msgHandle, &msg, 10);
		}
		else
		{
			MessageRecv(RemindMixCt.msgHandle, &msg, 2);
		}
		switch(msg.msgId)
		{
			case 0x00:
				break;
			default:
				break;
		}
	}
}
/*
 * ************************************************************************************
 *
 *
 **************************************************************************************
 */
int32_t RemindMixServiceInit(MessageHandle parentMsgHandle)
{
	uint32_t Size;
	memset(&RemindMixCt, 0, sizeof(RemindSoundServiceContextX));
//下列const data安全检查至少要开启项。
	if(!sound_clips_all_crcX())
	{
		return 0;
	}
	/* message handle */
	RemindMixCt.msgHandle = MessageRegister(MIX_REMIND_SOUND_SERVICE_NUM_MESSAGE_QUEUE);
	/* Parent message handle */
	RemindMixCt.parentMsgHandle = parentMsgHandle;
	RemindMixCt.RemindSoundServiceState = TaskStateCreating;
	RemindMixCt.RemindMemHandle.addr = NULL;//启用Callback后 实际此结构体未被解码器使用。保留api参数。
	RemindMixCt.RemindMemHandle.mem_capacity = 0;
	RemindMixCt.RemindMemHandle.mem_len = 0;
	RemindMixCt.RemindMemHandle.p = 0;
	RemindMixCt.RemindState = REMIND_STANDBY;
	RemindMixCt.RequestRemind[0] = 0;
	RemindMixCt.IsBlock = 0;
#if (DECODER_TPYE == MP2_DEC)
	Size = sizeof(Mix_Remind_decoder_buf);
	Size += sizeof(mp3_cnt);
	Size += sizeof(Mix_Remind_temp_buf);
	Size += sizeof( decoder_mem[626]);
#elif(DECODER_TPYE == SBC_DEC)
	Size += sizeof(Mix_Remind_decoder_buf);
	Size += sizeof(sbc_cnt);
	Size += sizeof(decoder_mem);
	Size += sizeof(file_buf);
	Size += 8;
#endif
	DBG("MixRemind RAM SIZE:%ld\n",Size);

	//if(RemindMixMutex == NULL)
	//{
		//RemindMixMutex = osMutexCreate();
	//}
    //---------------------------------------------------------------------------//
//	xTaskCreate(RemindMixCtServiceEntrance, "RemindMix", 256, NULL, 3, RemindMixCt.msgHandle);
	xTaskCreate(RemindMixCtServiceEntrance, "RemindMix", MIX_REMIND_SOUND_SERVICE_TASK_STACK_SIZE, NULL, MIX_REMIND_SOUND_SERVICE_TASK_PRIO, RemindMixCt.msgHandle);

	return 0;
}
/*
 * ************************************************************************************
 *
 *
 **************************************************************************************
 */
uint8_t MixRemindErrorProcess(void)
{
    uint8_t err;
    err = 0;
	if(mainAppCt.Source2Buf_MixRemind==NULL)
	{
		DBG("MIX Remind Buff Err\n");
		err = 1;
	}
//	if(GetSystemMode() == AppModeBtHfPlay)///wait.. audio core run
//	{
//	  if(RemindMixCt.MP2DecoderSta != 0)
//	  {
//		 // return 1;
//	  }
//	}

	if(GetSystemMode() < AppModeRestPlay)///wait.. audio core run
	{
	   if(GetAudioCoreServiceState() != TaskStateRunning)
	  {
		  DBG("AudioCore Paused 1\n");
		  AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
		  err = 0;
	   }
	}
	if(err)
	{
		AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
#if (DECODER_TPYE == MP2_DEC)
		RemindMixCt.MP2DecoderSta = 0;
#endif
	}
	return err;
}

void RemindMixServicePlayEnd(void)
{
	AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
}

void RemindMixServicePlayInit(void)
{
	if(RemindMixCt.MP2DecoderSta == 2)
	{
		AudioCoreSourceMute(MIX_REMIND_SOURCE_NUM, TRUE, TRUE);
		osTaskDelay(10);
	}
	AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
	AudioCoreSourceUnmute(MIX_REMIND_SOURCE_NUM, TRUE, TRUE);
	RemindMixCt.IsBlock = 1;
#if (DECODER_TPYE == MP2_DEC)
	RemindMixCt.MP2DecoderSta = 0;
	MP2_decode_init(&mp3_cnt);
	MCUCircular_Config(&RemindMixCircularBuf,Mix_Remind_decoder_buf,sizeof(Mix_Remind_decoder_buf));
	RemindMixCt.Remindaddr = RemindMixCt.ConstDataAddr;
	RemindMixCt.RemindFileSize = RemindMixCt.ConstDataSize;
#ifdef CFG_RES_EXTERN_FLASH_REMIND_EN
	if(FLASH_NONE_ERR != SPI_Flash_Read(RemindMixCt.Remindaddr, decoder_mem, 626))
#else
	if(FLASH_NONE_ERR != SpiFlashRead(RemindMixCt.Remindaddr, decoder_mem, 626, 1))
#endif
	{
		RemindMixCt.MP2DecoderSta = 0;
		RemindMixCt.IsBlock = 0;
		return;
	}
	if(MP2_decode_frame(Mix_Remind_temp_buf,decoder_mem) == FALSE)
	{
		RemindMixCt.MP2DecoderSta = 0;
		RemindMixCt.IsBlock = 0;
		return;
	}
	RemindMixCt.MP2FramSize = mp3_cnt.frame_size;
	RemindMixCt.MP2FrameLen = RemindMixCt.MP2FramSize;
	MCUCircular_PutData(&RemindMixCircularBuf,Mix_Remind_temp_buf,1152*2*mp3_cnt.nb_channels);
	//MCUCircular_PutData(&RemindMixCircularBuf,Mix_Remind_temp_buf,RemindMixCt.MP2FramSize*2*mp3_cnt.nb_channels);
	//DBG("MixRemind:sample_rate:%u\n  bit_rate:%u  channels:%u\n",mp3_cnt.sample_rate,mp3_cnt.bit_rate,mp3_cnt.nb_channels);
	RemindMixCt.MP2DecoderSta = 2;
	AudioCoreSourcePcmFormatConfig(MIX_REMIND_SOURCE_NUM,mp3_cnt.nb_channels);
	AudioCoreSourceEnable(MIX_REMIND_SOURCE_NUM);
#elif (DECODER_TPYE == SBC_DEC)
	sbc_decoder_init(sbc_cnt,2);
	RemindMixCt.MP2DecoderSta = 0;
	MCUCircular_Config(&RemindMixCircularBuf,Mix_Remind_decoder_buf,sizeof(Mix_Remind_decoder_buf));
	RemindMixCt.Remindaddr = RemindMixCt.ConstDataAddr;
	RemindMixCt.RemindFileSize = RemindMixCt.ConstDataSize;
	AudioCoreSourceEnable(MIX_REMIND_SOURCE_NUM);
#ifdef CFG_RES_EXTERN_FLASH_REMIND_EN
	if(FLASH_NONE_ERR != SPI_Flash_Read(RemindMixCt.Remindaddr, file_buf, 20))
#else
	if(FLASH_NONE_ERR != SpiFlashRead(RemindMixCt.Remindaddr, file_buf, 20, 1))
#endif
	{
		DBG("read flash error\n");
		RemindMixCt.MP2DecoderSta = 0;
		RemindMixCt.IsBlock = 0;
		return;
	}
	uint32_t frame_sample_rate = 0;
	if(sbc_fram_infor(sbc_cnt,file_buf,&frame_len,&frame_sample_rate,&channel) != 0)
	{
		DBG("get sbc info error\n");
		RemindMixCt.IsBlock = 0;
		return;
	}
	if((frame_sample_rate == 44100)&&(frame_len < 120))
	{
		if(channel == 1)
		{
			sbc_decoder_init(sbc_cnt,1);
		}
		else if(channel == 2)
		{
			sbc_decoder_init(sbc_cnt,2);
		}
		else
		{
			DBG("channel error\n");
			RemindMixCt.MP2DecoderSta = 0;
			RemindMixCt.IsBlock = 0;
			return;
		}
	}
	else
	{
		DBG("sample:%u\nfram len:%u\n",frame_sample_rate,frame_len);
		RemindMixCt.MP2DecoderSta = 0;
		RemindMixCt.IsBlock = 0;
		return;
	}
	if(FLASH_NONE_ERR != SpiFlashRead(RemindMixCt.Remindaddr+20, file_buf+20, frame_len-20, 1))
	{
		DBG("get sbc info error\n");
		RemindMixCt.MP2DecoderSta = 0;
		RemindMixCt.IsBlock = 0;
		return;
	}
	DBG("sample:%u\nfram len:%u\nchannel:%u\n",frame_sample_rate,frame_len,channel);
	RemindMixCt.MP2FrameLen = frame_len;
	MCUCircular_PutData(&RemindMixCircularBuf,file_buf,frame_len);
	AudioCoreSourcePcmFormatConfig(MIX_REMIND_SOURCE_NUM,channel);
	RemindMixCt.MP2DecoderSta = 2;
#endif
}

void RemindMixStop(void)
{
	RemindMixCt.MP2DecoderSta = 0;
	RemindMixCt.IsBlock = 0;
	AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
	DBG("RemindMixStop\n");
}

void RemindMixDecoder(void)
{
	if(MixRemindErrorProcess()) return;

#if (DECODER_TPYE == MP2_DEC)
	if(RemindMixCt.MP2DecoderSta==2)//开始播放
	{
		if(MCUCircular_GetSpaceLen(&RemindMixCircularBuf) >= MIX_ONE_SAMPLE*2*2*mp3_cnt.nb_channels)
		{
#ifdef CFG_RES_EXTERN_FLASH_REMIND_EN
			if(FLASH_NONE_ERR != SPI_Flash_Read(RemindMixCt.Remindaddr+RemindMixCt.MP2FrameLen, decoder_mem, RemindMixCt.MP2FramSize))
#else
			if(FLASH_NONE_ERR != SpiFlashRead(RemindMixCt.Remindaddr+RemindMixCt.MP2FrameLen, decoder_mem, RemindMixCt.MP2FramSize, 1))
#endif
			{
				RemindMixCt.MP2DecoderSta = 0;
				RemindMixCt.IsBlock = 0;
				AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
				return;
			}
			if(MP2_decode_frame(Mix_Remind_temp_buf,decoder_mem) == FALSE)
			{
#ifdef CFG_RES_EXTERN_FLASH_REMIND_EN
			if(FLASH_NONE_ERR != SPI_Flash_Read(Remindaddr+MP2FrameLen, decoder_mem, MP2FramSize+1))
#else
			if(FLASH_NONE_ERR != SpiFlashRead(RemindMixCt.Remindaddr+RemindMixCt.MP2FrameLen+1, decoder_mem, RemindMixCt.MP2FramSize, 1))
#endif
			{
				RemindMixCt.MP2DecoderSta = 0;
				RemindMixCt.IsBlock = 0;
				AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
				return;
			}
			if(MP2_decode_frame(Mix_Remind_temp_buf,decoder_mem) == FALSE)
			{
				RemindMixCt.MP2DecoderSta = 0;
				RemindMixCt.IsBlock = 0;
				AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
				return;
			}
			else
			{
				RemindMixCt.MP2FramSize = mp3_cnt.frame_size;
				RemindMixCt.MP2FrameLen += (RemindMixCt.MP2FramSize + 1);
			}
			}
			else
			{
				RemindMixCt.MP2FramSize = mp3_cnt.frame_size;
				RemindMixCt.MP2FrameLen += RemindMixCt.MP2FramSize;
			}

			MCUCircular_PutData(&RemindMixCircularBuf,Mix_Remind_temp_buf,MIX_ONE_SAMPLE*2*mp3_cnt.nb_channels);

			if(RemindMixCt.MP2FrameLen >= RemindMixCt.RemindFileSize)
			{
				RemindMixCt.MP2DecoderSta = 3;
				return;
			}
		}
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
		extern TIMER EffectChangeTimer;
		TimeOutSet(&EffectChangeTimer, 500);//临时修改方案，保证功能模式切换时，能快速解析调音参数，优化声音突变问题。
#endif
	}

	if(RemindMixCt.MP2DecoderSta==3)//开始播放//停止播放
	{
		if(MCUCircular_GetDataLen(&RemindMixCircularBuf) < mainAppCt.SamplesPreFrame * mp3_cnt.nb_channels*2)
		{
			AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
			RemindMixCt.MP2DecoderSta = 0;
			RemindMixCt.IsBlock = 0;
			//DBG("MixRemind:End\n");
			return;
		}
	}
#elif (DECODER_TPYE == SBC_DEC)
	if(RemindMixCt.MP2DecoderSta==2)//开始播放
	{
		//RemindMixCt.MP2DecoderSta = 2;
		//DBG("%u\n",MCUCircular_GetSpaceLen(&RemindMixCircularBuf));
		if(MCUCircular_GetSpaceLen(&RemindMixCircularBuf) >= frame_len)
		{
			//DBG("%08X\n",RemindMixCt.Remindaddr+RemindMixCt.MP2FrameLen);

#ifdef CFG_RES_EXTERN_FLASH_REMIND_EN
			if(FLASH_NONE_ERR != SPI_Flash_Read(Remindaddr+MP2FrameLen, file_buf, MP2FramSize+1))
#else
			if(FLASH_NONE_ERR != SpiFlashRead(RemindMixCt.Remindaddr+RemindMixCt.MP2FrameLen, file_buf, frame_len, 1))
#endif
			{
				DBG("get sbc info error\n");
				RemindMixCt.MP2DecoderSta = 0;
				RemindMixCt.IsBlock = 0;
				AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
				return;
			}
			RemindMixCt.MP2FrameLen += frame_len;
			MCUCircular_PutData(&RemindMixCircularBuf,file_buf,frame_len);
		}
		if(RemindMixCt.MP2FrameLen >= RemindMixCt.RemindFileSize)
		{
			RemindMixCt.MP2DecoderSta = 3;
			return;
		}
	}
	if(RemindMixCt.MP2DecoderSta==3)//开始播放//停止播放
	{
		if(MCUCircular_GetDataLen(&RemindMixCircularBuf) < frame_len)
		{
			DBG("fifo end\n\n");
			AudioCoreSourceDisable(MIX_REMIND_SOURCE_NUM);
			RemindMixCt.MP2DecoderSta = 0;
			RemindMixCt.IsBlock = 0;
			return;
		 }
	}
//	vTaskDelay(1);
#endif
}

//TRUE 可申请提示音播放 FALSE：忙
bool RemindMixServiceRequestPlayStatus(void)
{

	return RemindMixCt.MP2DecoderSta;

}
/***************************************************************************************
 *
 * APIs
 *
 */
MessageHandle GetRemindMixMessageHandle(void)
{
	return RemindMixCt.msgHandle;
}

//提示音请求，记录条目字符串。BlockPlay 指播放不被打断，复位除外。
bool RemindMixServiceItemRequest(char *SoundItem, bool IsBlock)
{
	if(MixRemindErrorProcess()) return FALSE;
	
	memcpy(RemindMixCt.RequestRemind, SoundItem, MIX_REMIND_SOUND_ID_LEN);
	
	if(RemindMixServiceReadItemInfo(RemindMixCt.RequestRemind))
	{
		RemindMixServicePlayInit();
		DBG("MIX REMIND FILE OK\n");
		return TRUE;
	}
	else
	{
		DBG("MIX REMIND FILE ERR\n");
		return FALSE;
	}
}
//TRUE 可申请提示音播放 FALSE：忙
bool RemindMixServiceStatus(void)
{
	return RemindMixCt.MP2DecoderSta;
}

//提示音无条件结束播放
void RemindMixServiceEnd(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_REMIND_SOUND_PLAY_END;
	MessageSend(RemindMixCt.msgHandle, &msgSend);
}
//--------------------------------------------------------//


uint16_t AudioRemindMixDataLenGet(void)
{
#if (DECODER_TPYE == MP2_DEC)
	uint16_t temp =  MCUCircular_GetDataLen(&RemindMixCircularBuf);
	return temp/(mp3_cnt.nb_channels*2);
#elif (DECODER_TPYE == SBC_DEC)
	uint16_t temp =  MCUCircular_GetDataLen(&RemindMixCircularBuf)/frame_len;
	return (temp*128);
#endif
}


uint16_t AudioRemindMixDataGet(void* Buf, uint16_t Len)
{
	uint16_t temp;
	if(Buf == NULL)
		return 0;
#if (DECODER_TPYE == MP2_DEC)
	temp =  MCUCircular_GetData(&RemindMixCircularBuf,Buf,Len*(mp3_cnt.nb_channels*2));
	return temp/(mp3_cnt.nb_channels*2);
#elif (DECODER_TPYE == SBC_DEC)
	int jj;
	int16_t *p = Buf;
	temp = AudioRemindMixDataLenGet();
	if(temp < Len)
	{
		memset(Buf,0,Len*4);
		return 0;
	}
	temp = Len/128;
	for(jj=0;jj<temp;jj++)
	{
		MCUCircular_GetData(&RemindMixCircularBuf,decoder_mem,frame_len);
		if(channel == 2)
		{
			sbc_decoder_apply(sbc_cnt,decoder_mem,frame_len,&p[256*jj]);
		}
		else
		{
			sbc_decoder_apply(sbc_cnt,decoder_mem,frame_len,&p[128*jj]);
		}
	}
#endif
}

//-------------------------------------------------------------//
void AudioMixRemind_Release(void)
{
 // if(mainAppCt.Source2Buf_MixRemind != NULL)
 //  {
	   //DBG("Source2Buf_MixRemind\n");
	   //osPortFree(mainAppCt.Source2Buf_MixRemind);
	  // mainAppCt.Source2Buf_MixRemind = NULL;
 //  }
}

void AudioMixRemind_ResMalloc(uint16_t SampleLen)
{
	uint16_t AudioCoreBufLen;
	AudioCoreBufLen = SampleLen*2*2;
	mainAppCt.Source2Buf_MixRemind = Source2Buf_MixRemind;//(int16_t*)osPortMallocFromEnd(AudioCoreBufLen);//stereo
	if(mainAppCt.Source2Buf_MixRemind != NULL)
	{
		memset(mainAppCt.Source2Buf_MixRemind, 0, AudioCoreBufLen);
	}
	else
	{
		DBG("malloc Source2Buf_MixRemind error\n");
	}
}

void AudioMixRemind_ConfigInit(void)
{
    mainAppCt.AudioCore->AudioSource[MIX_REMIND_SOURCE_NUM].PcmInBuf = (int16_t *)mainAppCt.Source2Buf_MixRemind;
}
#else//end of CFG_FUNC_REMIND_MIX_MODE

bool RemindMixServiceItemRequest(char *SoundItem, bool IsBlock)
{
	SoundItem = SoundItem;
	IsBlock =IsBlock;
	return FALSE;
}
#endif


