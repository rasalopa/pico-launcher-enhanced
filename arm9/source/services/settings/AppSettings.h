#pragma once
#include <memory>
#include "core/String.h"
#include "RomBrowserDisplaySettings.h"
#include "FileAssociation.h"

class AppSettings
{
public:
    String<char, 16> language = "english";
    String<char, 16> launcher = "pico";
    String<char, 64> theme = "material";
    String<char, 256> lastUsedFilePath = "";
    /// @brief DS Lite backlight level (0 = low .. 3 = max), or -1 to leave
    ///        the firmware's level untouched (the default until the user
    ///        picks one in display settings).
    s8 backlightLevel = -1;
    RomBrowserDisplaySettings romBrowserDisplaySettings;

    std::unique_ptr<FileAssociation[]> fileAssociations;
    u32 numberOfFileAssociations = 0;
};