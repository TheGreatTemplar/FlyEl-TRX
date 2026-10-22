#ifndef SI5351_H
#define SI5351_H

#include "stm32f10x.h"
#include <stdint.h>

typedef enum
{
    SI5351_OK = 0,
    SI5351_ERR_ARG = -1,
    SI5351_ERR_I2C = -2
} si5351_status_t;

si5351_status_t Si5351_Init(void);
si5351_status_t Si5351_SetFreq(uint8_t clk, uint32_t fout_hz);
si5351_status_t Si5351_SetVfoHighSide(uint8_t clk, uint32_t rf_hz, uint32_t if_hz);

#endif
