#pragma once
#include "../InternalFileInfo.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for SNES roms.
class alignas(32) SnesInternalFileInfo : public InternalFileInfo
{
public:
    explicit SnesInternalFileInfo(const FastFileRef& fastFileRef);

    const char16_t* GetPublisher() const override { return _publisher[0] != 0 ? _publisher : nullptr; }

private:
    /// @brief The publisher named by the developer ID or expanded maker code, when known.
    char16_t _publisher[64];
};
