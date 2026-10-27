#ifndef BUILD_INFO_H
#define BUILD_INFO_H
/* Генератор общий для Keil и scripts/build.ps1. Без Git/предсборки данные честно UNKNOWN. */
#if defined(__has_include)
#if __has_include("../build/build_info_generated.h")
#include "../build/build_info_generated.h"
#endif
#endif
#ifndef FW_GIT_HASH
#define FW_GIT_HASH "UNKNOWN"
#define FW_GIT_SHORT "UNKNOWN"
#define FW_GIT_BRANCH "UNKNOWN"
#define FW_GIT_TAG "NONE"
#define FW_GIT_COUNT "UNKNOWN"
#define FW_GIT_SUBJECT "BUILD METADATA NOT GENERATED"
#define FW_COMMIT_DATE "UNKNOWN"
#define FW_COMMIT_TIME "UNKNOWN"
#define FW_BUILD_DATE "UNKNOWN"
#define FW_BUILD_TIME "UNKNOWN"
#define FW_GIT_TREE "UNKNOWN"
#define FW_SOURCE_HASH "UNKNOWN"
#endif
#endif
