#ifndef SPI1_H
#define SPI1_H

#include "stm32f10x.h"
#include <stdint.h>

typedef enum
{
    SPI1_OK = 0,
    SPI1_ERR_TIMEOUT = -1
} spi1_status_t;

void SPI1_LCD_Init(void);
spi1_status_t SPI1_SendByte(uint8_t data);

#endif
