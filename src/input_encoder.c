#include "input_encoder.h"
static const int8_t transitions[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void Encoder_Reset(encoder_decoder_t *state, uint8_t ab, uint32_t now)
{
    state->last = ab;
    state->candidate = ab;
    state->change_tick = now;
    state->accum = 0;
}


int16_t Encoder_Update(encoder_decoder_t *state, uint8_t ab, uint32_t now, const hmi_config_t *config)
{
    int16_t steps;
    int8_t delta;
    if (ab != state->candidate)
    {
        state->candidate = ab;
        state->change_tick = now;
    }
    if (ab == state->last || (uint32_t)(now - state->change_tick) < config->encoder_filter)
        return 0;
    delta = transitions[(state->last << 2U) | ab];
    state->last = ab;
    if (delta == 0)
    {
        if (config->encoder_type != 0U)
            state->accum = 0;
        return 0;
    }
    state->accum += delta;
    if ((config->encoder_type == 1U && ab != 0U) || (config->encoder_type == 2U && ab != 3U) ||
        (config->encoder_type == 3U && ab != 0U && ab != 3U))
        return 0;
    steps = (int16_t)(state->accum / config->encoder_steps);
    state->accum = (int16_t)(state->accum % config->encoder_steps);
    return config->encoder_reverse != 0U ? (int16_t)-steps : steps;
}
