#include "common.h"
#include <string.h>
#include "fat/File.h"
#include "../RomPublisherCodes.h"
#include "NesInternalFileInfo.h"

// The Nintendo header sits at $FFE0-$FFFF of the PRG data: title, checksums, sizes,
// title encoding and the licensee code at $FFF8. No-Intro NES sets are headerless, so
// the header, when present at all, is at the end of one of the power of two PRG chunks.
#define NES_HEADER_SIZE                 32
#define NES_HEADER_LICENSEE_OFFSET      0x18
#define NES_HEADER_VALIDATION_OFFSET    0x12
#define NES_HEADER_VALIDATION_END       0x1A
#define NES_HEADER_TITLE_ENCODING       0x16
#define NES_HEADER_TITLE_LENGTH         0x17

static bool IsPowerOfTwo(FSIZE_t value)
{
    return value != 0 && (value & (value - 1)) == 0;
}

static bool IsValidHeader(const u8* header)
{
    u32 checksum = 0;
    for (u32 i = NES_HEADER_VALIDATION_OFFSET; i < NES_HEADER_VALIDATION_END; i++)
    {
        checksum += header[i];
    }

    if ((checksum & 0xFF) != 0)
        return false;
    if (header[NES_HEADER_TITLE_ENCODING] < 1 || header[NES_HEADER_TITLE_ENCODING] > 2)
        return false;
    if (header[NES_HEADER_TITLE_LENGTH] < 1 || header[NES_HEADER_TITLE_LENGTH] > 15)
        return false;
    if (header[NES_HEADER_LICENSEE_OFFSET] == 0)
        return false;

    // A readable ASCII title makes it much less likely that random PRG data happens to
    // satisfy the checksum. JIS titles are left alone.
    if (header[NES_HEADER_TITLE_ENCODING] == 1)
    {
        for (u32 i = 0; i < 16; i++)
        {
            const u8 c = header[i];
            if (c != 0 && c != ' ' && (c < 0x20 || c > 0x7E))
                return false;
        }
    }

    return true;
}

static bool TryReadPublisher(File& file, FSIZE_t headerOffset, char16_t* publisher, u32 publisherSize)
{
    u8 header[NES_HEADER_SIZE];
    if (file.Seek(headerOffset) != FR_OK || !file.ReadExact(header, sizeof(header)))
        return false;
    if (!IsValidHeader(header))
        return false;

    const char* name = GetLegacyPublisherName(header[NES_HEADER_LICENSEE_OFFSET]);
    if (name)
    {
        CopyPublisherName(publisher, publisherSize, name);
    }
    return true;
}

NesInternalFileInfo::NesInternalFileInfo(const FastFileRef& fastFileRef)
{
    memset(_publisher, 0, sizeof(_publisher));

    const auto file = std::make_unique<File>();
    file->Open(fastFileRef, FA_READ);
    const FSIZE_t fileSize = file->GetSize();

    // A file that still has its iNES header says where the PRG data ends itself.
    u8 first[16];
    if (fileSize >= sizeof(first) && file->Seek(0) == FR_OK && file->ReadExact(first, sizeof(first)) &&
        first[0] == 'N' && first[1] == 'E' && first[2] == 'S' && first[3] == 0x1A)
    {
        const u32 prgSize = first[4] != 0 ? first[4] * 0x4000 : 0x4000;
        FSIZE_t prgOffset = 16;
        if ((first[6] & 0x04) != 0)
        {
            prgOffset += 512; // trainer
        }

        if (prgOffset + prgSize <= fileSize)
        {
            TryReadPublisher(*file, prgOffset + prgSize - NES_HEADER_SIZE,
                _publisher, sizeof(_publisher) / sizeof(_publisher[0]));
        }
        return;
    }

    // Headerless sets: the PRG size is not recorded, so try each power of two that leaves
    // a valid CHR size and let the header checksum decide.
    for (u32 prgSize = 0x4000; prgSize <= 0x80000; prgSize <<= 1)
    {
        if ((FSIZE_t)prgSize + NES_HEADER_SIZE > fileSize)
            break;

        const FSIZE_t chrSize = fileSize - prgSize;
        if (chrSize != 0 && (!IsPowerOfTwo(chrSize) || chrSize > 0x40000))
            continue;

        if (TryReadPublisher(*file, prgSize - NES_HEADER_SIZE,
                _publisher, sizeof(_publisher) / sizeof(_publisher[0])))
        {
            return;
        }
    }
}
