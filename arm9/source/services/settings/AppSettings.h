#pragma once
#include <memory>
#include "core/String.h"
#include "RomBrowserDisplaySettings.h"
#include "SaveLocation.h"
#include "FileAssociation.h"

class AppSettings
{
public:
    String<char, 16> language = "english";
    String<char, 64> theme = "material";
    String<char, 256> lastUsedFilePath = "";
    /// @brief DS Lite backlight level (0 = low .. 3 = max), or -1 to leave
    ///        the firmware's level untouched (the default until the user
    ///        picks one in display settings).
    s8 backlightLevel = -1;
    SaveLocation saveLocation = SaveLocation::NextToRom;
    /// @brief Whether a launch is recorded in the game data (launch count,
    ///        play time, last played). Off, nothing about launches is
    ///        written; favorites and completed marks keep working.
    bool launchTracking = true;
    /// @brief Whether the "Favorites only" filter is on when the browser
    ///        starts. Toggling the filter in the menu does not change this.
    bool favoritesFilterAtStart = false;
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
};