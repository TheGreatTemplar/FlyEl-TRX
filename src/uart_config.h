#ifndef UART_CONFIG_H
#define UART_CONFIG_H
#include <stdint.h>

enum
{
    UART_PARITY_NONE,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
};
enum
{
    UART_STOP_ONE,
    UART_STOP_TWO,
    UART_STOP_HALF,
    UART_STOP_ONE_HALF
};
enum
{
    UART_BAUD_COUNT = 8U
};
typedef struct
{
    uint32_t baud;
    uint8_t data_bits;
    uint8_t parity;
    uint8_t stop;
    uint8_t reserved;
} uart_config_t;
extern const uint32_t uart_baud_rates[UART_BAUD_COUNT];
/* data_bits — полезные биты, без чётности; STM32F1 не поддерживает 9 данных + parity. */
uint8_t UARTConfig_Valid(const uart_config_t *config);
const char *UARTConfig_ParityName(uint8_t parity);
const char *UARTConfig_StopName(uint8_t stop);
#endif
