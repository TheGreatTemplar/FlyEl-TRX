#ifndef INPUT_H
#define INPUT_H

#include "stm32f10x.h"
#include <stdint.h>

typedef enum
{
    INPUT_KEY_1 = 0,
    INPUT_KEY_2,
    INPUT_KEY_3,
    INPUT_KEY_BAND,

    INPUT_KEY_4,
    INPUT_KEY_5,
    INPUT_KEY_6,
    INPUT_KEY_STEP,

    INPUT_KEY_7,
    INPUT_KEY_8,
    INPUT_KEY_9,
    INPUT_KEY_MODE,

    INPUT_KEY_ESC,
    INPUT_KEY_0,
    INPUT_KEY_DOT,
    INPUT_KEY_ENTER,

    /* Physical CD4017 package pin, then OUT number; no functions assigned. */
    Key_2_1,
    Key_2_2,
    Key_2_3,
    Key_4_1,
    Key_4_2,
    Key_4_3,
    Key_7_1,
    Key_7_2,
    Key_7_3,
    Key_10_1,
    Key_10_2,
    Key_10_3,
    Key_1_1,
    Key_1_2,
    Key_1_3,
    Key_5_1,
    Key_5_2,
    Key_5_3,
    Key_6_1,
    Key_6_2,
    Key_6_3,
    Key_9_1,
    Key_9_2,
    Key_9_3,
    Key_11_1,
    Key_11_2,
    Key_11_3,

    INPUT_KEY_COUNT
} input_key_t;

void Input_Init(void);
void Input_Poll500us(void);

int32_t Input_GetEncoderSteps(void);

uint8_t Input_KeyPressed(input_key_t key);
uint8_t Input_KeyIsDown(input_key_t key);
char Input_KeyToChar(input_key_t key);
uint32_t Input_GetRawKeyMask(void);
const char *Input_KeyName(input_key_t key);

#endif
