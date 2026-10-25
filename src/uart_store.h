#ifndef UART_STORE_H
#define UART_STORE_H
#include "uart_config.h"
/* EEPROM 0x0000/0x0040: два слота по 64 байта, версия, поколение и CRC16. */
void UARTStore_Init(void);
uint8_t UARTStore_Task(void);
uint8_t UARTStore_Busy(void);
uint8_t UARTStore_Ready(void);
/* Применить и асинхронно сохранить; при ошибке записи вернуть прежний UART. */
uint8_t UARTStore_Request(const uart_config_t *config);
const char *UARTStore_Status(void);
#endif
