/* Общие модели аппаратуры для сценариев управления и экрана. */
generator_menu_t menu;
static uint8_t key_down[INPUT_KEY_COUNT];
static uint32_t now_tick;
static uint32_t synth_frequency;
static uint32_t store_calls;
static uint8_t mock_saved_channel;
static uint8_t cache_used[MEMORY_CHANNEL_COUNT];
static radio_preset_t cache[MEMORY_CHANNEL_COUNT];
static uart_config_t mock_uart_config = {115200U, 8U, UART_PARITY_NONE, UART_STOP_ONE, 0U};
static uint32_t mock_uart_saves;

const uart_config_t *UART1_GetConfig(void)
{
    return &mock_uart_config;
}


uint8_t UARTStore_Request(const uart_config_t *config)
{
    if (UARTConfig_Valid(config) == 0U)
        return 0U;
    mock_uart_config = *config;
    mock_uart_saves++;
    return 1U;
}


const char *UARTStore_Status(void)
{
    return "SAVED EEPROM";
}


uint32_t App_GetTick500us(void)
{
    return now_tick;
}


uint8_t Input_KeyIsDown(input_key_t key)
{
    return key_down[key];
}


char Input_KeyToChar(input_key_t key)
{
    return KeyLayout_Character(key);
}


const char *Input_KeyName(input_key_t key)
{
    static const uint8_t pins[] = {2U, 4U, 7U, 10U, 1U, 5U, 6U, 9U, 11U};
    static char name[12];
    uint8_t pin, i = 4U, index;
    if (key < Key_2_1 || key >= INPUT_KEY_COUNT)
        return "";
    index = (uint8_t)((uint8_t)key - (uint8_t)Key_2_1);
    pin = pins[index / 3U];
    memcpy(name, "Key_", 4U);
    if (pin >= 10U)
        name[i++] = '1';
    name[i++] = (char)('0' + pin % 10U);
    name[i++] = '_';
    name[i++] = (char)('1' + index % 3U);
    name[i] = '\0';
    return name;
}


uint32_t Input_GetRawKeyMask(void)
{
    uint8_t i;
    uint32_t mask = 0U;
    for (i = 0U; i < 27U; i++)
        if (key_down[(uint8_t)Key_2_1 + i] != 0U)
            mask |= 1UL << i;
    return mask;
}


si5351_status_t Si5351_SetVfoHighSide(uint8_t clk, uint32_t hz, uint32_t if_hz)
{
    (void)clk;
    (void)if_hz;
    synth_frequency = hz;
    return SI5351_OK;
}


void MemoryStore_Init(void)
{
    memset(cache_used, 0, sizeof(cache_used));
    store_calls = 0U;
    mock_saved_channel = 0xFFU;
}


uint8_t MemoryStore_TakeSavedChannel(uint8_t *channel)
{
    if (mock_saved_channel >= MEMORY_CHANNEL_COUNT)
        return 0U;
    *channel = mock_saved_channel;
    mock_saved_channel = 0xFFU;
    return 1U;
}


uint8_t MemoryStore_Task(void)
{
    return 0U;
}


uint8_t MemoryStore_Ready(void)
{
    return 1U;
}


uint8_t MemoryStore_Busy(void)
{
    return 0U;
}


uint8_t MemoryStore_Get(uint8_t index, radio_preset_t *preset)
{
    if (cache_used[index] == 0U)
        return 0U;
    if (preset != 0)
        *preset = cache[index];
    return 1U;
}


uint8_t MemoryStore_Request(uint8_t index, const radio_preset_t *preset)
{
    store_calls++;
    cache_used[index] = (uint8_t)(preset != 0);
    if (preset != 0)
        cache[index] = *preset;
    return 1U;
}
