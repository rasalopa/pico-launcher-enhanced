#pragma once
#include <nds/ndstypes.h>

/// @brief Layout of an uncompressed palettized BMP, read from its BITMAPFILEHEADER and DIB header.
struct BmpHeader
{
    /// @brief Size of the BITMAPFILEHEADER plus the largest DIB header (BITMAPV5HEADER).
    static constexpr u32 MaxHeaderSize = 14 + 124;

    /// @brief File offset of the color table.
    u32 paletteOffset;

    /// @brief Number of entries in the color table, at least 1.
    u32 paletteCount;

    /// @brief File offset of the pixel data.
    u32 dataOffset;

    /// @brief \c true when the rows are stored top to bottom (negative biHeight).
    bool topDown;

    /// @brief Parses and validates the header of an uncompressed palettized BMP. Any DIB header from
    ///        BITMAPINFOHEADER to BITMAPV5HEADER is accepted, and so is a color table with fewer entries
    ///        than the bit depth allows, which image editors write for images with few colors.
    /// @param buffer The start of the BMP file. Must be at least MaxHeaderSize bytes.
    /// @param expectedWidth Expected width of the image.
    /// @param expectedHeight Expected height of the image.
    /// @param expectedBpp Expected bits per pixel.
    /// @param header Receives the layout of the file when it is valid.
    /// @return \c true when valid, or \c false otherwise.
    static bool Parse(const u8* buffer, u32 expectedWidth, u32 expectedHeight, u32 expectedBpp, BmpHeader& header)
    {
        if (buffer[0] != 'B' || buffer[1] != 'M')
        {
            return false;
        }

        u32 dataOffset = ReadU32(&buffer[0x0A]);
        u32 dibSize    = ReadU32(&buffer[0x0E]);
        u32 width      = ReadU32(&buffer[0x12]);
        s32 height     = (s32)ReadU32(&buffer[0x16]);
        u32 bpp        = buffer[0x1C] | (buffer[0x1D] << 8);
        u32 comp       = ReadU32(&buffer[0x1E]);
        u32 clrUsed    = ReadU32(&buffer[0x2E]);

        u32 maxColors = 1u << expectedBpp;
        u32 paletteOffset = 14 + dibSize;
        if (dibSize < 40 || dibSize > MaxHeaderSize - 14
            || width != expectedWidth
            || (height != (s32)expectedHeight && height != -(s32)expectedHeight)
            || bpp != expectedBpp
            || comp != 0
            || clrUsed > maxColors
            || dataOffset < paletteOffset)
        {
            return false;
        }

        // A clrUsed of 0 means a full table, but some writers store a shorter one anyway,
        // so the table also ends where the pixel data starts.
        u32 paletteCount = clrUsed == 0 ? maxColors : clrUsed;
        u32 paletteRoom = (dataOffset - paletteOffset) / 4;
        if (paletteCount > paletteRoom)
        {
            paletteCount = paletteRoom;
        }

        if (paletteCount == 0)
        {
            return false;
        }

        header.paletteOffset = paletteOffset;
        header.paletteCount = paletteCount;
        header.dataOffset = dataOffset;
        header.topDown = height < 0;
        return true;
    }

private:
    static u32 ReadU32(const u8* data)
    {
        return data[0] | (data[1] << 8) | (data[2] << 16) | ((u32)data[3] << 24);
    }
};
