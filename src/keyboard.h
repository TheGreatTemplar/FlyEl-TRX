#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "input.h"

/* Обработка пользовательских функций — в основном цикле, вне ISR. */
void Keyboard_Init(void);
void Keyboard_Task(input_key_t pressed);

#endif
