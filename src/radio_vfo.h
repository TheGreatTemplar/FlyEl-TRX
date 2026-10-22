#ifndef RADIO_VFO_H
#define RADIO_VFO_H

#include "radio_mode.h"

/* Два независимых VFO; при старте оба наследуют исходную частоту и LSB. */
void RadioVfo_Init(void);
void RadioVfo_Switch(void);
void RadioVfo_CopyAToB(void);
void RadioVfo_ChangeMode(uint8_t long_press);
void RadioVfo_CycleFilter(void);
void RadioVfo_AdjustFilter(int32_t steps);
uint8_t RadioVfo_Filter(void);
void RadioVfo_ToggleSplit(void);
uint8_t RadioVfo_Split(void);
uint8_t RadioVfo_Active(void);
uint8_t RadioVfo_Mode(void);
uint32_t RadioVfo_OtherHz(void);

/* Снимок текущей настройки и загрузка настройки в активный VFO. */
radio_preset_t RadioVfo_Capture(void);
void RadioVfo_Apply(const radio_preset_t *preset);

#endif
