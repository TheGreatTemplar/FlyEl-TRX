#include "i2c2_soft.h"
#include "stm32f10x.h"
#include "board_pins.h"

#define SCL_MASK (1UL << I2C2_SCL_PIN)
#define SDA_MASK (1UL << I2C2_SDA_PIN)
#define CLOCK_WAIT_LIMIT 10000UL

static void Delay(void)
{
    volatile uint32_t i;
    for (i = 0U; i < 80U; i++)
    {
        __NOP();
    }
}


static void Sda(uint8_t high)
{
    if (high != 0U)
        I2C2_GPIO->BSRR = SDA_MASK;
    else
        I2C2_GPIO->BRR = SDA_MASK;
}


static void SclLow(void)
{
    I2C2_GPIO->BRR = SCL_MASK;
}


/* Освобождает SCL и ждёт фактического высокого уровня с ограничением времени. */
static i2c2_soft_status_t SclHigh(void)
{
    uint32_t remaining = CLOCK_WAIT_LIMIT;
    I2C2_GPIO->BSRR = SCL_MASK;
    while ((I2C2_GPIO->IDR & SCL_MASK) == 0U)
    {
        if (remaining-- == 0U)
            return I2C2_SOFT_TIMEOUT;
    }
    return I2C2_SOFT_OK;
}


static i2c2_soft_status_t Start(void)
{
    Sda(1U);
    Delay();
    if (SclHigh() != I2C2_SOFT_OK)
        return I2C2_SOFT_TIMEOUT;
    Delay();
    if ((I2C2_GPIO->IDR & SDA_MASK) == 0U)
        return I2C2_SOFT_BUS_ERROR;
    Sda(0U);
    Delay();
    SclLow();
    Delay();
    return I2C2_SOFT_OK;
}


static i2c2_soft_status_t Stop(void)
{
    i2c2_soft_status_t status;
    SclLow();
    Sda(0U);
    Delay();
    status = SclHigh();
    Delay();
    Sda(1U);
    Delay();
    if (status != I2C2_SOFT_OK)
        return status;
    return ((I2C2_GPIO->IDR & SDA_MASK) != 0U) ? I2C2_SOFT_OK : I2C2_SOFT_BUS_ERROR;
}


static i2c2_soft_status_t WriteByte(uint8_t value)
{
    uint8_t bit;
    uint8_t ack;
    for (bit = 0U; bit < 8U; bit++)
    {
        Sda((uint8_t)((value & 0x80U) != 0U));
        Delay();
        if (SclHigh() != I2C2_SOFT_OK)
            return I2C2_SOFT_TIMEOUT;
        Delay();
        SclLow();
        Delay();
        value <<= 1U;
    }
    Sda(1U);
    Delay();
    if (SclHigh() != I2C2_SOFT_OK)
        return I2C2_SOFT_TIMEOUT;
    Delay();
    ack = (uint8_t)((I2C2_GPIO->IDR & SDA_MASK) == 0U);
    SclLow();
    Delay();
    return (ack != 0U) ? I2C2_SOFT_OK : I2C2_SOFT_NACK;
}


static i2c2_soft_status_t ReadByte(uint8_t *value, uint8_t ack)
{
    uint8_t bit;
    uint8_t result = 0U;
    Sda(1U);
    for (bit = 0U; bit < 8U; bit++)
    {
        Delay();
        if (SclHigh() != I2C2_SOFT_OK)
            return I2C2_SOFT_TIMEOUT;
        Delay();
        result = (uint8_t)((result << 1U) | ((I2C2_GPIO->IDR & SDA_MASK) != 0U));
        SclLow();
        Delay();
    }
    Sda((uint8_t)(ack == 0U));
    Delay();
    if (SclHigh() != I2C2_SOFT_OK)
        return I2C2_SOFT_TIMEOUT;
    Delay();
    SclLow();
    Sda(1U);
    Delay();
    *value = result;
    return I2C2_SOFT_OK;
}


/* Настраивает PB8/PB9 как open-drain для отдельной шины EEPROM. */
void I2C2_Soft_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    /* PB8/PB9, open drain 2 MHz. External I2C pull-ups are required. */
    I2C2_GPIO->BSRR = SCL_MASK | SDA_MASK;
    I2C2_GPIO->CRH = (I2C2_GPIO->CRH & ~0xFFUL) | 0x66UL;
}


/* Проверяет ACK адреса без записи данных в память. */
i2c2_soft_status_t I2C2_Soft_Probe(uint8_t address)
{
    i2c2_soft_status_t status;
    i2c2_soft_status_t stopped;
    if (address > 0x7FU)
        return I2C2_SOFT_ARG_ERROR;
    status = Start();
    if (status == I2C2_SOFT_OK)
        status = WriteByte((uint8_t)(address << 1U));
    stopped = Stop();
    return (status == I2C2_SOFT_OK) ? stopped : status;
}


/* STOP запускает внутреннюю запись EEPROM; ожидание готовности — задача вызывающего. */
i2c2_soft_status_t I2C2_Soft_Write(uint8_t address, const uint8_t *data, uint16_t length)
{
    i2c2_soft_status_t status;
    i2c2_soft_status_t stopped;
    uint16_t i;
    if (address > 0x7FU || data == 0 || length == 0U)
        return I2C2_SOFT_ARG_ERROR;
    status = Start();
    if (status == I2C2_SOFT_OK)
        status = WriteByte((uint8_t)((uint32_t)address << 1U));
    for (i = 0U; status == I2C2_SOFT_OK && i < length; i++)
        status = WriteByte(data[i]);
    stopped = Stop();
    return (status == I2C2_SOFT_OK) ? stopped : status;
}


/* Передаёт префикс, repeated START и чтение; последний байт завершается NACK. */
i2c2_soft_status_t I2C2_Soft_WriteRead(uint8_t address, const uint8_t *prefix, uint16_t prefix_len, uint8_t *data,
                                       uint16_t length)
{
    i2c2_soft_status_t status;
    i2c2_soft_status_t stopped;
    uint16_t i;
    if (address > 0x7FU || prefix == 0 || prefix_len == 0U || data == 0 || length == 0U)
        return I2C2_SOFT_ARG_ERROR;
    status = Start();
    if (status == I2C2_SOFT_OK)
        status = WriteByte((uint8_t)(address << 1U));
    for (i = 0U; status == I2C2_SOFT_OK && i < prefix_len; i++)
        status = WriteByte(prefix[i]);
    /* Repeated START: the prefix selects the address without programming data. */
    if (status == I2C2_SOFT_OK)
        status = Start();
    if (status == I2C2_SOFT_OK)
        status = WriteByte((uint8_t)(((uint32_t)address << 1U) | 1U));
    for (i = 0U; status == I2C2_SOFT_OK && i < length; i++)
        status = ReadByte(&data[i], (uint8_t)(i + 1U < length));
    stopped = Stop();
    return (status == I2C2_SOFT_OK) ? stopped : status;
}
