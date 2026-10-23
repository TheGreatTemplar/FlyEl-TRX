/* Позиционный ввод 6/7 цифр и автоточки проверяются исполнением Thumb-кода. */
#include "frequency_parse.c"
#include <string.h>
#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

int test_frequency_positional_digits_and_automatic_dots(void)
{
    uint32_t hz;
    char text[10];
    CHECK(Frequency_ParseDigits("2146978", 2U, &hz) != 0U && hz == 21469780U);
    CHECK(Frequency_ParseDigits("365000", 1U, &hz) != 0U && hz == 3650000U);
    CHECK(Frequency_ParseDigits("1499999", 2U, &hz) != 0U && hz == 14999990U);
    CHECK(Frequency_ParseDigits("7", 1U, &hz) != 0U && hz == 7000000U);
    CHECK(Frequency_ParseDigits("142", 2U, &hz) != 0U && hz == 14200000U);
    Frequency_FormatDigits("2146978", 2U, text); CHECK(strcmp(text, "21.469.78") == 0);
    Frequency_FormatDigits("365000", 1U, text); CHECK(strcmp(text, "3.650.00") == 0);
    Frequency_FormatDigits("21", 2U, text); CHECK(strcmp(text, "21.___.__") == 0);
    Frequency_FormatDigits("", 1U, text); CHECK(strcmp(text, "_.___.__") == 0);
    return 0;
}

int test_frequency_invalid_digits_and_limits(void)
{
    uint32_t hz = 123U;
    CHECK(Frequency_ParseDigits("", 2U, &hz) == 0U);
    CHECK(Frequency_ParseDigits("1.2", 2U, &hz) == 0U);
    CHECK(Frequency_ParseDigits("009999", 1U, &hz) == 0U);
    CHECK(Frequency_ParseDigits("3000001", 2U, &hz) == 0U);
    CHECK(Frequency_ParseDigits("3650000", 1U, &hz) == 0U);
    CHECK(Frequency_ParseDigits("21469780", 2U, &hz) == 0U);
    CHECK(Frequency_ParseDigits("7x", 1U, &hz) == 0U);
    CHECK(hz == 123U);
    return 0;
}

int test_channel_number_bounds(void)
{
    uint8_t channel;
    CHECK(Channel_Parse("001", &channel) != 0U && channel == 0U);
    CHECK(Channel_Parse("99", &channel) != 0U && channel == 98U);
    CHECK(Channel_Parse("100", &channel) == 0U);
    CHECK(Channel_Parse("0", &channel) == 0U);
    CHECK(Channel_Parse("101", &channel) == 0U);
    CHECK(Channel_Parse("1.0", &channel) == 0U);
    CHECK(Channel_Parse("99999999999", &channel) == 0U);
    return 0;
}
