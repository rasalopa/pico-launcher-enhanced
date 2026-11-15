#pragma once
#include "../IRomBrowserController.h"

/// @brief View model for the menu the app bar's "more" button opens. Every
///        entry forwards to the controller: the panels replace the sheet with
///        their own, and a filter closes it through the display mode change
///        it fires (see the Menu state in RomBrowserStateMachine), New save
///        launches the highlighted game, and Language moves it to its next language.
class MenuViewModel
{
public:
    /// @brief Finds the save New save would start and the game's languages once, as the
    ///        highlighted game can't change while the menu is open.
    explicit MenuViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _isDsGame(romBrowserController->SelectedHasSaveSlots())
        , _newSaveSlot(_isDsGame ? romBrowserController->FindNewSaveSlotOfSelected() : 0)
        , _gameLanguageMask(_isDsGame ? romBrowserController->GetGameLanguageMaskOfSelected() : 0)
        , _gameLanguage(_isDsGame ? romBrowserController->GetGameLanguageOfSelected() : PLOAD_GAME_LANGUAGE_AUTO) { }

    /// @brief Whether the highlighted entry is a DS game: only then are New save and Language
    ///        in the menu.
    bool IsDsGame() const { return _isDsGame; }
    /// @brief The save New save starts, 0 when it can't start one.
    u32 GetNewSaveSlot() const { return _newSaveSlot; }
    void StartNewSave()
    {
        if (_newSaveSlot != 0)
            _romBrowserController->LaunchSelectedOnSaveSlot(_newSaveSlot);
    }

    PicoLoaderGameLanguage GetGameLanguage() const { return _gameLanguage; }
    /// @brief Whether there is a language to pick, which there isn't without a game code.
    bool CanPickGameLanguage() const { return _gameLanguageMask != 0; }
    /// @brief Moves the game to the next language of its region, and after the last back to
    ///        the console's.
    void PickNextGameLanguage()
    {
        if (_gameLanguageMask == 0)
            return;
        // the console's language comes before Japanese, which is bit 0
        u32 first = _gameLanguage == PLOAD_GAME_LANGUAGE_AUTO ? 0 : _gameLanguage + 1;
        u32 next = _gameLanguageMask & (0xFFu << first);
        _gameLanguage = next != 0 ? (PicoLoaderGameLanguage)__builtin_ctz(next) : PLOAD_GAME_LANGUAGE_AUTO;
        _romBrowserController->SetGameLanguageOfSelected(_gameLanguage);
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

    void Close() { _romBrowserController->HideMenu(); }

private:
    IRomBrowserController* _romBrowserController;
    bool _isDsGame;
    u32 _newSaveSlot;
    /// @brief Bit N for PicoLoaderGameLanguage N.
    u32 _gameLanguageMask;
    PicoLoaderGameLanguage _gameLanguage;
};
