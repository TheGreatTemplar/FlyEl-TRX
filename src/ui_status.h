#ifndef UI_STATUS_H
#define UI_STATUS_H
#include "input.h"

/* Временный счётчик щелчков RIT/SHIFT для проверки макета; основной контекст. */
void StatusUI_AuxEncoder(input_aux_encoder_t encoder, int32_t steps);
/* Текущее сообщение дополнительных ручек; отдельное поле на фиксированной строке. */
const char *StatusUI_AuxMessage(void);
const char *StatusUI_Header(void);
void StatusUI_Draw(void);
#endif
