/* Реальный код стартового чтения/CRC/журнала; только шина и экран заменены моделью. */
#include "at24c256.c"
#include "memory_record.c"
#include "uart_config.c"
#include "text_utils.c"
#include "eeprom_startup.c"
#include "boot_audit.c"
#include "boot_startup.c"
#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
            return __LINE__;                                                                                           \
    } while (0)

generator_menu_t menu;
static uint8_t boot_chip[32768];
static uint32_t boot_tick;
static uint32_t boot_reads;
static uint16_t boot_fail_offset;
static uint8_t boot_absent;
static uint8_t boot_uart_ready;
static uint8_t boot_mem_ready;
static uint8_t boot_confirmed;
static uint32_t boot_spinners;
static uint8_t boot_journal_frames;
static char boot_lines[7][32];
static char boot_headers[4][32];
static uint8_t boot_saw_ready;
static uint8_t boot_saw_unread;

uint32_t App_GetTick500us(void)
{
    return boot_tick;
}


void App_DelayMs(uint32_t ms)
{
    boot_tick += ms * 2U;
}


i2c2_soft_status_t I2C2_Soft_Probe(uint8_t address)
{
    return (address == 0x50U && boot_absent == 0U) ? I2C2_SOFT_OK : I2C2_SOFT_NACK;
}


i2c2_soft_status_t I2C2_Soft_WriteRead(uint8_t address, const uint8_t *prefix, uint16_t prefix_len, uint8_t *data,
                                       uint16_t length)
{
    uint16_t offset = (uint16_t)(((uint16_t)prefix[0] << 8U) | prefix[1]);
    (void)address;
    (void)prefix_len;
    boot_tick += 20U;
    boot_reads++;
    if (offset == boot_fail_offset)
        return I2C2_SOFT_BUS_ERROR;
    memcpy(data, &boot_chip[offset], length);
    return I2C2_SOFT_OK;
}


i2c2_soft_status_t I2C2_Soft_Write(uint8_t address, const uint8_t *data, uint16_t length)
{
    (void)address;
    (void)data;
    (void)length;
    return I2C2_SOFT_BUS_ERROR;
}


uint8_t UARTStore_Task(void)
{
    return 0U;
}


uint8_t MemoryStore_Task(void)
{
    return 0U;
}


uint8_t UARTStore_Ready(void)
{
    return boot_uart_ready;
}


uint8_t MemoryStore_Ready(void)
{
    return boot_mem_ready;
}


const char *UARTStore_Status(void)
{
    return "EEPROM NOT FOUND";
}


uint8_t Input_KeyIsDown(input_key_t key)
{
    (void)key;
    return 0U;
}


uint8_t Input_KeyPressed(input_key_t key)
{
    if (key == Key_5_3 && boot_journal_frames >= 4U && boot_confirmed == 0U)
    {
        boot_confirmed = 1U;
        return 1U;
    }
    return 0U;
}


int32_t Input_GetEncoderSteps(void)
{
    if (boot_journal_frames == 1U)
        return 1000;
    if (boot_journal_frames == 2U)
        return -1000;
    if (boot_journal_frames == 3U)
        return 2;
    return 0;
}


void BootUI_Clear(void)
{
    memset(boot_lines, 0, sizeof(boot_lines));
}


void BootUI_Line(uint8_t row, const char *text, uint8_t inverse)
{
    (void)inverse;
    if (row < 7U)
        strcpy(boot_lines[row], text);
    if (strcmp(text, "FLYEL BOOT READY") == 0)
        boot_saw_ready = 1U;
    if (strcmp(text, "CHANNELS NOT READ") == 0)
        boot_saw_unread = 1U;
}


void BootUI_Spinner(uint8_t row, uint32_t tick)
{
    (void)row;
    (void)tick;
    boot_spinners++;
}


void BootUI_Update(void)
{
    if (strcmp(boot_lines[6], "ENC:LOG F-INP:GO") == 0 && boot_journal_frames < 4U)
    {
        strcpy(boot_headers[boot_journal_frames], boot_lines[0]);
        boot_journal_frames++;
    }
}


static void BootFixture(void)
{
    memset(boot_chip, 0xFF, sizeof(boot_chip));
    memset(&menu, 0, sizeof(menu));
    menu.si5351_ok = 1U;
    boot_tick = 0U;
    boot_reads = 0U;
    boot_fail_offset = 0xFFFFU;
    boot_absent = 0U;
    boot_uart_ready = 1U;
    boot_mem_ready = 1U;
    boot_confirmed = 0U;
    boot_spinners = 0U;
    boot_journal_frames = 0U;
    boot_saw_ready = 0U;
    boot_saw_unread = 0U;
    BootStartup_Begin();
    EEPROM_TestInit();
}


static void ScanBoot(void)
{
    while (EEPROM_TestBusy() != 0U)
        (void)EEPROM_TestTask();
}


int test_boot_empty_channels_and_crc(void)
{
    BootFixture();
    BootStartup_Run();
    CHECK(boot_reads == 256U);
    CHECK(BootAudit_Free() == 99U && BootAudit_Used() == 0U && BootAudit_Bad() == 0U);
    CHECK(BootAudit_Count() == 0U && boot_journal_frames == 0U && boot_saw_ready != 0U);
    CHECK(boot_spinners > 2U && boot_tick == 5120U + 1200U);
    CHECK(EEPROM_TestStatus()[0] == '\0' && eeprom_observer == 0);
    return 0;
}


int test_boot_selects_newest_copy_and_tombstone(void)
{
    radio_preset_t preset = {0};
    BootFixture();
    preset.hz = 7100000U;
    MemoryRecord_Encode(&boot_chip[0x4000], 0U, 0xFFFFFFFFU, &preset);
    MemoryRecord_Encode(&boot_chip[0x4040], 0U, 0U, 0);
    MemoryRecord_Encode(&boot_chip[0x4080], 1U, 1U, &preset);
    boot_chip[0x40C0] = 0U;
    ScanBoot();
    CHECK(BootAudit_Used() == 1U && BootAudit_Free() == 98U && BootAudit_Bad() == 0U);
    CHECK(BootAudit_Count() == 1U);
    {
        char line[32];
        BootAudit_Line(0U, line);
        CHECK(strcmp(line, "CH 02 B: CRC WARN") == 0);
        CHECK(BootAudit_Severity(0U) == 1U);
    }
    return 0;
}


int test_boot_block_read_failure_continues_and_invalidates_crc(void)
{
    char line[32];
    BootFixture();
    boot_fail_offset = 0x4080U;
    ScanBoot();
    CHECK(boot_reads == 256U && EEPROM_TestProgress() == 32768U);
    CHECK(strcmp(EEPROM_TestStatus(), "MEM ERR") == 0);
    CHECK(BootAudit_Free() == 98U && BootAudit_Bad() == 1U);
    CHECK(BootAudit_Count() == 2U);
    BootAudit_Line(0U, line);
    CHECK(strcmp(line, "READ FAIL @4080") == 0);
    BootAudit_Line(1U, line);
    CHECK(strcmp(line, "CH 02 NO VALID COPY") == 0);
    CHECK(BootAudit_Severity(0U) == 2U && BootAudit_Severity(1U) == 2U);
    return 0;
}


int test_boot_large_journal_scrolls_and_confirms(void)
{
    BootFixture();
    memset(boot_chip, 0, sizeof(boot_chip));
    BootStartup_Run();
    CHECK(BootAudit_Count() == 300U && BootAudit_Bad() == 99U);
    CHECK(BootAudit_Free() == 0U && BootAudit_Used() == 0U);
    CHECK(boot_confirmed != 0U && boot_saw_ready == 0U);
    CHECK(strcmp(boot_headers[0], "BOOT LOG 1/300") == 0);
    CHECK(strcmp(boot_headers[1], "BOOT LOG 300/300") == 0);
    CHECK(strcmp(boot_headers[2], "BOOT LOG 1/300") == 0);
    CHECK(strcmp(boot_headers[3], "BOOT LOG 3/300") == 0);
    return 0;
}


int test_boot_missing_eeprom_is_not_99_empty_channels(void)
{
    BootFixture();
    boot_absent = 1U;
    boot_uart_ready = 0U;
    boot_mem_ready = 0U;
    menu.si5351_ok = 0U;
    BootStartup_Run();
    CHECK(boot_reads == 0U && boot_confirmed != 0U && boot_saw_unread != 0U);
    CHECK(BootAudit_Free() == 0U && BootAudit_Bad() == 0U);
    CHECK(strcmp(boot_headers[0], "BOOT LOG 1/4") == 0);
    return 0;
}


int test_boot_all_block_errors_fit_compact_journal(void)
{
    uint16_t i;
    char line[32];
    BootFixture();
    for (i = 0U; i < 256U; i++)
        BootAudit_Block((uint16_t)(i * 128U), 0);
    CHECK(BootAudit_Count() == 356U);
    BootAudit_Line(255U, line);
    CHECK(strcmp(line, "READ FAIL @7F80") == 0);
    BootAudit_Line(355U, line);
    CHECK(strcmp(line, "CH 99 NO VALID COPY") == 0);
    BootAudit_Line(356U, line);
    CHECK(line[0] == '\0');
    return 0;
}


int test_boot_format_errors_are_distinct_from_crc_errors(void)
{
    uint16_t crc;
    char line[32];
    BootFixture();
    /* Правильная CRC, но неверный номер канала в записи. */
    MemoryRecord_Encode(&boot_chip[0x4000], 1U, 1U, 0);
    /* UART с правильной CRC, но недопустимыми 9 бит + чётность. */
    memcpy(boot_chip, "FU01", 4U);
    boot_chip[8] = 0x00U;
    boot_chip[9] = 0xC2U;
    boot_chip[10] = 0x01U;
    boot_chip[11] = 0U;
    boot_chip[12] = 9U;
    boot_chip[13] = UART_PARITY_EVEN;
    boot_chip[14] = UART_STOP_ONE;
    crc = AT24C256_Crc16Update(0xFFFFU, boot_chip, 62U);
    boot_chip[62] = (uint8_t)crc;
    boot_chip[63] = (uint8_t)(crc >> 8U);
    ScanBoot();
    CHECK(BootAudit_Count() == 4U && BootAudit_Bad() == 1U && BootAudit_Free() == 98U);
    BootAudit_Line(0U, line);
    CHECK(strcmp(line, "UART A: FORMAT ERR") == 0);
    BootAudit_Line(2U, line);
    CHECK(strcmp(line, "CH 01 A: FORMAT ERR") == 0);
    /* Исправление формата и CRC должно убрать только замечания UART. */
    boot_chip[12] = 8U;
    crc = AT24C256_Crc16Update(0xFFFFU, boot_chip, 62U);
    boot_chip[62] = (uint8_t)crc;
    boot_chip[63] = (uint8_t)(crc >> 8U);
    BootAudit_Block(0U, boot_chip);
    CHECK(BootAudit_Count() == 2U);
    return 0;
}


int test_boot_first_save_has_one_copy_without_error_and_bad_spare_is_warning(void)
{
    char line[32];
    radio_preset_t preset = {7100000U, RADIO_LSB, 0U, 0U, 2U};
    BootFixture();
    MemoryRecord_Encode(&boot_chip[0x4040], 0U, 1U, &preset);
    MemoryRecord_Encode(&boot_chip[0x4080], 1U, 1U, &preset);
    boot_chip[0x4100] = 0U;
    MemoryRecord_Encode(&boot_chip[0x4140], 2U, 1U, &preset);
    MemoryRecord_Encode(&boot_chip[0x4180], 3U, 1U, &preset);
    boot_chip[0x41C0] = 0U;
    boot_chip[0x4200] = 0U;
    ScanBoot();
    CHECK(BootAudit_Used() == 4U && BootAudit_Free() == 94U && BootAudit_Bad() == 1U);
    CHECK(BootAudit_Count() == 4U);
    BootAudit_Line(0U, line);
    CHECK(strcmp(line, "CH 03 A: CRC WARN") == 0 && BootAudit_Severity(0U) == 1U);
    BootAudit_Line(1U, line);
    CHECK(strcmp(line, "CH 04 B: CRC WARN") == 0 && BootAudit_Severity(1U) == 1U);
    BootAudit_Line(2U, line);
    CHECK(strcmp(line, "CH 05 A: CRC ERROR") == 0 && BootAudit_Severity(2U) == 2U);
    BootAudit_Line(3U, line);
    CHECK(strcmp(line, "CH 05 NO VALID COPY") == 0 && BootAudit_Severity(3U) == 2U);
    return 0;
}
