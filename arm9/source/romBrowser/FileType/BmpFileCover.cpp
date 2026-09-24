#include "common.h"
#include <string.h>
#include <algorithm>
#include <nds/arm9/cache.h>
#include <memory>
#include <libtwl/dma/dmaNitro.h>
#include "fat/File.h"
#include "core/math/ColorConverter.h"
#include "BmpHeader.h"
#include "BmpFileCover.h"

BmpFileCover::BmpFileCover(const FastFileRef& coverFileRef)
{
    const auto file = std::make_unique<File>();
    file->Open(coverFileRef, FA_READ);
    _isLoaded = Load(*file);
    file->Close();
}

bool BmpFileCover::Load(File& file)
{
    // The header and the color table are read into the pixel buffer first.
    // A valid cover file is always longer than the largest of both.
    constexpr u32 headerAndPaletteSize = BmpHeader::MaxHeaderSize + 256 * 4;
    static_assert(headerAndPaletteSize <= sizeof(_coverBuffer));

    BmpHeader header;
    if (!file.ReadExact(_coverBuffer, headerAndPaletteSize) ||
        !BmpHeader::Parse(_coverBuffer, 128, 96, 8, header))
    {
        return false;
    }

    // Entries missing from a short color table stay black.
    memset(_palette, 0, sizeof(_palette));
    const u8* paletteData32 = &_coverBuffer[header.paletteOffset];
    for (u32 i = 0; i < header.paletteCount; i++)
    {
        u32 b = *paletteData32++;
        u32 g = *paletteData32++;
        u32 r = *paletteData32++;
        paletteData32++;
        _palette[i] = ColorConverter::ToXBGR555(Rgb<5, 5, 5>(Rgb8(r, g, b)));
    }

    if (file.Seek(header.dataOffset) != FR_OK ||
        !file.ReadExact(_coverBuffer, sizeof(_coverBuffer)))
    {
        return false;
    }

    // Covers are drawn from rows in the usual bottom-up order of BMP.
    if (header.topDown)
    {
        for (u32 y = 0; y < 96 / 2; y++)
        {
            u8* row = &_coverBuffer[y * 128];
            std::swap_ranges(row, row + 128, &_coverBuffer[(95 - y) * 128]);
        }
    }

    DC_FlushRange(_coverBuffer, sizeof(_coverBuffer));
    DC_FlushRange(_palette, sizeof(_palette));
    return true;
}

void BmpFileCover::Upload2DCoverBitmap(void* destination) const
{
    cover_bitmapToTiledCopy(_coverBuffer, destination);
}

void BmpFileCover::Upload2DCoverPalette(void* destination) const
{
    dma_ntrCopy32(3, _palette, destination, sizeof(_palette));
}
