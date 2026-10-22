#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include <stdint.h>

/* Рисует строку сервисного меню с выделением выбранного пункта. */
void Widgets_MenuLine(uint8_t y, const char *left, const char *right, uint8_t selected);

/* Рисует подпись 5x7: по одному пустому пикселю внутри рамки с каждой стороны. */
uint8_t Widgets_Label(uint8_t x, uint8_t y, const char *text, uint8_t invert);

/* Рисует обычную рамку для SPLIT/RIT. */
uint8_t Widgets_CompactLabel(uint8_t x, uint8_t y, const char *text);

/* Рисует текущую подпись усиления/ATT; аппаратное управление пока не подключено. */
void Widgets_Gain(uint8_t x, uint8_t y);

#endif
