#ifndef AT24C256_H
#define AT24C256_H

#include "i2c2_soft.h"

#define AT24C256_CAPACITY 32768UL

i2c2_soft_status_t AT24C256_Find(uint8_t *address);
i2c2_soft_status_t AT24C256_Read(uint8_t address, uint16_t offset, uint8_t *data, uint16_t length);
/* CRC-16/CCITT-FALSE: initial FFFF, polynomial 1021, no final XOR. */
uint16_t AT24C256_Crc16Update(uint16_t crc, const uint8_t *data, uint16_t length);

#endif
