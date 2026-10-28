#pragma once

enum class RomBrowserStateTrigger
{
    None,
    Navigate,
    ChangeDisplayMode,
    ShowGameInfo,
    HideGameInfo,
    ShowCheats,
    FolderLoadDone,
    Launch,
    LaunchRefused,
    ShowDisplaySettings,
    HideDisplaySettings,
    GotoSettingsScreen,
    ShowRecents,
    HideRecents,
    ShowFavorites,
    HideFavorites,
    ShowStatistics,
    HideStatistics,
    ShowDeleteConfirm,
    HideDeleteConfirm,
    ShowMenu,
    HideMenu,
    ShowAbout,
    HideAbout
};
