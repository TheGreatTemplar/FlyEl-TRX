/* Настоящий bit-bang драйвер с моделью GPIO и счётчиком тактов DWT. */
#include "stm32f10x.h"
#include <string.h>
static GPIO_TypeDef timing_gpio;
static RCC_TypeDef timing_rcc;
static DWT_Type timing_dwt;
static CoreDebug_Type timing_debug;

static DWT_Type *TimingDwt(void)
{
    timing_dwt.CYCCNT += 9U;
    return &timing_dwt;
}


#undef GPIOB
#undef RCC
#undef DWT
#undef CoreDebug
#define GPIOB (&timing_gpio)
#define RCC (&timing_rcc)
#define DWT TimingDwt()
#define CoreDebug (&timing_debug)
#include "i2c2_soft.c"
#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
            return __LINE__;                                                                                           \
    } while (0)

int test_i2c_cycle_timing_wrap_and_shared_counter_preserved(void)
{
    uint32_t before;
    memset(&timing_gpio, 0, sizeof(timing_gpio));
    memset(&timing_rcc, 0, sizeof(timing_rcc));
    memset(&timing_dwt, 0, sizeof(timing_dwt));
    memset(&timing_debug, 0, sizeof(timing_debug));
    timing_dwt.CYCCNT = 12345U;
    I2C2_Soft_Init();
    CHECK(timing_dwt.CYCCNT >= 12345U);
    CHECK((timing_dwt.CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U);
    CHECK((timing_debug.DEMCR & CoreDebug_DEMCR_TRCENA_Msk) != 0U);
    before = timing_dwt.CYCCNT;
    Delay();
    CHECK((uint32_t)(timing_dwt.CYCCNT - before) >= I2C2_DELAY_CYCLES);
    CHECK((uint32_t)(timing_dwt.CYCCNT - before) < I2C2_DELAY_CYCLES + 27U);
    timing_dwt.CYCCNT = 0xFFFFFFE0U;
    Delay();
    CHECK(timing_dwt.CYCCNT < 100U);
    CHECK((timing_gpio.CRH & 0xFFU) == 0x66U);
    return 0;
}


int test_i2c_fast_timing_keeps_nack_bus_fault_and_clock_timeout(void)
{
    timing_gpio.IDR = SCL_MASK | SDA_MASK;
    CHECK(I2C2_Soft_Probe(0x50U) == I2C2_SOFT_NACK);
    timing_gpio.IDR = SCL_MASK;
    CHECK(I2C2_Soft_Probe(0x50U) == I2C2_SOFT_BUS_ERROR);
    timing_gpio.IDR = 0U;
    CHECK(I2C2_Soft_Probe(0x50U) == I2C2_SOFT_TIMEOUT);
    CHECK(I2C2_Soft_Probe(0x80U) == I2C2_SOFT_ARG_ERROR);
    return 0;
}
