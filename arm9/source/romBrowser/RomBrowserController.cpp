#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "FileType/Nds/NdsRomHeader.h"
#include "FileType/Nds/NdsFileType.h"
#include "core/Environment.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "services/gamedata/IGameDataService.h"
#include "bgm/IBgmService.h"
#include "core/mini-printf.h"
#include "rtcIpc.h"
#include "backlightIpc.h"
#include "SaveFileName.h"
#include "SaveSlotIndex.h"
#include "RomBrowserController.h"

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
    if (_triggerFileInfo.GetFileType()->UsesLoaderSave() &&
        !IsBlocked(ParentalFeature::SaveSlots))
    {
        // the picker must list every save, and the ones noted when the folder was read
        // can miss some when its saves folder couldn't be read to the end
        if (_triggerFileInfo.AreSaveSlotsIncomplete())
            _triggerFileInfo.AddSaveSlots(FindSaveSlotsOnCard(_triggerFileInfo.GetFileName()));
        if (_triggerFileInfo.GetSaveSlots() & ~1u)
        {
            _stateMachine.Fire(RomBrowserStateTrigger::ShowSavePicker);
            return;
        }
    }
    LaunchSaveSlot(0);
}

void RomBrowserController::LaunchSaveSlot(u32 saveSlot)
{
    // Save sandboxing replaces whatever slot was picked, including the main save.
    // It is the one slot that still starts while the save-slot permission is off.
    bool saveSandboxing = _appSettingsService->GetAppSettings().parentalControls.UsesSaveSandboxing();
    u32 launchSaveSlot = saveSandboxing ? kSandboxSaveSlot : saveSlot;
    if (!saveSandboxing && launchSaveSlot != 0 && IsBlocked(ParentalFeature::SaveSlots))
        return;
    _launchSaveSlot = launchSaveSlot;
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::HideSavePicker()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideSavePicker);
}

bool RomBrowserController::SelectedHasSaveSlots() const
{
    if (IsBlocked(ParentalFeature::SaveSlots))
        return false;
    const FileInfo* game = GetSelectedGame();
    return game && game->GetFileType()->UsesLoaderSave();
}

u32 RomBrowserController::FindNewSaveSlotOfSelected() const
{
    if (!SelectedHasSaveSlots())
        return 0;
    const FileInfo* game = GetSelectedGame();
    u16 saveSlots = game->GetSaveSlots();
    // the free one must really be free, as with the save picker's "+ New save"
    if (game->AreSaveSlotsIncomplete())
        saveSlots |= FindSaveSlotsOnCard(game->GetFileName());
    for (u32 slot = 1; slot <= IGameDataService::kMaxSaveSlot; slot++)
    {
        if (!(saveSlots & (1 << slot)))
            return slot;
    }
    return 0;
}

void RomBrowserController::LaunchSelectedOnSaveSlot(u32 saveSlot)
{
    if (IsBlocked(ParentalFeature::SaveSlots))
        return;
    const FileInfo* game = GetSelectedGame();
    if (!game)
        return;
    _triggerFileInfo = FileInfo(*game);
    const char* gameCode = FindGameCode(_romBrowserViewModel->GetSelectedItem());
    StringUtil::Copy(_triggerGameCode, gameCode ? gameCode : "",
        sizeof(_triggerGameCode) / sizeof(_triggerGameCode[0]));
    LaunchSaveSlot(saveSlot);
}

const FileInfo* RomBrowserController::GetSelectedGame() const
{
    if (!_romBrowserViewModel.IsValid())
        return nullptr;
    int selectedItem = _romBrowserViewModel->GetSelectedItem();
    if (selectedItem < 0)
        return nullptr;
    const auto& item = _romBrowserViewModel->GetFileInfoManager().GetItem(selectedItem);
    return item.GetFileType()->GetClassification() == FileTypeClassification::Game ? &item : nullptr;
}

const char* RomBrowserController::FindGameCode(int index) const
{
    auto& fileInfoManager = _romBrowserViewModel->GetFileInfoManager();
    if (!fileInfoManager.IsFileInfoLoaded(index))
        return nullptr;
    const auto* info = fileInfoManager.GetInternalFileInfo(index);
    return info ? info->GetGameCode() : nullptr;
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
            LaunchFile(item, FindGameCode(i));
            return;
        }
        pick--;
    }
}

FRESULT RomBrowserController::BuildCurrentFolderFilePath(const char* fileName,
    TCHAR* buffer, u32 bufferLength) const
{
    buffer[0] = 0;
    FRESULT folderResult = f_getcwd(buffer, bufferLength);
    int idx = strlcat(buffer, "/", bufferLength);
    // collapse a "//" at the root; guard idx>=2 in case f_getcwd left the
    // buffer empty (then idx==1 and buffer[idx-2] would read out of bounds)
    if (idx >= 2 && buffer[idx - 2] == '/')
    {
        buffer[idx - 1] = 0;
    }
    strlcat(buffer, fileName, bufferLength);
    return folderResult;
}

u16 RomBrowserController::FindSaveSlotsOnCard(const char* romFileName) const
{
    bool savesFolder = GetSaveLocation() == SaveLocation::SavesFolder;
    FILINFO fileInfo;
    // Only a plain "not there" counts as missing: after a read error the save may well be
    // there, and the save picker would then offer it as a new one.
    auto isOnCard = [&fileInfo] (const TCHAR* path)
    {
        FRESULT result = f_stat(path, &fileInfo);
        return result != FR_NO_FILE && result != FR_NO_PATH;
    };
    // "saves/<name>.savN", with the name alone at +6 for the save next to the game
    TCHAR slotPath[6 + 256];
    strcpy(slotPath, "saves/");
    u16 saveSlots = 0;
    for (u32 slot = 1; slot <= IGameDataService::kMaxSaveSlot; slot++)
    {
        if (BuildSaveFileName(romFileName, slot, &slotPath[6], sizeof(slotPath) - 6) &&
            ((savesFolder && isOnCard(slotPath)) || isOnCard(&slotPath[6])))
        {
            saveSlots |= 1 << slot;
        }
    }
    return saveSlots;
}

void RomBrowserController::ToggleFavorite(const FileInfo& fileInfo, const char* gameCode)
{
    if (IsBlocked(ParentalFeature::Badges))
        return;
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
    if (IsBlocked(ParentalFeature::Badges))
        return;
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
    // this sheet is the cheats sheet; parental controls can take it away
    if (IsBlocked(ParentalFeature::Cheats))
        return;
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
    // the menu entry is drawn faded from this, but RequestDeleteSelected and
    // ConfirmDelete ask it too, so no other route can delete while blocked
    if (IsBlocked(ParentalFeature::Delete))
        return false;
    return GetSelectedGame() != nullptr;
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
    // and by the gba emulators next to their roms
    StringUtil::Copy(_deleteSaveFileName, _deleteRomFileName,
        sizeof(_deleteSaveFileName) / sizeof(_deleteSaveFileName[0]));
    TCHAR* dot = strrchr(_deleteSaveFileName, '.');
    if (dot)
        *dot = 0;
    strlcat(_deleteSaveFileName, ".sav", sizeof(_deleteSaveFileName));
    // An emulator run through a file association may not follow that convention, and a
    // "<name>.sav" next to its rom may then be the save of a DS game with the same name,
    // so deleting such a rom keeps that save. The dialog says it stays.
    _deleteKeepsSave = !item.GetFileType()->HasKnownSave();
    // With the saves folder on, the loader looks in "saves/" first, so that is the save
    // that goes with the game when it is there. Only when the folder has none does the
    // save next to the game go, as before: with a folder save present, the one next to
    // it may belong to a game of another system with the same name.
    _deleteFolderSaveFileName[0] = 0;
    if (_appSettingsService->GetAppSettings().saveLocation == SaveLocation::SavesFolder &&
        item.GetFileType()->UsesLoaderSave() &&
        strlen(_deleteSaveFileName) + 6 < sizeof(_deleteFolderSaveFileName))
    {
        strcpy(_deleteFolderSaveFileName, "saves/");
        strcat(_deleteFolderSaveFileName, _deleteSaveFileName);
    }
    // Existence checks drive only the confirm dialog's wording; ConfirmDelete
    // deletes the save without checking it is there, so a wrong answer here
    // never leaves the save behind. At worst the dialog names the save next
    // to the game while the one in the folder is what goes.
    FILINFO fileInfo;
    _deleteSaveInFolder = _deleteFolderSaveFileName[0] != 0 &&
        f_stat(_deleteFolderSaveFileName, &fileInfo) == FR_OK;
    _deleteHasSave = _deleteSaveInFolder || f_stat(_deleteSaveFileName, &fileInfo) == FR_OK;
    // The slot saves hold the game's progress just like the regular one, and nothing else
    // would ever clean them up. Only a game the loader keeps the save of has slots: for an
    // emulator's rom, a "<name>.sav1" next to it would belong to a DS game.
    _deleteSaveSlots = !_deleteKeepsSave && item.GetFileType()->UsesLoaderSave();
    _deleteExtraSaveCount = 0;
    if (_deleteSaveSlots && !item.AreSaveSlotsIncomplete())
    {
        // noted when the folder was read, in either place, so counting them reads nothing
        _deleteExtraSaveCount = __builtin_popcount(item.GetSaveSlots() & ~1u);
    }
    else if (_deleteSaveSlots)
    {
        // The saves folder wasn't read to its end, so look for each one: the dialog
        // must not name fewer saves than ConfirmDelete deletes.
        _deleteExtraSaveCount = __builtin_popcount(FindSaveSlotsOnCard(_deleteRomFileName));
    }

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
    // the sheet can only be here because RequestDeleteSelected let it, but the
    // lock is checked again at the point the file actually goes
    if (IsBlocked(ParentalFeature::Delete))
        return;
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        FRESULT result = f_unlink(_deleteRomFileName);
        if (result != FR_OK)
        {
            LOG_ERROR("Couldn't delete file (%d)\n", result);
        }
        else if (!_deleteKeepsSave)
        {
            DeleteSave(_deleteFolderSaveFileName, _deleteSaveFileName);
            if (_deleteSaveSlots)
            {
                // Heap-allocated so it doesn't live on the io task's stack.
                // "saves/<name>.savN", with the name alone at +6 for the save next to the game
                const u32 slotPathLength = 6 + 256;
                auto slotPath = std::make_unique<TCHAR[]>(slotPathLength);
                strcpy(slotPath.get(), "saves/");
                for (u32 slot = 1; slot <= IGameDataService::kMaxSaveSlot; slot++)
                {
                    if (BuildSaveFileName(_deleteRomFileName, slot, &slotPath[6], slotPathLength - 6))
                    {
                        DeleteSave(_deleteFolderSaveFileName[0] != 0 ? slotPath.get() : "", &slotPath[6]);
                    }
                }
            }
        }
        _deleteCompleted = true;
        return TaskResult<void>::Completed();
    });
}

// Deletes the save without checking it is there: its name is derived from the rom and
// f_unlink harmlessly returns FR_NO_FILE when there is none. Do NOT gate on a main-thread
// existence check — SD access from that thread is unreliable and used to skip this. The
// saves folder goes first; only when it has nothing does the save next to the rom go, as
// before. Any other failure leaves both alone: the dialog named the folder save, so no
// other file may go.
void RomBrowserController::DeleteSave(const char* folderSaveFileName, const char* saveFileName)
{
    FRESULT saveResult = folderSaveFileName[0] == 0
        ? FR_NO_PATH
        : f_unlink(folderSaveFileName);
    if (saveResult == FR_NO_FILE || saveResult == FR_NO_PATH)
    {
        f_unlink(saveFileName);
    }
    else if (saveResult != FR_OK)
    {
        LOG_ERROR("Couldn't delete %s (%d)\n", folderSaveFileName, saveResult);
    }
}

void RomBrowserController::HideStatistics()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideStatistics);
}

void RomBrowserController::ShowMenu()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowMenu);
}

void RomBrowserController::HideMenu()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideMenu);
}

void RomBrowserController::ShowAbout()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowAbout);
}

void RomBrowserController::HideAbout()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideAbout);
}

void RomBrowserController::ShowParentalControls()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowParentalControls);
}

void RomBrowserController::HideParentalControls()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideParentalControls);
}

void RomBrowserController::SaveParentalControls()
{
    // Persist each change at once. Waiting for HideParentalControls lost the
    // PIN and switches when the sheet closed another way or the console was
    // powered off before a queued save could run.
    _appSettingsService->Save();
}

bool RomBrowserController::VerifyParentalPin(const char* pin) const
{
    const auto& parentalControls = _appSettingsService->GetAppSettings().parentalControls;
    return !parentalControls.HasPin() || parentalControls.MatchesPin(pin);
}

void RomBrowserController::SetRestrictedMode(bool enabled)
{
    auto& parentalControls = _appSettingsService->GetAppSettings().parentalControls;
    if (parentalControls.restrictedMode == enabled)
        return;
    parentalControls.restrictedMode = enabled;
    // Save sandboxing already locks save slots; enforce that even when a card
    // set the lock on while the old permission still said it was allowed.
    if (enabled && parentalControls.saveSandboxing)
        parentalControls.allowSaveSlots = false;
    SaveParentalControls();
}

void RomBrowserController::SetParentalPin(const char* pin)
{
    auto& parentalControls = _appSettingsService->GetAppSettings().parentalControls;
    if (strcmp(parentalControls.pin.GetString(), pin ? pin : "") == 0)
        return;
    parentalControls.pin = pin ? pin : "";
    SaveParentalControls();
}

void RomBrowserController::SetParentalFeatureAllowed(ParentalFeature feature, bool allowed)
{
    auto& parentalControls = _appSettingsService->GetAppSettings().parentalControls;
    if (feature == ParentalFeature::SaveSlots && allowed && parentalControls.saveSandboxing)
        return;
    if (parentalControls.IsAllowed(feature) == allowed)
        return;
    parentalControls.SetAllowed(feature, allowed);
    SaveParentalControls();
}

void RomBrowserController::SetParentalSaveSandboxing(bool enabled)
{
    auto& parentalControls = _appSettingsService->GetAppSettings().parentalControls;
    if (parentalControls.saveSandboxing == enabled)
        return;
    parentalControls.SetSaveSandboxing(enabled);
    SaveParentalControls();
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

bool RomBrowserController::HasBacklightLevels() const
{
    return backlight_hasLevels();
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
            // a launcher boot ends the play session the last launch opened;
            // with tracking switched off since, the session is dropped, not
            // credited
            bool sessionChanged;
            if (_appSettingsService->GetAppSettings().launchTracking)
            {
                TCHAR now[20];
                FormatNowDateTime(now, sizeof(now) / sizeof(now[0]));
                sessionChanged = _gameDataService->CloseOpenSession(now);
            }
            else
            {
                sessionChanged = _gameDataService->DiscardOpenSession();
            }
            if (sessionChanged)
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
        {
            if (_launchCheckTask.IsValid() && _launchCheckTask.GetTask().IsCompleted())
            {
                _launchCheckTask.Dispose();
                LaunchRefusal refusal = _launchRefusal;
                if (refusal == LaunchRefusal::None)
                {
                    BeginLaunch();
                }
                else
                {
                    if (refusal == LaunchRefusal::DsiOnly)
                    {
                        StringUtil::Copy(_launchRefusedText, "Needs a DSi or 3DS", sizeof(_launchRefusedText));
                    }
                    else if (refusal == LaunchRefusal::SaveNameTooLong)
                    {
                        // the picker calls it Save N, so the toast does too. 117 pixels of
                        // the toast's 128, which "File name too long" went past
                        mini_snprintf(_launchRefusedText, sizeof(_launchRefusedText),
                            "Name too long for Save %d", (int)_launchSaveSlot);
                    }
                    else if (refusal == LaunchRefusal::FolderPathTooLong)
                    {
                        StringUtil::Copy(_launchRefusedText, "Folder path too long", sizeof(_launchRefusedText));
                    }
                    else
                    {
                        StringUtil::Copy(_launchRefusedText, "Couldn't read the game", sizeof(_launchRefusedText));
                    }
                    _stateMachine.Fire(RomBrowserStateTrigger::LaunchRefused);
                }
            }
            break;
        }
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
            // The saves of each DS game are noted from names this navigation reads anyway:
            // the folder's own entries and, with the saves folder setting on, the saves
            // folder, which the probe below would otherwise read for its emptiness.
            bool savesFolder = _appSettingsService->GetAppSettings().saveLocation == SaveLocation::SavesFolder;
            SaveSlotIndex saveSlotIndex { *_newSdFolder };
            FileInfo* const* files = _newSdFolder->GetFiles();
            for (int i = 0; i < _newSdFolder->GetFileCount(); i++)
            {
                FileInfo* file = files[i];
                if (file->GetFileType()->GetClassification() != FileTypeClassification::Folder)
                {
                    saveSlotIndex.AddSaveFile(file->GetFileName());
                }
                else if (savesFolder && !strcasecmp(file->GetFileName(), "saves"))
                {
                    // The listing hides it in this mode, so its emptiness is never asked,
                    // while the loader uses it even when it is marked hidden.
                    saveSlotIndex.AddSavesFolder(file->GetFileName());
                }
                else
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
    // A launch that can't go as the player asked is turned away before anything about it
    // is recorded: on a DS or DS Lite a DSi-only game can only end in a white screen, a
    // save other than the main one that the loader can't be pointed at would have it open
    // the main save, and without the game's folder no save can be pointed at. All of it
    // needs the card, read on the io thread like every other file access; the browser sits
    // in the Launching state, with input off, until the answer is in.
    if (!IsDsiOnlyCheckNeeded() && !_triggerFileInfo.GetFileType()->UsesLoaderSave())
    {
        BeginLaunch();
        return;
    }
    _launchRefusal = LaunchRefusal::None;
    _launchCheckTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (IsDsiOnlyCheckNeeded() && NdsRomHeader::IsDsiOnly(_triggerFileInfo.GetFastFileRef()))
            _launchRefusal = LaunchRefusal::DsiOnly;
        else if (_triggerFileInfo.GetFileType()->UsesLoaderSave())
            _launchRefusal = ResolveSavePath();
        return TaskResult<void>::Completed();
    });
}

bool RomBrowserController::IsDsiOnlyCheckNeeded() const
{
    return !Environment::IsDsiMode() && _triggerFileInfo.GetFileType() == &NdsFileType::sInstance;
}

void RomBrowserController::BeginLaunch()
{
    // the save picker opens on the save started last time. it is written with launch
    // tracking off too, since the player picked it, but only when it changed
    bool gameDataChanged = false;
    if (_triggerFileInfo.GetFileType()->UsesLoaderSave())
    {
        const auto* entry = _gameDataService->GetEntry(_triggerFileInfo.GetFileName());
        // .sav10 is not one of the picker's slots, so it must not replace the
        // save remembered in gamedata.json. Turning save sandboxing off then
        // returns to the save the player was using before.
        if (_launchSaveSlot != kSandboxSaveSlot && (entry ? entry->saveSlot : 0) != _launchSaveSlot)
        {
            gameDataChanged = _gameDataService->SetSaveSlot(_triggerFileInfo.GetFileName(),
                _launchSaveSlot, _triggerGameCode[0] != 0 ? _triggerGameCode : nullptr);
        }
    }
    if (_appSettingsService->GetAppSettings().launchTracking)
    {
        char lastPlayed[20];
        FormatNowDateTime(lastPlayed, sizeof(lastPlayed));
        // full path into a local buffer: _navigatePath belongs to the navigation
        // flow (same construction the favorite/completed toggles use)
        TCHAR fullPath[256];
        BuildCurrentFolderFilePath(_triggerFileInfo.GetFileName(), fullPath,
            sizeof(fullPath) / sizeof(fullPath[0]));
        _gameDataService->RecordLaunch(_triggerFileInfo.GetFileName(),
            _triggerGameCode[0] != 0 ? _triggerGameCode : nullptr, fullPath, lastPlayed);
        gameDataChanged = true;
    }
    if (gameDataChanged)
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

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        if (_triggerFileInfo.GetFileType()->UsesLoaderSave())
        {
            // decided by the launch check, before the launch was recorded
            strcpy(loadParams->savePath, _launchSavePath);
        }
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

// Picks the save the loader uses: the slot in _launchSaveSlot, where
// saveLocation says. When the saves folder can't be used, the save stays next to the game
// under the same slot's name, so a slot never ends up on another slot's save. The main
// save can be left for the loader to name, as before there were slots. Any other save the
// loader can't be pointed at turns the launch away: it would open the main save. So does
// a game folder that can't be read, whichever save was picked.
RomBrowserController::LaunchRefusal RomBrowserController::ResolveSavePath()
{
    _launchSavePath[0] = 0;
    LaunchRefusal mainSaveInstead = _launchSaveSlot == 0 ? LaunchRefusal::None : LaunchRefusal::SaveNameTooLong;

    // Homebrew and DSiWare get no card save from the loader, so no folder or slot for them either.
    bool headerUnreadable = false;
    if (!NdsRomHeader::UsesCardSave(_triggerFileInfo.GetFastFileRef(), &headerUnreadable))
    {
        return headerUnreadable && _launchSaveSlot != 0 ? LaunchRefusal::RomUnreadable : LaunchRefusal::None;
    }

    // Heap-allocated so they don't live on the io task's stack.
    auto saveFileName = std::make_unique<char[]>(sizeof(_launchSavePath));
    if (!BuildSaveFileName(_triggerFileInfo.GetFileName(), _launchSaveSlot,
        saveFileName.get(), sizeof(_launchSavePath)))
    {
        LOG_ERROR("Save name for slot %d too long\n", _launchSaveSlot);
        return mainSaveInstead;
    }

    // "<game folder>/<file name>", built the same way UpdateLastUsedFilepath builds the
    // rom path the loader gets. That one reads the folder again, so if this read fails
    // the save would be pointed at the root while the game starts from its own folder:
    // the launch is turned away, whichever save was picked
    const u32 gamePathLength = sizeof(_navigatePath) / sizeof(_navigatePath[0]);
    auto gamePath = std::make_unique<TCHAR[]>(gamePathLength);
    const char* fileName = _triggerFileInfo.GetFileName();
    FRESULT folderResult = BuildCurrentFolderFilePath(fileName, gamePath.get(), gamePathLength);
    if (folderResult != FR_OK)
    {
        LOG_ERROR("Couldn't read the game's folder (%d)\n", folderResult);
        // FR_NOT_ENOUGH_CORE: the folder's own path is longer than the buffer, which a
        // shorter folder name fixes, not a card that failed to read
        return folderResult == FR_NOT_ENOUGH_CORE ? LaunchRefusal::FolderPathTooLong
            : LaunchRefusal::RomUnreadable;
    }
    u32 pathLength = strlen(gamePath.get());
    u32 fileNameLength = strlen(fileName);
    u32 folderLength = pathLength - fileNameLength;
    if (pathLength <= fileNameLength || gamePath[folderLength - 1] != '/' ||
        strcmp(&gamePath[folderLength], fileName) != 0)
    {
        LOG_ERROR("Unexpected launch path\n");
        return mainSaveInstead;
    }

    if (_appSettingsService->GetAppSettings().saveLocation == SaveLocation::SavesFolder &&
        SetSavesFolderPath(_launchSavePath, gamePath.get(), folderLength, saveFileName.get()))
    {
        return LaunchRefusal::None;
    }

    // Slot 0 next to the game is what the loader picks on its own, so the path stays empty
    // and the loader keeps naming it, exactly as before there were slots.
    _launchSavePath[0] = 0;
    if (_launchSaveSlot == 0)
    {
        return LaunchRefusal::None;
    }
    // "<game folder>/" + save name + terminator
    if (folderLength + strlen(saveFileName.get()) + 1 > sizeof(_launchSavePath))
    {
        LOG_ERROR("Save path for slot %d too long\n", _launchSaveSlot);
        return LaunchRefusal::SaveNameTooLong;
    }
    memcpy(_launchSavePath, gamePath.get(), folderLength);
    strcpy(&_launchSavePath[folderLength], saveFileName.get());
    return LaunchRefusal::None;
}

// Points the loader at "<game folder>/saves/<save name>", creating the saves folder.
// Returns false, so the save stays next to the game as the loader keeps it on its own,
// when the folder can't be made or the path would not fit: a truncated path would point
// the loader at another file, and a save must never end up somewhere unexpected.
bool RomBrowserController::SetSavesFolderPath(char* savePath, const TCHAR* gamePath,
    u32 folderLength, const char* saveFileName) const
{
    u32 saveFileNameLength = strlen(saveFileName);
    // "<game folder>/" + "saves/" + save name + terminator
    if (folderLength + 6 + saveFileNameLength + 1 > sizeof(_launchSavePath))
    {
        LOG_ERROR("Save path too long, keeping the save next to the game\n");
        return false;
    }

    memcpy(savePath, gamePath, folderLength);
    savePath[folderLength] = 0;
    strcat(savePath, "saves");

    // Create first and look only when something is already there: a folder that exists
    // must be recognised even if one read of the card fails, or the loader would start a
    // second save next to the game while the real one sits in the folder.
    FRESULT mkdirResult = f_mkdir(savePath);
    if (mkdirResult == FR_EXIST)
    {
        FILINFO folderInfo;
        if (f_stat(savePath, &folderInfo) != FR_OK || !(folderInfo.fattrib & AM_DIR))
        {
            LOG_ERROR("%s is not a usable folder, keeping the save next to the game\n", savePath);
            return false;
        }
    }
    else if (mkdirResult != FR_OK)
    {
        LOG_ERROR("Couldn't create %s (%d), keeping the save next to the game\n", savePath, mkdirResult);
        return false;
    }

    strcat(savePath, "/");
    strcat(savePath, saveFileName);

    // The save that is already there decides. One in the folder is used as it is. One still
    // next to the game is moved in: moved, not copied, so there is a single save to trust,
    // and nothing is ever deleted. If the move fails, the game keeps the save next to it.
    // Each slot's save goes through this on its own, the first time that slot is launched.
    FILINFO saveInfo;
    if (f_stat(savePath, &saveInfo) == FR_OK)
    {
        return true;
    }
    char oldPath[sizeof(_launchSavePath)];
    memcpy(oldPath, gamePath, folderLength);
    strcpy(&oldPath[folderLength], saveFileName);
    if (f_stat(oldPath, &saveInfo) != FR_OK)
    {
        return true; // a new game: the loader creates the save in the folder
    }
    FRESULT renameResult = f_rename(oldPath, savePath);
    if (renameResult == FR_EXIST)
    {
        return true; // the folder has a save after all: it wins, as above
    }
    if (renameResult != FR_OK)
    {
        LOG_ERROR("Couldn't move %s into the saves folder (%d), keeping it next to the game\n",
            oldPath, renameResult);
        return false;
    }
    return true;
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
