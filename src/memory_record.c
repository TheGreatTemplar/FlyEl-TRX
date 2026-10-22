#include "memory_record.h"
#include "app_config.h"
#include "at24c256.h"

static void Put32(uint8_t *data, uint32_t value)
{
    uint8_t i;
    for (i = 0U; i < 4U; i++) data[i] = (uint8_t)(value >> (8U * i));
}

static uint32_t Get32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}

void MemoryRecord_Encode(uint8_t *data, uint8_t channel, uint32_t sequence,
                         const radio_preset_t *preset)
{
    uint8_t i;
    uint16_t crc;
    for (i = 0U; i < MEMORY_RECORD_SIZE; i++) data[i] = 0xFFU;
    data[0] = 'F'; data[1] = 'E'; data[2] = 'L'; data[3] = '1';
    data[4] = channel;
    data[5] = (uint8_t)(preset != 0);
    Put32(&data[8], sequence);
    if (preset != 0)
    {
        Put32(&data[12], preset->hz);
        data[16] = preset->mode; data[17] = preset->ssb_usb;
        data[18] = preset->filter; data[19] = preset->band;
    }
    crc = AT24C256_Crc16Update(0xFFFFU, data, 62U);
    data[62] = (uint8_t)crc; data[63] = (uint8_t)(crc >> 8U);
}

uint8_t MemoryRecord_Decode(const uint8_t *data, uint8_t channel,
                           uint32_t *sequence, radio_preset_t *preset, uint8_t *used)
{
    uint16_t crc = AT24C256_Crc16Update(0xFFFFU, data, 62U);
    uint32_t hz = Get32(&data[12]);
    if (data[0] != 'F' || data[1] != 'E' || data[2] != 'L' || data[3] != '1' ||
        data[4] != channel || data[5] > 1U || data[62] != (uint8_t)crc ||
        data[63] != (uint8_t)(crc >> 8U)) return 0U;
    if (data[5] != 0U && (hz < RADIO_MIN_HZ || hz > RADIO_MAX_HZ ||
        data[16] >= RADIO_MODE_COUNT || data[17] > 1U ||
        data[18] >= RADIO_FILTER_COUNT || data[19] >= 9U)) return 0U;
    *sequence = Get32(&data[8]);
    *used = data[5];
    if (*used != 0U)
    {
        preset->hz = hz; preset->mode = data[16]; preset->ssb_usb = data[17];
        preset->filter = data[18]; preset->band = data[19];
    }
    return 1U;
}

/* Сравнение поколений работает и при переходе FFFFFFFF -> 0. */
uint8_t MemoryRecord_Newer(uint32_t a, uint32_t b)
{
    return (uint8_t)(a != b && (uint32_t)(a - b) < 0x80000000UL);
}
