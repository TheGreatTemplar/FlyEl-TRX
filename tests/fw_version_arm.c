/* Фиксированные метаданные позволяют проверить полный SHA, перенос и границы буферов. */
#define BUILD_INFO_H
#define FW_GIT_HASH "0123456789abcdef0123456789abcdef01234567"
#define FW_GIT_SHORT "0123456789"
#define FW_GIT_BRANCH "codex/branch-with-a-long-readable-name"
#define FW_GIT_TAG "NONE"
#define FW_GIT_COUNT "12345"
#define FW_GIT_SUBJECT                                                                                                 \
    "Implement version information with a deliberately long description and "                                          \
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa word to check pagination"
#define FW_COMMIT_DATE "2026-10-27"
#define FW_COMMIT_TIME "12:34:56"
#define FW_BUILD_DATE "2026-10-28"
#define FW_BUILD_TIME "01:02:03"
#define FW_GIT_TREE "MODIFIED"
#define FW_SOURCE_HASH "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
#include "text_utils.c"
#include "fw_version.c"
#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
            return __LINE__;                                                                                           \
    } while (0)

int test_fw_version_fields_full_hash_dates_and_source_fingerprint(void)
{
    char text[24];
    char combined[65] = "";
    uint8_t row;
    FWInfo_Open();
    FWInfo_Line(1U, text);
    CHECK(strcmp(text, "GIT 0123456789") == 0);
    FWInfo_Line(4U, text);
    CHECK(strcmp(text, "TREE MODIFIED") == 0);
    FWInfo_Encoder(1);
    FWInfo_Line(0U, text);
    strcat(combined, text);
    FWInfo_Line(1U, text);
    strcat(combined, text);
    CHECK(strcmp(combined, "0123456789ABCDEF0123456789ABCDEF01234567") == 0);
    FWInfo_Line(2U, text);
    CHECK(strcmp(text, "COMMIT 2026-10-27") == 0);
    FWInfo_Encoder(2);
    combined[0] = '\0';
    for (row = 1U; row < 5U; row++)
    {
        FWInfo_Line(row, text);
        strcat(combined, text);
    }
    CHECK(strlen(combined) == 64U &&
          strcmp(combined, "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF") == 0);
    return 0;
}


int test_fw_version_navigation_and_all_lines_fit_buffers(void)
{
    uint8_t page;
    uint8_t row;
    FWInfo_Open();
    CHECK(FWInfo_Pages() >= 6U);
    for (page = 0U; page < FWInfo_Pages(); page++)
    {
        for (row = 0U; row < 5U; row++)
        {
            char text[24];
            memset(text, 0x55, sizeof(text));
            FWInfo_Line(row, text);
            CHECK(strlen(text) <= 20U && text[23] == 0x55);
        }
        FWInfo_Encoder(1);
    }
    CHECK(FWInfo_Page() == 0U);
    FWInfo_Encoder(-1);
    CHECK(FWInfo_Page() == FWInfo_Pages() - 1U);
    FWInfo_Encoder(2147483647);
    FWInfo_Encoder(-2147483647 - 1);
    CHECK(FWInfo_Page() < FWInfo_Pages());
    CHECK(FWInfo_Key(Key_9_3) != 0U && FWInfo_Key(Key_5_1) != 0U);
    return 0;
}
