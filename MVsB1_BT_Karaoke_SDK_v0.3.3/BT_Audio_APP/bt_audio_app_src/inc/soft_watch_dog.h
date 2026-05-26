#ifndef _SOFT_WACTH_DOG_H_
#define _SOFT_WACTH_DOG_H_

enum {
	SWD_MainTask_ID 		= (1<<0),   
	SWD_ModeTask_ID 		= (1<<1),   
	SWD_AudioCoreTask_ID 	= (1<<2),    
	SWD_BtStackTask_ID 		= (1<<3),
	SWD_DeviceTask_ID 		= (1<<4),
	SWD_RecorderTask_ID 	= (1<<5),
	SWD_DecoderTask_ID 		= (1<<6),
	SWD_RemindTask_ID 		= (1<<7),
	SWD_User0_ID 			= (1<<8),
	SWD_User1_ID 			= (1<<9),
	SWD_User2_ID 			= (1<<10),
};

extern void SWD_task_register(uint32_t id);
extern void SWD_task_destroy(uint32_t id);
extern void SWD_task_reset(uint32_t id);
extern void SWD_task_init(uint32_t timeout);
extern bool SWD_task_is_timeout(void);

#endif

