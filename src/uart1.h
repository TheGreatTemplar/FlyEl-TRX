#ifndef UART1_H
#define UART1_H

#include "stm32f10x.h"
#include <stdint.h>
#include "uart_config.h"

void UART1_Init(uint32_t baudrate);
/* Применение из основного цикла после завершения TX; тайм-аут не меняет регистры. */
uint8_t UART1_ApplyConfig(const uart_config_t *config);
const uart_config_t *UART1_GetConfig(void);
/* Передача слова, в том числе 9-битного при 9N. */
void UART1_SendWord(uint16_t word);
void UART1_SendChar(char ch);
void UART1_SendString(const char *text);
void UART1_SendU32(uint32_t value);

#endif
