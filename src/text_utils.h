#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stdint.h>

/* Преобразует число в строку с ведущими нулями; буфер предоставляет вызывающая функция. */
void Text_U32ToDec(char *buf, uint32_t value, uint8_t min_digits);

/* Добавляет строку к уже завершённому нулём буферу; ёмкость проверяет вызывающий код. */
void Text_Append(char *dst, const char *src);

#endif
