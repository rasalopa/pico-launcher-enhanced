#pragma once
#include <string.h>
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "../IRomBrowserController.h"
#include "../FileInfo.h"
#include "services/gamedata/IGameDataService.h"

/// @brief View model for the save picker, which A opens on a DS game with saves besides
///        its main one: the main save first, even when its file is missing, then every
///        other save found when the folder was read, then a new save while one is free.
///        It opens on the save the game was started on last time. A save shows the name
///        PicoDex gave it and the day it was last started, when those are known.
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

        // never "+ New save": if the last save is gone, its slot may be the free one
        const auto* entry = romBrowserController->GetGameDataService()->GetEntry(fileInfo.GetFileName());
        u32 lastSaveSlot = entry ? entry->saveSlot : 0;
        u32 existingCount = _newSaveSlot != 0 ? _entryCount - 1 : _entryCount;
        for (u32 i = 0; i < existingCount; i++)
        {
            if (_slots[i] == lastSaveSlot)
                _initialEntry = i;
        }

        const char* fileName = fileInfo.GetFileName();
        const char* extension = strrchr(fileName, '.');
        u32 nameLength = extension ? extension - fileName : strlen(fileName);
        memcpy(_gameName, fileName, nameLength);
        _gameName[nameLength] = 0;
    }

    /// @brief The file name of the game, without its extension.
    const char* GetGameName() const { return _gameName; }

    u32 GetEntryCount() const { return _entryCount; }

    /// @brief The entry the sheet opens on: the save started last time, or the main one when
    ///        that save is gone.
    u32 GetInitialEntry() const { return _initialEntry; }

    /// @brief Writes the name PicoDex gave the save, else "Main save" or "Save N", or
    ///        "+ New save" for the entry that makes one.
    void FormatEntry(u32 index, char* buffer, u32 bufferLength) const
    {
        const GameSaveInfo* save = GetSaveInfo(index);
        if (IsNewSaveEntry(index))
            mini_snprintf(buffer, bufferLength, "+ New save");
        else if (save && save->name.GetString()[0] != 0)
            StringUtil::Copy(buffer, save->name.GetString(), bufferLength);
        else if (_slots[index] == 0)
            mini_snprintf(buffer, bufferLength, "Main save");
        else
            mini_snprintf(buffer, bufferLength, "Save %u", (u32)_slots[index]);
    }

    /// @brief Writes "DD/MM", the day the game was last started on the save, as the
    ///        recents list does. Returns false when there is no date to show.
    bool FormatDate(u32 index, char* buffer, u32 bufferLength) const
    {
        const GameSaveInfo* save = GetSaveInfo(index);
        if (IsNewSaveEntry(index) || !save)
            return false;
        // "YYYY-MM-DD HH:MM"
        const char* lastPlayed = save->lastPlayed.GetString();
        if (strlen(lastPlayed) < 10)
            return false;
        mini_snprintf(buffer, bufferLength, "%c%c/%c%c", lastPlayed[8], lastPlayed[9], lastPlayed[5], lastPlayed[6]);
        return true;
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
    bool IsNewSaveEntry(u32 index) const { return _newSaveSlot != 0 && index == _entryCount - 1; }

    /// @brief nullptr for the new save, and when nothing is stored for the game's saves.
    const GameSaveInfo* GetSaveInfo(u32 index) const
    {
        if (IsNewSaveEntry(index))
            return nullptr;
        return _romBrowserController->GetGameDataService()->GetSaveInfo(
            _romBrowserController->GetTriggerFileInfo().GetFileName(), _slots[index]);
    }

    IRomBrowserController* _romBrowserController;
    /// @brief The save each entry starts.
    u8 _slots[kMaxEntryCount];
    u32 _entryCount = 0;
    u32 _initialEntry = 0;
    /// @brief The slot "+ New save" starts, 0 when all of them are taken.
    u32 _newSaveSlot = 0;
    /// @brief Whole, as a file name is never longer than FF_LFN_BUF: cutting it could split a
    ///        UTF-8 character.
    char _gameName[FF_LFN_BUF + 1];
};
