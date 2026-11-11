#pragma once
#include "core/mini-printf.h"

/// @brief Formats play time as "3h20", or "45m" under an hour. Empty under a minute.
inline void FormatPlayTime(char* dst, u32 dstSize, u32 seconds)
{
    u32 minutes = seconds / 60;
    if (minutes >= 60)
        mini_snprintf(dst, dstSize, "%uh%02u", minutes / 60, minutes % 60);
    else if (minutes > 0)
        mini_snprintf(dst, dstSize, "%um", minutes);
    else
        dst[0] = 0;
}
