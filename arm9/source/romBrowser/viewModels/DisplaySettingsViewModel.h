#pragma once
#include "../IRomBrowserController.h"
#include "services/settings/RomBrowserDisplaySettings.h"

/// @brief View model for the display settings screen.
class DisplaySettingsViewModel
{
public:
    explicit DisplaySettingsViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _romBrowserDisplaySettings(_romBrowserController->GetRomBrowserDisplaySettings()) { }

    constexpr RomBrowserLayout GetRomBrowserDisplayMode() const
    {
        return _romBrowserDisplaySettings.layout;
    }

    void SetRomBrowserDisplayMode(RomBrowserLayout romBrowserDisplayMode)
    {
        if (_romBrowserDisplaySettings.layout != romBrowserDisplayMode)
        {
            _romBrowserDisplaySettings.layout = romBrowserDisplayMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    constexpr RomBrowserSortMode GetRomBrowserSortMode() const
    {
        return _romBrowserDisplaySettings.sortMode;
    }

    void SetRomBrowserSortMode(RomBrowserSortMode romBrowserSortMode)
    {
        if (_romBrowserDisplaySettings.sortMode != romBrowserSortMode)
        {
            _romBrowserDisplaySettings.sortMode = romBrowserSortMode;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    constexpr bool GetHideEmptyFolders() const
    {
        return _romBrowserDisplaySettings.hideEmptyFolders;
    }

    void SetHideEmptyFolders(bool hideEmptyFolders)
    {
        if (_romBrowserDisplaySettings.hideEmptyFolders != hideEmptyFolders)
        {
            _romBrowserDisplaySettings.hideEmptyFolders = hideEmptyFolders;
            _romBrowserController->SetRomBrowserDisplaySettings(_romBrowserDisplaySettings);
        }
    }

    int GetBacklightLevel() const
    {
        return _romBrowserController->GetBacklightLevel();
    }

    void SetBacklightLevel(int level)
    {
        _romBrowserController->SetBacklightLevel(level);
    }

    const char* GetLauncher() const
    {
        return _romBrowserController->GetLauncher();
    }

    void SetLauncher(const char* launcher)
    {
        _romBrowserController->SetLauncher(launcher);
    }

    const char* GetLanguage() const
    {
        return _romBrowserController->GetLanguage();
    }

    void SetLanguage(const char* language)
    {
        _romBrowserController->SetLanguage(language);
    }

    void Close()
    {
        _romBrowserController->HideDisplaySettings();
    }

    void GotoSettingsScreen()
    {
        _romBrowserController->GotoSettingsScreen();
    }

private:
    IRomBrowserController* _romBrowserController;
    RomBrowserDisplaySettings _romBrowserDisplaySettings;
};
