/* Реальные драйвер/формат/автомат EEPROM, шина подменена моделью 32 КБ.
 * Сценарии выполняются машинным кодом Cortex-M3 в Unicorn. */
#include "at24c256.c"
#include "memory_record.c"
#include "memory_store.c"
#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static uint8_t chip[AT24C256_CAPACITY];
static uint32_t test_tick;
static uint8_t absent;
static uint8_t write_protected;
static uint8_t partial_write;
static uint8_t startup_busy;
static uint16_t pending_address;
static uint32_t writes;
static uint32_t ready_at;

uint32_t App_GetTick500us(void) { return test_tick; }
uint8_t EEPROM_TestBusy(void) { return startup_busy; }

i2c2_soft_status_t I2C2_Soft_Probe(uint8_t device)
{
    return (device == 0x50U && absent == 0U && test_tick >= ready_at)
            ? I2C2_SOFT_OK : I2C2_SOFT_NACK;
}

i2c2_soft_status_t I2C2_Soft_WriteRead(uint8_t device, const uint8_t *prefix,
                                    uint16_t prefix_len, uint8_t *data, uint16_t length)
{
    uint16_t offset = (uint16_t)(((uint16_t)prefix[0] << 8U) | prefix[1]);
    (void)prefix_len;
    if (I2C2_Soft_Probe(device) != I2C2_SOFT_OK) return I2C2_SOFT_NACK;
    memcpy(data, &chip[offset], length);
    return I2C2_SOFT_OK;
}

i2c2_soft_status_t I2C2_Soft_Write(uint8_t device, const uint8_t *data, uint16_t length)
{
    if (I2C2_Soft_Probe(device) != I2C2_SOFT_OK) return I2C2_SOFT_NACK;
    pending_address = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
    if (write_protected == 0U)
        memcpy(&chip[pending_address], &data[2], (partial_write != 0U) ? 20U : (uint32_t)length - 2U);
    writes++;
    ready_at = test_tick + 10U;
    return I2C2_SOFT_OK;
}

static void Run(uint32_t count)
{
    uint32_t i;
    for (i = 0U; i < count; i++) { (void)MemoryStore_Task(); test_tick += 2U; }
}

static void InitChip(void)
{
    memset(chip, 0xFF, sizeof(chip));
    test_tick = 0U; writes = 0U; absent = 0U; write_protected = 0U;
    partial_write = 0U; startup_busy = 0U; ready_at = 0U;
    MemoryStore_Init();
    Run(205U);
}

int test_memory_save_reload_and_clear(void)
{
    uint8_t saved_channel;
    radio_preset_t preset = {21469780U, RADIO_USB, 1U, 2U, 6U};
    radio_preset_t result;
    InitChip();
    CHECK(MemoryStore_Ready() != 0U && writes == 0U && MemoryStore_Get(0U, 0) == 0U);
    CHECK(MemoryStore_Request(0U, &preset) != 0U);
    CHECK(MemoryStore_TakeSavedChannel(&saved_channel) == 0U);
    CHECK(MemoryStore_Get(0U, 0) == 0U); /* Нельзя считать ещё не проверенную запись сохранённой. */
    Run(10U);
    CHECK(MemoryStore_Get(0U, &result) != 0U && result.hz == preset.hz && result.mode == RADIO_USB);
    CHECK(result.filter == 2U && writes == 1U);
    CHECK(MemoryStore_TakeSavedChannel(&saved_channel) != 0U && saved_channel == 0U);
    CHECK(MemoryStore_TakeSavedChannel(&saved_channel) == 0U);
    MemoryStore_Init(); Run(205U);
    CHECK(MemoryStore_Get(0U, &result) != 0U && result.hz == preset.hz);
    CHECK(MemoryStore_Request(0U, 0) != 0U); Run(10U);
    CHECK(MemoryStore_Get(0U, 0) == 0U);
    CHECK(MemoryStore_TakeSavedChannel(&saved_channel) == 0U);
    MemoryStore_Init(); Run(205U);
    CHECK(MemoryStore_Get(0U, 0) == 0U && writes == 2U);
    return 0;
}

int test_memory_power_loss_keeps_old_copy(void)
{
    radio_preset_t preset = {7100000U, RADIO_LSB, 0U, 0U, 2U};
    radio_preset_t result;
    InitChip();
    CHECK(MemoryStore_Request(99U, &preset) == 0U && writes == 0U);
    CHECK(MemoryStore_Request(98U, &preset) != 0U); Run(10U);
    CHECK(MemoryStore_TakeSavedChannel(0) != 0U);
    preset.hz = 7200000U;
    partial_write = 1U;
    CHECK(MemoryStore_Request(98U, &preset) != 0U); Run(10U);
    CHECK(MemoryStore_Get(98U, &result) != 0U && result.hz == 7100000U);
    CHECK(MemoryStore_TakeSavedChannel(0) == 0U);
    MemoryStore_Init(); Run(205U);
    CHECK(MemoryStore_Get(98U, &result) != 0U && result.hz == 7100000U);
    CHECK(chip[MEMORY_EEPROM_BASE - 1U] == 0xFFU && chip[MEMORY_EEPROM_BASE + 12672U] == 0xFFU);
    return 0;
}

int test_memory_write_protection_and_absence(void)
{
    radio_preset_t preset = {7100000U, RADIO_LSB, 0U, 0U, 2U};
    InitChip(); write_protected = 1U;
    CHECK(MemoryStore_Request(0U, &preset) != 0U); Run(10U);
    CHECK(MemoryStore_Get(0U, 0) == 0U && strcmp(MemoryStore_Status(), "MEM ERR") == 0);
    CHECK(MemoryStore_TakeSavedChannel(0) == 0U);
    MemoryStore_Init(); absent = 1U; Run(205U);
    CHECK(MemoryStore_Ready() == 0U && strcmp(MemoryStore_Status(), "MEM ERR") == 0);
    return 0;
}

int test_memory_waits_for_startup_crc(void)
{
    InitChip(); MemoryStore_Init(); startup_busy = 1U;
    Run(300U); CHECK(MemoryStore_Ready() == 0U && writes == 0U);
    startup_busy = 0U; Run(205U); CHECK(MemoryStore_Ready() != 0U && writes == 0U);
    return 0;
}

int test_memory_record_rejects_corruption_and_wraps_sequence(void)
{
    radio_preset_t preset = {14200000U, RADIO_USB, 1U, 1U, 4U};
    radio_preset_t result;
    uint8_t data[64], occupied;
    uint32_t sequence;
    MemoryRecord_Encode(data, 3U, 0xFFFFFFFFU, &preset);
    CHECK(MemoryRecord_Decode(data, 3U, &sequence, &result, &occupied) != 0U);
    CHECK(sequence == 0xFFFFFFFFU && occupied == 1U && result.hz == 14200000U);
    CHECK(MemoryRecord_Decode(data, 4U, &sequence, &result, &occupied) == 0U);
    data[12] ^= 1U;
    CHECK(MemoryRecord_Decode(data, 3U, &sequence, &result, &occupied) == 0U);
    CHECK(MemoryRecord_Newer(0U, 0xFFFFFFFFU) != 0U);
    CHECK(MemoryRecord_Newer(0xFFFFFFFFU, 0U) == 0U);
    return 0;
}
