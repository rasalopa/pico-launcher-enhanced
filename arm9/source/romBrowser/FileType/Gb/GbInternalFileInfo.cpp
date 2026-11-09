#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "../RomPublisherCodes.h"
#include "GbInternalFileInfo.h"

// The cartridge header as Pan Docs lays it out, read from the start of the
// Nintendo logo: the logo at 0x104 (48 bytes), the new licensee code at 0x144 and
// the old one at 0x14B.
#define GB_HEADER_READ_OFFSET       0x104
#define GB_HEADER_READ_SIZE         0x48
#define GB_HEADER_LOGO_OFFSET       0x00
#define GB_HEADER_LOGO_SIZE         48
#define GB_HEADER_NEW_LICENSEE_OFFSET 0x40
#define GB_HEADER_OLD_LICENSEE_OFFSET 0x47
#define GB_HEADER_CGB_FLAG_OFFSET   0x3F
#define GB_HEADER_CGB_FLAG          0x80
#define GB_HEADER_OLD_LICENSEE_NEW  0x33

static const u8 sNintendoLogo[GB_HEADER_LOGO_SIZE] =
{
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

// The boot rom refuses to run a cartridge whose logo does not match, so a file that fails
// this is not a Game Boy rom, whatever its extension says.
static bool HasCartridgeHeader(const u8* header)
{
    return memcmp(&header[GB_HEADER_LOGO_OFFSET], sNintendoLogo, GB_HEADER_LOGO_SIZE) == 0;
}

GbInternalFileInfo::GbInternalFileInfo(const FastFileRef& fastFileRef)
{
    memset(_publisher, 0, sizeof(_publisher));

    const auto file = std::make_unique<File>();
    file->Open(fastFileRef, FA_READ);

    u8 header[GB_HEADER_READ_SIZE];
    if (file->Seek(GB_HEADER_READ_OFFSET) != FR_OK ||
        !file->ReadExact(header, sizeof(header)) ||
        !HasCartridgeHeader(header))
    {
        return;
    }

    // Before the Super Game Boy the publisher was a number of its own; 0x33 hands that job
    // to the two character code, which is the same numbering the GBA maker code uses.
    const char* publisher = header[GB_HEADER_OLD_LICENSEE_OFFSET] == GB_HEADER_OLD_LICENSEE_NEW
        ? GetGameBoyPublisherName((const char*)&header[GB_HEADER_NEW_LICENSEE_OFFSET],
            (header[GB_HEADER_CGB_FLAG_OFFSET] & GB_HEADER_CGB_FLAG) != 0)
        : GetLegacyPublisherName(header[GB_HEADER_OLD_LICENSEE_OFFSET]);
    if (!publisher)
    {
        return;
    }

    CopyPublisherName(_publisher, sizeof(_publisher) / sizeof(_publisher[0]), publisher);
}
