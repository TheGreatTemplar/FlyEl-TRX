#ifndef RADIO_CONTROL_H
#define RADIO_CONTROL_H

#include <stdint.h>
#include "input.h"

/* Возвращает количество диапазонов в таблице проекта. */
uint8_t Radio_BandsCount(void);
/* Индекс диапазона для частоты в Гц; Radio_BandsCount(), если она вне таблицы. */
uint8_t Radio_FindBand(uint32_t hz);

/* Ограничивает частоту границами выбранного диапазона. */
void Radio_ClampRf(void);

/* Передаёт RF + ПЧ на CLK1 и сохраняет результат обмена с синтезатором. */
void Radio_ApplySynth(void);

/* Выбирает диапазон, устанавливает стартовую частоту и обновляет синтезатор. */
void Radio_SetBand(uint8_t band);

/* Переключает полный список шагов сервисного меню; TS использует отдельный цикл. */
void Radio_CycleStep(void);

/* Обрабатывает Key_10_3 и применяет результат автомата TS к текущему шагу. */
void Radio_UpdateTS(input_key_t key);

/* Выбирает следующую ПЧ и перестраивает гетеродин. */
void Radio_CycleIF(void);

/* Изменяет RF на число щелчков энкодера с ограничением по диапазону. */
void Radio_TuneSteps(int32_t steps);
/* Внутреннее применение RF. Прямой ввод сначала проверяет Radio_FindBand(). */
void Radio_SetFrequency(uint32_t hz);

void Radio_InitTS(void);

#endif
