#ifndef APP_TIME_H
#define APP_TIME_H

#include <stdint.h>

/* Монотонный счётчик с шагом 500 мкс; разности uint32_t переживают переполнение. */
uint32_t App_GetTick500us(void);
/* Стартовая блокирующая задержка; TIM2 должен быть уже запущен. */
void App_DelayMs(uint32_t ms);

#endif
