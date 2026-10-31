#pragma once
#include <string.h>
#include "core/mini-printf.h"
#include "../IRomBrowserController.h"
#include "../FileInfo.h"
#include "services/gamedata/IGameDataService.h"

/// @brief View model for the save picker, which A opens on a DS game with saves besides
///        its main one: the main save first, even when its file is missing, then every
///        other save found when the folder was read, then a new save while one is free.
class SavePickerViewModel
{
public:
    /// @brief The most there can be: with every slot taken there is no new one.
    static constexpr u32 kMaxEntryCount = IGameDataService::kMaxSaveSlot + 1;

    explicit SavePickerViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
    {
        const auto& fileInfo = romBrowserController->GetTriggerFileInfo();
        u16 saveSlots = fileInfo.GetSaveSlots() | 1;
        for (u32 slot = 0; slot <= IGameDataService::kMaxSaveSlot; slot++)
        {
            if (saveSlots & (1 << slot))
                _slots[_entryCount++] = slot;
            else if (_newSaveSlot == 0)
                _newSaveSlot = slot;
        }
        // the lowest free one, so the names stay in order as saves are added
        if (_newSaveSlot != 0)
            _slots[_entryCount++] = _newSaveSlot;

        const char* fileName = fileInfo.GetFileName();
        const char* extension = strrchr(fileName, '.');
        u32 nameLength = extension ? extension - fileName : strlen(fileName);
        memcpy(_gameName, fileName, nameLength);
        _gameName[nameLength] = 0;
    }

    /// @brief The file name of the game, without its extension.
    const char* GetGameName() const { return _gameName; }

    u32 GetEntryCount() const { return _entryCount; }

    /// @brief Writes "Main save", "Save N" or "+ New save" for the entry.
    void FormatEntry(u32 index, char* buffer, u32 bufferLength) const
    {
        if (_newSaveSlot != 0 && index == _entryCount - 1)
            mini_snprintf(buffer, bufferLength, "+ New save");
        else if (_slots[index] == 0)
            mini_snprintf(buffer, bufferLength, "Main save");
        else
            mini_snprintf(buffer, bufferLength, "Save %u", (u32)_slots[index]);
    }

    void ActivateEntry(int index)
    {
        if (index >= 0 && (u32)index < _entryCount)
            _romBrowserController->LaunchSaveSlot(_slots[index]);
    }

    void Close()
    {
        _romBrowserController->HideSavePicker();
    }

private:
    IRomBrowserController* _romBrowserController;
    /// @brief The save each entry starts.
    u8 _slots[kMaxEntryCount];
    u32 _entryCount = 0;
    /// @brief The slot "+ New save" starts, 0 when all of them are taken.
    u32 _newSaveSlot = 0;
    /// @brief Whole, as a file name is never longer than FF_LFN_BUF: cutting it could split a
    ///        UTF-8 character.
    char _gameName[FF_LFN_BUF + 1];
};
