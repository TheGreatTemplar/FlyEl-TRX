#ifndef I2C_SOFT_H
#define I2C_SOFT_H

#include "stm32f10x.h"
#include <stdint.h>

typedef enum
{
    I2C_SOFT_OK = 0,
    I2C_SOFT_ERR_NACK = -1
} i2c_soft_status_t;

void I2C_Soft_Init(void);
i2c_soft_status_t I2C_Soft_Write(uint8_t addr7, const uint8_t *data, uint16_t len);

#endif
