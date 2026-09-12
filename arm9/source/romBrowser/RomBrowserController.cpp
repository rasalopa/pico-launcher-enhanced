#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "services/gamedata/IGameDataService.h"
#include "bgm/IBgmService.h"
#include "core/mini-printf.h"
#include "core/Environment.h"
#include "fat/File.h"
#include "rtcIpc.h"
#include "backlightIpc.h"
#include "RomBrowserController.h"
#include "services/settings/Localization.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, IGameDataService* gameDataService,
    IBgmService* bgmService, TaskQueueBase* ioTaskQueue, TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _gameDataService(gameDataService)
    , _bgmService(bgmService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings())
{
    // restore the user's backlight level at boot (harmless to re-apply when
    // this process is re-entered from the settings screen)
    int backlightLevel = appSettingsService->GetAppSettings().backlightLevel;
    if (backlightLevel >= 0)
        backlight_setLevel(backlightLevel);
}

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo, const char* gameCode)
{
    _triggerFileInfo = FileInfo(fileInfo);
    StringUtil::Copy(_triggerGameCode, gameCode ? gameCode : "",
        sizeof(_triggerGameCode) / sizeof(_triggerGameCode[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::LaunchRandomGame()
{
    if (!_romBrowserViewModel.IsValid())
        return;
    auto& fileInfoManager = _romBrowserViewModel->GetFileInfoManager();
    u32 gameCount = fileInfoManager.GetGameCount();
    if (gameCount == 0)
        return;
    u32 pick = gRandomGenerator->NextU32(gameCount);
    for (u32 i = 0; i < fileInfoManager.GetItemCount(); i++)
    {
        const auto& item = fileInfoManager.GetItem(i);
        if (item.GetFileType()->GetClassification() != FileTypeClassification::Game)
            continue;
        if (pick == 0)
        {
            // an off-screen random pick usually has no file info loaded yet, so
            // the code is simply not recorded; it is metadata, and the entry is
            // keyed by file name either way
            const char* gameCode = nullptr;
            if (fileInfoManager.IsFileInfoLoaded(i))
            {
                const auto* info = fileInfoManager.GetInternalFileInfo(i);
                if (info)
                    gameCode = info->GetGameCode();
            }
            LaunchFile(item, gameCode);
            return;
        }
        pick--;
    }
}

void RomBrowserController::BuildCurrentFolderFilePath(const char* fileName,
    TCHAR* buffer, u32 bufferLength) const
{
    buffer[0] = 0;
    f_getcwd(buffer, bufferLength);
    int idx = strlcat(buffer, "/", bufferLength);
    // collapse a "//" at the root; guard idx>=2 in case f_getcwd left the
    // buffer empty (then idx==1 and buffer[idx-2] would read out of bounds)
    if (idx >= 2 && buffer[idx - 2] == '/')
    {
        buffer[idx - 1] = 0;
    }
    strlcat(buffer, fileName, bufferLength);
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo, const char* gameCode)
{
    // the path makes the entry navigable from the favorites panel even for
    // games that were marked but never launched
    TCHAR fullPath[256];
    BuildCurrentFolderFilePath(fileInfo.GetFileName(), fullPath,
        sizeof(fullPath) / sizeof(fullPath[0]));
    _gameDataService->ToggleFavorite(fileInfo.GetFileName(), gameCode, fullPath);
    _gameDataService->SaveAsync(_ioTaskQueue);
    if (_favoritesFilter)
    {
        // an unfavorited game must drop out of the filtered view
        _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
    }
}

void RomBrowserController::ToggleCompleted(const FileInfo& fileInfo, const char* gameCode)
{
    TCHAR fullPath[256];
    BuildCurrentFolderFilePath(fileInfo.GetFileName(), fullPath,
        sizeof(fullPath) / sizeof(fullPath[0]));
    _gameDataService->ToggleCompleted(fileInfo.GetFileName(), gameCode, fullPath);
    _gameDataService->SaveAsync(_ioTaskQueue);
    if (_completedFilter)
    {
        // a game unmarked as completed must drop out of the filtered view
        _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
    }
}

void RomBrowserController::ToggleFavoritesFilter()
{
    _favoritesFilter = !_favoritesFilter;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::ToggleCompletedFilter()
{
    _completedFilter = !_completedFilter;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowDisplaySettings()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowDisplaySettings);
}

void RomBrowserController::ShowRecents()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowRecents);
}

void RomBrowserController::ShowFavorites()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowFavorites);
}

void RomBrowserController::HideFavorites()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideFavorites);
}

void RomBrowserController::HideRecents()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideRecents);
}

void RomBrowserController::ShowStatistics()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowStatistics);
}

// only games: deleting folders would need recursion, and deleting random
// support files from the launcher is asking for trouble. The app bar asks the
// same question to decide whether to dim its delete button, so the button and
// the action can never disagree about what is deletable.
bool RomBrowserController::CanDeleteSelected() const
{
    if (!_romBrowserViewModel.IsValid())
        return false;
    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    if (selectedItem < 0)
        return false;
    const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);
    return item.GetFileType()->GetClassification() == FileTypeClassification::Game;
}

void RomBrowserController::RequestDeleteSelected()
{
    if (!CanDeleteSelected())
        return;
    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);

    StringUtil::Copy(_deleteRomFileName, item.GetFileName(),
        sizeof(_deleteRomFileName) / sizeof(_deleteRomFileName[0]));
    // "<name minus extension>.sav" is the save convention used by the loader
    // and the emulators next to their roms
    StringUtil::Copy(_deleteSaveFileName, _deleteRomFileName,
        sizeof(_deleteSaveFileName) / sizeof(_deleteSaveFileName[0]));
    TCHAR* dot = strrchr(_deleteSaveFileName, '.');
    if (dot)
        *dot = 0;
    strlcat(_deleteSaveFileName, ".sav", sizeof(_deleteSaveFileName));
    // Existence check drives only the confirm dialog's wording; the actual
    // deletion in ConfirmDelete is unconditional, so a wrong answer here
    // never leaves the save behind.
    FILINFO fileInfo;
    _deleteHasSave = f_stat(_deleteSaveFileName, &fileInfo) == FR_OK;

    _stateMachine.Fire(RomBrowserStateTrigger::ShowDeleteConfirm);
}

void RomBrowserController::FormatNowDateTime(TCHAR* buffer, u32 bufferLength) const
{
    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);
    // the rtc registers hold BCD values, which %x renders as decimal digits
    mini_snprintf(buffer, bufferLength, "20%02x-%02x-%02x %02x:%02x",
        dateTime.date.year, dateTime.date.month, dateTime.date.monthDay,
        dateTime.time.hour, dateTime.time.minute);
}

void RomBrowserController::CancelDelete()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideDeleteConfirm);
}

void RomBrowserController::ConfirmDelete()
{
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        FRESULT result = f_unlink(_deleteRomFileName);
        if (result != FR_OK)
        {
            LOG_ERROR("Couldn't delete file (%d)\n", result);
        }
        else
        {
            // delete the save unconditionally: its name is derived from the
            // rom and f_unlink harmlessly returns FR_NO_FILE when there is
            // none. Do NOT gate on a main-thread existence check — SD access
            // from that thread is unreliable and used to skip this.
            f_unlink(_deleteSaveFileName);
        }
        _deleteCompleted = true;
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HideStatistics()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideStatistics);
}

void RomBrowserController::HideDisplaySettings()
{
    if (_saveSettingsPending)
    {
        _saveSettingsPending = false;
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
    _stateMachine.Fire(RomBrowserStateTrigger::HideDisplaySettings);
}

void RomBrowserController::GotoSettingsScreen()
{
    _stateMachine.Fire(RomBrowserStateTrigger::GotoSettingsScreen);
}

void RomBrowserController::SetRomBrowserDisplaySettings(
    const RomBrowserDisplaySettings& romBrowserDisplaySettings)
{
    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::SetBacklightLevel(int level)
{
    if (level < 0 || level > 3)
        return;
    auto& appSettings = _appSettingsService->GetAppSettings();
    if (appSettings.backlightLevel != level)
    {
        appSettings.backlightLevel = (s8)level;
        _saveSettingsPending = true;
    }
    // apply immediately; no browser rebuild is needed for this
    backlight_setLevel(level);
}

void RomBrowserController::SetLauncher(const char* launcher)
{
    if (!launcher || strcasecmp(launcher, "bootstrap") != 0)
        _appSettingsService->GetAppSettings().launcher = "pico";
    else
        _appSettingsService->GetAppSettings().launcher = "bootstrap";

    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        _appSettingsService->Save();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::SetLanguage(const char* language)
{
    if (!language || !*language)
        return;

    _appSettingsService->GetAppSettings().language = language;
    Localization::SetLanguage(language);

    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        _appSettingsService->Save();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::Update()
{
    if (_deleteCompleted)
    {
        _deleteCompleted = false;
        // the deleted game's favorite/stats entry goes with it
        _gameDataService->RemoveEntry(_deleteRomFileName);
        _gameDataService->SaveAsync(_ioTaskQueue);
        // reload the current folder so the deleted file disappears
        NavigateToPath(".");
    }
    _stateMachine.Update();
    if (_stateMachine.HasStateChanged())
    {
        HandleTrigger();
    }
    switch (_stateMachine.GetCurrentState())
    {
        case RomBrowserState::Start:
        {
            LOG_DEBUG("RomBrowserState::Start\n");
            // a launcher boot ends the play session the last launch opened
            TCHAR now[20];
            FormatNowDateTime(now, sizeof(now) / sizeof(now[0]));
            if (_gameDataService->CloseOpenSession(now))
            {
                _gameDataService->SaveAsync(_ioTaskQueue);
            }
            const auto& lastUsed = _appSettingsService->GetAppSettings().lastUsedFilePath;
            if (strlen(lastUsed.GetString()) != 0)
            {
                NavigateToPath(lastUsed.GetString());
            }
            else
            {
                NavigateToPath("/");
            }
            break;
        }
        case RomBrowserState::LoadingFolder:
        {
            if (_navigateTask.GetTask().IsCompletedSuccessfully())
            {
                _navigateTask.Dispose();
                _stateMachine.Fire(RomBrowserStateTrigger::FolderLoadDone);
            }
            break;
        }
        case RomBrowserState::Launching:
        default:
        {
            break;
        }
    }
}

void RomBrowserController::HandleTrigger()
{
    switch (_stateMachine.GetLastTrigger())
    {
        case RomBrowserStateTrigger::Navigate:
            HandleNavigateTrigger();
            break;

        case RomBrowserStateTrigger::FolderLoadDone:
            HandleFolderLoadDoneTrigger();
            break;

        case RomBrowserStateTrigger::Launch:
            HandleLaunchTrigger();
            break;

        case RomBrowserStateTrigger::ChangeDisplayMode:
            HandleChangeDisplayModeTrigger();
            break;

        case RomBrowserStateTrigger::GotoSettingsScreen:
            HandleGotoSettingsScreenTrigger();
            break;

        default:
            break;
    }
}

void RomBrowserController::HandleNavigateTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Navigate\n");
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_iconRepository)
        {
            _iconRepository = std::make_unique<IconRepository>();
            _iconRepository->Initialize();
        }
        if (!_bannerRepository)
        {
            _bannerRepository = std::make_unique<BannerRepository>();
            _bannerRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }

        u64 startTick = gTickCounter.GetValue();
        _navigateFileName = nullptr;
        // Going up: land on the folder just left instead of the first entry, so
        // stepping out of a folder does not lose the user's place. The current
        // directory is read here, on the IO thread, because that is the thread
        // that owns the cwd - f_chdir below runs here too, and reading the card
        // from the main thread is unreliable.
        if (strcmp(_navigatePath, "..") == 0)
        {
            TCHAR currentPath[256];
            if (f_getcwd(currentPath, sizeof(currentPath) / sizeof(currentPath[0])) == FR_OK)
            {
                // "fat:/Games/nds" -> "nds"; at the root the separator is the
                // last character, so there is nothing to preselect
                const TCHAR* folderName = strrchr(currentPath, '/');
                if (folderName && folderName[1] != 0)
                {
                    StringUtil::Copy(_navigateSelectName, folderName + 1,
                        sizeof(_navigateSelectName) / sizeof(_navigateSelectName[0]));
                    _navigateFileName = _navigateSelectName;
                }
            }
        }
        if (strcmp(_navigatePath, "/") != 0) // can't f_stat on root dir
        {
            FILINFO fileInfo;
            if (f_stat(_navigatePath, &fileInfo) != FR_OK)
            {
                StringUtil::Copy(_navigatePath, "/", sizeof(_navigatePath) / sizeof(_navigatePath[0]));
            }
            else if (!(fileInfo.fattrib & AM_DIR))
            {
                _navigateFileName = strrchr(_navigatePath, '/') + 1;
                _navigateFileName[-1] = 0;
            }
        }
        f_chdir(_navigatePath);
        SdFolderFactory sdFolderFactory { &_fileTypeProvider };
        _newSdFolder = sdFolderFactory.CreateFromPath(".");
        if (_newSdFolder)
        {
            // Probed unconditionally, not gated on the current "hide empty
            // folders" setting: the cached bit must already be correct
            // whenever the toggle is flipped later (main thread, no I/O -
            // see HandleChangeDisplayModeTrigger), including while the user
            // is already inside the folder. cwd is still _navigatePath here,
            // so bare relative names resolve without building full paths.
            // readBudget is declared once here and shared across every
            // sibling folder below (HasVisibleContent decrements it, never
            // resets it) - otherwise each sibling would get its own fresh
            // allowance and a folder full of large, mostly-empty siblings
            // (photos, DSi system data, ...) could cost their sum instead of
            // a single bounded worst case for the whole navigation.
            int readBudget = SdFolderFactory::kInitialReadBudget;
            FileInfo* const* files = _newSdFolder->GetFiles();
            for (int i = 0; i < _newSdFolder->GetFileCount(); i++)
            {
                FileInfo* file = files[i];
                if (file->GetFileType()->GetClassification() == FileTypeClassification::Folder)
                {
                    // Entries the listing always drops are not worth a single
                    // read: FilterAndSort discards dot-named and hidden ones
                    // unconditionally. On a card that has been in a Mac this is
                    // most of the cost (.Spotlight-V100 alone is a deep tree of
                    // files nobody ever sees) and it changes between sessions,
                    // which made the whole feature look random.
                    if (file->GetFileName()[0] == '.' || file->IsHidden())
                        continue;
                    // Deliberately NOT filter-aware. Emptiness means "has no
                    // content at all", the same answer whatever the favorites or
                    // completed filter is doing, for two reasons: a filter-aware
                    // probe cannot stop at the first game it finds, so a folder
                    // of 200 unmarked roms cost 200 reads and ate the whole
                    // budget (every folder after it then failed open and came
                    // back into view); and the cached answer would go stale the
                    // moment a filter is toggled, since that only rebuilds the
                    // view model - it does not reload the folder. The trade is
                    // that with a filter on, a listed folder may turn out to
                    // hold nothing that matches.
                    file->SetEmptyFolder(!sdFolderFactory.HasVisibleContent(
                        file->GetFileName(), readBudget));
                }
            }
        }
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));

        // folders can bring their own music via a bgm.bcstm inside them
        FILINFO bgmFileInfo;
        if (f_stat("bgm.bcstm", &bgmFileInfo) == FR_OK && !(bgmFileInfo.fattrib & AM_DIR))
        {
            TCHAR bgmPath[256];
            f_getcwd(bgmPath, sizeof(bgmPath) / sizeof(bgmPath[0]));
            int idx = strlcat(bgmPath, "/", sizeof(bgmPath));
            if (bgmPath[idx - 2] == '/')
            {
                bgmPath[idx - 1] = 0;
            }
            strlcat(bgmPath, "bgm.bcstm", sizeof(bgmPath));
            _bgmService->UpdateBgmForFolder(bgmPath);
        }
        else
        {
            _bgmService->UpdateBgmForFolder(nullptr);
        }
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
    BackfillFavoritePaths();
}

// Favorites/completed marks made before path recording existed have no
// stored path, so the panels can't navigate to them. The current folder is
// loaded and is the cwd here (main thread, after the navigate IO task), so
// fill in the path of any flagged game found in it — the marks self-heal as
// the user browses, with no card-wide scan.
void RomBrowserController::BackfillFavoritePaths()
{
    // cheap O(entries) gate: once every mark is navigable, skip the
    // O(files x entries) folder scan entirely (steady state on a big library)
    if (!_sdFolder || !_gameDataService->HasUnpathedFlaggedEntry())
        return;
    bool changed = false;
    TCHAR fullPath[256];
    const FileInfo* const* files = _sdFolder->GetFiles();
    for (int i = 0; i < _sdFolder->GetFileCount(); i++)
    {
        const FileInfo* file = files[i];
        if (file->GetFileType()->GetClassification() != FileTypeClassification::Game)
            continue;
        BuildCurrentFolderFilePath(file->GetFileName(), fullPath,
            sizeof(fullPath) / sizeof(fullPath[0]));
        if (_gameDataService->BackfillPath(file->GetFileName(), fullPath))
            changed = true;
    }
    if (changed)
        _gameDataService->SaveAsync(_ioTaskQueue);
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    char lastPlayed[20];
    FormatNowDateTime(lastPlayed, sizeof(lastPlayed));
    // full path into a local buffer: _navigatePath belongs to the navigation
    // flow (same construction the favorite/completed toggles use)
    TCHAR fullPath[256];
    BuildCurrentFolderFilePath(_triggerFileInfo.GetFileName(), fullPath,
        sizeof(fullPath) / sizeof(fullPath[0]));
    _gameDataService->RecordLaunch(_triggerFileInfo.GetFileName(),
        _triggerGameCode[0] != 0 ? _triggerGameCode : nullptr, fullPath, lastPlayed);
    _gameDataService->SaveAsync(_ioTaskQueue);
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        UpdateLastUsedFilepath();
        SetPicoLoaderParams();
        LoadCheats();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this);
}

void RomBrowserController::HandleGotoSettingsScreenTrigger()
{
    gProcessManager.Goto<SettingsProcess>();
}

void RomBrowserController::UpdateLastUsedFilepath()
{
    f_getcwd(_navigatePath, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    int idx = strlcat(_navigatePath, "/", sizeof(_navigatePath));
    if (_navigatePath[idx - 2] == '/')
    {
        _navigatePath[idx - 1] = 0;
    }
    strlcat(_navigatePath, _triggerFileInfo.GetFileName(), sizeof(_navigatePath));
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
    _appSettingsService->Save();
}

static const char* BootstrapDrivePrefix()
{
    const char* launcherPath = pload_getLauncherPath();
    if (launcherPath && launcherPath[0] == 's' && launcherPath[1] == 'd' && launcherPath[2] == ':')
        return "sd:";
    return "fat:";
}

static void BootstrapMakeDrivePath(char* output, u32 outputSize, const char* drive, const char* path)
{
    if (path && path[0] == 'f' && path[1] == 'a' && path[2] == 't' && path[3] == ':')
    {
        StringUtil::Copy(output, path, outputSize);
    }
    else if (path && path[0] == 's' && path[1] == 'd' && path[2] == ':')
    {
        StringUtil::Copy(output, path, outputSize);
    }
    else
    {
        mini_snprintf(output, outputSize, "%s%s", drive, path ? path : "");
    }
}

static bool FindNdsBootstrapPath(char* output, u32 outputSize)
{
    // On DSpico/DS Lite the DLDI-mounted FAT filesystem is the active drive.
    // The release bootstrap is kept at this fixed path. Avoid f_stat(): on
    // real hardware it can fail before the config is written even though the
    // file is present and can be launched directly.
    mini_snprintf(output, outputSize, "/_nds/nds-bootstrap-release.nds");
    return true;
}

static bool WriteNdsBootstrapConfig(const char* romPath, const char* appLanguage)
{
    const char* drive = BootstrapDrivePrefix();
    char bootstrapPath[256];
    char configPath[256];
    char romDrivePath[256];
    char saveDrivePath[256];
    char savePath[256];
    const char* launcherPath = pload_getLauncherPath();

    if (!FindNdsBootstrapPath(bootstrapPath, sizeof(bootstrapPath)))
        return false;

    // configPath is opened through FatFs, so it must use the mounted
    // filesystem path rather than nds-bootstrap's fat:/ or sd:/ notation.
    mini_snprintf(configPath, sizeof(configPath), "/_nds/nds-bootstrap.ini");
    BootstrapMakeDrivePath(romDrivePath, sizeof(romDrivePath), drive, romPath);

    StringUtil::Copy(savePath, romPath, sizeof(savePath));
    char* dot = strrchr(savePath, '.');
    if (dot)
        *dot = 0;
    strlcat(savePath, ".sav", sizeof(savePath));
    BootstrapMakeDrivePath(saveDrivePath, sizeof(saveDrivePath), drive, savePath);

    char quitPath[256];
    BootstrapMakeDrivePath(quitPath, sizeof(quitPath), drive, launcherPath ? launcherPath : "");

    const bool dsiMode = Environment::IsDsiMode();

    // nds-bootstrap uses separate settings for its own GUI language and the
    // language requested by the game. On 2DS/3DS SD, LANGUAGE=-1 may not
    // resolve the console language when launched through our frontend.
    // Map Pico Launcher's language setting to nds-bootstrap's game-language
    // values so the selected language is passed explicitly.
    int bootstrapLanguage = -1;
    const char* guiLanguage = "en";
    if (!strcasecmp(appLanguage, "spanish"))
    {
        bootstrapLanguage = 5;
        guiLanguage = "es";
    }
    else if (!strcasecmp(appLanguage, "french"))
    {
        bootstrapLanguage = 2;
        guiLanguage = "fr";
    }
    else if (!strcasecmp(appLanguage, "german"))
    {
        bootstrapLanguage = 3;
        guiLanguage = "de";
    }
    else if (!strcasecmp(appLanguage, "italian"))
    {
        bootstrapLanguage = 4;
        guiLanguage = "it";
    }
    else if (!strcasecmp(appLanguage, "english"))
    {
        bootstrapLanguage = 1;
        guiLanguage = "en";
    }
    else if (!strcasecmp(appLanguage, "portuguese"))
    {
        // nds-bootstrap's LANGUAGE field does not have a Portuguese value in
        // this configuration, so keep automatic selection rather than
        // incorrectly mapping Portuguese to another language.
        guiLanguage = "pt-BR";
    }

    char config[1536];
    mini_snprintf(config, sizeof(config),
        "[NDS-BOOTSTRAP]\n"
        "NDS_PATH = %s\n"
        "SAV_PATH = %s\n"
        "QUIT_PATH = %s\n"
        "BOOST_CPU = 0\n"
        "BOOST_VRAM = 0\n"
        "CARD_READ_DMA = 1\n"
        "ASYNC_CARD_READ = 0\n"
        "LANGUAGE = %d\n"
        "REGION = -1\n"
        "USE_ROM_REGION = 1\n"
        "DSI_MODE = %d\n"
        "B4DS_MODE = %d\n"
        "GUI_LANGUAGE = %s\n"
        "HOTKEY = 284\n"
        "LOGGING = 1\n",
        romDrivePath, saveDrivePath, quitPath, bootstrapLanguage,
        dsiMode ? 1 : 0, dsiMode ? 0 : 1, guiLanguage);

    File file;
    if (file.Open(configPath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open nds-bootstrap.ini for writing\n");
        return false;
    }

    u32 bytesWritten = 0;
    const u32 configLength = (u32)strlen(config);
    if (file.Write(config, configLength, bytesWritten) != FR_OK || bytesWritten != configLength)
    {
        LOG_ERROR("Couldn't write nds-bootstrap.ini\n");
        return false;
    }
    return true;
}

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;

    const auto& appSettings = _appSettingsService->GetAppSettings();
    const bool useBootstrap = !strcasecmp(appSettings.launcher.GetString(), "bootstrap")
        && _triggerFileInfo.GetFileType()->GetShortName()
        && !strcasecmp(_triggerFileInfo.GetFileType()->GetShortName(), "nds");

    if (useBootstrap)
    {
        if (WriteNdsBootstrapConfig(_navigatePath, _appSettingsService->GetAppSettings().language.GetString()))
        {
            FindNdsBootstrapPath(loadParams->romPath, sizeof(loadParams->romPath));
            gProcessManager.Goto<PicoLoaderProcess>();
            return;
        }
        LOG_ERROR("Bootstrap launch failed; falling back to Pico Loader.\n");
    }

    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

static u32 GetBootstrapCheatDataSize(const CheatEntry* cheatEntry)
{
    u32 size = 0;
    if (cheatEntry->IsCheatCategory())
    {
        u32 numberOfSubEntries = 0;
        auto subEntries = cheatEntry->GetSubEntries(numberOfSubEntries);
        for (u32 i = 0; i < numberOfSubEntries; i++)
        {
            size += GetBootstrapCheatDataSize(&subEntries[i]);
        }
    }
    else if (cheatEntry->GetIsCheatActive())
    {
        u32 cheatDataLength = 0;
        cheatEntry->GetCheatData(cheatDataLength);
        size += cheatDataLength;
    }
    return size;
}

static bool WriteBootstrapCheatData(const CheatEntry* cheatEntry, File& file)
{
    if (cheatEntry->IsCheatCategory())
    {
        u32 numberOfSubEntries = 0;
        auto subEntries = cheatEntry->GetSubEntries(numberOfSubEntries);
        for (u32 i = 0; i < numberOfSubEntries; i++)
        {
            if (!WriteBootstrapCheatData(&subEntries[i], file))
                return false;
        }
    }
    else if (cheatEntry->GetIsCheatActive())
    {
        u32 cheatDataLength = 0;
        auto cheatData = cheatEntry->GetCheatData(cheatDataLength);

        if (!cheatData || cheatDataLength == 0)
            return true;

        u32 bytesWritten = 0;
        if (file.Write(cheatData, cheatDataLength, bytesWritten) != FR_OK ||
            bytesWritten != cheatDataLength)
        {
            LOG_ERROR("Couldn't write nds-bootstrap cheat data\n");
            return false;
        }
    }

    return true;
}

static bool WriteNdsBootstrapCheatData(const std::unique_ptr<GameCheats>& cheats)
{
    const char* cheatPath = "/_nds/nds-bootstrap/cheatData.bin";

    // Remove stale cheats first. Otherwise nds-bootstrap could reuse the
    // previous game's cheatData.bin when the current game has no cheats.
    f_unlink(cheatPath);

    if (!cheats)
        return true;

    const u32 cheatCodeBytes = GetBootstrapCheatDataSize(cheats.get());
    if (cheatCodeBytes == 0)
        return true;

    // nds-bootstrap expects raw Action Replay code words followed by
    // 0xCF000000. It disables cheats when the file is larger than 0x4000.
    if (cheatCodeBytes + sizeof(u32) > 0x4000)
    {
        LOG_ERROR("Cheat data is too large for nds-bootstrap: %u bytes\n", cheatCodeBytes);
        return false;
    }

    File file;
    if (file.Open(cheatPath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LOG_ERROR("Couldn't open nds-bootstrap cheatData.bin for writing\n");
        return false;
    }

    if (!WriteBootstrapCheatData(cheats.get(), file))
        return false;

    const u32 terminator = 0xCF000000;
    u32 bytesWritten = 0;
    if (file.Write(&terminator, sizeof(terminator), bytesWritten) != FR_OK ||
        bytesWritten != sizeof(terminator))
    {
        LOG_ERROR("Couldn't write nds-bootstrap cheatData.bin terminator\n");
        return false;
    }

    if (file.Sync() != FR_OK)
    {
        LOG_ERROR("Couldn't flush nds-bootstrap cheatData.bin\n");
        return false;
    }

    return true;
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());

    const auto& appSettings = _appSettingsService->GetAppSettings();
    const bool useBootstrap = !strcasecmp(appSettings.launcher.GetString(), "bootstrap")
        && _triggerFileInfo.GetFileType()->GetShortName()
        && !strcasecmp(_triggerFileInfo.GetFileType()->GetShortName(), "nds");

    if (useBootstrap)
    {
        // nds-bootstrap does not consume Pico Loader's in-memory cheat API.
        // It reads the selected Action Replay words from cheatData.bin.
        if (!WriteNdsBootstrapCheatData(cheats))
        {
            LOG_ERROR("Failed to prepare nds-bootstrap cheat data.\n");
        }

        // Prevent Pico Loader from receiving a stale cheat buffer.
        pload_setCheatData(nullptr);
        return;
    }

    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
