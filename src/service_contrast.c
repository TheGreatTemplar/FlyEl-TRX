#include "service_contrast.h"
#include "gmg12864.h"
#include "uart_store.h"
#include "app_state.h"

static uint8_t contrast_original;
static uint8_t contrast_waiting;
static const char *contrast_message;

void ServiceContrast_Open(void)
{
    contrast_original = LCD_GetContrast();
    contrast_waiting = 0U;
    contrast_message = "ENC:CONTRAST";
}


const char *ServiceContrast_Status(void)
{
    return contrast_message;
}


uint8_t ServiceContrast_Task(void)
{
    if (contrast_waiting == 0U || UARTStore_Busy() != 0U)
        return 0U;
    contrast_waiting = 0U;
    /* При ошибке хранилище возвращает предыдущий сохранённый контраст. */
    contrast_original = LCD_GetContrast();
    contrast_message = UARTStore_Status();
    return 1U;
}


void ServiceContrast_Encoder(int32_t steps)
{
    int64_t value;
    (void)ServiceContrast_Task();
    if (contrast_waiting != 0U || steps == 0)
        return;
    value = (int64_t)LCD_GetContrast() + steps;
    if (value < 0)
        value = 0;
    if (value > LCD_CONTRAST_MAX)
        value = LCD_CONTRAST_MAX;
    LCD_SetContrast((uint8_t)value);
    contrast_message = "CHANGED";
    menu.need_redraw = 1U;
}


uint8_t ServiceContrast_Key(input_key_t key)
{
    (void)ServiceContrast_Task();
    if (contrast_waiting != 0U)
        return 0U;
    if (key == Key_9_3 || key == Key_5_1)
    {
        LCD_SetContrast(contrast_original);
        return 1U;
    }
    if (key == Key_9_2 || key == Key_5_3)
    {
        contrast_waiting = UARTStore_RequestContrast(LCD_GetContrast());
        contrast_message = UARTStore_Status();
    }
    return 0U;
}
