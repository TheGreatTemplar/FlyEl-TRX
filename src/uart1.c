#include "uart1.h"

#define UART1_PCLK_HZ 72000000UL
static uart_config_t uart_active_config = {115200U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};

const uart_config_t *UART1_GetConfig(void)
{
    return &uart_active_config;
}


uint8_t UART1_ApplyConfig(const uart_config_t *config)
{
    static const uint8_t stop_codes[] = {0U, 2U, 1U, 3U};
    uint32_t cr1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    uint32_t timeout = 1000000U;
    if (UARTConfig_Valid(config) == 0U)
        return 0U;
    /* M включает parity в длину слова: 8 данных с parity требуют M=1. RM0008 27.3.7. */
    if (config->data_bits == 9U || config->parity != UART_PARITY_NONE)
        cr1 |= USART_CR1_M;
    if (config->parity != UART_PARITY_NONE)
        cr1 |= USART_CR1_PCE;
    if (config->parity == UART_PARITY_ODD)
        cr1 |= USART_CR1_PS;
    if ((USART1->CR1 & USART_CR1_UE) != 0U)
    {
        while ((USART1->SR & USART_SR_TC) == 0U)
            if (--timeout == 0U)
                return 0U;
    }
    USART1->CR1 = 0U;
    USART1->CR2 = (uint16_t)((uint32_t)stop_codes[config->stop] << 12U);
    USART1->CR3 = 0U;
    USART1->BRR = (uint16_t)((UART1_PCLK_HZ + config->baud / 2U) / config->baud);
    USART1->CR1 = (uint16_t)cr1;
    uart_active_config = *config;
    return 1U;
}


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
    *reg |= (0xBUL << shift);
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
    *reg |= (0x4UL << shift);
}


/* Настраивает PA9/PA10 и скорость UART1; частота APB2 — 72 МГц. */
void UART1_Init(uint32_t baudrate)
{
    uart_config_t config = {baudrate, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN;

    /*
     * PA9  = USART1_TX
     * PA10 = USART1_RX
     */
    GPIO_ConfigAFPushPull(GPIOA, 9U);
    GPIO_ConfigInputFloating(GPIOA, 10U);

    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    if (UARTConfig_Valid(&config) == 0U)
        config.baud = 115200U;
    (void)UART1_ApplyConfig(&config);
}


/* Передаёт символ, ожидая свободного регистра передатчика. */
void UART1_SendChar(char ch)
{
    UART1_SendWord((uint16_t)(uint8_t)ch);
}


void UART1_SendWord(uint16_t word)
{
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }

    USART1->DR = word & ((uart_active_config.data_bits == 9U) ? 0x01FFU : 0x00FFU);
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
