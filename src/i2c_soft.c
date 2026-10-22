#include "i2c_soft.h"

/*
 * Программная I2C для Si5351.
 *
 * PB6 = SCL
 * PB7 = SDA
 */

#define I2C_GPIO                    GPIOB
#define I2C_SCL_PIN                 6U
#define I2C_SDA_PIN                 7U

#define I2C_SCL_MASK                (1UL << I2C_SCL_PIN)
#define I2C_SDA_MASK                (1UL << I2C_SDA_PIN)

static void I2C_Delay(void)
{
    volatile uint32_t i;

    for (i = 0U; i < 20U; i++)
    {
        __NOP();
    }
}

static void GPIO_ConfigOpenDrainOutput(GPIO_TypeDef *gpio, uint8_t pin)
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
     * MODE = 10: output 2 MHz.
     * CNF  = 01: open-drain.
     */
    *reg &= ~(0xFUL << shift);
    *reg |=  (0x6UL << shift);
}

static void SCL_High(void)
{
    I2C_GPIO->BSRR = I2C_SCL_MASK;
}

static void SCL_Low(void)
{
    I2C_GPIO->BRR = I2C_SCL_MASK;
}

static void SDA_High(void)
{
    I2C_GPIO->BSRR = I2C_SDA_MASK;
}

static void SDA_Low(void)
{
    I2C_GPIO->BRR = I2C_SDA_MASK;
}

static uint8_t SDA_Read(void)
{
    if ((I2C_GPIO->IDR & I2C_SDA_MASK) != 0U)
    {
        return 1U;
    }

    return 0U;
}

/* Освобождает программную I2C1 на PB6/PB7; внешние подтяжки обязательны. */
void I2C_Soft_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    GPIO_ConfigOpenDrainOutput(I2C_GPIO, I2C_SCL_PIN);
    GPIO_ConfigOpenDrainOutput(I2C_GPIO, I2C_SDA_PIN);

    SDA_High();
    SCL_High();
    I2C_Delay();
}

static void I2C_Start(void)
{
    SDA_High();
    SCL_High();
    I2C_Delay();

    SDA_Low();
    I2C_Delay();

    SCL_Low();
    I2C_Delay();
}

static void I2C_Stop(void)
{
    SDA_Low();
    I2C_Delay();

    SCL_High();
    I2C_Delay();

    SDA_High();
    I2C_Delay();
}

static uint8_t I2C_WriteByte(uint8_t data)
{
    uint8_t i;
    uint8_t ack;

    for (i = 0U; i < 8U; i++)
    {
        if ((data & 0x80U) != 0U)
        {
            SDA_High();
        }
        else
        {
            SDA_Low();
        }

        I2C_Delay();

        SCL_High();
        I2C_Delay();

        SCL_Low();
        I2C_Delay();

        data <<= 1U;
    }

    /*
     * ACK от ведомого.
     */
    SDA_High();
    I2C_Delay();

    SCL_High();
    I2C_Delay();

    ack = (uint8_t)(SDA_Read() == 0U);

    SCL_Low();
    I2C_Delay();

    return ack;
}

/* Передаёт адрес и буфер; при NACK выдаёт STOP и возвращает ошибку. */
i2c_soft_status_t I2C_Soft_Write(uint8_t addr7, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    I2C_Start();

    if (I2C_WriteByte((uint8_t)(addr7 << 1U)) == 0U)
    {
        I2C_Stop();
        return I2C_SOFT_ERR_NACK;
    }

    for (i = 0U; i < len; i++)
    {
        if (I2C_WriteByte(data[i]) == 0U)
        {
            I2C_Stop();
            return I2C_SOFT_ERR_NACK;
        }
    }

    I2C_Stop();

    return I2C_SOFT_OK;
}
