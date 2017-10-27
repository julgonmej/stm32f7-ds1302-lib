#include "DS1302.h"

// Write Register Address
#define DS1302_SEC				0x80
#define DS1302_MIN				0x82
#define DS1302_HOUR				0x84
#define DS1302_DATE				0x86
#define DS1302_MONTH			0x88
#define DS1302_DAY				0x8a
#define DS1302_YEAR				0x8c
#define DS1302_CONTROL		0x8e
#define DS1302_CHARGER		0x90 					 
#define DS1302_CLKBURST		0xbe
#define DS1302_RAMBURST 	0xfe

#define RAMSIZE 					0x31	// Ram Size in bytes
#define DS1302_RAMSTART		0xc0 	// First Address


#define HEX2BCD(v)	((v) % 10 + (v) / 10 * 16)
#define BCD2HEX(v)	((v) % 16 + (v) / 16 * 10)

// GPIO Pins
#define DS1302_GPIO	GPIOI
#define DS1302_SCLK	GPIO_PIN_0
#define DS1302_SDA	GPIO_PIN_3
#define DS1302_RST	GPIO_PIN_2

/* Clock signal need to be at least 1 micro second wide, those delays are generated with DWT		*/
/* More info:  https://www.carminenoviello.com/2015/09/04/precisely-measure-microseconds-stm32/ */
#pragma push
#pragma O3
static void delayUS_DWT(uint32_t us) {
	volatile uint32_t cycles = (SystemCoreClock/1000000L)*us;
	volatile uint32_t start = DWT->CYCCNT;
	do  {
	} while(DWT->CYCCNT - start < cycles);
}
#pragma pop


// SDA Write(output) Mode
static void writeSDA(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SDA;
  GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);
	
}

// SDA Read(input) Mode
static void readSDA(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SDA;
  GPIO_InitStructure.Mode =  GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLDOWN;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);	
}

/* Sends an address or command */
static void DS1302_SendCmd(uint8_t cmd) {
	uint8_t i;
	for (i = 0; i < 8; i ++) 
	{	
//		DS1302_SDA = (bit)(addr & 1); 
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (cmd & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
//		DS1302_SCK = 1;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_SET);
		delayUS_DWT(1);
//		DS1302_SCK = 0;
		HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
		delayUS_DWT(1);
		cmd >>= 1;
	}
}


/* Writes a byte to 'addr' */
static void DS1302_WriteByte(uint8_t addr, uint8_t d)
{
	uint8_t i;

//	DS1302_RST = 1;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);	
	
	//addr = addr & 0xFE;
	DS1302_SendCmd(addr);	// Sends address
	
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
	//	DS1302_SDA = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
}


/* Sends 'cmd' command and writes in burst mode 'len' bytes from 'temp' */
static void DS1302_WriteBurst(uint8_t cmd, uint8_t len, uint8_t * temp)
{
	uint8_t i, j;
	
	DS1302_WriteByte(DS1302_CONTROL,0x00);			// Disable write protection

//	DS1302_RST = 1;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);	
	
	DS1302_SendCmd(cmd);	// Sends burst write command
	
	for(j = 0; j < len; j++) {
		for (i = 0; i < 8; i ++) 
		{
//			DS1302_SDA = (bit)(d & 1);
			HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA, (temp[j] & 1) ?  GPIO_PIN_SET :  GPIO_PIN_RESET);
//			DS1302_SCK = 1;
			HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_SET);
			delayUS_DWT(1);
//			DS1302_SCK = 0;
			HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
			delayUS_DWT(1);
			temp[j] >>= 1;
		}
	}
	
//	DS1302_RST = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	//	DS1302_SDA = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
	
	DS1302_WriteByte(DS1302_CONTROL,0x80);			// Enable write protection
}


/* Reads a byte from addr */
static uint8_t DS1302_ReadByte(uint8_t addr) 
{
	uint8_t i;
	uint8_t temp = 0;

//	DS1302_RST = 1;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);	
	addr = addr | 0x01; // Generate Read Address

	DS1302_SendCmd(addr);	// Sends address
	
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
//	DS1302_SDA = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
	return temp;
}


/* Sends 'cmd' command and reads in burst mode 'len' bytes into 'temp' */
static void DS1302_ReadBurst(uint8_t cmd, uint8_t len, uint8_t * temp) 
{
	uint8_t i, j;

//	DS1302_RST = 1;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_SET);	
	cmd = cmd | 0x01; // Generate read command

	DS1302_SendCmd(cmd);	// Sends burst read command
	
	readSDA();
	for (j = 0; j < len; j ++) {
		temp[j] = 0;
		for (i = 0; i < 8; i ++) 
		{
			temp[j] >>= 1;
			//			if(DS1302_SDA)
			if(HAL_GPIO_ReadPin(DS1302_GPIO, DS1302_SDA))
				temp[j] |= 0x80;

//			DS1302_SCK = 1;
			HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_SET);
			delayUS_DWT(1);
		
//			DS1302_SCK = 0;
			HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);
			delayUS_DWT(1);

		}
	}
	writeSDA();

//	DS1302_RST = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SDA,  GPIO_PIN_RESET);
}


/* Writes time byte by byte from 'buf' */
void DS1302_WriteTime(uint8_t *buf) 
{	
	DS1302_WriteByte(DS1302_CONTROL,0x00);			// Disable write protection
	delayUS_DWT(1);
	DS1302_WriteByte(DS1302_SEC,0x80);
	DS1302_WriteByte(DS1302_YEAR,HEX2BCD(buf[1]));
	DS1302_WriteByte(DS1302_MONTH,HEX2BCD(buf[2]));
	DS1302_WriteByte(DS1302_DATE,HEX2BCD(buf[3]));
	DS1302_WriteByte(DS1302_HOUR,HEX2BCD(buf[4]));
	DS1302_WriteByte(DS1302_MIN,HEX2BCD(buf[5]));
	DS1302_WriteByte(DS1302_SEC,HEX2BCD(buf[6]));
	DS1302_WriteByte(DS1302_DAY,HEX2BCD(buf[7]));
	DS1302_WriteByte(DS1302_CONTROL,0x80);			// Enable write protection
	delayUS_DWT(1);
}


/* Reads time byte by byte to 'buf' */
void DS1302_ReadTime(uint8_t *buf)  
{ 
   	uint8_t tmp;
	
	tmp = DS1302_ReadByte(DS1302_YEAR); 	
	buf[1] = BCD2HEX(tmp);		 
	tmp = DS1302_ReadByte(DS1302_MONTH); 	
	buf[2] = BCD2HEX(tmp);	 
	tmp = DS1302_ReadByte(DS1302_DATE); 	
	buf[3] = BCD2HEX(tmp);
	tmp = DS1302_ReadByte(DS1302_HOUR);		
	buf[4] = BCD2HEX(tmp);
	tmp = DS1302_ReadByte(DS1302_MIN);		
	buf[5] = BCD2HEX(tmp); 
	tmp = DS1302_ReadByte((DS1302_SEC))&0x7F; 
	buf[6] = BCD2HEX(tmp);
	tmp = DS1302_ReadByte(DS1302_DAY);		
	buf[7] = BCD2HEX(tmp);
}

/* Initialization */
void DS1302_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = DS1302_SCLK | DS1302_SDA | DS1302_RST;
  GPIO_InitStructure.Mode =  GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(DS1302_GPIO, &GPIO_InitStructure);
	
	DS1302_WriteByte(DS1302_CHARGER,0x00);			// Disable Trickle Charger 
		
//	DS1302_RST = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_RST,  GPIO_PIN_RESET);
//	DS1302_SCK = 0;
	HAL_GPIO_WritePin(DS1302_GPIO, DS1302_SCLK,  GPIO_PIN_RESET);

	DWT->CTRL |= 1 ; // enable the counter for microsecond delay, see "void delayUS_DWT(uint32_t us)"
}

/* Writes 'val' to ram address 'addr' */
/* Ram addresses range from 0 to 30 */
void DS1302_WriteRam(uint8_t addr, uint8_t val) {
	DS1302_WriteByte(DS1302_CONTROL,0x00);			// Disable write protection
	delayUS_DWT(1);
	if (addr >= RAMSIZE) {
		return;
	}
	
	DS1302_WriteByte(DS1302_RAMSTART + (2 * addr), val);	
	
	DS1302_WriteByte(DS1302_CONTROL,0x80);			// Enable write protection
	delayUS_DWT(1);
}

/* Reads ram address 'addr' */
uint8_t DS1302_ReadRam(uint8_t addr) {
	if (addr >= RAMSIZE) {
		return 0;
	}
	
	return DS1302_ReadByte(DS1302_RAMSTART + (2 * addr));	
}


/* Clears the entire ram writing 0 */
void DS1302_ClearRam(void) {
	uint8_t i;
	for(i=0; i< RAMSIZE; i++){
		DS1302_WriteRam(i,0x00);
	}
}


/* Reads time in burst mode, includes control byte */
void DS1302_ReadTimeBurst(uint8_t * buf) {
	uint8_t temp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	DS1302_ReadBurst(DS1302_CLKBURST, 8, temp); 
	
	buf[1] = BCD2HEX(temp[6]);	// Year
	buf[2] = BCD2HEX(temp[4]);	// Month
	buf[3] = BCD2HEX(temp[3]);	// Date
	buf[4] = BCD2HEX(temp[2]);	// Hour
	buf[5] = BCD2HEX(temp[1]);	// Min
	buf[6] = BCD2HEX(temp[0]);	// Sec
	buf[7] = BCD2HEX(temp[5]);	// Day
	buf[0] = temp[7]; 					// Control
}


/* Writes time in burst mode, includes control byte */
void DS1302_WriteTimeBurst(uint8_t * buf) {
	uint8_t temp[8];
	
	temp[0]=HEX2BCD(buf[6]);	// Sec
	temp[1]=HEX2BCD(buf[5]);	// Min
	temp[2]=HEX2BCD(buf[4]);	// Hour
	temp[3]=HEX2BCD(buf[3]);	// Date
	temp[4]=HEX2BCD(buf[2]);	// Month
	temp[5]=HEX2BCD(buf[7]);	// Day
	temp[6]=HEX2BCD(buf[1]);	// Year
	temp[7]=buf[0];						// Control
	
	DS1302_WriteBurst(DS1302_CLKBURST, 8, temp); 
}

/* Reads ram in burst mode 'len' bytes into 'buf' */
void DS1302_ReadRamBurst(uint8_t len, uint8_t * buf) {
	uint8_t i;
	if(len <= 0) {
		return;
	}
	if (len > RAMSIZE) {
		len = RAMSIZE;
	}
	for(i = 0; i < len; i++) {
		buf[i] = 0;
	}
	DS1302_ReadBurst(DS1302_RAMBURST, len, buf);	
}

/* Writes ram in burst mode 'len' bytes from 'buf' */
void DS1302_WriteRamBurst(uint8_t len, uint8_t * buf) {
	if(len <= 0) {
		return;
	}
	if (len > RAMSIZE) {
		len = RAMSIZE;
	}
	DS1302_WriteBurst(DS1302_RAMBURST, len, buf);
}
