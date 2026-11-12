#pragma once
#include "../IRomBrowserController.h"

/// @brief View model for the menu the app bar's "more" button opens. Every
///        entry forwards to the controller: the panels replace the sheet with
///        their own, and a filter closes it through the display mode change
///        it fires (see the Menu state in RomBrowserStateMachine), and New save
///        launches the highlighted game.
class MenuViewModel
{
public:
    /// @brief Finds the save New save would start once, as the highlighted game can't
    ///        change while the menu is open.
    explicit MenuViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _showsNewSave(romBrowserController->SelectedHasSaveSlots())
        , _newSaveSlot(_showsNewSave ? romBrowserController->FindNewSaveSlotOfSelected() : 0) { }

    /// @brief Whether New save is in the menu at all: only for a DS game.
    bool ShowsNewSave() const { return _showsNewSave; }
    /// @brief The save New save starts, 0 when it can't start one.
    u32 GetNewSaveSlot() const { return _newSaveSlot; }
    void StartNewSave()
    {
        if (_newSaveSlot != 0)
            _romBrowserController->LaunchSelectedOnSaveSlot(_newSaveSlot);
    }

    void ShowRecents() { _romBrowserController->ShowRecents(); }
    void ShowFavorites() { _romBrowserController->ShowFavorites(); }
    void ShowStatistics() { _romBrowserController->ShowStatistics(); }
    bool CanDeleteSelected() const { return _romBrowserController->CanDeleteSelected(); }
    void RequestDeleteSelected() { _romBrowserController->RequestDeleteSelected(); }
    void ToggleFavoritesFilter() { _romBrowserController->ToggleFavoritesFilter(); }
    bool IsFavoritesFilterEnabled() const { return _romBrowserController->IsFavoritesFilterEnabled(); }
    void ToggleCompletedFilter() { _romBrowserController->ToggleCompletedFilter(); }
    bool IsCompletedFilterEnabled() const { return _romBrowserController->IsCompletedFilterEnabled(); }

    void ShowAbout() { _romBrowserController->ShowAbout(); }
    void ShowParentalControls() { _romBrowserController->ShowParentalControls(); }

    void Close() { _romBrowserController->HideMenu(); }

private:
    IRomBrowserController* _romBrowserController;
    bool _showsNewSave;
    u32 _newSaveSlot;
};
