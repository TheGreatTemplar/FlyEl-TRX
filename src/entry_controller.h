#ifndef ENTRY_CONTROLLER_H
#define ENTRY_CONTROLLER_H
#include "input.h"

void Entry_Init(void);
void Entry_Task(void);
void Entry_Cancel(void);
/* Возвращает 1, если кнопка была использована вводом и не должна выполнить основную функцию. */
uint8_t Entry_HandleKey(input_key_t key);
uint8_t Entry_Active(void);
uint8_t Entry_ChannelTarget(void);
const char *Entry_Status(void);
#endif
