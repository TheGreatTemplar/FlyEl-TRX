#include "stm32f10x.h"
#include "spi1.h"
#include "gmg12864.h"
#include "input.h"
#include "i2c_soft.h"
#include "i2c2_soft.h"
#include "uart1.h"
#include "adc1_app.h"
#include "generator_menu.h"
#include "startup_splash.h"
#include "app_time.h"
#include "boot_startup.h"
#include <stdint.h>

/*
 * STM32F103C8T6
 * HSE: 8 MHz
 * SYSCLK: 72 MHz
 * TIM2 interrupt: 2000 Hz
 * LCD GMG12864: SPI1 + PB10/PB11/PB12
 * Menu: font_tahoma_8_prop.h
 *
 * Среда: Keil uVision 5.36, ArmClang 6.16
 * Кодировка файла: Windows-1251
 */

#define HSE_VALUE_HZ 8000000UL
#define SYSCLK_HZ 72000000UL

#define TIMER_IRQ_HZ 2000UL
#define LED_TOGGLE_TICKS (TIMER_IRQ_HZ / 2UL)

#define LED_ACTIVE_LOW 1

#define PC13_PIN 13U
#define PC13_MASK (1UL << PC13_PIN)

uint32_t SystemCoreClock = HSE_VALUE_HZ;

void SystemInit(void);
void TIM2_IRQHandler(void);

static volatile uint32_t g_led_ticks;
static volatile uint32_t g_tick_500us;

void SystemInit(void)
{
}


__attribute__((noreturn)) static void Error_Handler(void)
{
    while (1)
    {
    }
}


/* Запускает PLL x9 от кварца 8 МГц; APB1 делится на 2, ADC — на 6. */
static void Clock_Init_72MHz_HSE8(void)
{
    uint32_t timeout;

    RCC->CR |= RCC_CR_HSEON;

    timeout = 1000000UL;
    while ((RCC->CR & RCC_CR_HSERDY) == 0U)
    {
        if (timeout == 0U)
        {
            Error_Handler();
        }

        timeout--;
    }

    FLASH->ACR &= ~(uint32_t)FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    RCC->CR &= ~RCC_CR_PLLON;

    timeout = 1000000UL;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U)
    {
        if (timeout == 0U)
        {
            Error_Handler();
        }

        timeout--;
    }

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | RCC_CFGR_ADCPRE | RCC_CFGR_PLLSRC |
                   RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);

    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_ADCPRE_DIV6 |
                 RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9;

    RCC->CR |= RCC_CR_PLLON;

    timeout = 1000000UL;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
        if (timeout == 0U)
        {
            Error_Handler();
        }

        timeout--;
    }

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    timeout = 1000000UL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
    {
        if (timeout == 0U)
        {
            Error_Handler();
        }

        timeout--;
    }

    SystemCoreClock = SYSCLK_HZ;
}


/* Освобождает PA15/PB3/PB4 для GPIO, оставляя отладку SWD. */
static void Board_DisableJTAG_KeepSWD(void)
{
    /*
     * SWJ_CFG = 010:
     * JTAG-DP disabled, SW-DP enabled.
     * PA15, PB3 and PB4 become GPIO.
     * PA13/PA14 remain SWD.
     */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    AFIO->MAPR = (AFIO->MAPR & ~(7UL << 24U)) | (2UL << 24U);
}


static void LED_PC13_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC->CRH &= ~(0xFUL << 20U);
    GPIOC->CRH |= (0x2UL << 20U);

#if LED_ACTIVE_LOW
    GPIOC->BSRR = PC13_MASK;
#else
    GPIOC->BRR = PC13_MASK;
#endif
}


static void LED_PC13_Toggle(void)
{
    if ((GPIOC->ODR & PC13_MASK) != 0U)
    {
        GPIOC->BRR = PC13_MASK;
    }
    else
    {
        GPIOC->BSRR = PC13_MASK;
    }
}


uint32_t App_GetTick500us(void)
{
    return g_tick_500us;
}


void App_DelayMs(uint32_t ms)
{
    uint32_t start;
    uint32_t ticks;

    ticks = ms * 2U;
    start = App_GetTick500us();

    while ((uint32_t)(App_GetTick500us() - start) < ticks)
    {
    }
}


/* Запускает базу 500 мкс для ввода, задержек и временных автоматов. */
static void TIM2_Init_2000Hz(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1 = 0U;
    TIM2->DIER = 0U;
    TIM2->SR = 0U;

    /*
     * TIM2 clock = 72 MHz.
     * IRQ = 72 MHz / 36 / 1000 = 2000 Hz.
     */
    TIM2->PSC = 36U - 1U;
    TIM2->ARR = 1000U - 1U;

    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;

    TIM2->DIER = TIM_DIER_UIE;

    NVIC_SetPriority(TIM2_IRQn, 2U);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1 = TIM_CR1_CEN;
}


/* Обновляет время и драйвер ввода; блокирующие операции интерфейса здесь запрещены. */
void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0U)
    {
        TIM2->SR &= ~TIM_SR_UIF;

        g_tick_500us++;
        g_led_ticks++;

        Input_Poll500us();

        if (g_led_ticks >= LED_TOGGLE_TICKS)
        {
            g_led_ticks = 0U;
            LED_PC13_Toggle();
        }
    }
}


/* Показывает логотип три секунды и отбрасывает события, собранные во время заставки. */
static void App_ShowStartupScreen(void)
{
    uint8_t x;
    uint8_t y;
    uint8_t key;

    LCD_ClearBuffer();
    for (y = 0U; y < LCD_HEIGHT; y++)
    {
        for (x = 0U; x < LCD_WIDTH; x++)
        {
            uint8_t bits = startup_splash_bitmap[(uint16_t)y * (LCD_WIDTH / 8U) + x / 8U];
            if ((bits & (uint8_t)(0x80U >> (x % 8U))) != 0U)
            {
                LCD_DrawPixel(x, y, PX_ON);
            }
        }
    }
    LCD_Update();
    App_DelayMs(3000U);

    /* Discard input collected while the splash was visible. */
    (void)Input_GetEncoderSteps();
    for (key = 0U; key < (uint8_t)INPUT_KEY_COUNT; key++)
    {
        while (Input_KeyPressed((input_key_t)key) != 0U)
        {
        }
    }
}


/* Инициализирует периферию в зависимом порядке и запускает кооперативный основной цикл. */
int main(void)
{
    Clock_Init_72MHz_HSE8();

    Board_DisableJTAG_KeepSWD();

    LED_PC13_Init();

    Input_Init();
    I2C_Soft_Init();
    I2C2_Soft_Init();
    UART1_Init(115200UL);
    ADC1_App_Init();

    TIM2_Init_2000Hz();

    SPI1_LCD_Init();
    LCD_GPIO_Init();
    LCD_Init();

    BootStartup_Begin();
    GeneratorMenu_Init();
    BootStartup_Run();
    App_ShowStartupScreen();

    while (1)
    {
        GeneratorMenu_Task();
    }
}
