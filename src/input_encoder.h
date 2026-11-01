#ifndef INPUT_ENCODER_H
#define INPUT_ENCODER_H
#include "hmi_config.h"
typedef struct
{
    uint32_t change_tick;
    int8_t accum;
    uint8_t last;
    uint8_t candidate;
    uint8_t anchor;
} encoder_decoder_t;
void Encoder_Reset(encoder_decoder_t *state, uint8_t ab, uint32_t now);
/* ISR, без задержек. ANY AB привязывает 2/4 перехода к исходным фазам щелчка.
 * Размер состояния 8 байт; оно не хранится в EEPROM. */
int16_t Encoder_Update(encoder_decoder_t *state, uint8_t ab, uint32_t now, const hmi_config_t *config);
#endif
