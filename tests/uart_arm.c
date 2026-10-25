/* Настоящий драйвер UART исполняется с моделью регистров STM32, включая тайм-аут. */
#include "uart1.h"
#include "uart_config.c"
#include <string.h>
static USART_TypeDef fake_usart;
static RCC_TypeDef fake_rcc;
static GPIO_TypeDef fake_gpio;
#undef USART1
#undef RCC
#undef GPIOA
#define USART1 (&fake_usart)
#define RCC (&fake_rcc)
#define GPIOA (&fake_gpio)
#include "uart1.c"
#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

int test_uart_rates_framing_and_ninth_data_bit(void)
{
    uint8_t i;
    uart_config_t config = {1200U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    memset(&fake_usart, 0, sizeof(fake_usart)); fake_usart.SR = USART_SR_TC | USART_SR_TXE;
    UART1_Init(115200U); CHECK(fake_usart.BRR == 625U);
    for (i = 0U; i < UART_BAUD_COUNT; i++)
    {
        config.baud = uart_baud_rates[i]; CHECK(UART1_ApplyConfig(&config) != 0U);
        CHECK(fake_usart.BRR == (72000000U + config.baud / 2U) / config.baud);
    }
    config.parity = UART_PARITY_EVEN; CHECK(UART1_ApplyConfig(&config) != 0U);
    CHECK((fake_usart.CR1 & (USART_CR1_M | USART_CR1_PCE | USART_CR1_PS)) == (USART_CR1_M | USART_CR1_PCE));
    config.parity = UART_PARITY_ODD; CHECK(UART1_ApplyConfig(&config) != 0U);
    CHECK((fake_usart.CR1 & USART_CR1_PS) != 0U);
    config.data_bits = 9U; CHECK(UART1_ApplyConfig(&config) == 0U);
    config.parity = UART_PARITY_NONE; CHECK(UART1_ApplyConfig(&config) != 0U);
    CHECK((fake_usart.CR1 & USART_CR1_M) != 0U && (fake_usart.CR1 & USART_CR1_PCE) == 0U);
    UART1_SendWord(0x1ABU); CHECK(fake_usart.DR == 0x1ABU);
    config.data_bits = 8U; CHECK(UART1_ApplyConfig(&config) != 0U);
    UART1_SendWord(0x1ABU); CHECK(fake_usart.DR == 0xABU);
    for (i = 0U; i < 4U; i++)
    {
        static const uint8_t codes[] = {0U, 2U, 1U, 3U};
        config.stop = i; CHECK(UART1_ApplyConfig(&config) != 0U);
        CHECK(fake_usart.CR2 == (uint32_t)codes[i] << 12U);
    }
    return 0;
}

int test_uart_invalid_or_busy_configuration_leaves_registers_unchanged(void)
{
    uart_config_t config = {115200U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
    uint32_t cr1, cr2, brr;
    fake_usart.SR = USART_SR_TC; CHECK(UART1_ApplyConfig(&config) != 0U);
    cr1 = fake_usart.CR1; cr2 = fake_usart.CR2; brr = fake_usart.BRR;
    config.baud = 0U; CHECK(UART1_ApplyConfig(&config) == 0U);
    config.baud = 1200U; config.data_bits = 7U; CHECK(UART1_ApplyConfig(&config) == 0U);
    config.data_bits = 8U; config.parity = 3U; CHECK(UART1_ApplyConfig(&config) == 0U);
    config.parity = UART_PARITY_NONE; fake_usart.SR = 0U;
    CHECK(UART1_ApplyConfig(&config) == 0U);
    CHECK(fake_usart.CR1 == cr1 && fake_usart.CR2 == cr2 && fake_usart.BRR == brr);
    CHECK(UART1_GetConfig()->baud == 115200U);
    return 0;
}
