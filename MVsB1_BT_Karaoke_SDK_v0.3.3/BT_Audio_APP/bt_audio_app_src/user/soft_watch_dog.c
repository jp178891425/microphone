#include "app_config.h"
#include "soft_watch_dog.h"
#include "string.h"
#include "debug.h"

#ifdef SOFT_WACTH_DOG_ENABLE

volatile static struct
{
	uint32_t active;
	uint32_t mask;
	uint32_t timeout;
	uint32_t cnt;
}SWD_control;

void SWD_task_register(uint32_t id)
{
	SWD_control.mask |= id;
}

void SWD_task_destroy(uint32_t id)
{
	SWD_control.mask &= ~id;
}

void SWD_task_reset(uint32_t id)
{
	SWD_control.active |= id;
}

void SWD_task_init(uint32_t timeout)
{
	memset((void *)&SWD_control,0,sizeof(SWD_control));
	SWD_control.timeout = timeout;
	SWD_control.cnt 	= timeout;
}

bool SWD_task_is_timeout(void)
{
	if((SWD_control.active & SWD_control.mask) == SWD_control.mask)
	{
		SWD_control.cnt 	= SWD_control.timeout;
		SWD_control.active 	= 0;
	}
	else
	{
		if(SWD_control.cnt > 0)
		{
			SWD_control.cnt--;
			if(SWD_control.cnt == 0)
			{
				APP_DBG("[SWD_task_is_timeout] Task ID : %lu\n",(SWD_control.active & SWD_control.mask) ^ SWD_control.mask);
				SWD_control.cnt = SWD_control.timeout;
				return TRUE;
			}
		}
	}

	return FALSE;
}

#endif

