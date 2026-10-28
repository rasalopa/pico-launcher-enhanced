#pragma once
#include "core/task/TaskQueue.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/IRomBrowserController.h"

/// @brief View model for the nds game details sheet (Y in the browser): the save slot the
///        loader uses for the game, and the way to its cheats.
class NdsGameDetailsViewModel
{
public:
    NdsGameDetailsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController);

    ~NdsGameDetailsViewModel()
    {
        // The load task writes into this view model, so make sure it does not start after it is gone
        _loadTask.CancelTask();
    }

    const FileInfo& GetRomFileInfo() const { return _romFileInfo; }

    /// @brief Whether the rom header and the existing saves have been read.
    bool IsLoaded() const { return _isLoaded; }

    /// @brief Whether the game has save slots at all. Homebrew and DSiWare have none: the
    ///        loader keeps no card save for them. Valid once IsLoaded.
    bool HasSaveSlots() const { return _hasSaveSlots; }

    u32 GetSaveSlot() const { return _saveSlot; }

    /// @brief Whether the selected slot has a save already, in the saves folder or next to
    ///        the game. A new one is created by the loader on launch. Valid once IsLoaded.
    bool SaveFileExists() const { return _existingSaveSlots & (1 << _saveSlot); }

    /// @brief Gets the save file of the selected slot as the user finds it on the card:
    ///        "saves/<name>" when the saves folder setting is on, "<name>" otherwise.
    /// @return \c true when the name was written, or \c false when it did not fit.
    bool GetSaveFileName(char* fileName, u32 fileNameLength) const;

    void NextSaveSlot();
    void PreviousSaveSlot();

    /// @brief Stores the slot and swaps this sheet for the cheats sheet.
    void ShowCheats();

    /// @brief Stores the slot and closes the sheet.
    void Close();

private:
    FileInfo _romFileInfo;
    IRomBrowserController* _romBrowserController;
    QueueTask<void> _loadTask;
    bool _savesFolder;
    volatile bool _isLoaded = false;
    bool _hasSaveSlots = false;
    u32 _saveSlot = 0;
    u32 _storedSaveSlot = 0;
    u16 _existingSaveSlots = 0;

    void StoreSaveSlot();
};
