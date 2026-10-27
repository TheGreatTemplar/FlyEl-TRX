/* Проверка всей матрицы тестовых экранов на настоящем framebuffer. */
#define CONTROL_REAL_LCD
#include "controls_arm.c"
#include "gmg12864.c"
#include "lcd_text.c"
#include "lcd_digits_large.c"
#include "lcd_digits_small.c"
void LCD_PatternUpdate(void);
#define LCD_Update LCD_PatternUpdate
#include "ui_lcd_test.c"
#undef LCD_Update

void LCD_PatternUpdate(void)
{
}


spi1_status_t SPI1_SendByte(uint8_t data)
{
    (void)data;
    return SPI1_OK;
}


static uint8_t Pixel(uint8_t x, uint8_t y)
{
    return (uint8_t)((lcd_buffer[(uint16_t)(y / 8U) * 128U + x] >> (y % 8U)) & 1U);
}


int test_lcd_test_full_matrix_patterns_and_complementary_pixels(void)
{
    uint8_t page;
    uint8_t x;
    uint8_t y;
    InitControls();
    LCDTest_Open();
    for (page = LCD_TEST_ON; page <= LCD_TEST_HORIZONTAL_INV; page++)
    {
        LCDTest_Encoder((int32_t)page - LCDTest_Page());
        ServiceTools_DrawLCDTest();
        for (y = 0U; y < 64U; y++)
        {
            for (x = 0U; x < 128U; x++)
            {
                uint8_t expected;
                if (page == LCD_TEST_ON)
                    expected = 1U;
                else if (page == LCD_TEST_OFF)
                    expected = 0U;
                else if (page <= LCD_TEST_CHECK_INV)
                    expected = (uint8_t)(((x ^ y) & 1U) ^ (page == LCD_TEST_CHECK_INV));
                else if (page <= LCD_TEST_VERTICAL_INV)
                    expected = (uint8_t)((x & 1U) ^ (page == LCD_TEST_VERTICAL_INV));
                else
                    expected = (uint8_t)((y & 1U) ^ (page == LCD_TEST_HORIZONTAL_INV));
                CHECK(Pixel(x, y) == expected);
            }
        }
    }
    LCDTest_Close();
    return 0;
}


int test_lcd_test_geometry_and_moving_line_leave_no_trails(void)
{
    uint8_t x;
    uint8_t y;
    uint8_t position;
    InitControls();
    LCDTest_Open();
    LCDTest_Encoder(LCD_TEST_GEOMETRY);
    ServiceTools_DrawLCDTest();
    for (x = 0U; x < 128U; x++)
        CHECK(Pixel(x, 0U) != 0U && Pixel(x, 63U) != 0U);
    for (y = 0U; y < 64U; y++)
        CHECK(Pixel(0U, y) != 0U && Pixel(127U, y) != 0U);
    for (position = 0U; position < 3U; position++)
    {
        uint8_t axis;
        for (axis = 0U; axis < 2U; axis++)
        {
            uint8_t limit = (axis == 0U) ? 127U : 63U;
            uint8_t at = (position == 0U) ? 0U : ((position == 1U) ? (uint8_t)(limit / 2U) : limit);
            LCDTest_Encoder((int32_t)(LCD_TEST_SWEEP_X + axis) - LCDTest_Page());
            now_tick += (uint32_t)at * 40U;
            (void)LCDTest_Task();
            ServiceTools_DrawLCDTest();
            for (y = 0U; y < 64U; y++)
                for (x = 0U; x < 128U; x++)
                    CHECK(Pixel(x, y) == (uint8_t)(axis == 0U ? x == at : y == at));
        }
    }
    LCDTest_Close();
    return 0;
}
