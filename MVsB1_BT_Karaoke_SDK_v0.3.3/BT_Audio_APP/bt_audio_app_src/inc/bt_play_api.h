//bt_play_api.h
#include "type.h"
#include "mcu_circular_buf.h"
#include "resampler.h"
#include "resampler_polyphase.h"
#include "sbc_frame_decoder.h"

#ifndef __BT_PLAY_API_H__
#define __BT_PLAY_API_H__


typedef enum _BT_PLAYER_STATE
{
	BT_PLAYER_STATE_STOP = 0,    // 空闲
	BT_PLAYER_STATE_PLAYING,     // 播放
	BT_PLAYER_STATE_PAUSED,       // 暂停
	BT_PLAYER_STATE_FWD_SEEK,
	BT_PLAYER_STATE_REV_SEEK,
	
	BT_PLAYER_STATE_ERROR = 0xff,
} BT_PLAYER_STATE;



#define SBC_FIFO_SIZE	10*1024
#define SBC_FIFO_LEVEL_HIGH 	(SBC_FIFO_SIZE / 10 * 7) //70% 开播
typedef struct _BT_A2DP_PLAYER
{
	uint8_t sbc_fifo[SBC_FIFO_SIZE];//sbc缓存


	//sbc相关参数初始化标志,未初始化前,不能收发数据
	uint32_t sbc_init_flag;

	MemHandle MemHandle;

}BT_A2DP_PLAYER;



#ifdef CFG_FUNC_SOFT_ADJUST_IN
#define BT_SOFT_ADJUST_LEVEL_L				(SBC_FIFO_SIZE / 10 * 4)//4000//注意buf size，当前是10K
#define BT_SOFT_ADJUST_LEVEL_H				(SBC_FIFO_SIZE / 10 * 9)//8000
#define BT_SBC_LEVEL_HIGH					(BT_SOFT_ADJUST_LEVEL_H)
#define BT_SBC_LEVEL_LOW					(BT_SOFT_ADJUST_LEVEL_L)
#else
#define BT_SBC_LEVEL_HIGH					(SBC_FIFO_LEVEL_HIGH)
#define BT_SBC_LEVEL_LOW					(SBC_FIFO_SIZE / 10 * 4)
#endif
#define BT_AAC_LEVEL_HIGH					12
#define BT_AAC_LEVEL_LOW					5
#define BT_AAC_START_FRAME					((BT_AAC_LEVEL_LOW + BT_AAC_LEVEL_HIGH) / 2 + 1)

uint32_t GetValidSbcDataSize(void);
uint32_t InsertDataToSbcBuffer(uint8_t * data, uint16_t dataLen);
void SetSbcDecoderStarted(bool flag);
bool GetSbcDecoderStarted(void);
int32_t SbcDecoderInit(void);
void BtSbcDecoderRefresh(void);


#ifdef CFG_APP_BT_MODE_EN
#ifdef CFG_FUNC_REMIND_SOUND_EN
#include "decoder_service.h"
bool BtPlayerBackup(void);
void SbcDecoderRefresh(void);
//void BtPlayerSetDecoderShare(DecoderShare State);
//DecoderShare BtPlayerGetDecoderShare(void);
#endif
#endif

int32_t SbcDecoderDeinit(void);

int32_t SbcDecoderStart(void);

void BtPlayerPlay(void);

void BtPlayerPause(void);

//播放&暂停
void BtPlayerPlayPause(void);

void BtAutoPlayMusic(void);

void a2dp_sbc_decoer_init(void);

void a2dp_stream_suspend_play_end(void);

void BtAudioCoreSourceFreqAdjustEnable(void);

#endif

