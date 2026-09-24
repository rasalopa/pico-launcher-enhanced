#pragma once
#include "FileCover.h"
#include "fat/FastFileRef.h"

class File;

/// @brief Class representing a BMP file cover.
class alignas(32) BmpFileCover : public FileCover
{
public:
    explicit BmpFileCover(const FastFileRef& coverFileRef);

    VBlankTextureLoadRequest CreateTextureLoadRequest() const override
    {
        return VBlankTextureLoadRequest(
            _coverBuffer, 128 * 96, _texVramOffset,
            _palette, sizeof(_palette), _plttVramOffset,
            nullptr, nullptr);
    }

    void Upload2DCoverBitmap(void* destination) const override;
    void Upload2DCoverPalette(void* destination) const override;

    bool IsActualCover() const override { return true; }

    /// @brief Returns whether the file was read as a 128x96 8 bpp BMP.
    /// @return \c true when the cover was loaded, or \c false when it should not be shown.
    bool IsLoaded() const { return _isLoaded; }

private:
    u8 _coverBuffer[128 * 96] alignas(32);
    u16 _palette[256] alignas(32);
    bool _isLoaded = false;

    bool Load(File& file);
};
