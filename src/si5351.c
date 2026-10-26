#include "si5351.h"
#include "i2c_soft.h"

#define SI5351_ADDR_7BIT 0x60U

#define SI5351_REG_OUTPUT_ENABLE 3U
#define SI5351_REG_CLK0_CTRL 16U
#define SI5351_REG_PLLA_PARAMS 26U
#define SI5351_REG_MS0_PARAMS 42U
#define SI5351_REG_PLL_RESET 177U
#define SI5351_REG_XTAL_LOAD 183U

//#define SI5351_PLLA_HZ              900000000UL
#define SI5351_PLLA_HZ 899960100UL
#define SI5351_FRAC_DENOM 1048575UL

#define SI5351_CLK_SRC_MS 0x0CU
#define SI5351_CLK_DRIVE_8MA 0x03U
#define SI5351_CLK_INTEGER_MODE 0x40U
#define SI5351_CLK_POWER_DOWN 0x80U

#define SI5351_XTAL_LOAD_10PF 0xD2U

static uint8_t si5351_oeb_mask = 0xFFU;

static si5351_status_t Si5351_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t data[2];

    data[0] = reg;
    data[1] = value;

    if (I2C_Soft_Write(SI5351_ADDR_7BIT, data, 2U) != I2C_SOFT_OK)
    {
        return SI5351_ERR_I2C;
    }

    return SI5351_OK;
}


static si5351_status_t Si5351_WriteBulk(uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t buf[9];
    uint8_t i;

    if ((data == 0) || (len == 0U) || (len > 8U))
    {
        return SI5351_ERR_ARG;
    }

    buf[0] = reg;

    for (i = 0U; i < len; i++)
    {
        buf[i + 1U] = data[i];
    }

    if (I2C_Soft_Write(SI5351_ADDR_7BIT, buf, (uint16_t)(len + 1U)) != I2C_SOFT_OK)
    {
        return SI5351_ERR_I2C;
    }

    return SI5351_OK;
}


static uint32_t Si5351_Gcd(uint32_t a, uint32_t b)
{
    while (b != 0U)
    {
        uint32_t t;

        t = b;
        b = a % b;
        a = t;
    }

    return a;
}


/* Упаковывает P1/P2/P3 и R-divider в восемь байтов регистров Si5351. */
static void Si5351_PackParams(uint32_t p1, uint32_t p2, uint32_t p3, uint8_t r_div_code, uint8_t *regs)
{
    regs[0] = (uint8_t)((p3 >> 8) & 0xFFU);
    regs[1] = (uint8_t)(p3 & 0xFFU);
    regs[2] = (uint8_t)(((r_div_code & 0x07U) << 4U) | ((p1 >> 16) & 0x03U));
    regs[3] = (uint8_t)((p1 >> 8) & 0xFFU);
    regs[4] = (uint8_t)(p1 & 0xFFU);
    regs[5] = (uint8_t)(((p3 >> 12) & 0xF0U) | ((p2 >> 16) & 0x0FU));
    regs[6] = (uint8_t)((p2 >> 8) & 0xFFU);
    regs[7] = (uint8_t)(p2 & 0xFFU);
}


/* Вычисляет целую/дробную части делителя и R-divider в допустимых пределах. */
static uint8_t Si5351_CalcMultisynth(uint32_t fout_hz, uint32_t *p1, uint32_t *p2, uint32_t *p3, uint8_t *r_div_code,
                                     uint8_t *integer_mode)
{
    uint32_t r_div;
    uint8_t r_code;
    uint64_t denom;
    uint32_t a;
    uint64_t rem;
    uint32_t b;
    uint32_t c;
    uint32_t floor_term;

    if (fout_hz == 0U)
    {
        return 0U;
    }

    r_div = 1U;
    r_code = 0U;

    while (((uint64_t)SI5351_PLLA_HZ > ((uint64_t)fout_hz * r_div * 2048ULL)) && (r_div < 128U))
    {
        r_div <<= 1U;
        r_code++;
    }

    denom = (uint64_t)fout_hz * r_div;

    a = (uint32_t)(SI5351_PLLA_HZ / denom);
    rem = (uint64_t)SI5351_PLLA_HZ % denom;

    c = SI5351_FRAC_DENOM;
    b = (uint32_t)(((rem * c) + (denom / 2U)) / denom);

    if (b >= c)
    {
        a++;
        b = 0U;
    }

    if ((a < 8U) || (a > 2048U) || ((a == 2048U) && (b != 0U)))
    {
        return 0U;
    }

    if (b == 0U)
    {
        c = 1U;
    }
    else
    {
        uint32_t g;

        g = Si5351_Gcd(b, c);
        b /= g;
        c /= g;
    }

    floor_term = (uint32_t)(((uint64_t)128U * b) / c);

    *p1 = (128U * a) + floor_term - 512U;
    *p2 = (uint32_t)(((uint64_t)128U * b) - ((uint64_t)c * floor_term));
    *p3 = c;
    *r_div_code = r_code;

    if ((b == 0U) && ((a & 1U) == 0U))
    {
        *integer_mode = 1U;
    }
    else
    {
        *integer_mode = 0U;
    }

    return 1U;
}


/* Меняет только бит выбранного выхода в общей маске регистра 3. */
static si5351_status_t Si5351_OutputEnable(uint8_t clk, uint8_t enable)
{
    if (clk > 2U)
    {
        return SI5351_ERR_ARG;
    }

    if (enable != 0U)
    {
        si5351_oeb_mask &= (uint8_t) ~(1U << clk);
    }
    else
    {
        si5351_oeb_mask |= (uint8_t)(1U << clk);
    }

    return Si5351_WriteReg(SI5351_REG_OUTPUT_ENABLE, si5351_oeb_mask);
}


static si5351_status_t Si5351_SetPLLA_900MHz(void)
{
    uint8_t regs[8];

    /*
     * 25 MHz * 36 = 900 MHz.
     * P1 = 128 * 36 - 512 = 4096.
     */
    Si5351_PackParams(4096U, 0U, 1U, 0U, regs);

    return Si5351_WriteBulk(SI5351_REG_PLLA_PARAMS, regs, 8U);
}


/* Отключает выбранный CLK, обновляет делитель и включает CLK после успешного обмена. */
si5351_status_t Si5351_SetFreq(uint8_t clk, uint32_t fout_hz)
{
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
    uint8_t r_div_code;
    uint8_t regs[8];
    uint8_t integer_mode;
    uint8_t ctrl;
    si5351_status_t st;

    if (clk > 2U)
    {
        return SI5351_ERR_ARG;
    }

    if (Si5351_CalcMultisynth(fout_hz, &p1, &p2, &p3, &r_div_code, &integer_mode) == 0U)
    {
        return SI5351_ERR_ARG;
    }

    st = Si5351_OutputEnable(clk, 0U);
    if (st != SI5351_OK)
    {
        return st;
    }

    Si5351_PackParams(p1, p2, p3, r_div_code, regs);

    st = Si5351_WriteBulk((uint8_t)(SI5351_REG_MS0_PARAMS + (8U * clk)), regs, 8U);
    if (st != SI5351_OK)
    {
        return st;
    }

    ctrl = SI5351_CLK_SRC_MS | SI5351_CLK_DRIVE_8MA;

    if (integer_mode != 0U)
    {
        ctrl |= SI5351_CLK_INTEGER_MODE;
    }

    st = Si5351_WriteReg((uint8_t)(SI5351_REG_CLK0_CTRL + clk), ctrl);
    if (st != SI5351_OK)
    {
        return st;
    }

    return Si5351_OutputEnable(clk, 1U);
}


/* Настраивает гетеродин выше принимаемой частоты: LO = RF + ПЧ. */
si5351_status_t Si5351_SetVfoHighSide(uint8_t clk, uint32_t rf_hz, uint32_t if_hz)
{
    return Si5351_SetFreq(clk, rf_hz + if_hz);
}


/* Отключает выходы, настраивает нагрузку кварца и PLLA, затем сбрасывает PLL. */
si5351_status_t Si5351_Init(void)
{
    si5351_status_t st;
    uint8_t i;

    si5351_oeb_mask = 0xFFU;

    st = Si5351_WriteReg(SI5351_REG_OUTPUT_ENABLE, si5351_oeb_mask);
    if (st != SI5351_OK)
    {
        return st;
    }

    for (i = 0U; i < 8U; i++)
    {
        st = Si5351_WriteReg((uint8_t)(SI5351_REG_CLK0_CTRL + i), SI5351_CLK_POWER_DOWN);
        if (st != SI5351_OK)
        {
            return st;
        }
    }

    st = Si5351_WriteReg(SI5351_REG_XTAL_LOAD, SI5351_XTAL_LOAD_10PF);
    if (st != SI5351_OK)
    {
        return st;
    }

    st = Si5351_SetPLLA_900MHz();
    if (st != SI5351_OK)
    {
        return st;
    }

    st = Si5351_WriteReg(SI5351_REG_PLL_RESET, 0x20U);
    if (st != SI5351_OK)
    {
        return st;
    }

    return SI5351_OK;
}
