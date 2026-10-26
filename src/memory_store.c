/* Два независимых 64-байтных слота на канал. Старая копия не переписывается,
 * пока новая не прочитана и не проверена. Чужие области EEPROM не форматируются. */
#include "memory_store.h"
#include "memory_record.h"
#include "app_config.h"
#include "app_time.h"
#include "eeprom_startup.h"
#include "at24c256.h"
#include <string.h>
#include "uart_store.h"

#if MEMORY_CHANNEL_COUNT > 99U || MEMORY_CHANNEL_COUNT < 1U
#error MEMORY_CHANNEL_COUNT must be in 1..99
#endif
#if MEMORY_EEPROM_BASE + MEMORY_CHANNEL_COUNT * 128U > 32768U
#error Channel storage exceeds EEPROM
#endif

enum
{
    STORE_WAIT_CRC,
    STORE_FIND,
    STORE_LOAD_A,
    STORE_LOAD_B,
    STORE_IDLE,
    STORE_WRITE,
    STORE_WAIT_WRITE,
    STORE_VERIFY,
    STORE_ERROR
};
static radio_preset_t presets[MEMORY_CHANNEL_COUNT];
static uint32_t generations[MEMORY_CHANNEL_COUNT];
static uint8_t used[MEMORY_CHANNEL_COUNT];
static uint8_t slots[MEMORY_CHANNEL_COUNT];
static uint8_t state;
static uint8_t address;
static uint8_t load_channel;
static uint8_t load_a_valid;
static uint8_t loaded;
static uint8_t write_channel;
static uint8_t write_slot;
static uint8_t saved_channel;
static uint8_t saved_event;
static uint8_t pending[MEMORY_RECORD_SIZE];
static uint32_t wait_tick;
static uint32_t poll_tick;
static uint32_t message_tick;
static const char *message;

static uint16_t Offset(uint8_t channel, uint8_t slot)
{
    return (uint16_t)(MEMORY_EEPROM_BASE + (uint32_t)channel * 128U + (uint32_t)slot * 64U);
}


static void Message(const char *text)
{
    message = text;
    message_tick = App_GetTick500us();
}


void MemoryStore_Init(void)
{
    memset(used, 0, sizeof(used));
    memset(generations, 0, sizeof(generations));
    memset(slots, 0, sizeof(slots));
    state = STORE_WAIT_CRC;
    loaded = 0U;
    load_channel = 0U;
    message = "";
    saved_event = 0U;
}


static void Fail(void)
{
    /* Не выдаём неподтверждённое содержимое за сохранённый канал. */
    state = (loaded != 0U) ? STORE_IDLE : STORE_ERROR;
    Message("MEM ERR");
}


static uint8_t LoadSlot(void)
{
    uint8_t data[MEMORY_RECORD_SIZE];
    radio_preset_t preset;
    uint32_t generation;
    uint8_t occupied;
    uint8_t slot = (uint8_t)(state == STORE_LOAD_B);
    uint8_t valid;
    if (AT24C256_Read(address, Offset(load_channel, slot), data, sizeof(data)) != I2C2_SOFT_OK)
    {
        Fail();
        return 1U;
    }
    valid = MemoryRecord_Decode(data, load_channel, &generation, &preset, &occupied);
    if (slot == 0U)
        load_a_valid = valid;
    if (valid != 0U &&
        (slot == 0U || load_a_valid == 0U || MemoryRecord_Newer(generation, generations[load_channel]) != 0U))
    {
        generations[load_channel] = generation;
        used[load_channel] = occupied;
        slots[load_channel] = slot;
        if (occupied != 0U)
            presets[load_channel] = preset;
    }
    if (slot == 0U)
        state = STORE_LOAD_B;
    else
    {
        load_channel++;
        if (load_channel == MEMORY_CHANNEL_COUNT)
        {
            loaded = 1U;
            state = STORE_IDLE;
            return 1U;
        }
        state = STORE_LOAD_A;
    }
    return 0U;
}


static uint8_t VerifyWrite(void)
{
    uint8_t data[MEMORY_RECORD_SIZE];
    radio_preset_t preset;
    uint32_t generation;
    uint8_t occupied;
    if (AT24C256_Read(address, Offset(write_channel, write_slot), data, sizeof(data)) != I2C2_SOFT_OK ||
        memcmp(data, pending, sizeof(data)) != 0 ||
        MemoryRecord_Decode(data, write_channel, &generation, &preset, &occupied) == 0U)
    {
        Fail();
        return 1U;
    }
    generations[write_channel] = generation;
    slots[write_channel] = write_slot;
    used[write_channel] = occupied;
    if (occupied != 0U)
        presets[write_channel] = preset;
    state = STORE_IDLE;
    Message((occupied != 0U) ? "SAVED" : "BLANK");
    if (occupied != 0U)
    {
        saved_channel = write_channel;
        saved_event = 1U;
    }
    return 1U;
}


uint8_t MemoryStore_Task(void)
{
    uint32_t now = App_GetTick500us();
    i2c2_soft_status_t status;
    if (UARTStore_Busy() != 0U)
        return 0U;
    if (message[0] != '\0' && (uint32_t)(now - message_tick) >= 6000U)
    {
        message = "";
        return 1U;
    }
    switch (state)
    {
        case STORE_WAIT_CRC:
            if (EEPROM_TestBusy() == 0U)
                state = STORE_FIND;
            break;
        case STORE_FIND:
            if (AT24C256_Find(&address) != I2C2_SOFT_OK)
            {
                Fail();
                return 1U;
            }
            state = STORE_LOAD_A;
            break;
        case STORE_LOAD_A:
        case STORE_LOAD_B:
            return LoadSlot();
        case STORE_WRITE:
            if (AT24C256_WritePage(address, Offset(write_channel, write_slot), pending, sizeof(pending)) !=
                I2C2_SOFT_OK)
            {
                Fail();
                return 1U;
            }
            wait_tick = App_GetTick500us();
            poll_tick = wait_tick;
            state = STORE_WAIT_WRITE;
            break;
        case STORE_WAIT_WRITE:
            if ((uint32_t)(now - wait_tick) >= 40U)
            {
                Fail();
                return 1U;
            }
            /* Максимум один ACK-poll за миллисекунду, без задержки основного цикла. */
            if ((uint32_t)(now - wait_tick) < 10U || (uint32_t)(now - poll_tick) < 2U)
                break;
            poll_tick = now;
            status = I2C2_Soft_Probe(address);
            if (status == I2C2_SOFT_OK)
                state = STORE_VERIFY;
            else if (status != I2C2_SOFT_NACK)
            {
                Fail();
                return 1U;
            }
            break;
        case STORE_VERIFY:
            return VerifyWrite();
        default:
            break;
    }
    return 0U;
}


uint8_t MemoryStore_Ready(void)
{
    return loaded;
}


uint8_t MemoryStore_Busy(void)
{
    return (uint8_t)(state == STORE_WRITE || state == STORE_WAIT_WRITE || state == STORE_VERIFY);
}


uint8_t MemoryStore_Get(uint8_t channel, radio_preset_t *preset)
{
    if (loaded == 0U || channel >= MEMORY_CHANNEL_COUNT || used[channel] == 0U)
        return 0U;
    if (preset != 0)
        *preset = presets[channel];
    return 1U;
}


uint8_t MemoryStore_Request(uint8_t channel, const radio_preset_t *preset)
{
    if (UARTStore_Busy() != 0U)
    {
        Message("MEM BUSY");
        return 0U;
    }
    if (loaded == 0U || state != STORE_IDLE || channel >= MEMORY_CHANNEL_COUNT)
    {
        Message((state == STORE_ERROR) ? "MEM ERR" : ((loaded == 0U) ? "MEM WAIT" : "MEM BUSY"));
        return 0U;
    }
    write_channel = channel;
    write_slot = (uint8_t)(slots[channel] ^ 1U);
    MemoryRecord_Encode(pending, channel, generations[channel] + 1U, preset);
    state = STORE_WRITE;
    Message("WRITING");
    return 1U;
}


uint8_t MemoryStore_TakeSavedChannel(uint8_t *channel)
{
    if (saved_event == 0U)
        return 0U;
    if (channel != 0)
        *channel = saved_channel;
    saved_event = 0U;
    return 1U;
}


const char *MemoryStore_Status(void)
{
    if (message[0] != '\0')
        return message;
    if (state == STORE_ERROR)
        return "MEM ERR";
    if (loaded == 0U)
        return "MEM LOAD";
    return "";
}
