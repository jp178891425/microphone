#include <string.h>
#include "type.h"
#include "spdif.h"
#include "dma.h"
#include "clk.h"
#include "debug.h"
//#include "rtos_api.h"
#if 1//def CFG_RES_AUDIO_SPDIFOUT_EN
#include "app_config.h"
#endif

#if defined(CFG_RES_AUDIO_SPDIFOUT_EN)
uint8_t SpdifBuff[CFG_PARA_MAX_SAMPLES_PER_FRAME*8];
#endif
uint16_t AudioSpdifRx_GetDataLen(void)
 {
   uint16_t NumSamples = 0;
   NumSamples = DMA_CircularDataLenGet(PERIPHERAL_ID_SPDIF_RX);
   return NumSamples / 8;
 }
void *osPortMalloc(uint16_t osWantedSize);
void osPortFree( void *ospv );
uint16_t AudioSpdifRx_GetData(void* Buf, uint16_t Len)
{
	uint16_t Length = 0;//Samples
	uint8_t *SpdifBuf;

   	Length = DMA_CircularDataLenGet(PERIPHERAL_ID_SPDIF_RX) / 8;

	if(Length > Len)
	{
		Length = Len;
	}

	SpdifBuf = (uint8_t *)osPortMalloc(Length * 8);
	if(SpdifBuf == NULL)
	{
		DBG("Malloc failure!\n");
		return 0;
	}
	Length = DMA_CircularDataGet(PERIPHERAL_ID_SPDIF_RX, SpdifBuf, Length * 8);
	Length = SPDIF_SPDIFDataToPCMData((int32_t *)SpdifBuf, Length, (int32_t *)Buf, SPDIF_WORDLTH_16BIT);
	osPortFree(SpdifBuf);
    return Length / 4;
}


#if defined(CFG_RES_AUDIO_SPDIFOUT_EN)
//#define SPDIF_FIFO_LEN				(10 * 1024)

uint16_t AudioSpdifTXDataSet(void* Buf, uint16_t Len)
{
	uint16_t Length;

	if(Buf == NULL) return 0;
	uint8_t *SpdifBuf= SpdifBuff;
	Length = Len * 4;

	//if(((DMA_CircularSpaceLenGet(PERIPHERAL_ID_SPDIF_TX) / 8) * 8) >=  Length * 2)
	{
		int m;

		m = SPDIF_PCMDataToSPDIFData((int32_t *)Buf, Length, (int32_t *)SpdifBuf, 16);
		//DBG("SPDIF_PCMDataToSPDIFData = %d Length = %d  \n",m,Length);
		  DMA_CircularDataPut(PERIPHERAL_ID_SPDIF_TX, (void *)SpdifBuf, m & 0xFFFFFFFC);
	}
	return 0;
}

uint16_t AudioSpdifTXDataSpaceLenGet(void)
{
	return DMA_CircularSpaceLenGet(PERIPHERAL_ID_SPDIF_TX) / 8;
}

void AudioSpdif_OutInit(GPIO_PortA gpio, uint32_t SampleRate, void *SpdifFifo, uint32_t  SPDIF_FIFO_LEN)
{
	if(SpdifFifo==NULL)
	{
		DBG("SPDIF OUT FIFO Err\n");
		return;
	}
	if((gpio == GPIOA28)||(gpio == GPIOA29)||(gpio == GPIOA30)||(gpio == GPIOA31))
	{
	  GPIO_PortAModeSet(gpio, 7);

	 if(SampleRate == 44100)
	     {
	        Clock_APllLock(225792);
	     }
	   else
	    {
	  		Clock_APllLock(122880*2);
	    }

	   SPDIF_ClockSourceSelect(SPIDF_CLK_SOURCE_AUPLL);

	   SPDIF_TXInit(1, 1, 0, 10);

	   SPDIF_SampleRateSet(SampleRate);

	   DMA_CircularConfig(PERIPHERAL_ID_SPDIF_TX, SPDIF_FIFO_LEN/2, SpdifFifo, SPDIF_FIFO_LEN);

	   SPDIF_ModuleEnable();

	   DMA_ChannelEnable(PERIPHERAL_ID_SPDIF_TX);
	}
	else
	{
		DBG("SPDIF OUT IO Err\n");
	}
}

void AudioSpdif_OutDeInit(void)
{
	SPDIF_ModuleDisable();
    DMA_ChannelDisable(PERIPHERAL_ID_SPDIF_TX);

}

void AudioSpdif_DeInit(void)
{
	SPDIF_ModuleDisable();
    DMA_ChannelDisable(PERIPHERAL_ID_SPDIF_RX);
    //DMA_ChannelClose(PERIPHERAL_ID_SPDIF_RX);
}
#endif
