/* Pure-C checks of the production EEPROM driver; I2C is replaced by a bus stub. */
#include "at24c256.c"
#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static uint8_t reply_address;
static uint8_t probe_count;
static uint8_t read_count;
static uint8_t sent_high;
static uint8_t sent_low;
static uint16_t sent_length;
static i2c2_soft_status_t bus_failure;
static uint8_t write_count;
static uint8_t write_first;
static uint8_t write_last;

i2c2_soft_status_t I2C2_Soft_Write(uint8_t address, const uint8_t *data, uint16_t length)
{
    (void)address;
    write_count++;
    sent_high = data[0]; sent_low = data[1]; sent_length = length;
    write_first = data[2]; write_last = data[length - 1U];
    return bus_failure;
}

i2c2_soft_status_t I2C2_Soft_Probe(uint8_t address)
{
    probe_count++;
    if (bus_failure != I2C2_SOFT_OK) return bus_failure;
    return (address == reply_address) ? I2C2_SOFT_OK : I2C2_SOFT_NACK;
}

i2c2_soft_status_t I2C2_Soft_WriteRead(uint8_t address, const uint8_t *prefix,
                                    uint16_t prefix_len, uint8_t *data, uint16_t length)
{
    (void)address;
    (void)data;
    if (prefix_len != 2U) return I2C2_SOFT_ARG_ERROR;
    read_count++;
    sent_high = prefix[0];
    sent_low = prefix[1];
    sent_length = length;
    return bus_failure;
}

int test_crc_check_vector(void)
{
    static const uint8_t text[] = "123456789";
    CHECK(AT24C256_Crc16Update(0xFFFFU, text, 9U) == 0x29B1U);
    return 0;
}

int test_crc_incremental(void)
{
    static const uint8_t text[] = "123456789";
    uint16_t crc = AT24C256_Crc16Update(0xFFFFU, text, 4U);
    CHECK(AT24C256_Crc16Update(crc, text + 4, 5U) == 0x29B1U);
    CHECK(AT24C256_Crc16Update(0x1234U, 0, 0U) == 0x1234U);
    return 0;
}

int test_find_address_range(void)
{
    uint8_t address = 0U;
    reply_address = 0x57U; probe_count = 0U; bus_failure = I2C2_SOFT_OK;
    CHECK(AT24C256_Find(&address) == I2C2_SOFT_OK);
    CHECK(address == 0x57U && probe_count == 8U);
    return 0;
}

int test_find_missing_or_stuck_bus(void)
{
    uint8_t address = 0U;
    reply_address = 0U; probe_count = 0U; bus_failure = I2C2_SOFT_OK;
    CHECK(AT24C256_Find(&address) == I2C2_SOFT_NACK && probe_count == 8U);
    probe_count = 0U; bus_failure = I2C2_SOFT_TIMEOUT;
    CHECK(AT24C256_Find(&address) == I2C2_SOFT_TIMEOUT && probe_count == 1U);
    return 0;
}

int test_read_address_bytes(void)
{
    uint8_t data[128];
    bus_failure = I2C2_SOFT_OK; read_count = 0U;
    CHECK(AT24C256_Read(0x50U, 0x7F80U, data, sizeof(data)) == I2C2_SOFT_OK);
    CHECK(read_count == 1U && sent_high == 0x7FU && sent_low == 0x80U && sent_length == 128U);
    return 0;
}

int test_read_rejects_bad_ranges(void)
{
    uint8_t data[128];
    read_count = 0U;
    CHECK(AT24C256_Read(0x50U, 0x7FFFU, data, 2U) == I2C2_SOFT_ARG_ERROR);
    CHECK(AT24C256_Read(0x50U, 0U, data, 0U) == I2C2_SOFT_ARG_ERROR);
    CHECK(AT24C256_Read(0x50U, 0U, 0, 1U) == I2C2_SOFT_ARG_ERROR);
    CHECK(AT24C256_Read(0x58U, 0U, data, 1U) == I2C2_SOFT_ARG_ERROR);
    CHECK(read_count == 0U);
    return 0;
}

int test_page_write_address_and_limits(void)
{
    uint8_t data[2] = { 0x12U, 0x34U };
    write_count = 0U; bus_failure = I2C2_SOFT_OK;
    CHECK(AT24C256_WritePage(0x57U, 0x7FFEU, data, 2U) == I2C2_SOFT_OK);
    CHECK(write_count == 1U && sent_high == 0x7FU && sent_low == 0xFEU);
    CHECK(sent_length == 4U && write_first == 0x12U && write_last == 0x34U);
    CHECK(AT24C256_WritePage(0x50U, 63U, data, 2U) == I2C2_SOFT_ARG_ERROR);
    CHECK(AT24C256_WritePage(0x50U, 0x7FFFU, data, 2U) == I2C2_SOFT_ARG_ERROR);
    CHECK(AT24C256_WritePage(0x58U, 0U, data, 1U) == I2C2_SOFT_ARG_ERROR);
    CHECK(AT24C256_WritePage(0x50U, 0U, data, 65U) == I2C2_SOFT_ARG_ERROR);
    CHECK(AT24C256_WritePage(0x50U, 0U, 0, 1U) == I2C2_SOFT_ARG_ERROR);
    CHECK(write_count == 1U);
    return 0;
}
