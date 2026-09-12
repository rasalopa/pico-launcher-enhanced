#pragma once

// UI localization selected by AppSettings.language (e.g. "english", "spanish").
// The default remains English so existing settings.json files behave exactly as before.
class Localization
{
public:
    static void SetLanguage(const char* language);
    static const char* GetLanguage();

    static const char* DisplaySettings();
    static const char* Layout();
    static const char* Sorting();
    static const char* Light();
    static const char* Language();
    static const char* Launcher();
    static const char* PicoLauncher();
    static const char* BootstrapLauncher();
    static const char* Cheats();
    static const char* Favorite();
    static const char* RecentGames();
    static const char* FavoriteGames();
    static const char* NothingPlayedYet();
    static const char* NoFavoritesYet();
    static const char* DeleteGame();
    static const char* DeleteHint();
    static const char* SaveAlsoDeleted();
    static const char* Statistics();
    static const char* NoCheatsFound();
    static const char* CheatsAllOff();
    static const char* NoPreview();
    static const char* ScreenshotSaved();
    static const char* ScreenshotSaveFailed();
    static const char* ScreenshotStillSaving();

    static const char* GameCountFormat();
    static const char* GameWord(unsigned count);
    static const char* PlayedFavoritesCompletedFormat();
    static const char* PlayedFavoritesFormat();
    static const char* LaunchesPlayedFormat();
    static const char* LaunchesTotalFormat();
    static const char* LastPlayedFormat();
    static const char* Month(unsigned month);
};
