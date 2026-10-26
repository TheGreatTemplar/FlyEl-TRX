/* Шкала S-метра и её компактные растровые обозначения. */
#include "ui_smeter.h"
#include "app_config.h"
#include "gmg12864.h"

static uint8_t MeterBarsFromMv(uint16_t mv, uint8_t max_bars)
{
    uint32_t bars;

    if (mv > ADC_VREF_MV)
    {
        mv = ADC_VREF_MV;
    }

    bars = ((uint32_t)mv * (uint32_t)max_bars) / ADC_VREF_MV;

    if (bars > max_bars)
    {
        bars = max_bars;
    }

    return (uint8_t)bars;
}


static void DrawMeterPlus3x3(uint8_t x, uint8_t y)
{
    LCD_DrawPixel((uint8_t)(x + 1U), y, PX_ON);
    LCD_DrawLine(x, (uint8_t)(y + 1U), (uint8_t)(x + 2U), (uint8_t)(y + 1U), PX_ON);
    LCD_DrawPixel((uint8_t)(x + 1U), (uint8_t)(y + 2U), PX_ON);
}


static void DrawMeterLabel(uint8_t index, uint8_t x, uint8_t y)
{
    if (index == 0U)
    {
        LCD_DrawDigit4x6(x, y, (uint8_t)'1', CH_UNINVERT);
    }
    else if (index == 1U)
    {
        LCD_DrawDigit4x6(x, y, (uint8_t)'3', CH_UNINVERT);
    }
    else if (index == 2U)
    {
        LCD_DrawDigit4x6(x, y, (uint8_t)'5', CH_UNINVERT);
    }
    else if (index == 3U)
    {
        LCD_DrawDigit4x6(x, y, (uint8_t)'7', CH_UNINVERT);
    }
    else if (index == 4U)
    {
        LCD_DrawDigit4x6(x, y, (uint8_t)'9', CH_UNINVERT);
    }
    else
    {
        if ((index == 5U) || (index == 6U))
        {
            /* +10 and +20: move only the plus sign one pixel right. */
            DrawMeterPlus3x3((uint8_t)(x + 1U), (uint8_t)(y + 2U));
        }
        else
        {
            DrawMeterPlus3x3(x, (uint8_t)(y + 2U));
        }

        if (index == 5U)
        {
            LCD_PrintDigits4x6((uint8_t)(x + 4U), y, "10", 1U, CH_UNINVERT);
        }
        else if (index == 6U)
        {
            LCD_PrintDigits4x6((uint8_t)(x + 4U), y, "20", 1U, CH_UNINVERT);
        }
        else if (index == 7U)
        {
            /* +30: move only the digits one pixel left. */
            LCD_PrintDigits4x6((uint8_t)(x + 3U), y, "30", 1U, CH_UNINVERT);
        }
        else
        {
            LCD_PrintDigits4x6((uint8_t)(x + 4U), y, "40", 1U, CH_UNINVERT);
        }
    }
}


static uint8_t MeterLabelWidth(uint8_t index)
{
    if (index < 5U)
    {
        return 4U;
    }

    return 13U;
}


static void DrawSMeterScale(void)
{
    static const uint8_t tick_x[9] = {3U, 20U, 36U, 53U, 69U, 83U, 97U, 110U, 124U};

    uint8_t i;
    uint8_t label_width;
    int16_t label_x;
    const uint8_t y = 58U;

    /*
     * S-meter reference points, measured from S1:
     *
     *   S1   =  0 dB -> x =   3
     *   S3   = 12 dB -> x =  20
     *   S5   = 24 dB -> x =  36
     *   S7   = 36 dB -> x =  53
     *   S9   = 48 dB -> x =  69
     *   +10  = 58 dB -> x =  83
     *   +20  = 68 dB -> x =  97
     *   +30  = 78 dB -> x = 110
     *   +40  = 88 dB -> x = 124
     *
     * Labels are centered on their ticks. At the two edges they are
     * clipped to the available 0...127 pixel display area.
     */
    for (i = 0U; i < 9U; i++)
    {
        label_width = MeterLabelWidth(i);
        label_x = (int16_t)tick_x[i] - ((int16_t)label_width / 2);

        if ((i == 5U) || (i == 6U))
        {
            label_x -= 3;
        }
        else if (i == 7U)
        {
            label_x -= 2;
        }

        if (label_x < 0)
        {
            label_x = 0;
        }

        if ((label_x + label_width) > 128)
        {
            label_x = (int16_t)(128 - label_width);
        }

        DrawMeterLabel(i, (uint8_t)label_x, y);
    }
}


static void DrawSMeterTicks(uint8_t baseline_y)
{
    static const uint8_t tick_x[9] = {3U, 20U, 36U, 53U, 69U, 83U, 97U, 110U, 124U};

    uint8_t i;

    /* Two pixels upward from the existing dotted baseline. */
    for (i = 0U; i < 9U; i++)
    {
        LCD_DrawLine(tick_x[i], (uint8_t)(baseline_y - 2U), tick_x[i], baseline_y, PX_ON);
    }
}


/* Рисует шкалу и линейную полосу из напряжения ADC0; калибровка S-единиц ещё не введена. */
void SMeter_Draw(uint16_t mv)
{
    uint8_t i;
    uint8_t bars;
    uint8_t x;

    /*
     * Full-width segmented "thermometer". Compared with the previous
     * The thermometer is now at y=52 and 3 pixels high.
     * The S-meter scale text is moved down by one pixel to y=54.
     * 42 segments of 2 pixels with 1-pixel gaps occupy 125 pixels.
     */
    const uint8_t meter_x = 1U;
    const uint8_t meter_y = 53U;
    const uint8_t bar_w = 2U;
    const uint8_t bar_h = 3U;
    const uint8_t gap = 1U;
    const uint8_t bar_count = 42U;

    bars = MeterBarsFromMv(mv, bar_count);

    for (i = 0U; i < bar_count; i++)
    {
        x = (uint8_t)(meter_x + (i * (bar_w + gap)));

        if (i < bars)
        {
            LCD_FillRectangle(x, meter_y, (uint8_t)(bar_w - 1U), (uint8_t)(bar_h - 1U), PX_ON);
        }
        else
        {
            LCD_DrawLine(x, (uint8_t)(meter_y + bar_h - 1U), (uint8_t)(x + bar_w - 1U), (uint8_t)(meter_y + bar_h - 1U),
                         PX_ON);
        }
    }

    DrawSMeterTicks((uint8_t)(meter_y + bar_h - 1U));
    DrawSMeterScale();
}
