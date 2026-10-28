#pragma once
#include <string.h>
#include "services/gamedata/IGameDataService.h"

/// @brief Builds the save file name for a rom file name and save slot: the name without its
///        extension plus ".sav" for slot 0, the loader's own default, or ".sav1" to ".sav9"
///        for the other slots, the same names TWiLight Menu++ gives its extra saves.
/// @param romFileName The rom file name, without a folder.
/// @param saveSlot The save slot, 0 to IGameDataService::kMaxSaveSlot.
/// @param buffer The buffer to write the save file name to.
/// @param bufferLength The length of \p buffer.
/// @return \c false when the name does not fit: a shortened name would be another file,
///         and a save must never end up somewhere unexpected.
inline bool BuildSaveFileName(const char* romFileName, u32 saveSlot, char* buffer, u32 bufferLength)
{
    const char* extension = strrchr(romFileName, '.');
    u32 baseLength = extension ? extension - romFileName : strlen(romFileName);
    u32 length = baseLength + (saveSlot == 0 ? 4 : 5);
    if (saveSlot > IGameDataService::kMaxSaveSlot || length + 1 > bufferLength)
    {
        return false;
    }

    memcpy(buffer, romFileName, baseLength);
    memcpy(&buffer[baseLength], ".sav", 4);
    if (saveSlot != 0)
    {
        buffer[baseLength + 4] = '0' + saveSlot;
    }
    buffer[length] = 0;
    return true;
}
