/* Парсер проверяется исполнением Thumb-кода, включая длинный дробный ввод. */
#include "frequency_parse.c"
#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

int test_frequency_decimal_without_overflow(void)
{
    uint32_t hz;
    CHECK(Frequency_Parse("21.46978", &hz) != 0U && hz == 21469780U);
    CHECK(Frequency_Parse("14.999999", &hz) != 0U && hz == 14999999U);
    CHECK(Frequency_Parse("3650", &hz) != 0U && hz == 3650000U);
    CHECK(Frequency_Parse(".123456", &hz) != 0U && hz == 123456U);
    CHECK(Frequency_Parse("30", &hz) == 0U);
    CHECK(Frequency_Parse("30000", &hz) != 0U && hz == 30000000U);
    return 0;
}

int test_frequency_invalid_input(void)
{
    uint32_t hz = 123U;
    CHECK(Frequency_Parse(".", &hz) == 0U);
    CHECK(Frequency_Parse("", &hz) == 0U);
    CHECK(Frequency_Parse("1..2", &hz) == 0U);
    CHECK(Frequency_Parse("0.099999", &hz) == 0U);
    CHECK(Frequency_Parse("30.000001", &hz) == 0U);
    CHECK(Frequency_Parse("99999999999", &hz) == 0U);
    CHECK(Frequency_Parse("14.1234567", &hz) == 0U);
    CHECK(Frequency_Parse("7x", &hz) == 0U);
    CHECK(hz == 123U);
    return 0;
}

int test_channel_number_bounds(void)
{
    uint8_t channel;
    CHECK(Channel_Parse("001", &channel) != 0U && channel == 0U);
    CHECK(Channel_Parse("100", &channel) != 0U && channel == 99U);
    CHECK(Channel_Parse("0", &channel) == 0U);
    CHECK(Channel_Parse("101", &channel) == 0U);
    CHECK(Channel_Parse("1.0", &channel) == 0U);
    CHECK(Channel_Parse("99999999999", &channel) == 0U);
    return 0;
}
