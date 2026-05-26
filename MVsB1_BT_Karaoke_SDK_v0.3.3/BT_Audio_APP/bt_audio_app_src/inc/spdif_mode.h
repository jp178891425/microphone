#ifndef __SPDIF_MODE_H__
#define __SPDIF_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus


bool SpdifPlayCreate(MessageHandle parentMsgHandle);

bool SpdifPlayStart(void);

bool SpdifPlayPause(void);

bool SpdifPlayResume(void);

bool SpdifPlayStop(void);

bool SpdifPlayKill(void);

#ifdef CFG_SPDIF_DTS_DOBLY_PLAY_EN
void SpdifPlayStatusClear();
#endif
MessageHandle GetSpdifPlayMessageHandle(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SPDIF_MODE_H__

