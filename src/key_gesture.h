#ifndef KEY_GESTURE_H
#define KEY_GESTURE_H

#include <stdint.h>

typedef enum
{
    KEY_GESTURE_NONE,
    KEY_GESTURE_SHORT,
    KEY_GESTURE_LONG,
    KEY_GESTURE_REPEAT
} key_gesture_event_t;

typedef struct
{
    uint32_t press_tick;
    uint32_t repeat_tick;
    uint8_t active;
    uint8_t handled;
    uint8_t reserved[2];
} key_gesture_t;

/* Тики по 500 мкс. Короткое событие — при отпускании, длинное — строго >1 с.
 * repeat разрешается только для UP/DOWN: сразу короткое событие, повтор через
 * 600 мс с периодом 200 мс; один вызов выдаёт не более одного события.
 * Обработчик команды исключает повтор переключения диапазонов. */
key_gesture_event_t KeyGesture_Update(key_gesture_t *state, uint8_t down,
                                      uint8_t pressed, uint8_t repeat, uint32_t now);

#endif
