#include "input.h"
#include "board_pins.h"

/* CD4017: physical pins Q1..Q9 = 2,4,7,10,1,5,6,9,11.
 * Q0 is the idle state. A pressed OUT is low in Q0 and rises when
 * its selected counter output goes high. EXTI4 wakes the keyboard;
 * scan one position per timer tick only while active.
 * Encoder polling remains 2000 Hz, four transitions per detent.
 */
#define ENC_A_GPIO GPIOB
#define ENC_B_GPIO GPIOB
#define ENC_A_PIN 13U
#define ENC_B_PIN 14U
#define ENCODER_COUNTS_PER_STEP 4
#define KEY_DEBOUNCE_TICKS 20U
#define KEY_EXTI_MASK (1UL << KEY_INT_PIN)

void EXTI4_IRQHandler(void);

static const int8_t encoder_transition_table[16] =
{
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};
static const char * const physical_key_names[27] =
{
    "Key_2_1",
    "Key_2_2",
    "Key_2_3",
    "Key_4_1",
    "Key_4_2",
    "Key_4_3",
    "Key_7_1",
    "Key_7_2",
    "Key_7_3",
    "Key_10_1",
    "Key_10_2",
    "Key_10_3",
    "Key_1_1",
    "Key_1_2",
    "Key_1_3",
    "Key_5_1",
    "Key_5_2",
    "Key_5_3",
    "Key_6_1",
    "Key_6_2",
    "Key_6_3",
    "Key_9_1",
    "Key_9_2",
    "Key_9_3",
    "Key_11_1",
    "Key_11_2",
    "Key_11_3",
};

static volatile int32_t encoder_steps;
static volatile int32_t encoder_raw_counts;
static volatile uint32_t key_raw_mask_debug;
static uint8_t encoder_last_state;
static int8_t encoder_accum;
static volatile uint8_t key_state[INPUT_KEY_COUNT];
static volatile uint8_t key_pressed_events[INPUT_KEY_COUNT];
static uint8_t key_last_raw[INPUT_KEY_COUNT];
static uint32_t key_change_tick[INPUT_KEY_COUNT];
static uint32_t input_ticks;
static uint8_t scan_position;
static uint8_t scan_baseline;
static uint32_t scan_mask;
static volatile uint8_t keyboard_active;
static uint8_t release_pending;
static uint32_t release_tick;

/* Сохраняет PRIMASK перед доступом к данным, общим для основного цикла и ISR. */
static uint32_t Input_CriticalEnter(void)
{
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();

    return primask;
}

/* Восстанавливает разрешение IRQ, только если оно было включено до входа. */
static void Input_CriticalExit(uint32_t primask)
{
    if ((primask & 1U) == 0U)
    {
        __enable_irq();
    }
}

static volatile uint32_t *GPIO_CfgReg(GPIO_TypeDef *gpio, uint8_t pin, uint32_t *shift)
{
    if (pin < 8U)
    {
        *shift = pin * 4U;
        return &gpio->CRL;
    }

    *shift = (pin - 8U) * 4U;
    return &gpio->CRH;
}

static void GPIO_ConfigInputPullup(GPIO_TypeDef *gpio, uint8_t pin)
{
    volatile uint32_t *reg;
    uint32_t shift;

    reg = GPIO_CfgReg(gpio, pin, &shift);

    /* MODE = 00, CNF = 10: input with pull-up/pull-down. */
    *reg &= ~(0xFUL << shift);
    *reg |=  (0x8UL << shift);

    gpio->BSRR = (uint32_t)(1UL << pin);
}

static void GPIO_ConfigPushPullOutput(GPIO_TypeDef *gpio, uint8_t pin)
{
    volatile uint32_t *reg;
    uint32_t shift;

    reg = GPIO_CfgReg(gpio, pin, &shift);

    /* MODE = 10: output 2 MHz. CNF = 00: push-pull. */
    *reg &= ~(0xFUL << shift);
    *reg |=  (0x2UL << shift);
}

static uint8_t GPIO_ReadPin(GPIO_TypeDef *gpio, uint8_t pin)
{
    if ((gpio->IDR & (1UL << pin)) != 0U)
    {
        return 1U;
    }

    return 0U;
}

static void GPIO_WriteLow(GPIO_TypeDef *gpio, uint8_t pin)
{
    gpio->BRR = (uint32_t)(1UL << pin);
}

/* Получает двухбитное состояние фаз энкодера для таблицы переходов. */
static uint8_t Encoder_ReadAB(void)
{
    uint8_t a;
    uint8_t b;

    a = GPIO_ReadPin(ENC_A_GPIO, ENC_A_PIN);
    b = GPIO_ReadPin(ENC_B_GPIO, ENC_B_PIN);

    return (uint8_t)((a << 1U) | b);
}

/* Даёт выходам счётчика время установиться после фронта CLK/RESET. */
static void Keyboard_Settle(void)
{
    volatile uint32_t i;
    /* At 72 MHz this leaves several microseconds for RESET/CLK settling. */
    for (i = 0U; i < 80U; i++)
    {
        __NOP();
    }
}

/* Выбирает Q0, в котором любая кнопка может сформировать запрос INT. */
static void Keyboard_Reset(void)
{
    GPIO_WriteLow(KEY_CLK_GPIO, KEY_CLK_PIN);
    KEY_RESET_GPIO->BSRR = 1UL << KEY_RESET_PIN;
    Keyboard_Settle();
    KEY_RESET_GPIO->BRR = 1UL << KEY_RESET_PIN;
    Keyboard_Settle();
}

/* Продвигает активный высокий выход CD4017 на следующую позицию. */
static void Keyboard_Clock(void)
{
    KEY_CLK_GPIO->BSRR = 1UL << KEY_CLK_PIN;
    Keyboard_Settle();
    KEY_CLK_GPIO->BRR = 1UL << KEY_CLK_PIN;
}

/* Считывает PB3–PB5 одновременно: низкий уровень в Q0 означает нажатую колонку. */
static uint8_t Keyboard_ReadOuts(void)
{
    /* OUT1..OUT3 are consecutive PB3..PB5. */
    return (uint8_t)((KEY_OUT_GPIO->IDR >> KEY_OUT_FIRST_PIN) & 7U);
}

/* Возвращает клавиатуру в ожидание; проверка уровня закрывает гонку при повторном разрешении EXTI. */
static void Keyboard_ArmInterrupt(void)
{
    /* Q0 is already selected. Discard edges caused by the scan itself. */
    EXTI->IMR &= ~KEY_EXTI_MASK;
    EXTI->PR = KEY_EXTI_MASK;
    keyboard_active = 0U;
    EXTI->IMR |= KEY_EXTI_MASK;

    /* An already-held key has no new falling edge. Also close the
     * clear/unmask race by sampling the levels once when entering idle. */
    if (GPIO_ReadPin(KEY_INT_GPIO, KEY_INT_PIN) == 0U || Keyboard_ReadOuts() != 7U)
    {
        EXTI->IMR &= ~KEY_EXTI_MASK;
        EXTI->PR = KEY_EXTI_MASK;
        keyboard_active = 1U;
    }
}

/* Маскирует INT и будит сканер; задержек и сканирования внутри обработчика нет. */
void EXTI4_IRQHandler(void)
{
    if ((EXTI->PR & KEY_EXTI_MASK) != 0U)
    {
        /* The timer performs the scan; no GPIO clocking or delays here. */
        EXTI->IMR &= ~KEY_EXTI_MASK;
        EXTI->PR = KEY_EXTI_MASK;
        keyboard_active = 1U;
    }
}

/* Подтверждает изменение после стабильных 10 мс; события возникают только на нажатие. */
static void Keyboard_Debounce(uint32_t raw_mask)
{
    uint8_t i;
    key_raw_mask_debug = raw_mask;
    for (i = 0U; i < (uint8_t)INPUT_KEY_COUNT; i++)
    {
        uint8_t raw = 0U;
        if (i >= (uint8_t)Key_2_1)
        {
            raw = (uint8_t)((raw_mask >> (i - (uint8_t)Key_2_1)) & 1U);
        }
        if (raw != key_last_raw[i])
        {
            key_last_raw[i] = raw;
            key_change_tick[i] = input_ticks;
        }
        else if (key_state[i] != raw &&
                 (uint32_t)(input_ticks - key_change_tick[i]) >= KEY_DEBOUNCE_TICKS)
        {
            key_state[i] = raw;
            if (raw != 0U && key_pressed_events[i] < 255U)
            {
                key_pressed_events[i]++;
            }
        }
    }
}

/* Обрабатывает одну позицию CD4017; после устойчивого отпускания снова разрешает INT. */
static void Keyboard_Poll500us(void)
{
    uint8_t outs = Keyboard_ReadOuts();
    uint8_t out;
    if (scan_position == 0U)
    {
        scan_baseline = (uint8_t)(~outs & 7U);
        if (scan_baseline == 0U)
        {
            Keyboard_Debounce(0U);
            if (release_pending == 0U)
            {
                release_pending = 1U;
                release_tick = input_ticks;
            }
            else if ((uint32_t)(input_ticks - release_tick) >= KEY_DEBOUNCE_TICKS)
            {
                release_pending = 0U;
                Keyboard_ArmInterrupt();
            }
            return;
        }
        release_pending = 0U;
        scan_mask = 0U;
        Keyboard_Clock();
        scan_position = 1U;
        return;
    }

    for (out = 0U; out < 3U; out++)
    {
        if ((outs & scan_baseline & (1U << out)) != 0U)
        {
            scan_mask |= 1UL << ((scan_position - 1U) * 3U + out);
        }
    }
    if (scan_position == 9U)
    {
        Keyboard_Reset();
        scan_position = 0U;
        Keyboard_Debounce(scan_mask);
    }
    else
    {
        Keyboard_Clock();
        scan_position++;
    }
}

/* Настраивает энкодер и CD4017, сбрасывает счётчик в Q0 и разрешает EXTI4. */
void Input_Init(void)
{
    uint8_t i;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    GPIO_ConfigInputPullup(GPIOB, ENC_A_PIN);
    GPIO_ConfigInputPullup(GPIOB, ENC_B_PIN);
    /* Preload output levels before switching the GPIO modes. */
    KEY_CLK_GPIO->BRR = 1UL << KEY_CLK_PIN;
    KEY_RESET_GPIO->BSRR = 1UL << KEY_RESET_PIN;
    GPIO_ConfigPushPullOutput(KEY_CLK_GPIO, KEY_CLK_PIN);
    GPIO_ConfigPushPullOutput(KEY_RESET_GPIO, KEY_RESET_PIN);
    GPIO_ConfigInputPullup(KEY_INT_GPIO, KEY_INT_PIN);
    for (i = 0U; i < 3U; i++)
    {
        GPIO_ConfigInputPullup(KEY_OUT_GPIO, (uint8_t)(KEY_OUT_FIRST_PIN + i));
    }
    /* PA4 -> EXTI4, falling edge only. Configure before arming. */
    EXTI->IMR &= ~KEY_EXTI_MASK;
    AFIO->EXTICR[1] &= ~0xFUL;
    EXTI->RTSR &= ~KEY_EXTI_MASK;
    EXTI->FTSR |= KEY_EXTI_MASK;
    EXTI->PR = KEY_EXTI_MASK;
    NVIC_ClearPendingIRQ(EXTI4_IRQn);
    /* Same priority as TIM2: keyboard state updates cannot preempt each other. */
    NVIC_SetPriority(EXTI4_IRQn, 2U);
    Keyboard_Reset();
    keyboard_active = 0U;
    release_pending = 0U;
    release_tick = 0U;
    scan_position = 0U;
    scan_baseline = 0U;
    scan_mask = 0U;
    input_ticks = 0U;
    for (i = 0U; i < (uint8_t)INPUT_KEY_COUNT; i++)
    {
        key_state[i] = 0U;
        key_last_raw[i] = 0U;
        key_change_tick[i] = 0U;
        key_pressed_events[i] = 0U;
    }
    key_raw_mask_debug = 0U;
    encoder_last_state = Encoder_ReadAB();
    encoder_accum = 0;
    encoder_steps = 0;
    encoder_raw_counts = 0;
    NVIC_EnableIRQ(EXTI4_IRQn);
    Keyboard_ArmInterrupt();
}

/* Вызывается из TIM2 каждые 500 мкс: энкодер всегда, клавиатура только после INT. */
void Input_Poll500us(void)
{
    uint8_t new_state;
    uint8_t index;
    int8_t delta;
    input_ticks++;
    new_state = Encoder_ReadAB();

    if (new_state != encoder_last_state)
    {
        index = (uint8_t)((encoder_last_state << 2U) | new_state);
        delta = encoder_transition_table[index];

        if (delta != 0)
        {
            encoder_raw_counts += delta;
            encoder_accum += delta;

            while (encoder_accum >= ENCODER_COUNTS_PER_STEP)
            {
                encoder_accum -= ENCODER_COUNTS_PER_STEP;
                encoder_steps++;
            }

            while (encoder_accum <= -ENCODER_COUNTS_PER_STEP)
            {
                encoder_accum += ENCODER_COUNTS_PER_STEP;
                encoder_steps--;
            }
        }

        encoder_last_state = new_state;
    }

    /* Idle keyboard cost: one flag check, with no GPIO reads or scanning. */
    if (keyboard_active != 0U)
    {
        Keyboard_Poll500us();
    }
}

/* Атомарно забирает накопленные щелчки; число переходов задаёт ENCODER_COUNTS_PER_STEP. */
int32_t Input_GetEncoderSteps(void)
{
    uint32_t primask;
    int32_t steps;

    primask = Input_CriticalEnter();

    steps = encoder_steps;
    encoder_steps = 0;

    Input_CriticalExit(primask);

    return steps;
}

/* Забирает одно событие нажатия после подавления дребезга. */
uint8_t Input_KeyPressed(input_key_t key)
{
    uint32_t primask;
    uint8_t result;

    if (key >= INPUT_KEY_COUNT)
    {
        return 0U;
    }

    result = 0U;

    primask = Input_CriticalEnter();

    if (key_pressed_events[key] != 0U)
    {
        key_pressed_events[key]--;
        result = 1U;
    }

    Input_CriticalExit(primask);

    return result;
}

/* Возвращает подтверждённое состояние удержания без извлечения события. */
uint8_t Input_KeyIsDown(input_key_t key)
{
    if (key >= INPUT_KEY_COUNT)
    {
        return 0U;
    }

    if (key_state[key] != 0U)
    {
        return 1U;
    }

    return 0U;
}

char Input_KeyToChar(input_key_t key)
{
    if (key == INPUT_KEY_0)
    {
        return '0';
    }

    if ((key >= INPUT_KEY_1) && (key <= INPUT_KEY_3))
    {
        return (char)('1' + (uint8_t)key);
    }

    if ((key >= INPUT_KEY_4) && (key <= INPUT_KEY_6))
    {
        return (char)('4' + ((uint8_t)key - (uint8_t)INPUT_KEY_4));
    }

    if ((key >= INPUT_KEY_7) && (key <= INPUT_KEY_9))
    {
        return (char)('7' + ((uint8_t)key - (uint8_t)INPUT_KEY_7));
    }

    if (key == INPUT_KEY_DOT)
    {
        return '.';
    }

    return '\0';
}

/* Возвращает маску 27 физических клавиш последнего полного прохода. */
uint32_t Input_GetRawKeyMask(void)
{
    return key_raw_mask_debug;
}

/* Возвращает Key_A_B: A — физический вывод CD4017, B — номер OUT. */
const char *Input_KeyName(input_key_t key)
{
    if (key >= Key_2_1 && key < INPUT_KEY_COUNT)
    {
        return physical_key_names[(uint8_t)key - (uint8_t)Key_2_1];
    }
    return "";
}
