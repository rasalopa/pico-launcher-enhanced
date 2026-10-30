#include "common.h"
#include <algorithm>
#include <string.h>
#include "fat/Directory.h"
#include "SaveFileName.h"
#include "SdFolderFactory.h"
#include "SaveSlotIndex.h"

namespace
{
    // Names before the extension, compared as the card compares them, ignoring case.
    // Only ASCII letters fold: a save whose name differs from its game only in the case
    // of an accented letter is not noted.
    int CompareBaseNames(const char* a, u32 aLength, const char* b, u32 bLength)
    {
        int result = strncasecmp(a, b, std::min(aLength, bLength));
        return result != 0 ? result : (int)aLength - (int)bLength;
    }
}

SaveSlotIndex::SaveSlotIndex(SdFolder& folder)
{
    FileInfo* const* files = folder.GetFiles();
    int fileCount = folder.GetFileCount();
    for (int i = 0; i < fileCount; i++)
    {
        if (files[i]->GetFileType()->UsesLoaderSave())
            _gameCount++;
    }
    if (_gameCount == 0)
        return;

    _games = std::make_unique_for_overwrite<Game[]>(_gameCount);
    u32 gameIndex = 0;
    for (int i = 0; i < fileCount; i++)
    {
        if (!files[i]->GetFileType()->UsesLoaderSave())
            continue;
        // the same base as BuildSaveFileName takes
        const char* fileName = files[i]->GetFileName();
        const char* extension = strrchr(fileName, '.');
        _games[gameIndex++] = { files[i], extension ? (u32)(extension - fileName) : strlen(fileName) };
    }
    std::sort(_games.get(), _games.get() + _gameCount, [] (const Game& a, const Game& b)
    {
        return CompareBaseNames(a.file->GetFileName(), a.baseLength,
            b.file->GetFileName(), b.baseLength) < 0;
    });
}

void SaveSlotIndex::AddSaveFile(const char* fileName)
{
    u32 baseLength;
    int saveSlot = ParseSaveFileName(fileName, baseLength);
    if (saveSlot < 0 || _gameCount == 0)
        return;

    const Game* end = _games.get() + _gameCount;
    const Game* game = std::lower_bound((const Game*)_games.get(), end, fileName,
        [baseLength] (const Game& entry, const char* value)
        {
            return CompareBaseNames(entry.file->GetFileName(), entry.baseLength, value, baseLength) < 0;
        });
    // "Game.nds" and "Game.dsi" share their saves, as BuildSaveFileName gives both the same name
    for (; game != end &&
        CompareBaseNames(game->file->GetFileName(), game->baseLength, fileName, baseLength) == 0; game++)
    {
        game->file->AddSaveSlot(saveSlot);
    }
}

void SaveSlotIndex::AddSavesFolder(const char* path)
{
    if (_gameCount == 0)
        return;

    Directory directory;
    if (directory.Open(path) != FR_OK)
    {
        LOG_ERROR("Save scan couldn't open '%s'\n", path);
        SetIncomplete();
        return;
    }

    // Heap-allocated so it doesn't live on the io task's stack.
    auto fileInfo = std::make_unique<FILINFO>();
    // Not the empty-folder probes' shared budget, so neither can run out because of the
    // other. Ten saves per game, twice over for the "._" files a Mac adds, plus room for
    // saves of games that are gone; still a backstop against a pathological folder.
    int budget = SdFolderFactory::kPerFolderReadBudget + 20 * _gameCount;
    while (true)
    {
        if (budget-- <= 0)
        {
            LOG_ERROR("Save scan out of budget in '%s'\n", path);
            SetIncomplete();
            return;
        }

        if (directory.Read(fileInfo.get()) != FR_OK)
        {
            LOG_ERROR("Save scan couldn't read '%s'\n", path);
            SetIncomplete();
            return;
        }

        if (fileInfo->fname[0] == 0)
            return;

        if (!(fileInfo->fattrib & AM_DIR))
            AddSaveFile(fileInfo->fname);
    }
}

void SaveSlotIndex::SetIncomplete()
{
    for (u32 i = 0; i < _gameCount; i++)
        _games[i].file->SetSaveSlotsIncomplete();
}
