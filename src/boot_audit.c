/* Компактный журнал: состояния копий и битовая карта ошибок вместо массива строк. */
#include "boot_audit.h"
#include "memory_record.h"
#include "app_config.h"
#include "uart_config.h"
#include "at24c256.h"
#include "text_utils.h"
#include <string.h>

enum
{
    AUDIT_UNKNOWN,
    AUDIT_EMPTY,
    AUDIT_FREE,
    AUDIT_USED,
    AUDIT_CRC,
    AUDIT_FORMAT,
    AUDIT_READ
};
static uint8_t channel_state[MEMORY_CHANNEL_COUNT][2];
static uint8_t selected_used[MEMORY_CHANNEL_COUNT];
static uint8_t uart_state[2];
static uint8_t read_errors[32];

void BootAudit_Init(void)
{
    memset(channel_state, 0, sizeof(channel_state));
    memset(selected_used, 0, sizeof(selected_used));
    memset(uart_state, 0, sizeof(uart_state));
    memset(read_errors, 0, sizeof(read_errors));
}


static uint8_t Inspect(const uint8_t *data, uint8_t channel, uint8_t uart, uint32_t *generation, uint8_t *used)
{
    uint8_t i;
    uint16_t crc;
    radio_preset_t preset;
    uart_config_t config;
    if (data == 0)
        return AUDIT_READ;
    for (i = 0U; i < 64U && data[i] == 0xFFU; i++)
    {
    }
    if (i == 64U)
        return AUDIT_EMPTY;
    crc = AT24C256_Crc16Update(0xFFFFU, data, 62U);
    if (data[62] != (uint8_t)crc || data[63] != (uint8_t)(crc >> 8U))
        return AUDIT_CRC;
    if (uart == 0U)
    {
        if (MemoryRecord_Decode(data, channel, generation, &preset, used) == 0U)
            return AUDIT_FORMAT;
        return (*used != 0U) ? AUDIT_USED : AUDIT_FREE;
    }
    if (memcmp(data, "FU01", 4U) != 0)
        return AUDIT_FORMAT;
    config.baud =
        (uint32_t)data[8] | ((uint32_t)data[9] << 8U) | ((uint32_t)data[10] << 16U) | ((uint32_t)data[11] << 24U);
    config.data_bits = data[12];
    config.parity = data[13];
    config.stop = data[14];
    config.reserved = 0U;
    return (UARTConfig_Valid(&config) != 0U) ? AUDIT_USED : AUDIT_FORMAT;
}


/* Обе копии находятся в одном блоке. Побеждает последнее поколение, включая очистку. */
void BootAudit_Block(uint16_t offset, const uint8_t *data)
{
    uint8_t slot;
    uint8_t used[2] = {0U, 0U};
    uint32_t generation[2] = {0U, 0U};
    uint8_t *states;
    uint8_t channel = 0U;
    uint8_t uart = (uint8_t)(offset == 0U);
    if (offset >= AT24C256_CAPACITY || offset % 128U != 0U)
        return;
    if (data == 0)
        read_errors[offset / 1024U] |= (uint8_t)(1U << ((offset / 128U) % 8U));
    if (uart != 0U)
        states = uart_state;
    else
    {
        if (offset < MEMORY_EEPROM_BASE || offset >= MEMORY_EEPROM_BASE + MEMORY_CHANNEL_COUNT * 128U)
            return;
        channel = (uint8_t)((offset - MEMORY_EEPROM_BASE) / 128U);
        states = channel_state[channel];
    }
    for (slot = 0U; slot < 2U; slot++)
        states[slot] =
            Inspect((data != 0) ? &data[(uint16_t)slot * 64U] : 0, channel, uart, &generation[slot], &used[slot]);
    if (uart == 0U)
    {
        slot = 0U;
        if ((states[1] == AUDIT_FREE || states[1] == AUDIT_USED) &&
            ((states[0] != AUDIT_FREE && states[0] != AUDIT_USED) ||
             MemoryRecord_Newer(generation[1], generation[0]) != 0U))
            slot = 1U;
        selected_used[channel] = used[slot];
    }
}


static uint8_t PairGood(const uint8_t *states)
{
    return (uint8_t)(states[0] == AUDIT_FREE || states[0] == AUDIT_USED || states[1] == AUDIT_FREE ||
                     states[1] == AUDIT_USED);
}


uint8_t BootAudit_Bad(void)
{
    uint8_t i;
    uint8_t count = 0U;
    for (i = 0U; i < MEMORY_CHANNEL_COUNT; i++)
        if (channel_state[i][0] != AUDIT_UNKNOWN && PairGood(channel_state[i]) == 0U &&
            !(channel_state[i][0] == AUDIT_EMPTY && channel_state[i][1] == AUDIT_EMPTY))
            count++;
    return count;
}


uint8_t BootAudit_Used(void)
{
    uint8_t i;
    uint8_t count = 0U;
    for (i = 0U; i < MEMORY_CHANNEL_COUNT; i++)
        if (PairGood(channel_state[i]) != 0U && selected_used[i] != 0U)
            count++;
    return count;
}


uint8_t BootAudit_Free(void)
{
    uint8_t i;
    uint8_t count = 0U;
    for (i = 0U; i < MEMORY_CHANNEL_COUNT; i++)
    {
        if ((channel_state[i][0] == AUDIT_EMPTY && channel_state[i][1] == AUDIT_EMPTY) ||
            (PairGood(channel_state[i]) != 0U && selected_used[i] == 0U))
            count++;
    }
    return count;
}


static void Hex4(char *text, uint16_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t i;
    for (i = 0U; i < 4U; i++)
        text[i] = hex[(value >> (12U - 4U * i)) & 15U];
    text[4] = '\0';
}


/* Генерирует строки по запросу; даже сотни ошибок занимают менее 400 байт SRAM. */
static uint16_t Journal(uint16_t wanted, char *text, uint8_t *severity)
{
    uint16_t count = 0U;
    uint16_t block;
    uint8_t item;
    uint8_t slot;
    for (block = 0U; block < 256U; block++)
    {
        if ((read_errors[block / 8U] & (uint8_t)(1U << (block % 8U))) == 0U)
            continue;
        if (count++ == wanted)
        {
            if (severity != 0)
                *severity = 2U;
            if (text != 0)
            {
                strcpy(text, "READ FAIL @");
                Hex4(&text[11], (uint16_t)(block * 128U));
            }
        }
    }
    for (item = 0U; item <= MEMORY_CHANNEL_COUNT; item++)
    {
        const uint8_t *states = (item == 0U) ? uart_state : channel_state[item - 1U];
        for (slot = 0U; slot < 3U; slot++)
        {
            uint8_t issue =
                (slot < 2U) ? (uint8_t)(states[slot] == AUDIT_CRC || states[slot] == AUDIT_FORMAT)
                            : (uint8_t)(PairGood(states) == 0U && (states[0] >= AUDIT_CRC || states[1] >= AUDIT_CRC));
            if (issue == 0U)
                continue;
            if (count++ != wanted)
                continue;
            if (severity != 0)
                *severity = PairGood(states) ? 1U : 2U;
            if (text == 0)
                continue;
            if (item == 0U)
                strcpy(text, "UART");
            else
            {
                strcpy(text, "CH ");
                Text_U32ToDec(&text[3], item, 2U);
            }
            if (slot == 2U)
                Text_Append(text, " NO VALID COPY");
            else
            {
                Text_Append(text, (slot == 0U) ? " A: " : " B: ");
                if (PairGood(states) != 0U)
                    Text_Append(text, (states[slot] == AUDIT_CRC) ? "CRC WARN" : "FORMAT WARN");
                else
                    Text_Append(text, (states[slot] == AUDIT_CRC) ? "CRC ERROR" : "FORMAT ERR");
            }
        }
    }
    return count;
}


uint16_t BootAudit_Count(void)
{
    return Journal(0U, 0, 0);
}


void BootAudit_Line(uint16_t index, char *text)
{
    text[0] = '\0';
    (void)Journal(index, text, 0);
}


uint8_t BootAudit_Severity(uint16_t index)
{
    uint8_t severity = 0U;
    (void)Journal(index, 0, &severity);
    return severity;
}
