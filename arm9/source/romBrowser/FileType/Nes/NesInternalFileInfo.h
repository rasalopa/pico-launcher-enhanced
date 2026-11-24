#pragma once
#include "../InternalFileInfo.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for NES roms.
class alignas(32) NesInternalFileInfo : public InternalFileInfo
{
public:
    explicit NesInternalFileInfo(const FastFileRef& fastFileRef);

    const char16_t* GetPublisher() const override { return _publisher[0] != 0 ? _publisher : nullptr; }

private:
    /// @brief The publisher named by the optional Nintendo header, when it is known.
    char16_t _publisher[64];
};
