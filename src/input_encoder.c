#include "input_encoder.h"
static const int8_t transitions[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void Encoder_Reset(encoder_decoder_t *state, uint8_t ab, uint32_t now)
{
    state->last = ab;
    state->candidate = ab;
    state->change_tick = now;
    state->accum = 0;
    state->anchor = ab;
}


/* Профиль ANY AB: один оборот электрического цикла (4) или полпериода (2).
 * Начальная фаза может быть любой: у разных ручек фиксация бывает 00/11 либо 01/10. */
static uint8_t Encoder_AtDetent(uint8_t ab, uint8_t anchor, const hmi_config_t *config)
{
    if (config->encoder_type == 0U)
    {
        if (config->encoder_steps == 4U)
            return (uint8_t)(ab == anchor);
        if (config->encoder_steps == 2U)
            return (uint8_t)(ab == anchor || ab == (3 - anchor));
        return 1U;
    }
    if (config->encoder_type == 1U)
        return (uint8_t)(ab == 0U);
    if (config->encoder_type == 2U)
        return (uint8_t)(ab == 3U);
    return (uint8_t)(ab == 0U || ab == 3U);
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
        /* Потерянный/недопустимый переход не оставляет старый остаток направления. */
        state->accum = 0;
        return 0;
    }
    state->accum += delta;
    if (Encoder_AtDetent(ab, state->anchor, config) == 0U)
        return 0;
    steps = (int16_t)(state->accum / config->encoder_steps);
    /* Возврат на исходный щелчок отменяет недокрученное движение. После ошибки
     * неполный цикл также сбрасывается здесь, а не гасит первый шаг при реверсе. */
    state->accum = config->encoder_type == 0U ? 0 : (int8_t)(state->accum % config->encoder_steps);
    return config->encoder_reverse != 0U ? (int16_t)-steps : steps;
}
