#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class IIconRepository;
class IBannerRepository;
class ICheatRepository;
class IGameDataService;
class IBgmService;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    /// @brief gameCode is the file's internal game code when the caller has
    ///        it loaded, or nullptr; it keys the game's persisted data. A DS game
    ///        with saves besides its main one opens the save picker instead.
    virtual void LaunchFile(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void LaunchRandomGame() = 0;
    /// @brief Launches the game the save picker shows with one of its saves: 0 for
    ///        the main one, 1 to IGameDataService::kMaxSaveSlot for the others.
    virtual void LaunchSaveSlot(u32 saveSlot) = 0;
    virtual void HideSavePicker() = 0;
    /// @brief Whether the highlighted entry is a DS game, the only kind with extra saves.
    virtual bool SelectedHasSaveSlots() const = 0;
    /// @brief The save the menu's "New save" would start the highlighted game on: the
    ///        lowest of 1 to IGameDataService::kMaxSaveSlot not on the card, or 0 when the
    ///        entry is not a DS game or all of them are taken. Looks on the card when the
    ///        saves folder wasn't read to its end.
    virtual u32 FindNewSaveSlotOfSelected() const = 0;
    /// @brief Launches the highlighted game with that save, as the save picker would.
    virtual void LaunchSelectedOnSaveSlot(u32 saveSlot) = 0;
    virtual void ToggleFavorite(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleCompleted(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleFavoritesFilter() = 0;
    virtual bool IsFavoritesFilterEnabled() const = 0;
    virtual void ToggleCompletedFilter() = 0;
    virtual bool IsCompletedFilterEnabled() const = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    /// @brief The Game info sheet, reached from the menu: edits the title,
    ///        publisher and developer shown for the highlighted game.
    virtual void ShowGameInfoEdit() = 0;
    /// @brief Stores the edited values for the game the sheet was opened on and
    ///        rebuilds the browser on the same game. Empty strings clear a field.
    virtual void SetGameMetadata(const char* title, const char* publisher,
        const char* developer) = 0;
    /// @brief Whether the highlighted entry has game info worth editing. Games
    ///        yes, folders and support files no.
    virtual bool CanEditSelected() const = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void ShowRecents() = 0;
    virtual void HideRecents() = 0;
    virtual void ShowFavorites() = 0;
    virtual void HideFavorites() = 0;
    virtual void ShowStatistics() = 0;
    virtual void HideStatistics() = 0;
    /// @brief The menu behind the app bar's "more" button.
    virtual void ShowMenu() = 0;
    virtual void HideMenu() = 0;
    /// @brief The about sheet, reached from the menu.
    virtual void ShowAbout() = 0;
    virtual void HideAbout() = 0;
    /// @brief Whether the highlighted entry can be deleted at all. Folders and
    ///        support files cannot, so the app bar dims its delete button rather
    ///        than offering one that does nothing.
    virtual bool CanDeleteSelected() const = 0;
    virtual void RequestDeleteSelected() = 0;
    virtual void CancelDelete() = 0;
    virtual void ConfirmDelete() = 0;
    /// @brief File that will be deleted. Deliberately a dedicated buffer, NOT
    ///        _triggerFileInfo: the launch flows overwrite that one, and a
    ///        delete must never act on anything but the file the user saw.
    virtual const char* GetDeleteRomFileName() const = 0;
    /// @brief Save file that goes with the game, empty when there is none. It is
    ///        deleted along with the game unless DeleteKeepsSave says it stays.
    ///        Valid while the delete confirmation is shown.
    virtual const char* GetDeleteSaveFileName() const = 0;
    /// @brief Whether deleting the game leaves its save alone, as for a rom an
    ///        emulator runs. Valid while the delete confirmation is shown.
    virtual bool DeleteKeepsSave() const = 0;
    /// @brief How many save slot files (.sav1 to .sav9) go with the game, on top of
    ///        the save GetDeleteSaveFileName names. Valid while the delete
    ///        confirmation is shown.
    virtual u32 GetDeleteExtraSaveCount() const = 0;
    virtual void GotoSettingsScreen() = 0;

    virtual void Update() = 0;

    /// @brief Records that the last list move was a big jump by L/R, so the
    ///        letter chip shows for those and not for a d-pad step.
    virtual void NotifyBigStepJump() = 0;
    /// @brief Returns whether a big jump has happened since the last call, and
    ///        clears the flag.
    virtual bool ConsumeBigStepJump() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const IIconRepository& GetIconRepository() const = 0;
    virtual const IBannerRepository& GetBannerRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;
    virtual IGameDataService* GetGameDataService() = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    /// @brief Returns where the launcher keeps the save file of a DS game.
    virtual SaveLocation GetSaveLocation() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    /// @brief DS Lite backlight level (0..3), or -1 when the user never
    ///        picked one (the firmware's level is left untouched then).
    virtual int GetBacklightLevel() const = 0;
    virtual void SetBacklightLevel(int level) = 0;
    /// @brief Whether this console has backlight levels to set (the DS Lite
    ///        does; the original DS, the DSi and the 3DS do not).
    virtual bool HasBacklightLevels() const = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
