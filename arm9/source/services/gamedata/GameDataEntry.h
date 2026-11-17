#pragma once
#include <string.h>
#include "core/String.h"
#include "fat/ff.h"
#include "picoLoader7.h"

/// @brief The name PicoDex gave one of a game's saves, and when the game was last
///        started on it. See IGameDataService::GetSaveInfo.
struct GameSaveInfo
{
    /// @brief The longest name in bytes: PicoDex takes 20 characters, and a kana
    ///        is three bytes in UTF-8.
    static constexpr u32 kMaxNameBytes = 60;
    /// @brief "YYYY-MM-DD HH:MM"
    static constexpr u32 kDateLength = 16;

    String<char, kMaxNameBytes> name;
    /// @brief "YYYY-MM-DD HH:MM", empty when the save was not started since the
    ///        launcher began keeping a date for each save.
    String<char, kDateLength> lastPlayed;
};

/// @brief Persisted data for ONE rom file, keyed by its file name (same
///        convention as the /_pico/icons|covers/user folders). Marks and play
///        statistics belong to the file, so two copies of a game are tracked
///        separately and renaming a rom starts it over.
struct GameDataEntry
{
    /// @brief Whole: FatFs returns names of up to FF_LFN_BUF bytes.
    String<char, FF_LFN_BUF> fileName;
    /// @brief Internal game code (NDS/GBA header), empty when the file has
    ///        none. Stored as information only - it is never used to look an
    ///        entry up, because the browser filter only ever sees file names and
    ///        the two would then disagree (issue #7).
    String<char, 8> gameCode;
    u32 launchCount = 0;
    /// @brief Play time from older versions, which timed the clock until the next
    ///        launcher boot. Kept in the file, no longer added to or shown.
    u32 playMinutes = 0;
    /// @brief Play time Pico Loader Enhanced measured, from the frames the game ran.
    u32 playSeconds = 0;
    bool favorite = false;
    /// @brief Marked as finished by the user (long-press X in the browser).
    bool completed = false;
    /// @brief The save slot the game was started on last time, which the save
    ///        picker opens on (see RomBrowserController::LaunchFile).
    ///        0 is the regular .sav, 1 to IGameDataService::kMaxSaveSlot use
    ///        .sav1 to .sav9.
    u8 saveSlot = 0;
    /// @brief The language the game starts in, a PicoLoaderGameLanguage:
    ///        PLOAD_GAME_LANGUAGE_AUTO for the console's.
    u8 gameLanguage = PLOAD_GAME_LANGUAGE_AUTO;
    /// @brief "YYYY-MM-DD HH:MM", empty when never launched. Lexicographic
    ///        order equals chronological order.
    String<char, 20> lastPlayed;
    /// @brief Full path of the file at its last launch, empty when never
    ///        launched. Used by the recents list to navigate back to it.
    String<char, 256> path;

    static constexpr u16 kNoGameSaves = 0xFFFF;
    /// @brief Where JsonGameDataService keeps this game's GameSaveInfo, apart from
    ///        the entries; kNoGameSaves for none. Only the service reads it.
    u16 gameSavesIndex = kNoGameSaves;
};

/// @brief Orders games by launches, ties by file name regardless of case, so
///        the star on the top screen and the statistics panel's list are built
///        from the same rule and never name different games.
inline bool LaunchedMoreThan(const GameDataEntry& a, const GameDataEntry& b)
{
    return a.launchCount > b.launchCount ||
        (a.launchCount == b.launchCount &&
            strcasecmp(a.fileName.GetString(), b.fileName.GetString()) < 0);
}
