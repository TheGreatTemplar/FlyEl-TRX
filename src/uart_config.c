#include "uart_config.h"

const uint32_t uart_baud_rates[UART_BAUD_COUNT] = {1200U, 2400U, 4800U, 9600U, 19200U, 38400U, 57600U, 115200U};

uint8_t UARTConfig_Valid(const uart_config_t *config)
{
    uint8_t i;
    if (config == 0 || (config->data_bits != 8U && config->data_bits != 9U) || config->parity > UART_PARITY_ODD ||
        config->stop > UART_STOP_ONE_HALF || (config->data_bits == 9U && config->parity != UART_PARITY_NONE))
        return 0U;
    for (i = 0U; i < UART_BAUD_COUNT; i++)
        if (config->baud == uart_baud_rates[i])
            return 1U;
    return 0U;
}


const char *UARTConfig_ParityName(uint8_t parity)
{
    static const char *const names[] = {"NONE", "EVEN", "ODD"};
    return (parity <= UART_PARITY_ODD) ? names[parity] : "INVALID";
}


const char *UARTConfig_StopName(uint8_t stop)
{
    static const char *const names[] = {"1", "2", "0.5", "1.5"};
    return (stop <= UART_STOP_ONE_HALF) ? names[stop] : "INVALID";
}
