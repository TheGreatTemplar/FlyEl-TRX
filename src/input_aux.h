#ifndef INPUT_AUX_H
#define INPUT_AUX_H

#include "input.h"

/* Внутренние функции сканера: Reset до запуска TIM2, Sample из TIM2.
 * Take вызывается основным циклом при запрещённых IRQ. */
void AuxEncoder_Reset(void);
void AuxEncoder_Sample(uint8_t position, uint8_t level, uint32_t now);
int32_t AuxEncoder_Take(input_aux_encoder_t encoder);

#endif
