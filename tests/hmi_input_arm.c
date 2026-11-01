/* Реальный драйвер ввода с моделью GPIO: параметры, дребезг и квадратура. */
#include "stm32f10x.h"
#include <string.h>
static GPIO_TypeDef hmi_gpio_a;
static GPIO_TypeDef hmi_gpio_b;
#undef GPIOA
#undef GPIOB
#define GPIOA (&hmi_gpio_a)
#define GPIOB (&hmi_gpio_b)
#include "hmi_config.c"
#include "input_encoder.c"
#include "input_aux.c"
#include "input.c"
#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
            return __LINE__;                                                                                           \
    } while (0)

static void InputFixture(void)
{
    hmi_config_t config;
    HMI_Defaults(&config);
    memset(&hmi_gpio_b, 0, sizeof(hmi_gpio_b));
    memset((void *)key_state, 0, sizeof(key_state));
    memset((void *)key_pressed_events, 0, sizeof(key_pressed_events));
    memset(key_last_raw, 0, sizeof(key_last_raw));
    memset(key_change_tick, 0, sizeof(key_change_tick));
    input_ticks = 0U;
    memset(&hmi_gpio_a, 0, sizeof(hmi_gpio_a));
    scan_position = 0U;
    scan_baseline = 0U;
    scan_mask = 0U;
    AuxEncoder_Reset();
    Input_Configure(&config);
}


static void PollAB(uint8_t ab)
{
    hmi_gpio_b.IDR = ((uint32_t)(ab >> 1U) << 13U) | ((uint32_t)(ab & 1U) << 14U);
    Input_Poll500us();
}


/* Модель диодной матрицы: на входах уровни выбранного Q до очередного тика. */
static void ScanFrame(uint8_t q12_ab, uint8_t q34_ab, uint32_t keys)
{
    uint8_t i;
    uint8_t columns = (uint8_t)((keys | (keys >> 3U) | (keys >> 6U) | (keys >> 9U) | (keys >> 12U) | (keys >> 15U) |
                                 (keys >> 18U) | (keys >> 21U) | (keys >> 24U)) &
                                7U);
    for (i = 0U; i < 10U; i++)
    {
        uint8_t outs = (uint8_t)(~columns & 7U);
        uint8_t level = 0U;
        if (scan_position != 0U)
            outs |= (uint8_t)((keys >> ((scan_position - 1U) * 3U)) & 7U);
        if (scan_position == 1U)
            level = (uint8_t)(q12_ab >> 1U);
        else if (scan_position == 2U)
            level = (uint8_t)(q12_ab & 1U);
        else if (scan_position == 3U)
            level = (uint8_t)(q34_ab >> 1U);
        else if (scan_position == 4U)
            level = (uint8_t)(q34_ab & 1U);
        hmi_gpio_b.IDR = (hmi_gpio_b.IDR & ~(7UL << 3U)) | ((uint32_t)outs << 3U);
        hmi_gpio_a.IDR = (uint32_t)level << 15U;
        Input_Poll500us();
    }
}


int test_continuous_scan_reads_both_encoders_without_key_interrupt(void)
{
    uint8_t i;
    static const uint8_t forward[] = {2U, 3U, 1U, 0U};
    static const uint8_t reverse[] = {1U, 3U, 2U, 0U};
    InputFixture();
    ScanFrame(0U, 0U, 0U);
    for (i = 0U; i < 4U; i++)
        ScanFrame(forward[i], reverse[i], 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == -2);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == -2);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 0);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_COUNT) == 0);
    CHECK(Input_GetRawKeyMask() == 0U && Input_GetEncoderSteps() == 0);
    for (i = 0U; i < 100U; i++)
        ScanFrame(0U, 0U, 0U);
    CHECK(scan_position == 0U && input_ticks == 1050U);
    for (i = 0U; i < 4U; i++)
        ScanFrame(reverse[i], forward[i], 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 2);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == 2);
    return 0;
}


int test_aux_scan_preserves_keyboard_main_encoder_and_rejects_bounce(void)
{
    uint8_t i;
    hmi_config_t config;
    static const uint8_t bounce[] = {2U, 0U, 2U, 3U, 2U, 3U, 1U, 0U};
    InputFixture();
    ScanFrame(0U, 0U, 0U);
    HMI_Defaults(&config);
    config.encoder_reverse = 1U;
    config.encoder_steps = 1U;
    Input_Configure(&config);
    for (i = 0U; i < 8U; i++)
        ScanFrame(bounce[i], 0U, 1UL << 11U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == -2);
    CHECK(Input_KeyIsDown(Key_10_3) != 0U);
    CHECK(Input_KeyPressed(Key_10_3) == 1U && Input_KeyPressed(Key_10_3) == 0U);
    PollAB(2U);
    CHECK(Input_GetEncoderSteps() == -1);
    for (i = 0U; i < 5U; i++)
        ScanFrame(0U, 0U, 0U);
    CHECK(Input_KeyIsDown(Key_10_3) == 0U);
    CHECK(Input_GetRawKeyMask() == 0U);
    ScanFrame(3U, 3U, 0U);
    ScanFrame(0U, 0U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 0);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == 0);
    return 0;
}


int test_aux_startup_closed_contacts_do_not_emit_steps(void)
{
    uint8_t i;
    InputFixture();
    for (i = 0U; i < 10U; i++)
        ScanFrame(3U, 3U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 0);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == 0);
    return 0;
}


int test_hmi_real_input_default_steps_reverse_and_partial_reset(void)
{
    hmi_config_t config;
    InputFixture();
    PollAB(2U);
    PollAB(3U);
    PollAB(1U);
    PollAB(0U);
    CHECK(Input_GetEncoderSteps() == 1);
    PollAB(1U);
    PollAB(3U);
    PollAB(2U);
    PollAB(0U);
    CHECK(Input_GetEncoderSteps() == -1);
    PollAB(2U);
    HMI_Defaults(&config);
    config.encoder_steps = 2U;
    config.encoder_reverse = 1U;
    Input_Configure(&config);
    CHECK(Input_GetEncoderSteps() == 0);
    PollAB(3U);
    PollAB(1U);
    PollAB(0U);
    PollAB(2U);
    CHECK(Input_GetEncoderSteps() == -2);
    return 0;
}


int test_hmi_debounce_parameter_controls_press_release_and_wrap(void)
{
    hmi_config_t config;
    InputFixture();
    HMI_Defaults(&config);
    config.debounce_ms = 25U;
    Input_Configure(&config);
    input_ticks = 0xFFFFFFF0U;
    Keyboard_Debounce(1U);
    input_ticks += 49U;
    Keyboard_Debounce(1U);
    CHECK(Input_KeyIsDown(Key_2_1) == 0U);
    input_ticks++;
    Keyboard_Debounce(1U);
    CHECK(Input_KeyIsDown(Key_2_1) != 0U && Input_KeyPressed(Key_2_1) != 0U);
    CHECK(Input_KeyPressed(Key_2_1) == 0U);
    input_ticks++;
    Keyboard_Debounce(0U);
    input_ticks += 49U;
    Keyboard_Debounce(0U);
    CHECK(Input_KeyIsDown(Key_2_1) != 0U);
    input_ticks++;
    Keyboard_Debounce(0U);
    CHECK(Input_KeyIsDown(Key_2_1) == 0U);
    return 0;
}


int test_hmi_detent_profiles_filter_and_fast_rotation(void)
{
    encoder_decoder_t state;
    hmi_config_t config;
    uint32_t tick;
    uint8_t type;
    static const uint8_t cw[] = {2U, 3U, 1U, 0U};
    HMI_Defaults(&config);
    for (type = 0U; type < 4U; type++)
    {
        int32_t total = 0;
        config.encoder_type = type;
        Encoder_Reset(&state, 0U, 0U);
        for (tick = 1U; tick <= 400U; tick++)
            total += Encoder_Update(&state, cw[(tick - 1U) % 4U], tick, &config);
        CHECK(total >= 99 && total <= 100);
    }
    config.encoder_type = 0U;
    config.encoder_steps = 1U;
    config.encoder_filter = 2U;
    Encoder_Reset(&state, 0U, 0xFFFFFFF0U);
    CHECK(Encoder_Update(&state, 2U, 0xFFFFFFF1U, &config) == 0);
    CHECK(Encoder_Update(&state, 0U, 0xFFFFFFF2U, &config) == 0);
    CHECK(Encoder_Update(&state, 2U, 0xFFFFFFFFU, &config) == 0);
    CHECK(Encoder_Update(&state, 2U, 0U, &config) == 0);
    CHECK(Encoder_Update(&state, 2U, 1U, &config) == 1);
    CHECK(Encoder_Update(&state, 1U, 2U, &config) == 0);
    CHECK(Encoder_Update(&state, 1U, 4U, &config) == 0);
    CHECK(Encoder_Update(&state, 0U, 5U, &config) == 0);
    CHECK(Encoder_Update(&state, 0U, 7U, &config) == 1);
    return 0;
}


/* Каждый полупериод — отдельный щелчок, без двойной выдачи при следующем опросе. */
int test_aux_each_detent_counts_once_on_00_and_11_with_swapped_roles(void)
{
    InputFixture();
    ScanFrame(0U, 0U, 0U);
    ScanFrame(2U, 1U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == 0);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 0);
    ScanFrame(3U, 3U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == -1);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == -1);
    ScanFrame(3U, 3U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == 0);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 0);
    ScanFrame(1U, 2U, 0U);
    ScanFrame(0U, 0U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == -1);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == -1);
    ScanFrame(1U, 2U, 0U);
    ScanFrame(3U, 3U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == 1);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 1);
    ScanFrame(2U, 1U, 0U);
    ScanFrame(0U, 0U, 0U);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_SHIFT) == 1);
    CHECK(Input_GetAuxEncoderSteps(INPUT_AUX_RIT) == 1);
    return 0;
}
