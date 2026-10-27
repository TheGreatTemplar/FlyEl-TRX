#ifndef INPUT_ENCODER_H
#define INPUT_ENCODER_H
#include "hmi_config.h"
typedef struct
{
    uint32_t change_tick;
    int16_t accum;
    uint8_t last;
    uint8_t candidate;
} encoder_decoder_t;
void Encoder_Reset(encoder_decoder_t *state, uint8_t ab, uint32_t now);
/* ISR 500 мкс; без задержек. Результат — логические шаги с учётом направления. */
int16_t Encoder_Update(encoder_decoder_t *state, uint8_t ab, uint32_t now, const hmi_config_t *config);
#endif
