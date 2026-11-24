#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "../RomPublisherCodes.h"
#include "SnesInternalFileInfo.h"

// The SNES header is at $FFC0 in the CPU address space: LoROM puts it at 0x7FC0 in
// the file, HiROM at 0xFFC0 and ExHiROM at 0x40FFC0. The developer ID at +0x1A is a
// one byte code, except 0x33 which points at the two character maker code at -0x10.
#define SNES_HEADER_SIZE                0x20
#define SNES_HEADER_MAP_MODE            0x15
#define SNES_HEADER_DEVELOPER_ID        0x1A
#define SNES_HEADER_CHECKSUM_COMPLEMENT 0x1C
#define SNES_HEADER_CHECKSUM            0x1E
#define SNES_HEADER_EXPANDED_MAKER      0x10
#define SNES_DEVELOPER_ID_EXPANDED      0x33

struct SnesHeaderLocation
{
    u32 offset;
    u8 mode;
};

static const SnesHeaderLocation sHeaderLocations[] =
{
    { 0x7FC0, 0 },  // LoROM
    { 0xFFC0, 1 },  // HiROM
    { 0x40FFC0, 5 } // ExHiROM
};

static bool IsValidHeader(const u8* header, const SnesHeaderLocation& location)
{
    const u16 complement = header[SNES_HEADER_CHECKSUM_COMPLEMENT] |
        (header[SNES_HEADER_CHECKSUM_COMPLEMENT + 1] << 8);
    const u16 checksum = header[SNES_HEADER_CHECKSUM] |
        (header[SNES_HEADER_CHECKSUM + 1] << 8);
    if (complement == 0 || ((complement + checksum) & 0xFFFF) != 0xFFFF)
        return false;

    const u8 mode = header[SNES_HEADER_MAP_MODE] & 0x0F;
    if (location.offset == 0x40FFC0)
        return mode == 5;
    if (location.offset == 0xFFC0)
        return mode == 1;
    return mode == 0 || mode == 2 || mode == 3 || mode == 0x0A;
}

static bool TryReadPublisher(File& file, u32 headerOffset, const SnesHeaderLocation& location,
    char16_t* publisher, u32 publisherSize)
{
    u8 header[SNES_HEADER_SIZE];
    if (file.Seek(headerOffset) != FR_OK || !file.ReadExact(header, sizeof(header)))
        return false;
    if (!IsValidHeader(header, location))
        return false;

    const char* name = nullptr;
    const u8 developerId = header[SNES_HEADER_DEVELOPER_ID];
    if (developerId == SNES_DEVELOPER_ID_EXPANDED)
    {
        char maker[2];
        if (headerOffset < SNES_HEADER_EXPANDED_MAKER ||
            file.Seek(headerOffset - SNES_HEADER_EXPANDED_MAKER) != FR_OK ||
            !file.ReadExact(maker, sizeof(maker)))
        {
            return true;
        }
        name = GetSnesPublisherName(maker);
    }
    else
    {
        name = GetLegacyPublisherName(developerId);
    }

    if (name)
    {
        CopyPublisherName(publisher, publisherSize, name);
    }
    return true;
}

SnesInternalFileInfo::SnesInternalFileInfo(const FastFileRef& fastFileRef)
{
    memset(_publisher, 0, sizeof(_publisher));

    const auto file = std::make_unique<File>();
    file->Open(fastFileRef, FA_READ);
    const FSIZE_t fileSize = file->GetSize();

    // Some dumps carry a 512 byte copier header; the offsets are the same after it.
    static const u32 bases[] = { 0, 512 };
    for (u32 base : bases)
    {
        for (const auto& location : sHeaderLocations)
        {
            const u32 headerOffset = base + location.offset;
            if ((FSIZE_t)headerOffset + SNES_HEADER_SIZE > fileSize)
                continue;
            if (TryReadPublisher(*file, headerOffset, location,
                    _publisher, sizeof(_publisher) / sizeof(_publisher[0])))
            {
                return;
            }
        }
    }
}
