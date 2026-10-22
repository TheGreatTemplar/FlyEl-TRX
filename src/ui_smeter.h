#ifndef UI_SMETER_H
#define UI_SMETER_H

#include <stdint.h>

/* Рисует шкалу и линейную полосу из напряжения ADC0; калибровка S-единиц ещё не введена. */
void SMeter_Draw(uint16_t mv);

#endif
