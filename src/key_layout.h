#ifndef KEY_LAYOUT_H
#define KEY_LAYOUT_H

#include "input.h"

/* Соответствие физических выводов CD4017 таблице пользователя.
 * NR/ANF оставлены назначенными, но их DSP-функции пока отключены. */
const char *KeyLayout_Name(input_key_t key);
char KeyLayout_Character(input_key_t key);

#endif
