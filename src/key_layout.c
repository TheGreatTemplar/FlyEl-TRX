/* Раскладка из doc/Раскладка клавиатуры FlyEl.xlsx, лист 1, C9:H43. */
#include "key_layout.h"

const char *KeyLayout_Name(input_key_t key)
{
    /* Старые логические INPUT_KEY_* намеренно проходят через default. */
    switch ((uint8_t)key)
    {
        case Key_1_1:
            return "V/M";
        case Key_1_2:
            return "A=B";
        case Key_1_3:
            return "A/B";
        case Key_2_1:
            return "MW";
        case Key_2_2:
            return "M=CL";
        case Key_2_3:
            return "M>V";
        case Key_4_1:
            return "SPL";
        case Key_4_2:
            return "SCN";
        case Key_4_3:
            return "VOX";
        case Key_5_1:
            return "NR";
        case Key_5_2:
            return "ANF";
        case Key_5_3:
            return "F-INP";
        case Key_6_1:
            return "NB";
        case Key_6_2:
            return "COMP";
        case Key_6_3:
            return "SET";
        case Key_7_1:
            return "P.AMP";
        case Key_7_2:
            return "ATT";
        case Key_7_3:
            return "TUNER";
        case Key_9_1:
            return "CH";
        case Key_9_2:
            return "DOWN";
        case Key_9_3:
            return "UP";
        case Key_10_1:
            return "MODE";
        case Key_10_2:
            return "FILTER";
        case Key_10_3:
            return "TS";
        default:
            return Input_KeyName(key);
    }
}


/* Цифровые подписи — дополнительная функция; вне ввода они не подменяют команды. */
char KeyLayout_Character(input_key_t key)
{
    switch ((uint8_t)key)
    {
        case Key_1_1:
            return '1';
        case Key_1_2:
            return '2';
        case Key_1_3:
            return '3';
        case Key_2_1:
            return '4';
        case Key_2_2:
            return '5';
        case Key_2_3:
            return '6';
        case Key_4_1:
            return '7';
        case Key_4_2:
            return '8';
        case Key_4_3:
            return '9';
        case Key_5_1:
            return '.';
        case Key_5_2:
            return '0';
        default:
            return '\0';
    }
}
