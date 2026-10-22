#ifndef UART1_H
#define UART1_H

#include "stm32f10x.h"
#include <stdint.h>

void UART1_Init(uint32_t baudrate);
void UART1_SendChar(char ch);
void UART1_SendString(const char *text);
void UART1_SendU32(uint32_t value);

#endif
