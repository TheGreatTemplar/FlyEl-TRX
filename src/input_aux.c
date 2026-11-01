/* Квадратура на общем PA15: SHIFT — Q1/Q2, RIT — Q3/Q4 CD4017. */
#include "input_aux.h"
#include "input_encoder.h"

static encoder_decoder_t aux_decoders[INPUT_AUX_COUNT];
static volatile int32_t aux_steps[INPUT_AUX_COUNT];
static uint8_t aux_ready[INPUT_AUX_COUNT];
static uint8_t aux_a[INPUT_AUX_COUNT];
static hmi_config_t aux_config;

void AuxEncoder_Reset(void)
{
    uint8_t i;
    HMI_Defaults(&aux_config);
    /* Отдельный профиль: настройки основного энкодера не влияют на RIT/SHIFT.
     * Щелчки чередуют 00 и 11: два перехода дают шаг на каждом из этих состояний.
     * Неверный переход сбрасывает остаток. */
    aux_config.encoder_type = 3U;
    aux_config.encoder_steps = 2U;
    for (i = 0U; i < INPUT_AUX_COUNT; i++)
    {
        aux_steps[i] = 0;
        aux_ready[i] = 0U;
        aux_a[i] = 0U;
        Encoder_Reset(&aux_decoders[i], 0U, 0U);
    }
}


void AuxEncoder_Sample(uint8_t position, uint8_t level, uint32_t now)
{
    uint8_t index;
    uint8_t ab;
    if (position < 1U || position > 4U)
    {
        return;
    }
    index = (uint8_t)(position <= 2U ? INPUT_AUX_SHIFT : INPUT_AUX_RIT);
    if ((position & 1U) != 0U)
    {
        aux_a[index] = (uint8_t)(level & 1U);
        return;
    }
    ab = (uint8_t)(((uint32_t)aux_a[index] << 1U) | (level & 1U));
    if (aux_ready[index] == 0U)
    {
        /* Первое измерение задаёт исходную фазу и не порождает ложный шаг. */
        Encoder_Reset(&aux_decoders[index], ab, now);
        aux_ready[index] = 1U;
        return;
    }
    aux_steps[index] += Encoder_Update(&aux_decoders[index], ab, now, &aux_config);
}


int32_t AuxEncoder_Take(input_aux_encoder_t encoder)
{
    int32_t steps;
    if ((uint8_t)encoder >= INPUT_AUX_COUNT)
    {
        return 0;
    }
    steps = aux_steps[encoder];
    aux_steps[encoder] = 0;
    return steps;
}
