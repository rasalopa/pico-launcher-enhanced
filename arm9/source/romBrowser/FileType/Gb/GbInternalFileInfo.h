#pragma once
#include "../InternalFileInfo.h"
#include "fat/FastFileRef.h"

/// @brief Internal file info for game boy and game boy color roms.
class alignas(32) GbInternalFileInfo : public InternalFileInfo
{
public:
    explicit GbInternalFileInfo(const FastFileRef& fastFileRef);

    const char16_t* GetPublisher() const override { return _publisher[0] != 0 ? _publisher : nullptr; }

private:
    /// @brief The publisher named by the licensee code, when it is known.
    char16_t _publisher[64];
};
