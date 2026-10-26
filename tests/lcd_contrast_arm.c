/* Проверка настоящих команд контраста без записи в адреса GPIO на эмуляторе. */
#include "gmg12864.h"
#include "spi1.h"
static GPIO_TypeDef contrast_gpio;
#undef GPIOB
#define GPIOB (&contrast_gpio)
#include "gmg12864.c"
static uint8_t contrast_commands[4];
static uint8_t contrast_count;
spi1_status_t SPI1_SendByte(uint8_t data)
{
    if (contrast_count < sizeof(contrast_commands))
        contrast_commands[contrast_count++] = data;
    return SPI1_OK;
}


uint32_t App_GetTick500us(void)
{
    return 0U;
}


#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
            return __LINE__;                                                                                           \
    } while (0)

int test_lcd_contrast_command_bounds_and_framebuffer_preserved(void)
{
    contrast_count = 0U;
    LCD_ClearBuffer();
    LCD_DrawPixel(17U, 12U, PX_ON);
    LCD_SetContrast(0U);
    CHECK(LCD_GetContrast() == 0U);
    LCD_SetContrast(255U);
    CHECK(LCD_GetContrast() == 63U && contrast_count == 4U);
    CHECK(contrast_commands[0] == 0x81U && contrast_commands[1] == 0U);
    CHECK(contrast_commands[2] == 0x81U && contrast_commands[3] == 63U);
    CHECK(lcd_buffer[128U + 17U] == 0x10U);
    CHECK(contrast_gpio.BSRR == (1UL << LCD_CS_PIN));
    return 0;
}
