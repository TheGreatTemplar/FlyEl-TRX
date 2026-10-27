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
    keyboard_active = 0U;
    Input_Configure(&config);
}


static void PollAB(uint8_t ab)
{
    hmi_gpio_b.IDR = ((uint32_t)(ab >> 1U) << 13U) | ((uint32_t)(ab & 1U) << 14U);
    Input_Poll500us();
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
