#include "uart1.h"

#define UART1_PCLK_HZ               72000000UL

static void GPIO_ConfigAFPushPull(GPIO_TypeDef *gpio, uint8_t pin)
{
    volatile uint32_t *reg;
    uint32_t shift;

    if (pin < 8U)
    {
        reg = &gpio->CRL;
        shift = pin * 4U;
    }
    else
    {
        reg = &gpio->CRH;
        shift = (pin - 8U) * 4U;
    }

    /*
     * MODE = 11: output 50 MHz.
     * CNF  = 10: alternate function push-pull.
     */
    *reg &= ~(0xFUL << shift);
    *reg |=  (0xBUL << shift);
}

static void GPIO_ConfigInputFloating(GPIO_TypeDef *gpio, uint8_t pin)
{
    volatile uint32_t *reg;
    uint32_t shift;

    if (pin < 8U)
    {
        reg = &gpio->CRL;
        shift = pin * 4U;
    }
    else
    {
        reg = &gpio->CRH;
        shift = (pin - 8U) * 4U;
    }

    /*
     * MODE = 00.
     * CNF  = 01: floating input.
     */
    *reg &= ~(0xFUL << shift);
    *reg |=  (0x4UL << shift);
}

/* Настраивает PA9/PA10 и скорость UART1; частота APB2 — 72 МГц. */
void UART1_Init(uint32_t baudrate)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_AFIOEN |
                    RCC_APB2ENR_USART1EN;

    /*
     * PA9  = USART1_TX
     * PA10 = USART1_RX
     */
    GPIO_ConfigAFPushPull(GPIOA, 9U);
    GPIO_ConfigInputFloating(GPIOA, 10U);

    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    /*
     * Для oversampling by 16 регистр BRR можно считать как PCLK / baud.
     * Для 72 MHz и 115200 получается 625 = 0x0271.
     */
    USART1->BRR = (uint16_t)((UART1_PCLK_HZ + (baudrate / 2U)) / baudrate);

    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/* Передаёт символ, ожидая свободного регистра передатчика. */
void UART1_SendChar(char ch)
{
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }

    USART1->DR = (uint16_t)ch;
}

/* Передаёт строку с нулевым завершением без добавления перевода строки. */
void UART1_SendString(const char *text)
{
    while (*text != '\0')
    {
        UART1_SendChar(*text);
        text++;
    }
}

/* Передаёт беззнаковое число десятичными цифрами без printf. */
void UART1_SendU32(uint32_t value)
{
    char buf[11];
    uint8_t i;
    uint8_t j;

    if (value == 0U)
    {
        UART1_SendChar('0');
        return;
    }

    i = 0U;

    while ((value != 0U) && (i < sizeof(buf)))
    {
        buf[i] = (char)('0' + (value % 10U));
        value /= 10U;
        i++;
    }

    for (j = i; j > 0U; j--)
    {
        UART1_SendChar(buf[j - 1U]);
    }
}
