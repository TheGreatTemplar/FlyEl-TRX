#include "fw_version.h"
#include "build_info.h"
#include "text_utils.h"
#include <string.h>

static uint8_t fw_page;

/* Приводит ASCII к регистру шрифта 5x7; строки Git не интерпретируются как команды. */
static void CopyPart(char *text, const char *source, uint16_t offset, uint8_t count)
{
    uint8_t i = 0U;
    uint16_t length = (uint16_t)strlen(source);
    while (i < count && offset + i < length)
    {
        char c = source[offset + i];
        text[i++] = (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
    }
    text[i] = '\0';
}


/* Перенос описания по словам; очень длинное слово делится на части по 20 символов. */
static const char *MessageLine(const char *cursor, char *text)
{
    uint8_t length = 0U;
    uint8_t split = 0U;
    while (*cursor == ' ')
        cursor++;
    while (length < 20U && cursor[length] != '\0')
    {
        if (cursor[length] == ' ')
            split = length;
        length++;
    }
    if (cursor[length] != '\0' && split != 0U)
        length = split;
    if (text != 0)
        CopyPart(text, cursor, 0U, length);
    return cursor + length;
}


uint8_t FWInfo_Pages(void)
{
    const char *cursor = FW_GIT_SUBJECT;
    uint8_t lines = 0U;
    while (*cursor != '\0')
    {
        cursor = MessageLine(cursor, 0);
        while (*cursor == ' ')
            cursor++;
        lines++;
    }
    return (uint8_t)(4U + (lines != 0U ? (lines + 4U) / 5U : 1U));
}


void FWInfo_Open(void)
{
    fw_page = 0U;
}


uint8_t FWInfo_Page(void)
{
    return fw_page;
}


void FWInfo_Encoder(int32_t steps)
{
    int32_t next = ((int32_t)fw_page + steps % FWInfo_Pages()) % FWInfo_Pages();
    if (next < 0)
        next += FWInfo_Pages();
    fw_page = (uint8_t)next;
}


uint8_t FWInfo_Key(input_key_t key)
{
    if (key == Key_9_3 || key == Key_5_1)
        return 1U;
    if (key == Key_9_2 || key == Key_5_3)
        FWInfo_Encoder(1);
    return 0U;
}


const char *FWInfo_Title(void)
{
    switch (fw_page)
    {
        case 0U:
            return "01.02 FW VERSION";
        case 1U:
            return "COMMIT";
        case 2U:
            return "BRANCH / TAG";
        case 3U:
            return "SOURCE SHA256";
        default:
            return "COMMIT MESSAGE";
    }
}


void FWInfo_Line(uint8_t row, char *text)
{
    text[0] = '\0';
    if (row >= 5U)
        return;
    if (fw_page == 0U)
    {
        switch (row)
        {
            case 0U:
                strcpy(text, "FLYEL TRX");
                break;
            case 1U:
                strcpy(text, "GIT ");
                CopyPart(text + 4U, FW_GIT_SHORT, 0U, 12U);
                break;
            case 2U:
                strcpy(text, "BUILD ");
                Text_Append(text, FW_BUILD_DATE);
                break;
            case 3U:
                strcpy(text, FW_BUILD_TIME);
                Text_Append(text, " UTC");
                break;
            default:
                strcpy(text, "TREE ");
                Text_Append(text, FW_GIT_TREE);
                break;
        }
    }
    else if (fw_page == 1U)
    {
        if (row < 2U)
            CopyPart(text, FW_GIT_HASH, (uint16_t)(row * 20U), 20U);
        else if (row == 2U)
        {
            strcpy(text, "COMMIT ");
            Text_Append(text, FW_COMMIT_DATE);
        }
        else if (row == 3U)
        {
            strcpy(text, FW_COMMIT_TIME);
            Text_Append(text, " UTC");
        }
        else
        {
            strcpy(text, "REV ");
            Text_Append(text, FW_GIT_COUNT);
        }
    }
    else if (fw_page == 2U)
    {
        if (row == 0U)
            strcpy(text, "BRANCH");
        else if (row < 3U)
            CopyPart(text, FW_GIT_BRANCH, (uint16_t)((row - 1U) * 20U), 20U);
        else if (row == 3U)
            strcpy(text, "TAG");
        else
            CopyPart(text, FW_GIT_TAG, 0U, 20U);
    }
    else if (fw_page == 3U)
    {
        if (row == 0U)
            strcpy(text, "SOURCE FILES SHA256");
        else
            CopyPart(text, FW_SOURCE_HASH, (uint16_t)((row - 1U) * 16U), 16U);
    }
    else
    {
        uint8_t line;
        uint8_t wanted = (uint8_t)((fw_page - 4U) * 5U + row);
        const char *cursor = FW_GIT_SUBJECT;
        for (line = 0U; line <= wanted && *cursor != '\0'; line++)
            cursor = MessageLine(cursor, line == wanted ? text : 0);
    }
}
