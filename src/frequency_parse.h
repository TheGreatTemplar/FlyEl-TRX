#ifndef FREQUENCY_PARSE_H
#define FREQUENCY_PARSE_H
#include <stdint.h>
/* 1/2 разряда МГц + 3 кГц + 2 десятков Гц; незаполненный хвост = нули. */
uint8_t Frequency_ParseDigits(const char *text, uint8_t mhz_digits, uint32_t *hz);
/* text: минимум 10 байт для __.___.__ и завершающего ноля. */
void Frequency_FormatDigits(const char *digits, uint8_t mhz_digits, char *text);
uint8_t Channel_Parse(const char *text, uint8_t *zero_based);
#endif
