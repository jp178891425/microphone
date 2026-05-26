#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif
#ifdef CFG_FUNC_RECORD_FLASH
#include "spim_flash.h"
#include "timeout.h"
#include "spim.h"
#include "debug.h"
#include "spi_flash.h"
#include "delay.h"

#define   SPI_FLASH_IO_GROUP	0	//0:SPIM_PORT0_A5_A6_A7;1:SPIM_PORT1_A20_A21_A22
/***************************************
 * GPIOA5 A20 SPI_FLASH_MOSI  output
 * GPIOA6 A21 SPI_FLASH_CLK   时钟信号
 * GPIOA7 A22 SPI_FLASH_MISO  input
 * ***********************************/
#define   SPI_FLASH_CS      GPIOA4  //片选信号

#define SPI_FLASH_CS_PIN    GPIOA4
#define	SPI_FLASH_CS_PORT	GPIO_A_OUT
#define	SPI_FLASH_CS_BIT	(1<<4)
#define SPI_FLASH_CS_HIGH   GPIO_RegOneBitSet(SPI_FLASH_CS_PORT, SPI_FLASH_CS_BIT);
#define SPI_FLASH_CS_LOW    GPIO_RegOneBitClear(SPI_FLASH_CS_PORT, SPI_FLASH_CS_BIT);




inline void SpiMasterSendByte(uint8_t data)
{
	SPIM_Send(&data, 1);
}

uint8_t SpiMasterRecvByte(void)
{
	uint8_t ret;
	SPIM_Recv(&ret, 1);
	return ret;
}

void SPI_Flash_Init(void)
{
	GPIO_RegOneBitSet(GPIO_A_OE, SPI_FLASH_CS_PIN);
	GPIO_RegOneBitClear(GPIO_A_IE, SPI_FLASH_CS_PIN);
	SPI_FLASH_CS_HIGH;
	SPIM_IoConfig(SPI_FLASH_IO_GROUP);
}



void SPI_CS_HIGH(void)
{
	SPI_FLASH_CS_HIGH;
}
void SPI_CS_low(void)
{
	SPI_FLASH_CS_LOW;
}


uint16_t  SPI_Flash_ReadMID(void) //读取FLASH ID
{
    uint16_t FlashId = 0;
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_ManufactDeviceID);//0x90:读取DeviceId的命令
    SpiMasterSendByte(0x00);//发送24位的地址
    SpiMasterSendByte(0x00);
    SpiMasterSendByte(0x00);
    
    FlashId = SpiMasterRecvByte()<<8;//读取高8位
    FlashId += SpiMasterRecvByte();//读取低8位
    
    SPI_FLASH_CS_HIGH;
    return FlashId;
}

uint32_t SPI_Flash_ReadDeviceID()//读flash device id
{
    //读取Flash ID  
    uint32_t FlashDeviceID = 0;
    SPI_FLASH_CS_LOW;
	SpiMasterSendByte(0x9f);//发送读取ID命令	    
	FlashDeviceID |= SpiMasterRecvByte()<<16;  
	FlashDeviceID |= SpiMasterRecvByte()<<0;
	FlashDeviceID |= SpiMasterRecvByte()<<8;

	SPI_FLASH_CS_HIGH;	       
    return FlashDeviceID;
}

void SPI_FLASH_Write_Enable(void)//写使能
{
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_WriteEnable);//0x06:写使能
    SPI_FLASH_CS_HIGH;
}

void SPI_FLASH_Write_Disable(void)//写保护
{
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_WriteDisable);//0x04:写保护
    SPI_FLASH_CS_HIGH;
}

void SPI_FLASH_Write_SR(uint8_t sr)//写状态寄存器
{
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_EnableWriteStatusReg);//0x50:使能写状态寄存器
    SpiMasterSendByte(Flash_WriteStatusReg);//0x01:写状态寄存器
    SpiMasterSendByte(sr);//写入一个字节    
    SPI_FLASH_CS_HIGH;
}

uint8_t SPI_Flash_ReadSR(void) //读取状态寄存器
{   
    uint8_t byte=0;
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_ReadStatusReg);//0x05:读状态寄存器命令
    SpiMasterSendByte(0xFF); 
    byte = SpiMasterRecvByte();
    SPI_FLASH_CS_HIGH;
    return byte;
}

uint16_t SPI_Flash_ReadStatus()//读取完整的16bit状态值
{
    unsigned short	Status;	
    SPI_FLASH_CS_LOW;
	SpiMasterSendByte(0x05);	//low byte
	((unsigned char *)&Status)[0] = SpiMasterRecvByte();
    SPI_FLASH_CS_HIGH;

    SPI_FLASH_CS_LOW;
	SpiMasterSendByte(0x35);	//high byte
	((unsigned char *)&Status)[1] = SpiMasterRecvByte();
    SPI_FLASH_CS_HIGH;
    return 0;
}


void SPI_Flash_Wait_Busy(void)//忙则等待
{
    while((SPI_Flash_ReadSR()&0x01) == 0x01);//等待BUSY清空
}

void SPI_Flash_Erase_Chip(void)//整片擦除
{
    //DBG("Start erase flash...\n");
    //DBG("Erasing, please waiting...\n");

    SPI_FLASH_Write_Enable();//写使能
    SPI_Flash_Wait_Busy();
	SPI_FLASH_CS_LOW;
	SpiMasterSendByte(Flash_ChipErase);
	SPI_FLASH_CS_HIGH;
    SPI_Flash_Wait_Busy();
    
    //DBG(("Erase over!\n"));    
}

void SPI_Flash_Erase_Sector(uint32_t Dst_Addr)//扇区擦除Dst_Addr = 0,1,2...511, 擦除一个扇区最少时间为150ms
{
    Dst_Addr*=4096;
    SPI_FLASH_Write_Enable();//写使能
    SPI_Flash_Wait_Busy();//等待总线空闲
    
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_4KByte_BlockERASE);
    SpiMasterSendByte((uint8_t)(Dst_Addr>>16));
    SpiMasterSendByte((uint8_t)(Dst_Addr>>8));
    SpiMasterSendByte((uint8_t)Dst_Addr);
    
    SPI_FLASH_CS_HIGH;
    SPI_Flash_Wait_Busy();//等待芯片擦除结束
}


void SPI_Flash_Erase_32Block(uint32_t Dst_Addr)//32kBlock擦除0,1,2...
{
    Dst_Addr*=(32*1024);
    SPI_FLASH_Write_Enable();//写使能
    SPI_Flash_Wait_Busy();//等待总线空闲
    
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_32KByte_BlockErase);
    SpiMasterSendByte((uint8_t)(Dst_Addr>>16));
    SpiMasterSendByte((uint8_t)(Dst_Addr>>8));
    SpiMasterSendByte((uint8_t)Dst_Addr);
    SPI_FLASH_CS_HIGH;
    SPI_Flash_Wait_Busy();//等待芯片擦除结束
}



void SPI_Flash_Erase_64Block(uint32_t Dst_Addr)
{

uint32_t i_count=0;
#if 0
//DBG("KJKJHJHJKHJJHJKKKJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ\n")
    Dst_Addr*=(64*1024);
    SPI_FLASH_Write_Enable();//写使能
    SPI_Flash_Wait_Busy();//等待总线空闲
    
    SPI_FLASH_CS_LOW;
    SpiMasterSendByte(Flash_64KByte_BlockErase);
    SpiMasterSendByte((uint8_t)(Dst_Addr>>16));
    SpiMasterSendByte((uint8_t)(Dst_Addr>>8));
    SpiMasterSendByte((uint8_t)Dst_Addr);
    SPI_FLASH_CS_HIGH;
    SPI_Flash_Wait_Busy();//等待芯片擦除结束
#else
	
    Dst_Addr=Dst_Addr*64*1024/4096;
	
	for(i_count=0;i_count<16;i_count++)
	{
		SPI_Flash_Erase_Sector(i_count+Dst_Addr);
		WDG_Feed();		
	}


#endif

	
}
void SPI_Flash_Erase(ERASE_TYPE_ENUM type,uint32_t Dst_Addr)
{
	GIE_DISABLE();

	switch(type)
	{
		case CHIP_ERASE:
			SPI_Flash_Erase_Chip();
		break;
		case SECTOR_ERASE:
			SPI_Flash_Erase_Sector(Dst_Addr);
		break;
		case BLOCK_ERASE:
			SPI_Flash_Erase_64Block(Dst_Addr);
		break;
	}
	
	GIE_ENABLE();
	
	DelayUs(100);
}



//void SPI_Flash_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead)//读取flash
//{

//}

//void SPI_Flash_Write(uint8_t pBuffer[],uint32_t WriteAddr,uint16_t NumByteToWrite)//写入Flash
//{


//}


//SPI在一页(0~65535)内写入少于256个字节的数据
//在指定地址开始写入最大256字节的数据
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!	 
void SPI_Flash_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	//uint16_t i;
    SPI_FLASH_Write_Enable();                      //SET WEL 
	SPI_FLASH_CS_LOW;                              //使能器件   
    SpiMasterSendByte(Flash_PageProgramm);         //发送写页命令   
    SpiMasterSendByte((uint8_t)((WriteAddr)>>16)); //发送24bit地址    
    SpiMasterSendByte((uint8_t)((WriteAddr)>>8));   
    SpiMasterSendByte((uint8_t)WriteAddr);   
//    for(i=0;i<NumByteToWrite;i++)
//        SpiMasterSendByte(pBuffer[i]);             //循环写数
    SPIM_Send(pBuffer, NumByteToWrite);
	SPI_FLASH_CS_HIGH;                             //取消片选 
	SPI_Flash_Wait_Busy();					       //等待写入结束
} 


//无检验写SPI FLASH 
//必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
//具有自动换页功能 
//在指定地址开始写入指定长度的数据,但是要确保地址不越界!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
//CHECK OK
void SPI_Flash_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
	uint16_t pageremain;	   
	pageremain=256-WriteAddr%256; //单页剩余的字节数		 	    
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//不大于256个字节
	while(1)
	{
		SPI_Flash_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//写入结束了
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //减去已经写入了的字节数
			if(NumByteToWrite>256)pageremain=256; //一次可以写入256个字节
			else pageremain=NumByteToWrite; 	  //不够256个字节了
		}
	};	
}

uint8_t SPI_Flash_Read(uint32_t ReadAddr,uint8_t* pBuffer,uint32_t NumByteToRead)//读取flash
{
    //uint16_t i;
    
	GIE_DISABLE();
	SPI_FLASH_CS_LOW;                            //使能器件   
    SpiMasterSendByte(Flash_ReadData);         //发送读取命令   
    SpiMasterSendByte((uint8_t)((ReadAddr)>>16));  //发送24bit地址    
    SpiMasterSendByte((uint8_t)((ReadAddr)>>8));   
    SpiMasterSendByte((uint8_t)ReadAddr);

    SPIM_Recv(pBuffer, NumByteToRead);
//    for(i=0;i<NumByteToRead;i++)
//	{
//        pBuffer[i]=SpiMasterRecvByte();   //循环读数
//    }
	SPI_FLASH_CS_HIGH;
	GIE_ENABLE();

	DelayUs(20);
	return 0;
}

//写SPI FLASH  
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)						
//NumByteToWrite:要写入的字节数(最大65535)   	 
void SPI_Flash_Write(uint32_t WriteAddr, uint8_t* pBuffer, uint32_t NumByteToWrite)
{ 
//	GIE_DISABLE();
	SPI_Flash_Write_NoCheck(pBuffer,WriteAddr,NumByteToWrite);
//	GIE_ENABLE();
	
#if 0
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;	   
 	uint16_t i;    
	//uint8_t * SPI_FLASH_BUF;
   	//SPI_FLASH_BUF=SPI_FLASH_BUFFER;
 	secpos=WriteAddr/4096;//扇区地址  
	secoff=WriteAddr%4096;//在扇区内的偏移
	secremain=4096-secoff;//扇区剩余空间大小   
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//测试用
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//不大于4096个字节
	while(1) 
	{
#if 0
		SPI_Flash_Read(SPI_FLASH_BUF,secpos*4096,4096);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(SPI_FLASH_BUF[secoff+i]!=0XFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			SPI_Flash_Erase_Sector(secpos);//擦除这个扇区
			for(i=0;i<secremain;i++)	   //复制
			{
				SPI_FLASH_BUF[i+secoff]=pBuffer[i];	  
			}
			SPI_Flash_Write_NoCheck(SPI_FLASH_BUF,secpos*4096,4096);//写入整个扇区  
		}
        else
#endif
        {  
			SPI_Flash_Erase_Sector(secpos);//擦除这个扇区
            SPI_Flash_Write_NoCheck(pBuffer,WriteAddr,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
        }

		if(NumByteToWrite==secremain)
        {
            break;//写入结束了

        }
        else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0 	 

		   	pBuffer+=secremain;  //指针偏移
			WriteAddr+=secremain;//写地址偏移	   
		   	NumByteToWrite-=secremain;				//字节数递减
			if(NumByteToWrite>4096)secremain=4096;	//下一个扇区还是写不完
			else secremain=NumByteToWrite;			//下一个扇区可以写完了
		}	 
	}
#endif
}

//进入掉电模式
void SPI_Flash_PowerDown(void)   
{ 
  	SPI_FLASH_CS_LOW;                     //使能器件   
    SpiMasterSendByte(Flash_PowerDown);   //发送掉电命令  
	SPI_FLASH_CS_HIGH;                    //取消片选     	      
    DelayMs(3);                           //等待TPD  
}   
//唤醒
void SPI_Flash_WAKEUP(void)   
{  
  	SPI_FLASH_CS_LOW;                     //使能器件   
    SpiMasterSendByte(Flash_ReleasePowerDown);//send Flash_PowerDown command 0xAB    
	SPI_FLASH_CS_HIGH;                    //取消片选     	      
    DelayMs(3);                           //等待TRES1
}   


void SPI_FLASH_LOCK()   //Flash加锁保护
{  
    //SPI_FLASH_Write_SR(0x7E);
}


void SPI_FLASH_UNLOCK() //Flash解锁
{
    //SPI_FLASH_Write_SR(0x98);
}

#endif











