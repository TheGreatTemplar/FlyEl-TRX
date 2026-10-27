#ifndef HMI_CONFIG_H
#define HMI_CONFIG_H
#include <stdint.h>

typedef struct
{
    uint16_t long_ms;
    uint8_t debounce_ms;
    uint8_t func_seconds;
    uint8_t encoder_type;
    uint8_t encoder_steps;
    uint8_t encoder_reverse;
    uint8_t encoder_filter;
} hmi_config_t;

/* Тип детента: любой AB (старый алгоритм), 00, 11, оба 00/11. Фильтр — тики 500 мкс. */
void HMI_Defaults(hmi_config_t *config);
uint8_t HMI_Valid(const hmi_config_t *config);
const hmi_config_t *HMI_Get(void);
/* Только основной цикл; драйвер ввода копирует параметры при запрещённых IRQ. */
uint8_t HMI_Apply(const hmi_config_t *config);
uint32_t HMI_LongPressTicks(void);
uint32_t HMI_FuncTicks(void);
/* Дополнение общей 64-байтной записи FU01, байты 16..25. FF — старая версия. */
uint8_t HMI_Decode(const uint8_t *record, hmi_config_t *config);
void HMI_Encode(uint8_t *record, const hmi_config_t *config);
#endif
