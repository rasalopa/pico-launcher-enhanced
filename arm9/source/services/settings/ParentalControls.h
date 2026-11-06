#pragma once
#include <string.h>
#include "core/String.h"

/// @brief What parental controls can take away.
enum class ParentalFeature
{
    /// @brief Deleting a game and its saves.
    Delete,
    /// @brief The cheats sheet.
    Cheats,
    /// @brief Choosing or creating a save slot.
    SaveSlots,
    /// @brief Toggling a game's favourite mark.
    Favorites,
    /// @brief Toggling a game's completed mark.
    Completion,
    /// @brief Editing a game's title, publisher or developer.
    GameInfo
};

/// @brief The stored parental controls.
///
/// The PIN and the permissions are plain text in settings.json. This is a lock
/// against someone using the launcher, not against someone with the SD card:
/// the file is the way back in, and `"restrictedMode": false` turns the lock
/// off without needing the PIN.
class ParentalControls
{
public:
    bool enabled = false;
    /// @brief Plain-text PIN, 4 to 8 characters when set through the launcher.
    String<char, 16> pin = "";

    bool allowDelete = true;
    bool allowCheats = true;
    bool allowSaveSlots = true;
    bool allowFavorites = true;
    bool allowCompletion = true;
    bool allowGameInfo = true;

    bool HasPin() const
    {
        return pin.GetString()[0] != 0;
    }

    bool MatchesPin(const char* value) const
    {
        return value && strcmp(pin.GetString(), value) == 0;
    }

    /// @brief Whether a feature is available right now.
    bool Allows(ParentalFeature feature) const
    {
        return !enabled || IsAllowed(feature);
    }

    /// @brief The stored permission, whether or not the lock is on.
    bool IsAllowed(ParentalFeature feature) const
    {
        switch (feature)
        {
            case ParentalFeature::Delete:
                return allowDelete;
            case ParentalFeature::Cheats:
                return allowCheats;
            case ParentalFeature::SaveSlots:
                return allowSaveSlots;
            case ParentalFeature::Favorites:
                return allowFavorites;
            case ParentalFeature::Completion:
                return allowCompletion;
            case ParentalFeature::GameInfo:
                return allowGameInfo;
            default:
                return true;
        }
    }

    void SetAllowed(ParentalFeature feature, bool allowed)
    {
        switch (feature)
        {
            case ParentalFeature::Delete:
                allowDelete = allowed;
                break;
            case ParentalFeature::Cheats:
                allowCheats = allowed;
                break;
            case ParentalFeature::SaveSlots:
                allowSaveSlots = allowed;
                break;
            case ParentalFeature::Favorites:
                allowFavorites = allowed;
                break;
            case ParentalFeature::Completion:
                allowCompletion = allowed;
                break;
            case ParentalFeature::GameInfo:
                allowGameInfo = allowed;
                break;
        }
    }
};
