/**
 **************************************************************************************
 * @file    flash_interface.c
 * @brief   flash interface
 *
 * @author  Peter
 * @version V1.0.0
 *
 * $Created: 2019-05-30 11:30:00$
 *
 * @Copyright (C) 2019, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "type.h"
#include "spi_flash.h"
#include <stdarg.h>
#include "watchdog.h"

#ifdef FUNC_OS_EN
#include "rtos_api.h"

osMutexId FlashMutex = NULL;
#endif

//SPI_FLASH_ERR_CODE SpiFlashInitWpr(uint32_t flash_clk, SPIFLASH_IO_MODE IOMode, bool HpmEn, FSHC_CLK_MODE ClkSrc)
//{
//	SPI_FLASH_ERR_CODE ret;
//#ifdef FUNC_OS_EN
//	osMutexLock(FlashMutex);
//#endif
//	ret = SpiFlashInit(flash_clk, IOMode, HpmEn, ClkSrc);
//#ifdef FUNC_OS_EN
//	osMutexUnlock(FlashMutex);
//#endif
//	return ret;
//}

SPI_FLASH_ERR_CODE SpiFlashReadWpr( uint32_t StartAddr, uint8_t* Buffer, uint32_t Length, uint32_t TimeOut)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashRead(StartAddr, Buffer, Length, TimeOut);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}


SPI_FLASH_ERR_CODE SpiFlashWriteWpr( uint32_t	Addr, uint8_t	*Buffer, uint32_t 	Length, uint32_t TimeOut)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashWrite(Addr, Buffer, Length, TimeOut);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}

void SpiFlashEraseWpr(ERASE_TYPE_ENUM erase_type, uint32_t index, bool IsSuspend)
{
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	SpiFlashErase(erase_type, index, IsSuspend);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
}

int32_t FlashEraseWpr(uint32_t Offset,uint32_t Size)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = FlashErase(Offset, Size);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}


SPI_FLASH_ERR_CODE SpiFlashClkSwitchWpr(FSHC_CLK_MODE ClkSrc, uint32_t FlashClk)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashClkSwitch(ClkSrc, FlashClk);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}


SPI_FLASH_INFO* SpiFlashInfoGetWpr(void)
{
	SPI_FLASH_INFO *ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashInfoGet();
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}

SPI_FLASH_ERR_CODE SpiFlashResumDelaySetWpr(uint32_t time)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashResumDelaySet(time);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}

//SPI_FLASH_ERR_CODE SpiFlashIOCtrlWpr(IOCTL_FLASH_T Cmd, ...)
//{
//	SPI_FLASH_ERR_CODE ret;
//	va_list List;
//#ifdef FUNC_OS_EN
//	osMutexLock(FlashMutex);
//#endif
//	va_start(List,Cmd);
//	ret = SpiFlashIOCtrl(Cmd, List);
//	va_end(List);
//#ifdef FUNC_OS_EN
//	osMutexUnlock(FlashMutex);
//#endif
//
//	return ret;
//}

SPI_FLASH_ERR_CODE SpiFlashReadUniqueIDWpr(uint8_t* Buffer, uint8_t BufLen)
{
    SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
    osMutexLock(FlashMutex);
#endif
    ret = SpiFlashReadUniqueID(Buffer, BufLen);
#ifdef FUNC_OS_EN
    osMutexUnlock(FlashMutex);
#endif
    return ret;
}

SPI_FLASH_ERR_CODE FlashEraseExt(uint32_t Offset, uint32_t Size, bool IsSuspend)
{
    SPI_FLASH_ERR_CODE ret = ERASE_FLASH_ERR;
    SPI_FLASH_INFO *pInfo = NULL;
    int32_t i, Round;
    
#ifdef FUNC_OS_EN
    osMutexLock(FlashMutex);
#endif
    
    //Check if Offset and Size is at 4K alignment
    if (((Offset & 0xFFF) == 0) && ((Size & 0xFFF) == 0))
    {
        pInfo = SpiFlashInfoGet();
        
        //Validate input parameters
        if(pInfo->Capacity >= Offset + Size)
        {
            if(Size >= 64 * 1024)
            {
                if(Offset & (0x10000 - 1))
                {
                    Round = (0x10000 - (Offset & 0xFFFF)) >> 12;
                    for(i = 0; i < Round; i++)
                    {
                        WDG_Feed();
                        //4K erase
                        SpiFlashErase(SECTOR_ERASE, Offset / 0x1000, IsSuspend);
                        Size -= 0x1000;
                        Offset += 0x1000;
                    }
                }

                if(Size >= 64 * 1024)
                {
                    Round = Size >> 16;
                    for(i = 0; i < Round; i++)
                    {
                        WDG_Feed();
                        //64K erase
                        SpiFlashErase(BLOCK_ERASE, Offset / 0x10000, IsSuspend);
                        Size -= 0x10000;
                        Offset += 0x10000;
                    }
                }
            }

            Round = Size >> 12;
            for(i = 0; i < Round; i++)
            {
                WDG_Feed();
                //4K erase
                SpiFlashErase(SECTOR_ERASE, Offset / 0x1000, IsSuspend);
                Offset += 0x1000;
            }

            ret = FLASH_NONE_ERR;
        }
    }
    
#ifdef FUNC_OS_EN
    osMutexUnlock(FlashMutex);
#endif

    return ret;
}

