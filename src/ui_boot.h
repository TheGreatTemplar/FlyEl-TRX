#ifndef UI_BOOT_H
#define UI_BOOT_H
#include <stdint.h>
/* Семь строк 5x7 с шагом 9: между видимыми буквами два пустых ряда. */
void BootUI_Clear(void);
void BootUI_Line(uint8_t row, const char *text, uint8_t inverse);
void BootUI_Spinner(uint8_t row, uint32_t tick);
void BootUI_Update(void);
#endif
