#include "common.h"
#include <memory>
#include <string.h>
#include <nds/arm9/cache.h>
#include "fat/File.h"
#include "core/math/ColorConverter.h"
#include "BmpHeader.h"
#include "BmpFileIconData.h"

BmpFileIconData::BmpFileIconData(const FastFileRef& iconFileRef)
{
    auto file = std::make_unique<File>();
    file->Open(iconFileRef, FA_READ);

    memset(_iconGfx, 0, sizeof(_iconGfx));
    memset(_iconPltt, 0, sizeof(_iconPltt));
    _isLoaded = Load(std::move(file));
    DC_FlushRange(_iconGfx, sizeof(_iconGfx));
    DC_FlushRange(_iconPltt, sizeof(_iconPltt));
}

bool BmpFileIconData::Load(std::unique_ptr<File> file)
{
    // Heap-allocate the staging buffer so it doesn't live on the task thread stack. It holds the
    // header and the color table first, then the pixels. A valid icon file is always longer than both.
    constexpr u32 headerAndPaletteSize = BmpHeader::MaxHeaderSize + 16 * 4;
    static_assert(headerAndPaletteSize <= GfxSize);

    auto rawPixelData = std::make_unique<u8[]>(GfxSize);
    BmpHeader header;
    if (!rawPixelData ||
        !file->ReadExact(rawPixelData.get(), headerAndPaletteSize) ||
        !BmpHeader::Parse(rawPixelData.get(), 32, 32, 4, header))
    {
        return false;
    }

    const bool topDown = header.topDown;

    // Entries missing from a short color table stay black.
    const u8* paletteData = &rawPixelData[header.paletteOffset];
    for (u32 i = 0; i < header.paletteCount; i++)
    {
        u32 b = *paletteData++;
        u32 g = *paletteData++;
        u32 r = *paletteData++;
        paletteData++;
        _iconPltt[i] = ColorConverter::ToGBGR565(Rgb<8, 8, 8>(r, g, b));
    }

    if (file->Seek(header.dataOffset) != FR_OK ||
        !file->ReadExact(rawPixelData.get(), GfxSize))
    {
        memset(_iconPltt, 0, sizeof(_iconPltt));
        return false;
    }

    // Convert BMP rows (bottom-up or top-down) to the DS tiled 4 bpp sprite format.
    // BMP is high-nibble-first; DS tiles are low-nibble-first -- swap nibbles per 4-byte group.
    for (int y = 0; y < 32; y++)
    {
        // Bottom-up BMP (normal, positive height): row 0 is the bottom of the image.
        // Top-down BMP (negative height): row 0 is the top of the image.
        const u8* srcRowPtr = topDown
            ? rawPixelData.get() + y * 16
            : rawPixelData.get() + (31 - y) * 16;

        int ty = y / 8;
        int py = y % 8;

        for (int tx = 0; tx < 4; tx++)
        {
            u32 val;
            memcpy(&val, srcRowPtr + tx * 4, 4);
            val = ((val >> 4) & 0x0F0F0F0F) | ((val & 0x0F0F0F0F) << 4);
            memcpy(&_iconGfx[(ty * 4 + tx) * 32 + py * 4], &val, 4);
        }
    }

    return true;
}
