#ifndef UI_FREQUENCY_H
#define UI_FREQUENCY_H

#include <stdint.h>

/* Вычисляет левую координату основной частоты для выравнивания вправо. */
uint8_t FrequencyUI_MainLeftX(uint32_t hz);

/* Рисует основную частоту; SW2019 использует высоты 17/15 пикселей. */
void FrequencyUI_DrawMain(uint32_t hz, uint8_t y);

/* Вычисляет левую координату строки второго VFO. */
uint8_t FrequencyUI_SecondLeftX(uint32_t hz);

/* Рисует VFO B: старшие разряды 7x11, последние два — 5x7. */
void FrequencyUI_DrawSecond(uint32_t hz, uint8_t y);

/* Рисует числовую частоту в кГц для сервисного меню. */
void FrequencyUI_DrawKHz(uint8_t x, uint8_t y, uint32_t hz, uint8_t invert);

/* Форматирует выбранную ПЧ в МГц для экрана меню. */
void FrequencyUI_FormatIF(char *buf);

#endif
