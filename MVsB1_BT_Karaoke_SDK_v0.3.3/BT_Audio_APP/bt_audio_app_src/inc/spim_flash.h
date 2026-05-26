
#ifndef __SPIM_FLASH_H__
#define __SPIM_FLASH_H__

#include "type.h"
/**
 *****************************************************************************
 * @file     spim_flash.h
 * @author   
 * @version  
 * @date     
 * @brief    
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */
#include "spi_flash.h"

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

//指令表
#define Flash_ReadData                   0x03
#define Flash_FastReadData               0x0B
#define Flash_4KByte_BlockERASE          0x20
#define Flash_32KByte_BlockErase         0x52
#define Flash_64KByte_BlockErase         0xD8
#define Flash_ChipErase                  0xC7
#define Flash_ByteProgram                0x02
#define Flash_AAI_WordProgram            0xAD
#define Flash_ReadStatusReg              0x05
#define Flash_WriteStatusReg             0x01
#define Flash_EnableWriteStatusReg       0x50
#define Flash_WriteEnable                0x06
#define Flash_WriteDisable               0x04
#define Flash_ManufactDeviceID           0x90
#define Flash_JedecDeviceID              0x9F
#define Flash_EBSY_COMMAND               0x70
#define Flash_DBSY_COMMAND               0x80 
#define Flash_PowerDown			         0xB9 
#define Flash_ReleasePowerDown      	 0xAB
#define Flash_PageProgramm               0x02

void      SPI_Flash_Init(void);
uint16_t  SPI_Flash_ReadMID(void);       //读取FLASH ID
uint32_t  SPI_Flash_ReadDeviceID(void); //读取FLASH DeviceID
void      SPI_Flash_Erase_Chip(void);   //整片擦除
void      SPI_Flash_Erase_Sector(uint32_t Dst_Addr);  //扇区擦除
void      SPI_Flash_Erase_32Block(uint32_t Dst_Addr); //32k块擦除
uint8_t   SPI_Flash_Read(uint32_t ReadAddr,uint8_t* pBuffer,uint32_t NumByteToRead);   //读取flash
void SPI_Flash_Erase(ERASE_TYPE_ENUM type,uint32_t Dst_Addr);




#ifdef  __cplusplus
}
#endif//__cplusplus

#endif  
#endif //__SPIM_FLASH_H__
















































