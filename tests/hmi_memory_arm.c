#include "memory_arm.c"

int test_hmi_verified_persistence_preserves_uart_contrast_and_old_format(void)
{
    hmi_config_t config;
    uart_config_t uart = {9600U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    uint32_t applies;
    uint16_t offset;
    uint16_t crc;
    InitChip();
    RunBoth(5U);
    CHECK(UARTStore_Request(&uart) != 0U);
    RunBoth(12U);
    CHECK(UARTStore_RequestContrast(27U) != 0U);
    RunBoth(12U);
    HMI_Defaults(&config);
    config.debounce_ms = 25U;
    config.long_ms = 1500U;
    config.func_seconds = 7U;
    config.encoder_steps = 2U;
    config.encoder_reverse = 1U;
    config.encoder_filter = 1U;
    applies = uart_apply_calls;
    CHECK(UARTStore_RequestHMI(&config) != 0U);
    CHECK(HMI_Get()->long_ms == 1000U);
    RunBoth(12U);
    CHECK(HMI_Get()->long_ms == 1500U && uart_apply_calls == applies);
    UARTStore_Init();
    RunBoth(5U);
    CHECK(memcmp(HMI_Get(), &config, sizeof(config)) == 0);
    CHECK(LCD_GetContrast() == 27U && uart_actual.baud == 9600U);
    CHECK(UARTStore_RequestContrast(31U) != 0U);
    RunBoth(12U);
    uart.baud = 38400U;
    CHECK(UARTStore_Request(&uart) != 0U);
    RunBoth(12U);
    CHECK(memcmp(HMI_Get(), &config, sizeof(config)) == 0);
    offset = (uint16_t)((uint16_t)uart_store_slot * 64U);
    chip[offset + 16U] = 0xFFU;
    crc = AT24C256_Crc16Update(0xFFFFU, &chip[offset], 62U);
    chip[offset + 62U] = (uint8_t)crc;
    chip[offset + 63U] = (uint8_t)(crc >> 8U);
    UARTStore_Init();
    RunBoth(5U);
    CHECK(HMI_Get()->long_ms == 1000U && HMI_Get()->encoder_steps == 4U);
    CHECK(LCD_GetContrast() == 31U && uart_actual.baud == 38400U);
    return 0;
}


int test_hmi_invalid_record_uses_previous_settings_copy(void)
{
    hmi_config_t config;
    uint16_t offset;
    uint16_t crc;
    uint8_t record[64];
    InitChip();
    RunBoth(5U);
    config = *HMI_Get();
    config.encoder_reverse = 1U;
    CHECK(UARTStore_RequestHMI(&config) != 0U);
    RunBoth(12U);
    config.debounce_ms = 25U;
    CHECK(UARTStore_RequestHMI(&config) != 0U);
    RunBoth(12U);
    offset = (uint16_t)((uint16_t)uart_store_slot * 64U);
    chip[offset + 23U] = 3U;
    crc = AT24C256_Crc16Update(0xFFFFU, &chip[offset], 62U);
    chip[offset + 62U] = (uint8_t)crc;
    chip[offset + 63U] = (uint8_t)(crc >> 8U);
    UARTStore_Init();
    RunBoth(5U);
    CHECK(HMI_Get()->encoder_reverse == 1U && HMI_Get()->debounce_ms == 10U);
    memset(record, 0xFF, sizeof(record));
    CHECK(HMI_Decode(record, &config) != 0U && config.long_ms == 1000U);
    HMI_Encode(record, &config);
    record[17] = 2U;
    CHECK(HMI_Decode(record, &config) == 0U);
    config.encoder_filter = 4U;
    CHECK(HMI_Valid(&config) == 0U);
    return 0;
}


int test_hmi_failed_write_keeps_live_config_and_factory_restores_defaults(void)
{
    hmi_config_t config;
    InitChip();
    RunBoth(5U);
    config = *HMI_Get();
    config.encoder_reverse = 1U;
    write_protected = 1U;
    CHECK(UARTStore_RequestHMI(&config) != 0U);
    RunBoth(12U);
    CHECK(strcmp(UARTStore_Status(), "EEPROM VERIFY FAIL") == 0 && HMI_Get()->encoder_reverse == 0U);
    write_protected = 0U;
    CHECK(UARTStore_RequestHMI(&config) != 0U);
    RunBoth(12U);
    CHECK(HMI_Get()->encoder_reverse == 1U);
    CHECK(EEPROMFactory_Start() != 0U);
    RunFactory(5000U);
    CHECK(EEPROMFactory_State() == EEPROM_FACTORY_DONE);
    factory_state = EEPROM_FACTORY_IDLE;
    UARTStore_Init();
    RunBoth(5U);
    CHECK(HMI_Get()->encoder_reverse == 0U && HMI_Get()->long_ms == 1000U);
    return 0;
}
