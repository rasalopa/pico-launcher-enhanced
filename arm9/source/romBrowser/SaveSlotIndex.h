#pragma once
#include <memory>
#include "SdFolder.h"

/// @brief Notes on each DS game of a folder which of its saves exist (FileInfo::GetSaveSlots),
///        from the file names that reading the folder already gives, so nothing is read
///        again for it. Only lives for the navigate task that reads the folder.
class SaveSlotIndex
{
public:
    /// @brief Indexes the games in \p folder whose save the loader keeps.
    /// @param folder The folder just read; its entries get the slots marked on them.
    explicit SaveSlotIndex(SdFolder& folder);

    /// @brief When \p fileName is a save file name, marks its slot on every indexed game
    ///        with the same name before the extension. Any other name is ignored.
    /// @param fileName The file name, without a folder.
    void AddSaveFile(const char* fileName);

    /// @brief Runs AddSaveFile for every file in the folder at \p path, within an allowance
    ///        of its own sized to the indexed games. When the folder can't be read to its
    ///        end, every indexed game is marked with FileInfo::SetSaveSlotsIncomplete.
    ///        Reads nothing when no game was indexed.
    /// @param path The folder path, relative to the current folder.
    void AddSavesFolder(const char* path);

private:
    void SetIncomplete();

    struct Game
    {
        FileInfo* file;
        /// @brief Length of the file name before its extension.
        u32 baseLength;
    };

    /// @brief The games, sorted by the name before the extension, ignoring case.
    std::unique_ptr<Game[]> _games;
    u32 _gameCount = 0;
};
