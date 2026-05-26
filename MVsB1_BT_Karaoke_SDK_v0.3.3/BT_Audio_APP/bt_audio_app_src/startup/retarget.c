/*
 * retarget.c
 *
 *  Created on: Mar 8, 2017
 *      Author: peter
 */

#include <stdio.h>
#include "uarts_interface.h"
#include "type.h"
#include "remap.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif
#include "mcu_circular_buf.h"
uint8_t DebugPrintPort = UART_PORT0;
uint32_t gSramEndAddr = SRAM_END_ADDR;
uint8_t IsSwUartActedAsUARTFlag = 0;

void EnableSwUartAsUART(uint8_t EnableFlag)
{
	IsSwUartActedAsUARTFlag = EnableFlag;
}

#ifdef MODIFY_USE_UART_DEBUG_DELAY
__attribute__((section(".tcm_section"), optimize("Og")))
void DbgUartDelay(unsigned int us)//200ns
{
	int i;
	for(i=0;i<us;i++)
	{
		//200ns@320M
		__asm __volatile__(
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n""nop\n""nop\n"
		"nop\n"
		);
	}
}

void UartSendByteDelay(void)
{
	uint8_t i;
	DbgUartDelay(10);
	for(i=0; i<8; i++)
	{
		DbgUartDelay(10);
	}
	DbgUartDelay(10);
	DbgUartDelay(10);
}
#endif

//This is used as dummy function in case that appilcation dont define this function.
__attribute__((weak))
void SwUartSend(unsigned char* Buf, unsigned int BufLen)
{

}

#ifdef CFG_FUNC_USBDEBUG_EN
uint8_t 	usb_buffer[4096];
MCU_CIRCULAR_CONTEXT usb_fifo;

void usb_hid_printf_init(void)
{
	MCUCircular_Config(&usb_fifo,usb_buffer,sizeof(usb_buffer));
}
#endif

__attribute__((used))
int putchar(int c)
{
#if defined(CFG_APP_CONFIG)  && !defined(CFG_FUNC_DEBUG_EN)
#else
    if(IsSwUartActedAsUARTFlag)
    {
        if((unsigned char)c == '\n')
        {
            const char lfca[2] = "\r\n";
            SwUartSend((unsigned char*)lfca, 2);
        }
        else
        {
            SwUartSend((unsigned char*)&c, 1);
        }
    }
    else
    {
    	if (c == '\n')
    	{
		#ifdef MODIFY_USE_UART_DEBUG_DELAY
			UartSendByteDelay();
			UartSendByteDelay();
		#else
    		UARTS_SendByte(DebugPrintPort, '\r');
    		UARTS_SendByte(DebugPrintPort, '\n');
		#endif
    	}
    	else
    	{	
    		#ifdef MODIFY_USE_UART_DEBUG_DELAY
			UartSendByteDelay();
			#else
    		UARTS_SendByte(DebugPrintPort, (uint8_t)c);
			#endif
    	}
    }
#endif
	return c;
}

__attribute__((used))
void nds_write(const unsigned char *buf, int size)
{
#ifdef CFG_FUNC_DEBUG_EN
	int i;
	for (i = 0; i < size; i++)
	{
		putchar(buf[i]);
	}
#endif
#ifdef CFG_FUNC_USBDEBUG_EN
	MCUCircular_PutData(&usb_fifo,(void*)buf,size);
#endif	
}

int DbgUartInit(int Which, unsigned int BaudRate, unsigned char DatumBits, unsigned char Parity, unsigned char StopBits)
{
	DebugPrintPort = Which;
	return UARTS_Init(Which, BaudRate, DatumBits,  Parity,  StopBits);
}

uint8_t GetDebugPrintPort(void)
{
	return DebugPrintPort;
}


