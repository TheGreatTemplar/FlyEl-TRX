/* Автомат смены тестов: статический экран 1 с, движущаяся линия — полный проход. */
#include "service_lcd_test.h"
#include "app_time.h"
#include "gmg12864.h"

static uint8_t lcd_test_active;
static uint8_t lcd_test_page;
static uint8_t lcd_test_auto;
static uint8_t lcd_test_position;
static uint32_t lcd_test_tick;

void LCDTest_Close(void)
{
    lcd_test_active = 0U;
}


void LCDTest_Open(void)
{
    lcd_test_active = 1U;
    lcd_test_page = LCD_TEST_ON;
    lcd_test_auto = 1U;
    lcd_test_position = 0U;
    lcd_test_tick = App_GetTick500us();
}


uint8_t LCDTest_Page(void)
{
    return lcd_test_page;
}


uint8_t LCDTest_Auto(void)
{
    return lcd_test_auto;
}


uint8_t LCDTest_Position(void)
{
    return lcd_test_position;
}


uint8_t LCDTest_Task(void)
{
    uint32_t elapsed;
    uint32_t duration = 2000U;
    uint8_t span = 0U;
    uint8_t position;
    if (lcd_test_active == 0U)
        return 0U;
    elapsed = App_GetTick500us() - lcd_test_tick;
    if (lcd_test_page == LCD_TEST_SWEEP_X)
        span = LCD_WIDTH;
    if (lcd_test_page == LCD_TEST_SWEEP_Y)
        span = LCD_HEIGHT;
    /* Один пиксель за 20 мс; автомат не обрывает проход через всю матрицу. */
    if (span != 0U)
        duration = (uint32_t)span * 40U;
    if (lcd_test_auto != 0U && elapsed >= duration)
    {
        lcd_test_page = (uint8_t)((lcd_test_page + 1U) % LCD_TEST_COUNT);
        lcd_test_tick = App_GetTick500us();
        lcd_test_position = 0U;
        return 1U;
    }
    if (span == 0U)
        return 0U;
    position = (uint8_t)((elapsed / 40U) % span);
    if (position == lcd_test_position)
        return 0U;
    lcd_test_position = position;
    return 1U;
}


/* Энкодер останавливает только смену экранов; выбранная бегущая линия продолжает движение. */
void LCDTest_Encoder(int32_t steps)
{
    int32_t page;
    if (lcd_test_active == 0U || steps == 0)
        return;
    page = ((int32_t)lcd_test_page + steps % LCD_TEST_COUNT) % LCD_TEST_COUNT;
    if (page < 0)
        page += LCD_TEST_COUNT;
    lcd_test_page = (uint8_t)page;
    lcd_test_auto = 0U;
    lcd_test_position = 0U;
    lcd_test_tick = App_GetTick500us();
}


uint8_t LCDTest_Key(input_key_t key)
{
    if (key == Key_9_3 || key == Key_5_1)
    {
        LCDTest_Close();
        return 1U;
    }
    if (key == Key_9_2 || key == Key_5_3)
    {
        lcd_test_auto = 1U;
        lcd_test_position = 0U;
        lcd_test_tick = App_GetTick500us();
    }
    return 0U;
}
