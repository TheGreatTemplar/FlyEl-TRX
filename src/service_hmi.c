#include "service_hmi.h"
#include "uart_store.h"
#include "text_utils.h"
#include "app_state.h"

static hmi_config_t hmi_draft;
static uint16_t hmi_id;
static uint8_t hmi_waiting;
static const char *hmi_message;

void ServiceHMI_Open(uint16_t id)
{
    hmi_draft = *HMI_Get();
    hmi_id = id;
    hmi_waiting = 0U;
    hmi_message = "ENC:CHANGE";
}


const char *ServiceHMI_Status(void)
{
    return hmi_message;
}


uint8_t ServiceHMI_Task(void)
{
    if (hmi_waiting == 0U || UARTStore_Busy() != 0U)
        return 0U;
    hmi_waiting = 0U;
    hmi_draft = *HMI_Get();
    hmi_message = UARTStore_Status();
    return 1U;
}


static uint16_t ClampStep(uint16_t value, int32_t steps, uint16_t low, uint16_t high, uint16_t increment)
{
    int64_t next = value + (int64_t)steps * increment;
    if (next < low)
        next = low;
    if (next > high)
        next = high;
    return (uint16_t)next;
}


void ServiceHMI_Encoder(int32_t steps)
{
    (void)ServiceHMI_Task();
    if (hmi_waiting != 0U || steps == 0)
        return;
    switch (hmi_id)
    {
        case 1406U:
            hmi_draft.debounce_ms = (uint8_t)ClampStep(hmi_draft.debounce_ms, steps, 5U, 50U, 5U);
            break;
        case 1407U:
            hmi_draft.long_ms = ClampStep(hmi_draft.long_ms, steps, 500U, 2000U, 100U);
            break;
        case 1408U:
            hmi_draft.func_seconds = (uint8_t)ClampStep(hmi_draft.func_seconds, steps, 5U, 10U, 1U);
            break;
        case 1409U:
            hmi_draft.encoder_type = (uint8_t)ClampStep(hmi_draft.encoder_type, steps, 0U, 3U, 1U);
            break;
        case 1410U:
        {
            uint16_t index = hmi_draft.encoder_steps == 1U ? 0U : (hmi_draft.encoder_steps == 2U ? 1U : 2U);
            index = ClampStep(index, steps, 0U, 2U, 1U);
            hmi_draft.encoder_steps = (uint8_t)(1U << index);
            break;
        }
        case 1411U:
            hmi_draft.encoder_reverse = (uint8_t)ClampStep(hmi_draft.encoder_reverse, steps, 0U, 1U, 1U);
            break;
        case 1412U:
            hmi_draft.encoder_filter = (uint8_t)ClampStep(hmi_draft.encoder_filter, steps, 0U, 3U, 1U);
            break;
        default:
            return;
    }
    hmi_message = "CHANGED";
    menu.need_redraw = 1U;
}


uint8_t ServiceHMI_Key(input_key_t key)
{
    (void)ServiceHMI_Task();
    if (hmi_waiting != 0U)
        return 0U;
    if (key == Key_9_3 || key == Key_5_1)
        return 1U;
    if (key == Key_9_2 || key == Key_5_3)
    {
        hmi_waiting = UARTStore_RequestHMI(&hmi_draft);
        hmi_message = UARTStore_Status();
    }
    return 0U;
}


void ServiceHMI_Value(char *text)
{
    static const char *const types[] = {"ANY AB", "DETENT 00", "DETENT 11", "DETENT 00/11"};
    uint32_t value = 0U;
    text[0] = '\0';
    switch (hmi_id)
    {
        case 1406U:
            value = hmi_draft.debounce_ms;
            break;
        case 1407U:
            value = hmi_draft.long_ms;
            break;
        case 1408U:
            value = hmi_draft.func_seconds;
            break;
        case 1409U:
            Text_Append(text, types[hmi_draft.encoder_type]);
            return;
        case 1410U:
            value = hmi_draft.encoder_steps;
            break;
        case 1411U:
            Text_Append(text, hmi_draft.encoder_reverse ? "REVERSED" : "NORMAL");
            return;
        case 1412U:
            Text_U32ToDec(text, hmi_draft.encoder_filter / 2U, 1U);
            Text_Append(text, hmi_draft.encoder_filter % 2U != 0U ? ".5 MS" : ".0 MS");
            return;
        default:
            return;
    }
    Text_U32ToDec(text, value, 1U);
    Text_Append(text, hmi_id == 1408U ? " S" : (hmi_id == 1410U ? " EDGES" : " MS"));
}


const char *ServiceHMI_Range(void)
{
    switch (hmi_id)
    {
        case 1406U:
            return "5..50 MS STEP 5";
        case 1407U:
            return "500..2000 STEP 100";
        case 1408U:
            return "5..10 S";
        case 1409U:
            return "DETENT AB LEVEL";
        case 1410U:
            return "1 / 2 / 4 EDGES";
        case 1411U:
            return "ENCODER DIRECTION";
        case 1412U:
            return "0..1.5 MS STEP 0.5";
        default:
            return "";
    }
}
