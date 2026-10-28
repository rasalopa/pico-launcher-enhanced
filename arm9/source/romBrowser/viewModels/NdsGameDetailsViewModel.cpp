#include "common.h"
#include <memory>
#include <string.h>
#include "fat/ff.h"
#include "services/gamedata/IGameDataService.h"
#include "romBrowser/FileType/Nds/NdsRomHeader.h"
#include "romBrowser/SaveFileName.h"
#include "NdsGameDetailsViewModel.h"

NdsGameDetailsViewModel::NdsGameDetailsViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController)
    : _romFileInfo(romFileInfo), _romBrowserController(romBrowserController)
    , _savesFolder(romBrowserController->GetSaveLocation() == SaveLocation::SavesFolder)
{
    // the game data belongs to the main thread, so the slot is read here and not in the task
    const GameDataEntry* entry = _romBrowserController->GetGameDataService()->GetEntry(_romFileInfo.GetFileName());
    _saveSlot = entry ? entry->saveSlot : 0;
    _storedSaveSlot = _saveSlot;

    _loadTask = _romBrowserController->GetIoTaskQueue()->Enqueue([this] (const vu8& cancelRequested)
    {
        bool hasSaveSlots = NdsRomHeader::UsesCardSave(_romFileInfo.GetFastFileRef());

        // Which slots have a save already, so the sheet can say when a slot starts fresh.
        // With the saves folder on, a save still next to the game counts: the launch moves
        // it in. Relative paths, since the browser sits in the game's folder.
        // Heap-allocated so it doesn't live on the io task's stack.
        // "saves/<name>.savN", with the name alone at +6 for the save next to the game
        const u32 slotPathLength = 6 + 256;
        auto slotPath = std::make_unique<char[]>(slotPathLength);
        strcpy(slotPath.get(), "saves/");
        u16 existingSaveSlots = 0;
        for (u32 slot = 0; hasSaveSlots && slot <= IGameDataService::kMaxSaveSlot; slot++)
        {
            if (BuildSaveFileName(_romFileInfo.GetFileName(), slot, &slotPath[6], slotPathLength - 6) &&
                ((_savesFolder && f_stat(slotPath.get(), nullptr) == FR_OK) ||
                    f_stat(&slotPath[6], nullptr) == FR_OK))
            {
                existingSaveSlots |= 1 << slot;
            }
        }

        _hasSaveSlots = hasSaveSlots;
        _existingSaveSlots = existingSaveSlots;
        _isLoaded = true;
        return TaskResult<void>::Completed();
    });
}

bool NdsGameDetailsViewModel::GetSaveFileName(char* fileName, u32 fileNameLength) const
{
    u32 prefixLength = _savesFolder ? 6 : 0;
    if (fileNameLength <= prefixLength)
    {
        return false;
    }
    memcpy(fileName, "saves/", prefixLength);
    return BuildSaveFileName(_romFileInfo.GetFileName(), _saveSlot,
        &fileName[prefixLength], fileNameLength - prefixLength);
}

void NdsGameDetailsViewModel::NextSaveSlot()
{
    if (_isLoaded && _hasSaveSlots)
    {
        _saveSlot = _saveSlot == IGameDataService::kMaxSaveSlot ? 0 : _saveSlot + 1;
    }
}

void NdsGameDetailsViewModel::PreviousSaveSlot()
{
    if (_isLoaded && _hasSaveSlots)
    {
        _saveSlot = _saveSlot == 0 ? IGameDataService::kMaxSaveSlot : _saveSlot - 1;
    }
}

void NdsGameDetailsViewModel::ShowCheats()
{
    StoreSaveSlot();
    _romBrowserController->ShowCheats();
}

void NdsGameDetailsViewModel::Close()
{
    StoreSaveSlot();
    _romBrowserController->HideGameInfo();
}

void NdsGameDetailsViewModel::StoreSaveSlot()
{
    // written once when the sheet closes, not on every press, since each write rewrites
    // the whole game data file
    if (_saveSlot == _storedSaveSlot)
    {
        return;
    }

    _storedSaveSlot = _saveSlot;
    _romBrowserController->SetSaveSlot(_romFileInfo, _saveSlot);
}
