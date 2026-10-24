/* Жесты физических кнопок; сканирование и дребезг остаются в input.c. */
#include "keyboard.h"
#include "key_gesture.h"
#include "radio_vfo.h"
#include "radio_control.h"
#include "app_time.h"
#include "app_state.h"
#include "entry_controller.h"
#include "memory_control.h"
#include "radio_panel.h"
#include "settings_control.h"
#include "scan_control.h"
#include "key_layout.h"

static key_gesture_t gestures[27];
static uint8_t suppressed[27];

void Keyboard_Init(void)
{
    uint8_t i;
    for (i = 0U; i < 27U; i++)
    { gestures[i].active = 0U; suppressed[i] = 0U; }
}

static void Execute(input_key_t key, key_gesture_event_t event)
{
    uint8_t long_press = (uint8_t)(event == KEY_GESTURE_LONG);
    if (Settings_Page() != 0U)
    { (void)Settings_HandleKey(key, long_press); return; }
    if (key == Key_5_3 && long_press != 0U && Entry_Active() != 0U)
    { Entry_Cancel(); return; }
    if (Entry_HandleKey(key) != 0U) return;
    if (Settings_HandleKey(key, long_press) != 0U) return;
    /* Обрабатываем физические коды; старые INPUT_KEY_* и резерв не имеют команд. */
    switch ((uint8_t)key)
    {
        case Key_1_1: MemoryControl_ToggleMode(); break;
        case Key_1_2: MemoryControl_LeaveMode(); RadioVfo_CopyAToB(); break;
        case Key_1_3: MemoryControl_LeaveMode(); RadioVfo_Switch(); break;
        case Key_2_1: if (long_press != 0U) MemoryControl_Write(); break;
        case Key_2_2: if (long_press != 0U) MemoryControl_Clear(); break;
        case Key_2_3: if (long_press != 0U) MemoryControl_ToVfo(); break;
        case Key_4_1: RadioVfo_ToggleSplit(); break;
        case Key_4_2: Scan_Toggle(); break;
        case Key_4_3:
            if (RadioMode_IsSSB(RadioVfo_Mode()) != 0U) panel.vox ^= 1U;
            break;
        case Key_6_1: panel.nb ^= 1U; break;
        case Key_6_2: panel.comp ^= 1U; break;
        case Key_7_1: RadioPanel_TogglePreamp(); break;
        case Key_7_2: RadioPanel_CycleAtt(); break;
        case Key_7_3: RadioPanel_Tuner(long_press); break;
        case Key_9_1: MemoryControl_ToggleSelection(); break;
        case Key_10_1: RadioVfo_ChangeMode(long_press); break;
        case Key_10_2: RadioVfo_CycleFilter(); break;
        case Key_9_2:
        case Key_9_3:
        {
            if (MemoryControl_Selecting() != 0U)
            { MemoryControl_Move((key == Key_9_3) ? 1 : -1); break; }
            /* Диапазон меняется один раз на нажатие: реле не получают автоповтор. */
            if (event == KEY_GESTURE_REPEAT)
            {
                return;
            }
            MemoryControl_LeaveMode();
            /* UP/DOWN перелистывают диапазоны с циклическим переходом. */
            uint8_t band = menu.band_index;
            if (key == Key_9_3) band = (uint8_t)(band + 1U);
            else band = (band == 0U) ? (uint8_t)(Radio_BandsCount() - 1U) : (uint8_t)(band - 1U);
            Radio_SetBand(band);
            break;
        }
        default: break;
    }
    menu.need_redraw = 1U;
}

void Keyboard_Task(input_key_t pressed)
{
    uint8_t i;
    uint32_t now = App_GetTick500us();
    uint8_t stopped_scan = 0U;
    if (pressed >= Key_2_1 && pressed < INPUT_KEY_COUNT && pressed != Key_4_2 && Scan_Active() != 0U)
    { Scan_Stop(); stopped_scan = 1U; }

    /* TS сохраняет исходный автомат, но цифровой ввод/меню имеют приоритет. */
    if (pressed == Key_10_3)
    {
        uint8_t consume = (uint8_t)(Settings_Page() != 0U);
        if (consume == 0U && Entry_Active() != 0U) consume = Entry_HandleKey(pressed);
        if (consume != 0U) pressed = INPUT_KEY_COUNT;
    }
    Radio_UpdateTS(pressed);
    for (i = 0U; i < 27U; i++)
    {
        input_key_t key = (input_key_t)((uint8_t)Key_2_1 + i);
        key_gesture_event_t event;
        uint8_t down = Input_KeyIsDown(key);
        if (key == Key_10_3) continue; /* Работающий автомат TS не заменяем. */
        /* Цифра принимается сразу и поглощает весь жест, даже если FUNC истечёт. */
        if (key == pressed && Entry_Active() != 0U && Settings_Page() == 0U &&
            KeyLayout_Character(key) != '\0')
        { (void)Entry_HandleKey(key); suppressed[i] = 1U; gestures[i].active = 0U; }
        if (stopped_scan != 0U && key == pressed && (key == Key_9_2 || key == Key_9_3))
        { suppressed[i] = 1U; gestures[i].active = 0U; }
        if (suppressed[i] != 0U)
        { if (down == 0U) suppressed[i] = 0U; continue; }
        event = KeyGesture_Update(&gestures[i], down,
                    (uint8_t)(key == pressed),
                    (uint8_t)(key == Key_9_2 || key == Key_9_3), now);
        if (event != KEY_GESTURE_NONE) Execute(key, event);
    }
}
