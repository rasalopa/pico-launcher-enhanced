#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "SdFolder.h"
#include "viewModels/RomBrowserViewModel.h"
#include "RomBrowserStateMachine.h"
#include "core/task/TaskQueue.h"
#include "IRomBrowserController.h"
#include "CoverRepository.h"
#include "IconRepository.h"
#include "BannerRepository.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/ICheatRepository.h"

class RomBrowserController : public IRomBrowserController
{
public:
    RomBrowserController(IAppSettingsService* appSettingsService,
        IGameDataService* gameDataService, IBgmService* bgmService,
        TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue);

    void NavigateUp() override
    {
        NavigateToPath("..");
    }

    void NavigateToPath(const TCHAR* name) override;
    void LaunchFile(const FileInfo& fileInfo, const char* gameCode) override;
    void LaunchRandomGame() override;
    void LaunchSaveSlot(u32 saveSlot) override;
    void HideSavePicker() override;
    bool SelectedHasSaveSlots() const override;
    u32 FindNewSaveSlotOfSelected() const override;
    void LaunchSelectedOnSaveSlot(u32 saveSlot) override;
    void ToggleFavorite(const FileInfo& fileInfo, const char* gameCode) override;
    void ToggleCompleted(const FileInfo& fileInfo, const char* gameCode) override;
    void ToggleFavoritesFilter() override;
    bool IsFavoritesFilterEnabled() const override { return _favoritesFilter; }
    void ToggleCompletedFilter() override;
    bool IsCompletedFilterEnabled() const override { return _completedFilter; }
    void ShowGameInfo(const FileInfo& fileInfo) override;
    void HideGameInfo() override;
    void ShowDisplaySettings() override;
    void HideDisplaySettings() override;
    void ShowRecents() override;
    void HideRecents() override;
    void ShowFavorites() override;
    void HideFavorites() override;
    void ShowStatistics() override;
    void HideStatistics() override;
    void ShowMenu() override;
    void HideMenu() override;
    void ShowAbout() override;
    void HideAbout() override;
    void ShowParentalControls() override;
    void HideParentalControls() override;
    const ParentalControls& GetParentalControls() const override
    {
        return _appSettingsService->GetAppSettings().parentalControls;
    }
    bool VerifyParentalPin(const char* pin) const override;
    void SetParentalEnabled(bool enabled) override;
    void SetParentalPin(const char* pin) override;
    void SetParentalFeatureAllowed(ParentalFeature feature, bool allowed) override;
    void SetParentalKidSaves(bool enabled) override;
    bool IsBlocked(ParentalFeature feature) const override
    {
        return !_appSettingsService->GetAppSettings().parentalControls.Allows(feature);
    }
    bool CanDeleteSelected() const override;
    void RequestDeleteSelected() override;
    void CancelDelete() override;
    void ConfirmDelete() override;
    const char* GetDeleteRomFileName() const override { return _deleteRomFileName; }
    const char* GetDeleteSaveFileName() const override
    {
        return !_deleteHasSave ? "" : _deleteSaveInFolder ? _deleteFolderSaveFileName : _deleteSaveFileName;
    }
    bool DeleteKeepsSave() const override { return _deleteKeepsSave; }
    u32 GetDeleteExtraSaveCount() const override { return _deleteExtraSaveCount; }
    void GotoSettingsScreen() override;

    void Update() override;

    void NotifyBigStepJump() override { _bigStepJumpPending = true; }
    bool ConsumeBigStepJump() override
    {
        bool pending = _bigStepJumpPending;
        _bigStepJumpPending = false;
        return pending;
    }

    const SdFolder& GetSdFolder() const override { return *_sdFolder; }

    const RomBrowserStateMachine& GetStateMachine() const override { return _stateMachine; }

    const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() override { return _romBrowserViewModel; }

    TaskQueueBase* GetIoTaskQueue() const override { return _ioTaskQueue; }
    TaskQueueBase* GetBgTaskQueue() const override { return _bgTaskQueue; }
    const ICoverRepository& GetCoverRepository() const override { return *_coverRepository; }
    const IIconRepository& GetIconRepository() const override { return *_iconRepository; }
    const IBannerRepository& GetBannerRepository() const override { return *_bannerRepository; }
    const ICheatRepository& GetCheatRepository() const override { return *_cheatRepository; }
    IGameDataService* GetGameDataService() override { return _gameDataService; }

    void SetRomBrowserDisplaySettings(const RomBrowserDisplaySettings& romBrowserDisplaySettings) override;

    int GetBacklightLevel() const override
    {
        return _appSettingsService->GetAppSettings().backlightLevel;
    }

    void SetBacklightLevel(int level) override;
    bool HasBacklightLevels() const override;

    const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const override
    {
        return _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    }

    SaveLocation GetSaveLocation() const override
    {
        return _appSettingsService->GetAppSettings().saveLocation;
    }

    virtual const FileInfo& GetTriggerFileInfo() const override { return _triggerFileInfo; }

    /// @brief What the toast says when the last launch was turned away.
    const char* GetLaunchRefusedText() const { return _launchRefusedText; }

private:
    /// @brief Why the game about to launch is turned away.
    enum class LaunchRefusal : u8
    {
        None,
        /// @brief A DSi-only game on a DS or DS Lite.
        DsiOnly,
        /// @brief The path of the chosen save doesn't fit, so the loader would open the main save.
        SaveNameTooLong,
        /// @brief The rom header or its folder can't be read, so it isn't known where the
        ///        chosen save goes.
        RomUnreadable,
        /// @brief The game's folder path is too long to read back, so no save can be
        ///        pointed at.
        FolderPathTooLong
    };

    IAppSettingsService* _appSettingsService;
    IGameDataService* _gameDataService;
    IBgmService* _bgmService;
    TaskQueueBase* _ioTaskQueue;
    TaskQueueBase* _bgTaskQueue;
    bool _favoritesFilter = false;
    bool _completedFilter = false;
    bool _bigStepJumpPending = false;
    TCHAR _triggerGameCode[8];

    void FormatNowDateTime(TCHAR* buffer, u32 bufferLength) const;
    /// @brief Full path of a file in the current folder (f_getcwd + name).
    /// @return What f_getcwd returned. When it isn't FR_OK the buffer holds "/" + name.
    FRESULT BuildCurrentFolderFilePath(const char* fileName,
        TCHAR* buffer, u32 bufferLength) const;
    /// @brief Which saves of the rom in the current folder are on the card, bit N for
    ///        "<name>.savN", looking each one up. Slot 0 is not looked for.
    u16 FindSaveSlotsOnCard(const char* romFileName) const;
    TCHAR _deleteRomFileName[256];
    TCHAR _deleteSaveFileName[256];
    /// @brief "saves/<name>.sav", or empty when the saves folder setting does not apply.
    TCHAR _deleteFolderSaveFileName[256];
    bool _deleteHasSave = false;
    /// @brief Whether the save shown in the dialog is the one in the saves folder.
    bool _deleteSaveInFolder = false;
    /// @brief Whether the save stays when the game goes, see RequestDeleteSelected.
    bool _deleteKeepsSave = false;
    /// @brief Whether the slot saves (.sav1 to .sav9) go with the game. Only for games
    ///        the loader keeps the save of, since only those have slots.
    bool _deleteSaveSlots = false;
    /// @brief How many slot saves were found, for the dialog's wording only.
    u32 _deleteExtraSaveCount = 0;
    volatile bool _deleteCompleted = false;

    std::unique_ptr<SdFolder> _sdFolder;
    SharedPtr<RomBrowserViewModel> _romBrowserViewModel;
    std::unique_ptr<SdFolder> _newSdFolder;
    RomBrowserStateMachine _stateMachine;
    TCHAR _navigatePath[256];
    TCHAR* _navigateFileName;
    /// @brief Entry the next listing should land on, when it is not part of
    ///        _navigatePath. Used when going up a folder: the parent listing
    ///        preselects the folder just left instead of its first entry.
    TCHAR _navigateSelectName[256];
    FileInfo _triggerFileInfo;
    QueueTask<void> _navigateTask;
    /// @brief Finds out, on the io thread, whether the game about to launch can start, and
    ///        where its save goes.
    QueueTask<void> _launchCheckTask;
    /// @brief The answer, written by the task before it completes.
    volatile LaunchRefusal _launchRefusal = LaunchRefusal::None;
    /// @brief Save slot of the game being launched, set on the main thread before the
    ///        io task that builds the save path starts.
    u32 _launchSaveSlot = 0;
    /// @brief Save path for the loader, written by the launch check. Empty when the loader
    ///        names the save itself.
    char _launchSavePath[sizeof(pload_params_t::savePath)];
    char _launchRefusedText[40] = "";
    bool _saveSettingsPending = false;
    std::unique_ptr<CoverRepository> _coverRepository;
    std::unique_ptr<IconRepository> _iconRepository;
    std::unique_ptr<BannerRepository> _bannerRepository;
    ExtensionFileTypeProvider _fileTypeProvider;
    std::unique_ptr<ICheatRepository> _cheatRepository;

    /// @brief Whether the highlighted entry is a game at all: the question
    ///        delete and game info share, before parental controls are asked.
    bool IsSelectedGame() const;

    void HandleTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void BackfillFavoritePaths();
    void HandleLaunchTrigger();
    /// @brief The highlighted entry when it is a game, which is what the menu acts on.
    const FileInfo* GetSelectedGame() const;
    /// @brief The game code of an entry of the folder, when its file info is loaded.
    const char* FindGameCode(int index) const;
    /// @brief On a DS or DS Lite, an nds file may be a DSi-only game that can't run there.
    bool IsDsiOnlyCheckNeeded() const;
    /// @brief Records the launch and hands the game to the loader.
    void BeginLaunch();
    void HandleChangeDisplayModeTrigger();
    void HandleGotoSettingsScreenTrigger();
    void UpdateLastUsedFilepath();
    void SetPicoLoaderParams() const;
    /// @brief Writes the save path for the game being launched to _launchSavePath.
    LaunchRefusal ResolveSavePath();
    bool SetSavesFolderPath(char* savePath, const TCHAR* gamePath, u32 folderLength,
        const char* saveFileName) const;
    /// @brief Deletes one save of the game being deleted, the one in the saves folder first.
    static void DeleteSave(const char* folderSaveFileName, const char* saveFileName);
    void LoadCheats() const;
};
