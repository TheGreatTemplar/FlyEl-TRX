#ifndef FREQ_ENTRY_H
#define FREQ_ENTRY_H

#include <stdint.h>
#include "input.h"

/* Проверяет, относится ли логическая клавиша к ручному вводу частоты. */
uint8_t FreqEntry_IsCharacterKey(input_key_t key);

/* Открывает ручной ввод; необязательный первый символ сразу добавляется в буфер. */
void FreqEntry_Start(char ch);

/* Добавляет символ, соблюдая размер буфера и запрет второй точки. */
void FreqEntry_Append(char ch);

/* Применяет введённую частоту в пределах диапазона и закрывает ввод. */
void FreqEntry_Apply(void);

/* Закрывает ввод, оставляя прежнюю частоту. */
void FreqEntry_Cancel(void);

#endif
