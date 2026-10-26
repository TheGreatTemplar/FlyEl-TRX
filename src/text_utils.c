/* Числа и короткие строки интерфейса. */
#include "text_utils.h"

/* Преобразует число в строку с ведущими нулями; буфер предоставляет вызывающая функция. */
void Text_U32ToDec(char *buf, uint32_t value, uint8_t min_digits)
{
    char tmp[10];
    uint8_t i;
    uint8_t j;

    i = 0U;

    do
    {
        tmp[i] = (char)('0' + (value % 10U));
        value /= 10U;
        i++;
    } while ((value != 0U) && (i < sizeof(tmp)));

    while ((i < min_digits) && (i < sizeof(tmp)))
    {
        tmp[i] = '0';
        i++;
    }

    for (j = 0U; j < i; j++)
    {
        buf[j] = tmp[i - 1U - j];
    }

    buf[i] = '\0';
}


/* Добавляет строку к уже завершённому нулём буферу; ёмкость проверяет вызывающий код. */
void Text_Append(char *dst, const char *src)
{
    while (*dst != '\0')
    {
        dst++;
    }

    while (*src != '\0')
    {
        *dst = *src;
        dst++;
        src++;
    }

    *dst = '\0';
}
