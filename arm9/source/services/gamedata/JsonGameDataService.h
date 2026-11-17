#pragma once
#include <memory>
#include "IGameDataService.h"

class JsonGameDataService : public IGameDataService
{
public:
    JsonGameDataService();

    const GameDataEntry* GetEntry(const char* fileName) const override;
    void ToggleFavorite(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) override;
    void ToggleCompleted(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) override;
    bool BackfillPath(const char* fileName, const char* fullPath) override;
    bool HasUnpathedFlaggedEntry() const override;
    void RecordLaunch(const char* fileName, const char* gameCode,
        const char* fullPath, const char* lastPlayedDateTime) override;
    bool SetSaveSlot(const char* fileName, u32 saveSlot, const char* gameCode = nullptr) override;
    const GameSaveInfo* GetSaveInfo(const char* fileName, u32 saveSlot) const override;
    bool RecordSaveLaunch(const char* fileName, u32 saveSlot, bool newSave,
        const char* lastPlayedDateTime, const char* gameCode = nullptr) override;
    bool SetGameLanguage(const char* fileName, PicoLoaderGameLanguage gameLanguage,
        const char* gameCode = nullptr) override;
    void RemoveEntry(const char* fileName) override;
    bool CloseOpenSession(u32 playSeconds) override;
    u32 GetEntryCount() const override { return _entryCount; }
    const GameDataEntry& GetEntryByIndex(u32 index) const override { return _entries[index]; }
    u32 GetVersion() const override { return _version; }
    void SaveAsync(TaskQueueBase* ioTaskQueue) override;

private:
    /// @brief The saves of one game. Kept out of GameDataEntry because few games
    ///        have more than one save, and inside every entry the names would add
    ///        close to 800 bytes for each game in the file.
    struct GameSaves
    {
        GameSaveInfo saves[kMaxSaveSlot + 1];
        bool inUse = false;
    };

    std::unique_ptr<GameDataEntry[]> _entries;
    u32 _entryCount = 0;
    u32 _entryCapacity = 0;
    /// @brief Indexed by GameDataEntry::gameSavesIndex. Freed ones stay in the
    ///        array, marked unused, so the indices of the others hold.
    std::unique_ptr<GameSaves[]> _gameSaves;
    u32 _gameSavesCount = 0;
    u32 _gameSavesCapacity = 0;
    u32 _version = 0;
    /// @brief Set when the data file was present at boot but could not be read
    ///        or parsed. Saving is then refused: writing out an empty or partial
    ///        set would overwrite the file and destroy every mark and play stat
    ///        it still holds.
    bool _loadFailed = false;
    String<char, FF_LFN_BUF> _sessionGameFileName;
    String<char, 8> _sessionGameCode;
    String<char, 20> _sessionStart;

    GameDataEntry* Find(const char* fileName);
    /// @brief Returns nullptr when the name cannot be stored (too long), so
    ///        callers refuse the operation instead of writing a truncated key.
    GameDataEntry* GetOrCreateEntry(const char* fileName, const char* gameCode);
    const GameSaves* FindGameSaves(const GameDataEntry& entry) const;
    GameSaves& GetOrCreateGameSaves(GameDataEntry& entry);
    void ClearSession();
    void Load();
};
