#pragma once
#include "../IRomBrowserController.h"

/// @brief View model for the rom browser app bar
class RomBrowserAppBarViewModel
{
public:
    explicit RomBrowserAppBarViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void NavigateUp()
    {
        _romBrowserController->NavigateUp();
    }

    void ShowDisplaySettings()
    {
        _romBrowserController->ShowDisplaySettings();
    }

    void ToggleFavoritesFilter()
    {
        _romBrowserController->ToggleFavoritesFilter();
    }

    bool IsFavoritesFilterEnabled() const
    {
        return _romBrowserController->IsFavoritesFilterEnabled();
    }

    void ToggleCompletedFilter()
    {
        _romBrowserController->ToggleCompletedFilter();
    }

    bool IsCompletedFilterEnabled() const
    {
        return _romBrowserController->IsCompletedFilterEnabled();
    }

    bool CanDeleteSelected() const
    {
        return _romBrowserController->CanDeleteSelected();
    }

    void ShowRecents()
    {
        _romBrowserController->ShowRecents();
    }

    void ShowFavorites()
    {
        _romBrowserController->ShowFavorites();
    }

    void ShowStatistics()
    {
        _romBrowserController->ShowStatistics();
    }

    void RequestDeleteSelected()
    {
        _romBrowserController->RequestDeleteSelected();
    }

    constexpr RomBrowserLayout GetRomBrowserLayout() const
    {
        return _romBrowserController->GetRomBrowserDisplaySettings().layout;
    }

private:
    IRomBrowserController* _romBrowserController;
};
