#ifndef SERVICE_UART_H
#define SERVICE_UART_H
#include "input.h"
#include "uart_config.h"
void ServiceUart_Open(void);
void ServiceUart_Encoder(int32_t steps);
/* 1 — отменить несохранённую копию и выйти к списку пунктов. */
uint8_t ServiceUart_Key(input_key_t key);
uint8_t ServiceUart_Item(void);
uint8_t ServiceUart_Editing(void);
const char *ServiceUart_Label(uint8_t item);
void ServiceUart_Value(uint8_t item, char *text);
const char *ServiceUart_Status(void);
#endif
