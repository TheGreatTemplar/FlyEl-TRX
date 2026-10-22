#include "at24c256.h"

/* По datasheet AT24C256C: страница 64 байта; переход через границу запрещён. */
i2c2_soft_status_t AT24C256_WritePage(uint8_t address, uint16_t offset,
                                   const uint8_t *data, uint16_t length)
{
    uint8_t packet[AT24C256_PAGE_SIZE + 2U];
    uint16_t i;
    if (address < 0x50U || address > 0x57U || data == 0 || length == 0U ||
        length > AT24C256_PAGE_SIZE || (uint32_t)offset + length > AT24C256_CAPACITY ||
        (offset % AT24C256_PAGE_SIZE) + length > AT24C256_PAGE_SIZE)
        return I2C2_SOFT_ARG_ERROR;
    packet[0] = (uint8_t)(offset >> 8U);
    packet[1] = (uint8_t)offset;
    for (i = 0U; i < length; i++) packet[i + 2U] = data[i];
    return I2C2_Soft_Write(address, packet, (uint16_t)(length + 2U));
}

/* Ищет первый ответивший адрес 0x50–0x57; таймаут не трактуется как отсутствие устройства. */
i2c2_soft_status_t AT24C256_Find(uint8_t *address)
{
    uint8_t candidate;
    i2c2_soft_status_t status;
    if (address == 0) return I2C2_SOFT_ARG_ERROR;
    for (candidate = 0x50U; candidate <= 0x57U; candidate++)
    {
        status = I2C2_Soft_Probe(candidate);
        if (status == I2C2_SOFT_OK)
        {
            *address = candidate;
            return I2C2_SOFT_OK;
        }
        if (status != I2C2_SOFT_NACK) return status;
    }
    return I2C2_SOFT_NACK;
}

/* Проверяет границы 32 КБ и читает блок по двухбайтному адресу без записи содержимого. */
i2c2_soft_status_t AT24C256_Read(uint8_t address, uint16_t offset, uint8_t *data, uint16_t length)
{
    uint8_t prefix[2];
    if (address < 0x50U || address > 0x57U || data == 0 || length == 0U ||
        (uint32_t)offset + length > AT24C256_CAPACITY)
        return I2C2_SOFT_ARG_ERROR;
    prefix[0] = (uint8_t)(offset >> 8U);
    prefix[1] = (uint8_t)offset;
    return I2C2_Soft_WriteRead(address, prefix, 2U, data, length);
}

/* Добавляет блок в CRC-16/CCITT-FALSE; первый блок должен получать начальное FFFF. */
uint16_t AT24C256_Crc16Update(uint16_t crc, const uint8_t *data, uint16_t length)
{
    uint16_t i;
    uint8_t bit;
    for (i = 0U; i < length; i++)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8U);
        for (bit = 0U; bit < 8U; bit++)
            crc = (uint16_t)((crc & 0x8000U) ? ((uint32_t)crc << 1U) ^ 0x1021U : (uint32_t)crc << 1U);
    }
    return crc;
}
