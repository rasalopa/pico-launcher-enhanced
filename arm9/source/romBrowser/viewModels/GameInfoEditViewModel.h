#pragma once
#include "core/StringUtil.h"
#include "../FileInfo.h"
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"

/// @brief View model for the Game info sheet: the title, publisher and
///        developer stored for the game it opened on, and where the edited
///        values go.
class GameInfoEditViewModel
{
public:
    explicit GameInfoEditViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
    {
        const auto* entry = romBrowserController->GetGameDataService()->GetEntry(
            romBrowserController->GetTriggerFileInfo().GetFileName());
        if (entry)
        {
            StringUtil::Copy(_title, entry->title.GetString(), sizeof(_title));
            StringUtil::Copy(_publisher, entry->publisher.GetString(), sizeof(_publisher));
            StringUtil::Copy(_developer, entry->developer.GetString(), sizeof(_developer));
        }
    }

    const char* GetTitle() const { return _title; }
    const char* GetPublisher() const { return _publisher; }
    const char* GetDeveloper() const { return _developer; }

    /// @brief Stores the values and rebuilds the browser on the same game.
    void Save(const char* title, const char* publisher, const char* developer)
    {
        _romBrowserController->SetGameMetadata(title, publisher, developer);
    }

private:
    IRomBrowserController* _romBrowserController;
    char _title[GameDataEntry::kMaxTitleLength + 1] = {};
    char _publisher[GameDataEntry::kMaxPublisherLength + 1] = {};
    char _developer[GameDataEntry::kMaxDeveloperLength + 1] = {};
};
