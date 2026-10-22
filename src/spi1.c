#include "spi1.h"

/*
 * SPI1 для дисплея GMG12864.
 *
 * PA5 = SPI1_SCK
 * PA6 = SPI1_MISO, можно не подключать
 * PA7 = SPI1_MOSI
 *
 * CS, RST и DC настраиваются в gmg12864.c.
 */

#define SPI1_SCK_PIN                5U
#define SPI1_MISO_PIN               6U
#define SPI1_MOSI_PIN               7U

#define SPI1_TIMEOUT                1000000UL

static spi1_status_t SPI1_WaitFlagSet(uint16_t flag)
{
    uint32_t timeout;

    timeout = SPI1_TIMEOUT;

    while ((SPI1->SR & flag) == 0U)
    {
        if (timeout == 0U)
        {
            return SPI1_ERR_TIMEOUT;
        }

        timeout--;
    }

    return SPI1_OK;
}

static spi1_status_t SPI1_WaitNotBusy(void)
{
    uint32_t timeout;

    timeout = SPI1_TIMEOUT;

    while ((SPI1->SR & SPI_SR_BSY) != 0U)
    {
        if (timeout == 0U)
        {
            return SPI1_ERR_TIMEOUT;
        }

        timeout--;
    }

    return SPI1_OK;
}

/* Настраивает SPI1 для LCD: режим 0, APB2/4 = 18 МГц. */
void SPI1_LCD_Init(void)
{
    /*
     * GPIOA, AFIO и SPI1.
     */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_AFIOEN |
                    RCC_APB2ENR_SPI1EN;

    /*
     * PA5 = SCK, alternate function push-pull, 50 MHz.
     * MODE5 = 11, CNF5 = 10, поле = 0xB.
     */
    GPIOA->CRL &= ~(0xFUL << (SPI1_SCK_PIN * 4U));
    GPIOA->CRL |=  (0xBUL << (SPI1_SCK_PIN * 4U));

    /*
     * PA6 = MISO, input floating.
     * Для дисплея обычно не нужен, но пусть будет входом.
     */
    GPIOA->CRL &= ~(0xFUL << (SPI1_MISO_PIN * 4U));
    GPIOA->CRL |=  (0x4UL << (SPI1_MISO_PIN * 4U));

    /*
     * PA7 = MOSI, alternate function push-pull, 50 MHz.
     * MODE7 = 11, CNF7 = 10, поле = 0xB.
     */
    GPIOA->CRL &= ~(0xFUL << (SPI1_MOSI_PIN * 4U));
    GPIOA->CRL |=  (0xBUL << (SPI1_MOSI_PIN * 4U));

    /*
     * Отключаем SPI перед настройкой.
     */
    SPI1->CR1 = 0U;
    SPI1->CR2 = 0U;

    /*
     * SPI master, mode 0, 8 bit, MSB first, software NSS.
     *
     * PCLK2 = 72 MHz.
     * BR = 001: делитель /4, SCK = 18 MHz.
     * Для первого запуска это безопаснее, чем 12 MHz из исходного HAL-проекта.
     */
    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_SSM |
                SPI_CR1_SSI |
                SPI_CR1_BR_0; // | SPI_CR1_BR_0;

    SPI1->CR1 |= SPI_CR1_SPE;
}

/* Передаёт байт с ограниченным ожиданием флагов и завершения передачи. */
spi1_status_t SPI1_SendByte(uint8_t data)
{
    spi1_status_t st;
    volatile uint16_t dummy;

    st = SPI1_WaitFlagSet(SPI_SR_TXE);
    if (st != SPI1_OK)
    {
        return st;
    }

    *((volatile uint8_t *)&SPI1->DR) = data;

    st = SPI1_WaitFlagSet(SPI_SR_RXNE);
    if (st != SPI1_OK)
    {
        return st;
    }

    dummy = (uint16_t)SPI1->DR;
    (void)dummy;

    st = SPI1_WaitNotBusy();

    return st;
}
