#include "gmg12864.h"
#include "spi1.h"
#include <string.h>
#include "app_time.h"

/*
 * Адаптация библиотеки gmg12864_r9ofg под чистый CMSIS.
 * Из исходного архива оставлены нужные функции:
 * - инициализация дисплея;
 * - буфер кадра 128x64;
 * - обновление дисплея по SPI;
 * - текст, пиксели, линии, прямоугольники, окружности.
 *
 * Убраны зависимости от HAL_SPI_Transmit(), HAL_Delay() и main.h.
 */

static uint8_t lcd_buffer[LCD_WIDTH * LCD_HEIGHT / 8U];

static void LCD_DelayMs(uint32_t ms)
{
    uint32_t start;
    uint32_t ticks;

    /*
     * TIM2 генерирует прерывания 2000 раз в секунду.
     * Один тик = 500 мкс.
     * Поэтому 1 мс = 2 тика.
     *
     * Важно: TIM2_Init_2000Hz() должен быть вызван до LCD_Init().
     */
    ticks = ms * 2U;
    start = App_GetTick500us();

    while ((uint32_t)(App_GetTick500us() - start) < ticks)
    {
        /*
         * Ждем. Прерывания не запрещаем.
         */
    }
}


static void LCD_PinHigh(GPIO_TypeDef *gpio, uint8_t pin)
{
    gpio->BSRR = (uint32_t)(1UL << pin);
}


static void LCD_PinLow(GPIO_TypeDef *gpio, uint8_t pin)
{
    gpio->BRR = (uint32_t)(1UL << pin);
}


static void LCD_CS_Active(void)
{
    LCD_PinLow(LCD_CS_GPIO, LCD_CS_PIN);
}


static void LCD_CS_Idle(void)
{
    LCD_PinHigh(LCD_CS_GPIO, LCD_CS_PIN);
}


static void LCD_RST_Active(void)
{
    LCD_PinLow(LCD_RST_GPIO, LCD_RST_PIN);
}


static void LCD_RST_Idle(void)
{
    LCD_PinHigh(LCD_RST_GPIO, LCD_RST_PIN);
}


static void LCD_DC_Command(void)
{
    LCD_PinLow(LCD_DC_GPIO, LCD_DC_PIN);
}


static void LCD_DC_Data(void)
{
    LCD_PinHigh(LCD_DC_GPIO, LCD_DC_PIN);
}


static void LCD_SendCommand(uint8_t command)
{
    LCD_DC_Command();
    (void)SPI1_SendByte(command);
    LCD_DC_Data();
}


static void LCD_SendData(uint8_t data)
{
    LCD_DC_Data();
    (void)SPI1_SendByte(data);
}


static void LCD_SetX(uint8_t x)
{
    LCD_SendCommand((uint8_t)(((x & 0xF0U) >> 4U) | 0x10U));
    LCD_SendCommand((uint8_t)(x & 0x0FU));
}


static void LCD_SetY(uint8_t y)
{
    LCD_SendCommand((uint8_t)((y & 0x07U) | 0xB0U));
}


static void LCD_GPIO_SetOutput(GPIO_TypeDef *gpio, uint8_t pin)
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

    *reg &= ~(0xFUL << shift);
    *reg |= (0x2UL << shift); /* output push-pull, 2 MHz */
}


/* Настраивает CS/RST/DC и устанавливает неактивные уровни сигналов LCD. */
void LCD_GPIO_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    LCD_GPIO_SetOutput(LCD_CS_GPIO, LCD_CS_PIN);
    LCD_GPIO_SetOutput(LCD_RST_GPIO, LCD_RST_PIN);
    LCD_GPIO_SetOutput(LCD_DC_GPIO, LCD_DC_PIN);

    LCD_CS_Idle();
    LCD_RST_Idle();
    LCD_DC_Data();
}


/* Очищает только RAM-буфер; для обновления экрана нужен LCD_Update. */
void LCD_ClearBuffer(void)
{
    memset(lcd_buffer, 0x00, sizeof(lcd_buffer));
}


/* Передаёт весь буфер 128x64 по восьми страницам SPI; из ISR не вызывается. */
void LCD_Update(void)
{
    uint8_t page;
    uint8_t x;

    LCD_CS_Active();

    for (page = 0U; page < 8U; page++)
    {
        LCD_SetX(0U);
        LCD_SetY(page);

        for (x = 0U; x < LCD_WIDTH; x++)
        {
            LCD_SendData(lcd_buffer[x + (LCD_WIDTH * page)]);
        }
    }

    LCD_CS_Idle();
}


/* Сбрасывает LCD, задаёт контраст/ориентацию и очищает кадр; TIM2 уже должен работать. */
void LCD_Init(void)
{
    LCD_CS_Active();

    LCD_RST_Active();
    LCD_DelayMs(10U);
    LCD_RST_Idle();
    LCD_DelayMs(10U);

    /* Bias 1/9. */
    LCD_SendCommand(0xA2U);
    LCD_DelayMs(1U);

    /* SEG и COM orientation. Если изображение зеркальное - поменять 0xA0/0xA1 или 0xC8/0xC0. */
    LCD_SendCommand(0xA0U);
    LCD_SendCommand(0xC8U);
    LCD_DelayMs(1U);

    /* Внутренний resistor ratio. */
    LCD_SendCommand(0x20U | 0x07U);
    LCD_DelayMs(1U);

    /* Power control: booster, regulator, follower. */
    LCD_SendCommand(0x28U | 0x07U);
    LCD_DelayMs(1U);

    /* Электронная регулировка контраста. */
    LCD_SendCommand(0x81U);
    LCD_SendCommand(15U);
    LCD_DelayMs(1U);

    /* Start line = 0. */
    LCD_SendCommand(0x40U);
    LCD_DelayMs(1U);

    /* Display ON, normal mode. */
    LCD_SendCommand(0xAFU);
    LCD_SendCommand(0xA6U);
    LCD_DelayMs(1U);

    LCD_CS_Idle();

    LCD_ClearBuffer();
    LCD_Update();
}


/* Меняет пиксель RAM-буфера с отсечением координат за границами экрана. */
void LCD_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    uint16_t pos;
    uint8_t mask;

    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
    {
        return;
    }

    pos = (uint16_t)x + (uint16_t)((y / 8U) * LCD_WIDTH);
    mask = (uint8_t)(1U << (y % 8U));

    if (color != 0U)
    {
        lcd_buffer[pos] |= mask;
    }
    else
    {
        lcd_buffer[pos] &= (uint8_t)~mask;
    }
}


void LCD_InvertBufferRange(uint16_t start, uint16_t end)
{
    if (start >= sizeof(lcd_buffer))
    {
        return;
    }

    if (end > sizeof(lcd_buffer))
    {
        end = sizeof(lcd_buffer);
    }

    while (start < end)
    {
        lcd_buffer[start] = (uint8_t)~lcd_buffer[start];
        start++;
    }
}


void LCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
    int16_t dx;
    int16_t dy;
    int16_t sx;
    int16_t sy;
    int16_t err;

    dx = (x1 >= x0) ? (int16_t)(x1 - x0) : (int16_t)(x0 - x1);
    dy = (y1 >= y0) ? (int16_t)(y1 - y0) : (int16_t)(y0 - y1);
    sx = (x0 < x1) ? 1 : -1;
    sy = (y0 < y1) ? 1 : -1;
    err = dx - dy;

    for (;;)
    {
        LCD_DrawPixel(x0, y0, color);

        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }

        if ((err + err) > -dy)
        {
            err -= dy;
            x0 = (uint8_t)((int16_t)x0 + sx);
        }

        if ((err + err) < dx)
        {
            err += dx;
            y0 = (uint8_t)((int16_t)y0 + sy);
        }
    }
}


void LCD_DrawRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    LCD_DrawLine(x, y, (uint8_t)(x + width), y, color);
    LCD_DrawLine(x, (uint8_t)(y + height), (uint8_t)(x + width), (uint8_t)(y + height), color);
    LCD_DrawLine(x, y, x, (uint8_t)(y + height), color);
    LCD_DrawLine((uint8_t)(x + width), y, (uint8_t)(x + width), (uint8_t)(y + height), color);
}


/* Рисует заполненную область: width/height включительны, 1x1 означает 2x2 пикселя. */
void LCD_FillRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t i;

    for (i = 0U; i <= height; i++)
    {
        LCD_DrawLine(x, (uint8_t)(y + i), (uint8_t)(x + width), (uint8_t)(y + i), color);
    }
}


void LCD_DrawCircle(uint8_t x, uint8_t y, uint8_t radius, uint8_t color)
{
    int16_t f;
    int16_t dd_f_x;
    int16_t dd_f_y;
    int16_t x0;
    int16_t y0;

    f = 1 - (int16_t)radius;
    dd_f_x = 1;
    dd_f_y = (int16_t)(-2 * (int16_t)radius);
    x0 = 0;
    y0 = radius;

    LCD_DrawPixel(x, (uint8_t)(y + radius), color);
    LCD_DrawPixel(x, (uint8_t)(y - radius), color);
    LCD_DrawPixel((uint8_t)(x + radius), y, color);
    LCD_DrawPixel((uint8_t)(x - radius), y, color);

    while (x0 < y0)
    {
        if (f >= 0)
        {
            y0--;
            dd_f_y += 2;
            f += dd_f_y;
        }

        x0++;
        dd_f_x += 2;
        f += dd_f_x;

        LCD_DrawPixel((uint8_t)(x + x0), (uint8_t)(y + y0), color);
        LCD_DrawPixel((uint8_t)(x - x0), (uint8_t)(y + y0), color);
        LCD_DrawPixel((uint8_t)(x + x0), (uint8_t)(y - y0), color);
        LCD_DrawPixel((uint8_t)(x - x0), (uint8_t)(y - y0), color);
        LCD_DrawPixel((uint8_t)(x + y0), (uint8_t)(y + x0), color);
        LCD_DrawPixel((uint8_t)(x - y0), (uint8_t)(y + x0), color);
        LCD_DrawPixel((uint8_t)(x + y0), (uint8_t)(y - x0), color);
        LCD_DrawPixel((uint8_t)(x - y0), (uint8_t)(y - x0), color);
    }
}


void LCD_FillCircle(int8_t x, int8_t y, int8_t radius, uint8_t color)
{
    int16_t f;
    int16_t dd_f_x;
    int16_t dd_f_y;
    int16_t x0;
    int16_t y0;

    f = 1 - radius;
    dd_f_x = 1;
    dd_f_y = (int16_t)(-2 * radius);
    x0 = 0;
    y0 = radius;

    LCD_DrawLine((uint8_t)(x - radius), (uint8_t)y, (uint8_t)(x + radius), (uint8_t)y, color);

    while (x0 < y0)
    {
        if (f >= 0)
        {
            y0--;
            dd_f_y += 2;
            f += dd_f_y;
        }

        x0++;
        dd_f_x += 2;
        f += dd_f_x;

        LCD_DrawLine((uint8_t)(x - x0), (uint8_t)(y + y0), (uint8_t)(x + x0), (uint8_t)(y + y0), color);
        LCD_DrawLine((uint8_t)(x - x0), (uint8_t)(y - y0), (uint8_t)(x + x0), (uint8_t)(y - y0), color);
        LCD_DrawLine((uint8_t)(x - y0), (uint8_t)(y + x0), (uint8_t)(x + y0), (uint8_t)(y + x0), color);
        LCD_DrawLine((uint8_t)(x - y0), (uint8_t)(y - x0), (uint8_t)(x + y0), (uint8_t)(y - x0), color);
    }
}
