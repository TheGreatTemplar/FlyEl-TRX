#ifndef FREQ_ENTRY_H
#define FREQ_ENTRY_H
#include <stdint.h>

/* channel_mode=0: частота, 6/7 цифр по диапазону; иначе номер канала. */
void FreqEntry_Start(uint8_t channel_mode);
/* Возвращает 1 только при добавлении допустимой цифры. */
uint8_t FreqEntry_Append(char ch);
/* Буфер результата: минимум FREQ_INPUT_MAX_LEN + 3 символа (две точки и ноль). */
void FreqEntry_Format(char *text);
uint8_t FreqEntry_Parse(uint32_t *hz);
void FreqEntry_Cancel(void);
#endif
