#include "hmi_config.h"
#include "input.h"
#include <string.h>

static hmi_config_t hmi_active = {1000U, 10U, 10U, 0U, 4U, 0U, 0U};

void HMI_Defaults(hmi_config_t *config)
{
    *config = (hmi_config_t){1000U, 10U, 10U, 0U, 4U, 0U, 0U};
}


uint8_t HMI_Valid(const hmi_config_t *config)
{
    return (uint8_t)(config != 0 && config->long_ms >= 500U && config->long_ms <= 2000U &&
                     config->long_ms % 100U == 0U && config->debounce_ms >= 5U && config->debounce_ms <= 50U &&
                     config->debounce_ms % 5U == 0U && config->func_seconds >= 5U && config->func_seconds <= 10U &&
                     config->encoder_type <= 3U &&
                     (config->encoder_steps == 1U || config->encoder_steps == 2U || config->encoder_steps == 4U) &&
                     config->encoder_reverse <= 1U && config->encoder_filter <= 3U);
}


const hmi_config_t *HMI_Get(void)
{
    return &hmi_active;
}


uint8_t HMI_Apply(const hmi_config_t *config)
{
    if (HMI_Valid(config) == 0U)
        return 0U;
    if (memcmp(&hmi_active, config, sizeof(hmi_active)) == 0)
        return 1U;
    Input_Configure(config);
    hmi_active = *config;
    return 1U;
}


uint32_t HMI_LongPressTicks(void)
{
    return (uint32_t)hmi_active.long_ms * 2U;
}


uint32_t HMI_FuncTicks(void)
{
    return (uint32_t)hmi_active.func_seconds * 2000U;
}


uint8_t HMI_Decode(const uint8_t *record, hmi_config_t *config)
{
    if (record[16] == 0xFFU)
    {
        HMI_Defaults(config);
        return 1U;
    }
    if (record[16] != 'H' || record[17] != 1U)
        return 0U;
    config->long_ms = (uint16_t)((uint16_t)record[18] | ((uint16_t)record[19] << 8U));
    config->debounce_ms = record[20];
    config->func_seconds = record[21];
    config->encoder_type = record[22];
    config->encoder_steps = record[23];
    config->encoder_reverse = record[24];
    config->encoder_filter = record[25];
    return HMI_Valid(config);
}


void HMI_Encode(uint8_t *record, const hmi_config_t *config)
{
    record[16] = 'H';
    record[17] = 1U;
    record[18] = (uint8_t)config->long_ms;
    record[19] = (uint8_t)(config->long_ms >> 8U);
    record[20] = config->debounce_ms;
    record[21] = config->func_seconds;
    record[22] = config->encoder_type;
    record[23] = config->encoder_steps;
    record[24] = config->encoder_reverse;
    record[25] = config->encoder_filter;
}
