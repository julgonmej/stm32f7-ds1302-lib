#ifndef _H_DS1302_H
#define _H_DS1302_H
#include "stm32f7xx_hal.h"

// Prepares the device,
// GPIO and DWT
void DS1302_Init(void);

void DS1302_ReadTime(uint8_t *buf);

void DS1302_WriteTime(uint8_t *buf); 

void writeSDA(void);

void readSDA(void);

void delayUS_DWT(uint32_t us);

void DS1302_writeRam(uint8_t addr, uint8_t val);

uint8_t DS1302_readRam(uint8_t addr);

void DS1302_clearRam(void);
#endif
