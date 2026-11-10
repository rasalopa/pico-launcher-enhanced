#pragma once
#include <string.h>
#include "services/gamedata/IGameDataService.h"

/// @brief The save slot Kid saves use: .sav10, apart from the picker's 1 to 9.
inline constexpr u32 kKidSaveSlot = 10;

/// @brief Builds the save file name for a rom file name and save slot: the name without its
///        extension plus ".sav" for slot 0, the loader's own default, ".sav1" to ".sav9"
///        for the picker's slots, or ".sav10" for Kid saves.
/// @param romFileName The rom file name, without a folder.
/// @param saveSlot The save slot, 0 to kKidSaveSlot.
/// @param buffer The buffer to write the save file name to.
/// @param bufferLength The length of \p buffer.
/// @return \c false when the name does not fit: a shortened name would be another file,
///         and a save must never end up somewhere unexpected.
inline bool BuildSaveFileName(const char* romFileName, u32 saveSlot, char* buffer, u32 bufferLength)
{
    const char* extension = strrchr(romFileName, '.');
    u32 baseLength = extension ? extension - romFileName : strlen(romFileName);
    u32 slotLength = saveSlot == 0 ? 0 : saveSlot >= 10 ? 2 : 1;
    u32 length = baseLength + 4 + slotLength;
    if (saveSlot > kKidSaveSlot || length + 1 > bufferLength)
    {
        return false;
    }

    memcpy(buffer, romFileName, baseLength);
    memcpy(&buffer[baseLength], ".sav", 4);
    if (saveSlot != 0)
    {
        buffer[baseLength + 4] = '0' + (saveSlot >= 10 ? saveSlot / 10 : saveSlot);
        if (saveSlot >= 10)
            buffer[baseLength + 5] = '0' + saveSlot % 10;
    }
    buffer[length] = 0;
    return true;
}

/// @brief The inverse of BuildSaveFileName: whether \p fileName is a save file name, and of
///        which slot. The extension is matched in any case, as the card's names are.
/// @param fileName The file name, without a folder.
/// @param baseLength Set to the length of the name before the extension when it is a save.
/// @return The save slot, or -1 when \p fileName is not a save file name.
inline int ParseSaveFileName(const char* fileName, u32& baseLength)
{
    const char* extension = strrchr(fileName, '.');
    if (!extension || strncasecmp(extension, ".sav", 4) != 0)
    {
        return -1;
    }

    int saveSlot = 0;
    if (extension[4] != 0)
    {
        if (extension[4] < '1' || extension[4] > '0' + (int)IGameDataService::kMaxSaveSlot ||
            extension[5] != 0)
        {
            return -1;
        }
        saveSlot = extension[4] - '0';
    }
    baseLength = extension - fileName;
    return saveSlot;
}
