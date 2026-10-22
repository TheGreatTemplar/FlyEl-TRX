#ifndef I2C2_SOFT_H
#define I2C2_SOFT_H

#include <stdint.h>

typedef enum
{
    I2C2_SOFT_OK = 0,
    I2C2_SOFT_NACK = -1,
    I2C2_SOFT_TIMEOUT = -2,
    I2C2_SOFT_BUS_ERROR = -3,
    I2C2_SOFT_ARG_ERROR = -4
} i2c2_soft_status_t;

void I2C2_Soft_Init(void);
i2c2_soft_status_t I2C2_Soft_Write(uint8_t address, const uint8_t *data, uint16_t length);
i2c2_soft_status_t I2C2_Soft_Probe(uint8_t address);
i2c2_soft_status_t I2C2_Soft_WriteRead(uint8_t address, const uint8_t *prefix,
                                    uint16_t prefix_len, uint8_t *data, uint16_t length);

#endif
