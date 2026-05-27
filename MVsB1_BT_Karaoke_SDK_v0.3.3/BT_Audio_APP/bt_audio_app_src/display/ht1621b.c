#include "ht1621b.h"
#include "delay.h"

/*
    HT1621B编程说明：
	①SEG0~31是"地址"
	②COM0~3 是"数据"
*/

/*################################### 宏定义 ###################################*/
#define HT1621B_GPIO_MAP(port, pin) { \
    STRING_CONNECT(GPIO, port, GPOE), \
    STRING_CONNECT(GPIO, port, GPIE), \
    STRING_CONNECT(GPIO, port, GPPU), \
    STRING_CONNECT(GPIO, port, GPPD), \
    STRING_CONNECT(GPIO, port, GPOUT), \
    pin \
}

/*################################### 变量(静态声明) ###################################*/
static const struct {
    uint32_t oe, ie, pu, pd, out, pin;
} GPIO_Init_Tab[] = {
    #define XX_MAP(name, port, pin) HT1621B_GPIO_MAP(port, pin),
    HT1621B_IC_MAP(XX_MAP)
    HT1621B_CTRL_MAP(XX_MAP)
    #undef XX_MAP
};
#define HT1621B_CS_Enable(ic)   GPIO_RegOneBitClear(GPIO_Init_Tab[ic].out, GPIO_Init_Tab[ic].pin)
#define HT1621B_CS_Disable(ic)  GPIO_RegOneBitSet(GPIO_Init_Tab[ic].out, GPIO_Init_Tab[ic].pin)

#define HT1621B_WR_CLR()      GPIO_RegOneBitClear(GPIO_Init_Tab[HT1621B_WR].out, GPIO_Init_Tab[HT1621B_WR].pin)
#define HT1621B_WR_SET()      GPIO_RegOneBitSet(GPIO_Init_Tab[HT1621B_WR].out, GPIO_Init_Tab[HT1621B_WR].pin)

#define HT1621B_DATA_CLR()    GPIO_RegOneBitClear(GPIO_Init_Tab[HT1621B_DATA].out, GPIO_Init_Tab[HT1621B_DATA].pin)
#define HT1621B_DATA_SET()    GPIO_RegOneBitSet(GPIO_Init_Tab[HT1621B_DATA].out, GPIO_Init_Tab[HT1621B_DATA].pin)

/*################################### 变量(全局声明) ###################################*/

/*################################### 变量(静态定义) ###################################*/

/*################################### 变量(全局定义) ###################################*/

/*################################### 函数(静态声明) ###################################*/

/*################################### 函数(全局声明) ###################################*/

/*################################### 函数(静态定义) ###################################*/
__attribute__((section(".tcm_section"), optimize("Og")))
static void HT1621B_Delay(unsigned int time)
{
	int i;
	for(i=0; i<time; i++)
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

static void HT1621B_Send_Bit(uint8_t data, uint8_t len)
{	
	for(uint8_t i=0; i<len; i++)
	{		
		HT1621B_WR_CLR();
        HT1621B_Delay(1);
		if(data&0x80) HT1621B_DATA_SET(); else HT1621B_DATA_CLR();
        HT1621B_Delay(1);
		HT1621B_WR_SET();
        HT1621B_Delay(1);
		data <<= 1;	
	}
}
static void HT1621B_Send_Cmd(uint8_t ic, uint8_t cmd)
{
	HT1621B_CS_Enable(ic);
    HT1621B_Delay(1);
	HT1621B_Send_Bit(0x80, 4);
	HT1621B_Send_Bit(cmd, 8);
	HT1621B_CS_Disable(ic);
    //HT1621B_Delay(1);
}

/*################################### 函数(全局定义) ###################################*/
void HT1621B_Init(void)
{
	volatile uint32_t i,pu,pd,ie,oe,out,pin;
	
	for(i=0; i<HT1621B_GPIO_NUM; i++)
	{
		oe  = GPIO_Init_Tab[i].oe;
		ie  = GPIO_Init_Tab[i].ie;
		pu  = GPIO_Init_Tab[i].pu;
		pd  = GPIO_Init_Tab[i].pd;
        out = GPIO_Init_Tab[i].out;
		pin = GPIO_Init_Tab[i].pin;

		///input en
		GPIO_RegOneBitClear(ie, pin);
		GPIO_RegOneBitSet(oe, pin);
		///pull enable
		GPIO_RegOneBitClear(pu, pin);
		GPIO_RegOneBitClear(pd, pin);
        //out
		(i < HT1621B_IC_NUM) ? GPIO_RegOneBitSet(out, pin) : GPIO_RegOneBitClear(out, pin);
	}
	
	DelayMs(5); //等待LCD工作电压稳定

    for(i=0; i<HT1621B_IC_NUM; i++)
    {
    	HT1621B_Send_Cmd(i, SYS_EN);
    	HT1621B_Send_Cmd(i, RC_OSC);
    	HT1621B_Send_Cmd(i, COM_MODE_1);
    	HT1621B_Send_Cmd(i, LCD_ON); 
    }
}

void HT1621B_Write_4Bit(uint8_t ic, uint8_t addr, uint8_t data)
{
	HT1621B_CS_Enable(ic);
	HT1621B_Send_Bit(0xa0, 3);
	HT1621B_Send_Bit(addr<<2, 6);
	HT1621B_Send_Bit(data, 4);
    HT1621B_CS_Disable(ic);
	//HT1621B_Delay(1);
}

void HT1621B_Write_8Bit(uint8_t ic, uint8_t addr, uint8_t data)
{
	HT1621B_CS_Enable(ic);
	HT1621B_Send_Bit(0xa0, 3);
	HT1621B_Send_Bit(addr<<2, 6);
	HT1621B_Send_Bit(data, 8);
    HT1621B_CS_Disable(ic);
	//HT1621B_Delay(1);
}

void HT1621B_Write_All(uint8_t ic, uint8_t addr, uint8_t *p, uint8_t len)
{
	HT1621B_CS_Enable(ic);
	HT1621B_Send_Bit(0xa0, 3);
	HT1621B_Send_Bit(addr<<2, 6);
	for(uint8_t i=0; i<len; i++,p++)
		HT1621B_Send_Bit(*p, 8);
    HT1621B_CS_Disable(ic);
	//HT1621B_Delay(1);
}

void HT1621B_All_Off(uint8_t ic)
{ 
	uint8_t addr = 0;
    
	for(uint8_t i=0; i<16 ;i++)
	{
		HT1621B_Write_8Bit(ic, addr, 0x00);
		addr += 2;
	}
}

void HT1621B_All_On(uint8_t ic)
{
	uint8_t addr = 0;
    
	for(uint8_t i=0; i<16 ;i++)
	{
		HT1621B_Write_8Bit(ic, addr, 0xFF);
		addr += 2;
	}
}