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
    bool CanDeleteSelected() const override;
    void RequestDeleteSelected() override;
    void CancelDelete() override;
    void ConfirmDelete() override;
    const char* GetDeleteRomFileName() const override { return _deleteRomFileName; }
    const char* GetDeleteSaveFileName() const override { return _deleteHasSave ? _deleteSaveFileName : ""; }
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

    const char* GetLauncher() const override
    {
        return _appSettingsService->GetAppSettings().launcher.GetString();
    }

    void SetLauncher(const char* launcher) override;
    const char* GetLanguage() const override
    {
        return _appSettingsService->GetAppSettings().language.GetString();
    }
    void SetLanguage(const char* language) override;

    const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const override
    {
        return _appSettingsService->GetAppSettings().romBrowserDisplaySettings;
    }

    virtual const FileInfo& GetTriggerFileInfo() const override { return _triggerFileInfo; }

private:
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
    void BuildCurrentFolderFilePath(const char* fileName,
        TCHAR* buffer, u32 bufferLength) const;
    TCHAR _deleteRomFileName[256];
    TCHAR _deleteSaveFileName[256];
    bool _deleteHasSave = false;
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
    bool _saveSettingsPending = false;
    std::unique_ptr<CoverRepository> _coverRepository;
    std::unique_ptr<IconRepository> _iconRepository;
    std::unique_ptr<BannerRepository> _bannerRepository;
    ExtensionFileTypeProvider _fileTypeProvider;
    std::unique_ptr<ICheatRepository> _cheatRepository;

    void HandleTrigger();
    void HandleNavigateTrigger();
    void HandleFolderLoadDoneTrigger();
    void BackfillFavoritePaths();
    void HandleLaunchTrigger();
    void HandleChangeDisplayModeTrigger();
    void HandleGotoSettingsScreenTrigger();
    void UpdateLastUsedFilepath();
    void SetPicoLoaderParams() const;
    void LoadCheats() const;
};
