#ifndef UI_STATUS_H
#define UI_STATUS_H
#include "input.h"

/* Показ текущего смещения в Гц на две секунды; основной контекст. */
void StatusUI_AuxEncoder(input_aux_encoder_t encoder);
/* Текущее сообщение дополнительных ручек; отдельное поле на фиксированной строке. */
const char *StatusUI_AuxMessage(void);
const char *StatusUI_Header(void);
void StatusUI_Draw(void);
#endif
