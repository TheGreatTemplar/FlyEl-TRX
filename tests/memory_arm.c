/* Реальные драйвер/формат/автомат EEPROM, шина подменена моделью 32 КБ.
 * Сценарии выполняются машинным кодом Cortex-M3 в Unicorn. */
#include "at24c256.c"
#include "memory_record.c"
#include "memory_store.c"
#include "uart_config.c"
#include "uart_store.c"
#define CHECK(expr)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
            return __LINE__;                                                                                           \
    } while (0)

static uint8_t chip[AT24C256_CAPACITY];
static uint32_t test_tick;
static uint8_t absent;
static uint8_t write_protected;
static uint8_t partial_write;
static uint8_t startup_busy;
static uint16_t pending_address;
static uint32_t writes;
static uint32_t ready_at;
static uint32_t transfer_ticks;
static uint32_t write_cycle_ticks;
static uint8_t write_bus_error;
static uart_config_t uart_actual;
const uart_config_t *UART1_GetConfig(void)
{
    return &uart_actual;
}


uint8_t UART1_ApplyConfig(const uart_config_t *config)
{
    if (UARTConfig_Valid(config) == 0U)
        return 0U;
    uart_actual = *config;
    return 1U;
}


uint32_t App_GetTick500us(void)
{
    return test_tick;
}


uint8_t EEPROM_TestBusy(void)
{
    return startup_busy;
}


i2c2_soft_status_t I2C2_Soft_Probe(uint8_t device)
{
    return (device == 0x50U && absent == 0U && test_tick >= ready_at) ? I2C2_SOFT_OK : I2C2_SOFT_NACK;
}


i2c2_soft_status_t I2C2_Soft_WriteRead(uint8_t device, const uint8_t *prefix, uint16_t prefix_len, uint8_t *data,
                                       uint16_t length)
{
    uint16_t offset = (uint16_t)(((uint16_t)prefix[0] << 8U) | prefix[1]);
    (void)prefix_len;
    if (I2C2_Soft_Probe(device) != I2C2_SOFT_OK)
        return I2C2_SOFT_NACK;
    memcpy(data, &chip[offset], length);
    return I2C2_SOFT_OK;
}


i2c2_soft_status_t I2C2_Soft_Write(uint8_t device, const uint8_t *data, uint16_t length)
{
    if (I2C2_Soft_Probe(device) != I2C2_SOFT_OK)
        return I2C2_SOFT_NACK;
    if (write_bus_error != 0U)
        return I2C2_SOFT_BUS_ERROR;
    pending_address = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
    if (write_protected == 0U)
        memcpy(&chip[pending_address], &data[2], (partial_write != 0U) ? 20U : (uint32_t)length - 2U);
    writes++;
    test_tick += transfer_ticks;
    ready_at = test_tick + write_cycle_ticks;
    return I2C2_SOFT_OK;
}


static void Run(uint32_t count)
{
    uint32_t i;
    for (i = 0U; i < count; i++)
    {
        (void)MemoryStore_Task();
        test_tick += 2U;
    }
}


static void InitChip(void)
{
    memset(chip, 0xFF, sizeof(chip));
    test_tick = 0U;
    writes = 0U;
    absent = 0U;
    write_protected = 0U;
    partial_write = 0U;
    startup_busy = 0U;
    ready_at = 0U;
    transfer_ticks = 0U;
    write_cycle_ticks = 10U;
    write_bus_error = 0U;
    MemoryStore_Init();
    UARTStore_Init();
    uart_actual = (uart_config_t){115200U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
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
    MemoryStore_Init();
    Run(205U);
    CHECK(MemoryStore_Get(0U, &result) != 0U && result.hz == preset.hz);
    CHECK(MemoryStore_Request(0U, 0) != 0U);
    Run(10U);
    CHECK(MemoryStore_Get(0U, 0) == 0U);
    CHECK(MemoryStore_TakeSavedChannel(&saved_channel) == 0U);
    MemoryStore_Init();
    Run(205U);
    CHECK(MemoryStore_Get(0U, 0) == 0U && writes == 2U);
    return 0;
}


int test_memory_power_loss_keeps_old_copy(void)
{
    radio_preset_t preset = {7100000U, RADIO_LSB, 0U, 0U, 2U};
    radio_preset_t result;
    InitChip();
    CHECK(MemoryStore_Request(99U, &preset) == 0U && writes == 0U);
    CHECK(MemoryStore_Request(98U, &preset) != 0U);
    Run(10U);
    CHECK(MemoryStore_TakeSavedChannel(0) != 0U);
    preset.hz = 7200000U;
    partial_write = 1U;
    CHECK(MemoryStore_Request(98U, &preset) != 0U);
    Run(10U);
    CHECK(MemoryStore_Get(98U, &result) != 0U && result.hz == 7100000U);
    CHECK(MemoryStore_TakeSavedChannel(0) == 0U);
    MemoryStore_Init();
    Run(205U);
    CHECK(MemoryStore_Get(98U, &result) != 0U && result.hz == 7100000U);
    CHECK(chip[MEMORY_EEPROM_BASE - 1U] == 0xFFU && chip[MEMORY_EEPROM_BASE + 12672U] == 0xFFU);
    return 0;
}


int test_memory_write_protection_and_absence(void)
{
    radio_preset_t preset = {7100000U, RADIO_LSB, 0U, 0U, 2U};
    InitChip();
    write_protected = 1U;
    CHECK(MemoryStore_Request(0U, &preset) != 0U);
    Run(10U);
    CHECK(MemoryStore_Get(0U, 0) == 0U && strcmp(MemoryStore_Status(), "MEM ERR") == 0);
    CHECK(MemoryStore_TakeSavedChannel(0) == 0U);
    MemoryStore_Init();
    absent = 1U;
    Run(205U);
    CHECK(MemoryStore_Ready() == 0U && strcmp(MemoryStore_Status(), "MEM ERR") == 0);
    return 0;
}


int test_memory_waits_for_startup_crc(void)
{
    InitChip();
    MemoryStore_Init();
    startup_busy = 1U;
    Run(300U);
    CHECK(MemoryStore_Ready() == 0U && writes == 0U);
    startup_busy = 0U;
    Run(205U);
    CHECK(MemoryStore_Ready() != 0U && writes == 0U);
    return 0;
}


static void RunBoth(uint32_t count)
{
    uint32_t i;
    for (i = 0U; i < count; i++)
    {
        (void)UARTStore_Task();
        (void)MemoryStore_Task();
        test_tick += 2U;
    }
}


int test_uart_eeprom_defaults_save_reload_and_channel_isolation(void)
{
    uart_config_t config = {9600U, 8U, UART_PARITY_EVEN, UART_STOP_TWO, 0U};
    radio_preset_t preset = {7100000U, RADIO_LSB, 0U, 0U, 2U}, result;
    InitChip();
    RunBoth(5U);
    CHECK(UARTStore_Ready() != 0U && writes == 0U && uart_actual.baud == 115200U);
    CHECK(MemoryStore_Request(0U, &preset) != 0U);
    CHECK(UARTStore_Request(&config) == 0U && uart_actual.baud == 115200U);
    RunBoth(12U);
    CHECK(MemoryStore_Get(0U, &result) != 0U);
    CHECK(UARTStore_Request(&config) != 0U && UARTStore_Busy() != 0U);
    CHECK(MemoryStore_Request(1U, &preset) == 0U);
    RunBoth(12U);
    CHECK(UARTStore_Busy() == 0U && strcmp(UARTStore_Status(), "SAVED EEPROM") == 0);
    CHECK(MemoryStore_Get(0U, &result) != 0U && result.hz == 7100000U && writes == 2U);
    CHECK(chip[128U] == 0xFFU && chip[MEMORY_EEPROM_BASE - 1U] == 0xFFU);
    uart_actual = (uart_config_t){115200U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    UARTStore_Init();
    RunBoth(5U);
    CHECK(uart_actual.baud == 9600U && uart_actual.parity == UART_PARITY_EVEN && uart_actual.stop == UART_STOP_TWO);
    CHECK(writes == 2U);
    return 0;
}


int test_uart_eeprom_partial_write_and_wp_keep_old_copy_and_uart(void)
{
    uart_config_t config = {19200U, 8U, UART_PARITY_ODD, UART_STOP_ONE, 0U};
    InitChip();
    RunBoth(5U);
    CHECK(UARTStore_Request(&config) != 0U);
    RunBoth(12U);
    config.baud = 1200U;
    partial_write = 1U;
    CHECK(UARTStore_Request(&config) != 0U);
    RunBoth(12U);
    CHECK(strcmp(UARTStore_Status(), "EEPROM VERIFY FAIL") == 0 && uart_actual.baud == 19200U);
    UARTStore_Init();
    RunBoth(5U);
    CHECK(uart_actual.baud == 19200U);
    partial_write = 0U;
    write_protected = 1U;
    CHECK(UARTStore_Request(&config) != 0U);
    RunBoth(12U);
    CHECK(strcmp(UARTStore_Status(), "EEPROM VERIFY FAIL") == 0 && uart_actual.baud == 19200U);
    UARTStore_Init();
    RunBoth(5U);
    CHECK(uart_actual.baud == 19200U);
    return 0;
}


int test_uart_eeprom_power_loss_crc_and_startup_arbitration(void)
{
    uart_config_t config = {38400U, 9U, UART_PARITY_NONE, UART_STOP_HALF, 0U};
    InitChip();
    UARTStore_Init();
    startup_busy = 1U;
    RunBoth(30U);
    CHECK(UARTStore_Ready() == 0U && writes == 0U);
    startup_busy = 0U;
    RunBoth(5U);
    CHECK(UARTStore_Request(&config) != 0U);
    RunBoth(12U);
    config.baud = 57600U;
    partial_write = 1U;
    CHECK(UARTStore_Request(&config) != 0U);
    (void)UARTStore_Task();
    test_tick = ready_at;
    UARTStore_Init();
    RunBoth(5U);
    CHECK(uart_actual.baud == 38400U);
    /* Повредить единственную валидную копию: остаются безопасные начальные настройки. */
    chip[64U + 8U] ^= 1U;
    uart_actual = (uart_config_t){115200U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    UARTStore_Init();
    RunBoth(5U);
    CHECK(uart_actual.baud == 115200U);
    return 0;
}


int test_uart_save_waits_from_stop_after_slow_i2c_page_transfer(void)
{
    uart_config_t config = {9600U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    InitChip();
    RunBoth(5U);
    transfer_ticks = 60U; /* Передача страницы занимает 30 мс: больше ACK-тайм-аута. */
    CHECK(UARTStore_Request(&config) != 0U);
    RunBoth(12U);
    CHECK(UARTStore_Busy() == 0U && strcmp(UARTStore_Status(), "SAVED EEPROM") == 0);
    CHECK(uart_actual.baud == 9600U);
    UARTStore_Init();
    RunBoth(5U);
    CHECK(uart_actual.baud == 9600U);
    return 0;
}


int test_uart_save_accepts_ready_eeprom_after_delayed_main_loop(void)
{
    uart_config_t config = {1200U, 8U, UART_PARITY_ODD, UART_STOP_TWO, 0U};
    InitChip();
    RunBoth(5U);
    CHECK(UARTStore_Request(&config) != 0U);
    (void)UARTStore_Task(); /* Передача страницы и STOP. */
    test_tick += 200U; /* Перерисовка/другая задача заняла 100 мс. EEPROM уже готова. */
    (void)UARTStore_Task();
    (void)UARTStore_Task();
    CHECK(UARTStore_Busy() == 0U && strcmp(UARTStore_Status(), "SAVED EEPROM") == 0);
    CHECK(uart_actual.baud == 1200U);
    return 0;
}


int test_uart_save_reports_real_ack_timeout_write_error_and_missing_chip(void)
{
    uart_config_t config = {2400U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    InitChip();
    RunBoth(5U);
    write_cycle_ticks = 1000U;
    CHECK(UARTStore_Request(&config) != 0U);
    RunBoth(30U);
    CHECK(strcmp(UARTStore_Status(), "EEPROM ACK TIMEOUT") == 0 && uart_actual.baud == 115200U);
    InitChip();
    RunBoth(5U);
    write_bus_error = 1U;
    CHECK(UARTStore_Request(&config) != 0U);
    RunBoth(2U);
    CHECK(strcmp(UARTStore_Status(), "EEPROM WRITE FAIL") == 0 && uart_actual.baud == 115200U);
    InitChip();
    absent = 1U;
    RunBoth(5U);
    CHECK(strcmp(UARTStore_Status(), "EEPROM NOT FOUND") == 0);
    CHECK(UARTStore_Request(&config) == 0U && strcmp(UARTStore_Status(), "EEPROM NOT FOUND") == 0);
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
