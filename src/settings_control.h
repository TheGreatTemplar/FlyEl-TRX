#ifndef SETTINGS_CONTROL_H
#define SETTINGS_CONTROL_H
#include "input.h"

void Settings_Init(void);
uint8_t Settings_HandleKey(input_key_t key, uint8_t long_press);
void Settings_Encoder(int32_t steps);
uint8_t Settings_Page(void);
uint8_t Settings_Item(void);
#endif
