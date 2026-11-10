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
    /// @brief Toggling a game's favourite or completed badge.
    Badges,
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
    bool restrictedMode = false;
    /// @brief Plain-text PIN, 4 to 8 digits when set through the launcher.
    String<char, 16> pin = "";

    bool allowDelete = true;
    bool allowCheats = true;
    bool allowSaveSlots = true;
    bool allowBadges = true;
    bool allowGameInfo = true;
    /// @brief Launch every DS game on .sav10 while restricted mode is on.
    bool saveSandboxing = false;

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
        if (!restrictedMode)
            return true;
        // Save sandboxing owns the save choice, so the slot picker and New save
        // stay away even if a card still says the old permission is on.
        if (feature == ParentalFeature::SaveSlots && saveSandboxing)
            return false;
        return IsAllowed(feature);
    }

    /// @brief Whether games launch on .sav10 right now.
    bool UsesSaveSandboxing() const
    {
        return restrictedMode && saveSandboxing;
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
            case ParentalFeature::Badges:
                return allowBadges;
            case ParentalFeature::GameInfo:
                return allowGameInfo;
            default:
                return true;
        }
    }

    /// @brief Turns save sandboxing on or off; turning it on also locks save slots.
    void SetSaveSandboxing(bool value)
    {
        saveSandboxing = value;
        if (value)
            allowSaveSlots = false;
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
            case ParentalFeature::Badges:
                allowBadges = allowed;
                break;
            case ParentalFeature::GameInfo:
                allowGameInfo = allowed;
                break;
        }
    }
};
