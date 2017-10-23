#ifndef _H_DS1302_H
#define _H_DS1302_H
#include "stm32f7xx_hal.h"
void DS1302_Init(void);
void DS1302_ReadTime(unsigned char *buf);
void DS1302_WriteTime(unsigned char *buf); 
void writeSDA(void);
void readSDA(void);
void delayUS_DWT(uint32_t us);
#endif
