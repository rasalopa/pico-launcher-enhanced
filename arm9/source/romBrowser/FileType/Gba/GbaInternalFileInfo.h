#pragma once
#include "../InternalFileInfo.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for gba roms.
class alignas(32) GbaInternalFileInfo : public InternalFileInfo
{
public:
    explicit GbaInternalFileInfo(const FastFileRef& fastFileRef);

    constexpr const char* GetGameCode() const override { return _gameCode; }

    const char16_t* GetPublisher() const override { return _publisher[0] != 0 ? _publisher : nullptr; }

private:
    char _gameCode[5];
    /// @brief The publisher named by the header's maker code, when it is known.
    char16_t _publisher[64];
};
