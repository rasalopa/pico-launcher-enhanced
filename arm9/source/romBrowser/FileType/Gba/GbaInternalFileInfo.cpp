#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "../RomPublisherCodes.h"
#include "GbaInternalFileInfo.h"

// The cartridge header as GBATEK lays it out: the game code at 0xAC, the maker
// code at 0xB0 and the fixed 0x96 at 0xB2.
#define GBA_HEADER_READ_OFFSET      0xA0
#define GBA_HEADER_READ_SIZE        0x13
#define GBA_HEADER_GAME_CODE_OFFSET 0x0C
#define GBA_HEADER_MAKER_CODE_OFFSET 0x10
#define GBA_HEADER_FIXED_OFFSET     0x12
#define GBA_HEADER_FIXED_VALUE      0x96

// A real cartridge header carries 0x96 right after the maker code. Homebrew and
// dumps that carry junk there also tend to have something other than a maker
// code, so this decides whether the publisher is worth showing at all.
static bool HasCartridgeHeader(const u8* header)
{
    return header[GBA_HEADER_FIXED_OFFSET] == GBA_HEADER_FIXED_VALUE;
}

GbaInternalFileInfo::GbaInternalFileInfo(const FastFileRef& fastFileRef)
{
    memset(_gameCode, 0, sizeof(_gameCode));
    memset(_publisher, 0, sizeof(_publisher));

    const auto file = std::make_unique<File>();
    file->Open(fastFileRef, FA_READ);

    u8 header[GBA_HEADER_READ_SIZE];
    if (file->Seek(GBA_HEADER_READ_OFFSET) != FR_OK ||
        !file->ReadExact(header, sizeof(header)))
    {
        return;
    }

    memcpy(_gameCode, &header[GBA_HEADER_GAME_CODE_OFFSET], 4);

    if (!HasCartridgeHeader(header))
    {
        return;
    }

    const char* publisher = GetRomPublisherName((const char*)&header[GBA_HEADER_MAKER_CODE_OFFSET]);
    if (!publisher)
    {
        return;
    }

    CopyPublisherName(_publisher, sizeof(_publisher) / sizeof(_publisher[0]), publisher);
}
