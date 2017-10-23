#include "DS1302.h"
#include "stm32f7xx_hal.h"

#define ds1302_sec_add			0x80
#define ds1302_min_add			0x82
#define ds1302_hr_add				0x84
#define ds1302_date_add			0x86
#define ds1302_month_add		0x88
#define ds1302_day_add			0x8a
#define ds1302_year_add			0x8c
#define ds1302_control_add		0x8e
#define ds1302_charger_add		0x90 					 
#define ds1302_clkburst_add		0xbe

#define RAMSIZE 31							// Ram Size in bytes
#define ds1302_ram_start 0xc0 	// First Address
#define ds1302_ram_end 0xfc			// Last Address

#define ds1302_ram_burst_write 0xfe
#define ds1302_ram_burst_read 0xff


#define HEX2BCD(v)	((v) % 10 + (v) / 10 * 16)
#define BCD2HEX(v)	((v) % 16 + (v) / 16 * 10)


#define DS1302_GPIO	GPIOI
#define DS1302_SCLK	GPIO_PIN_0
#define DS1302_SDA	GPIO_PIN_3
#define DS1302_RST	GPIO_PIN_2

static void DS1302_WriteByte(uint8_t addr, uint8_t d)
{
	uint8_t i;

//	DS1302_RST = 1;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);	
	
	addr = addr & 0xFE;
	for (i = 0; i < 8; i ++) 
	{	
//		DS1302_SDA = (bit)(addr & 1); 
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (addr & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
//		DS1302_SCK = 1;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_SET);
		delayUS_DWT(1);
//		DS1302_SCK = 0;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
		delayUS_DWT(1);
		addr >>= 1;
	}
	
	for (i = 0; i < 8; i ++) 
	{
//		DS1302_SDA = (bit)(d & 1);
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (d & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
//		DS1302_SCK = 1;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_SET);
		delayUS_DWT(1);
//		DS1302_SCK = 0;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
		delayUS_DWT(1);
		d >>= 1;
	}
	
//	DS1302_RST = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
}

static uint8_t DS1302_ReadByte(uint8_t addr) 
{
	uint8_t i;
	uint8_t temp = 0;

//	DS1302_RST = 1;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);	
	addr = addr | 0x01;

	for (i = 0; i < 8; i ++) 
	{
//		DS1302_SDA = (bit)(addr & 1);
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (addr & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
//		DS1302_SCK = 1;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_SET);
		delayUS_DWT(1);
//		DS1302_SCK = 0; 
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
		delayUS_DWT(1);
		addr >>= 1;
	}
	
	readSDA();
	for (i = 0; i < 8; i ++) 
	{
		temp >>= 1;
//		if(DS1302_SDA)
		if(HAL_GPIO_ReadPin(DS1302_GPIO, DS1302_SDA))
			temp |= 0x80;
//		DS1302_SCK = 1;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_SET);
		delayUS_DWT(1);
//		DS1302_SCK = 0;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
		delayUS_DWT(1);
	}
	writeSDA();

//	DS1302_RST = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
	return temp;
}


void DS1302_WriteTime(uint8_t *buf) 
{	
	DS1302_WriteByte(ds1302_control_add,0x00);			// Disable write protection
	DS1302_WriteByte(ds1302_sec_add,0x80);
	//Ds1302_Write_Byte(ds1302_charger_add,0xa9);			//Trickle charger, use only with rechargeable batteries 
	DS1302_WriteByte(ds1302_year_add,HEX2BCD(buf[1]));
	DS1302_WriteByte(ds1302_month_add,HEX2BCD(buf[2]));
	DS1302_WriteByte(ds1302_date_add,HEX2BCD(buf[3]));
	DS1302_WriteByte(ds1302_hr_add,HEX2BCD(buf[4]));
	DS1302_WriteByte(ds1302_min_add,HEX2BCD(buf[5]));
	DS1302_WriteByte(ds1302_sec_add,HEX2BCD(buf[6]));
	DS1302_WriteByte(ds1302_day_add,HEX2BCD(buf[7]));
	DS1302_WriteByte(ds1302_control_add,0x80);			// Enable write protection
}


void DS1302_ReadTime(uint8_t *buf)  
{ 
   	uint8_t tmp;
	
	tmp = DS1302_ReadByte(ds1302_year_add); 	
	buf[1] = BCD2HEX(tmp);
		 
	tmp = DS1302_ReadByte(ds1302_month_add); 	
	buf[2] = BCD2HEX(tmp);
	 
	tmp = DS1302_ReadByte(ds1302_date_add); 	
	buf[3] = BCD2HEX(tmp);
	 
	tmp = DS1302_ReadByte(ds1302_hr_add);		
	buf[4] = BCD2HEX(tmp);
	 
	tmp = DS1302_ReadByte(ds1302_min_add);		
	buf[5] = BCD2HEX(tmp);
	 
	tmp = DS1302_ReadByte((ds1302_sec_add))&0x7F; 
	buf[6] = BCD2HEX(tmp);
	 
	tmp = DS1302_ReadByte(ds1302_day_add);		
	buf[7] = BCD2HEX(tmp);
}


// SDA Write(output) Mode
void writeSDA(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SDA;
  GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);
	
}

// SDA Read(input) Mode
void readSDA(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SDA;
  GPIO_InitStructure.Mode =  GPIO_MODE_INPUT;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);	
}


void DS1302_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SCLK | DS1302_SDA | DS1302_RST;
  GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);
		
//	DS1302_RST = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
//	DS1302_SCK = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
    DS1302_WriteByte(ds1302_sec_add,0x00);	

	DWT->CTRL |= 1 ; // enable the counter for microsecond delay, see "void delayUS_DWT(uint32_t us)"
}


void DS1302_writeRam(uint8_t addr, uint8_t val) {
	if (addr >= RAMSIZE) {
		return;
	}
	
	DS1302_WriteByte(ds1302_ram_start + addr, val);	
}


uint8_t DS1302_readRam(uint8_t addr) {
	if (addr >= RAMSIZE) {
		return 0;
	}
	
	return DS1302_ReadByte(ds1302_ram_start + addr);	
}


// Clears the entire ram writing 0
void DS1302_clearRam(void) {
	uint8_t i;
	for(i=0; i<RAMSIZE; i++){
		DS1302_writeRam(i,0x00);
	}
}


// Clock signal need to be at least 1 micro second wide, those delays are generated with DWT
// More info:  https://www.carminenoviello.com/2015/09/04/precisely-measure-microseconds-stm32/
#pragma push
#pragma O3
void delayUS_DWT(uint32_t us) {
	volatile uint32_t cycles = (SystemCoreClock/1000000L)*us;
	volatile uint32_t start = DWT->CYCCNT;
	do  {
	} while(DWT->CYCCNT - start < cycles);
}
#pragma pop
