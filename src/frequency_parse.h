#ifndef FREQUENCY_PARSE_H
#define FREQUENCY_PARSE_H
#include <stdint.h>
/* С точкой — МГц (до шести дробных знаков), без точки — кГц. */
uint8_t Frequency_Parse(const char *text, uint32_t *hz);
uint8_t Channel_Parse(const char *text, uint8_t *zero_based);
#endif
