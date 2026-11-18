#include "common.h"
#include <string.h>
#include "UnknownFileType.h"
#include "Nds/NdsFileType.h"
#include "Gba/GbaFileType.h"
#include "Gb/GbFileType.h"
#include "Nes/NesFileType.h"
#include "Snes/SnesFileType.h"
#include "ExtensionFileTypeProvider.h"

static bool isNdsExtension(const char* extension)
{
    return !strcasecmp(extension, "nds")
        || !strcasecmp(extension, "srl")
        || !strcasecmp(extension, "dsi");
}

static bool isGbaExtension(const char* extension)
{
    return !strcasecmp(extension, "gba")
        || !strcasecmp(extension, "agb");
}

static bool isNesExtension(const char* extension)
{
    return !strcasecmp(extension, "nes");
}

static bool isSnesExtension(const char* extension)
{
    return !strcasecmp(extension, "sfc")
        || !strcasecmp(extension, "smc");
}

// A file opened through a file association counts as a game, so the roms of any system
// the user sets up an emulator for can be marked, deleted and randomly picked. Only these
// are known not to be games: text, pictures, video and music. An association missing
// here only gains buttons it does not need, while a rom left out could never be marked.
static bool isNonGameExtension(const char* extension)
{
    static const char* const kNonGameExtensions[] =
    {
        "txt",
        "bmp", "png", "jpg", "jpeg", "gif",
        "dpg", "avi",
        "mp3", "ogg", "wav", "mod", "xm", "mid", "nsf"
    };
    for (const char* nonGameExtension : kNonGameExtensions)
    {
        if (!strcasecmp(extension, nonGameExtension))
        {
            return true;
        }
    }
    return false;
}

ExtensionFileTypeProvider::ExtensionFileTypeProvider(const AppSettings& appSettings)
    : _appSettings(appSettings)
{
    _customFileTypes = std::make_unique_for_overwrite<CustomFileType[]>(appSettings.numberOfFileAssociations);
    for (u32 i = 0; i < appSettings.numberOfFileAssociations; i++)
    {
        const FileType* baseFileType = nullptr;
        const char* extension = appSettings.fileAssociations[i].extension;
        if (isGbaExtension(extension))
        {
            baseFileType = &GbaFileType::sInstance;
        }
        else if (!strcasecmp(extension, "gbc"))
        {
            baseFileType = &GbFileType::sGbcInstance;
        }
        else if (!strcasecmp(extension, "gb"))
        {
            // The header is where a Game Boy rom keeps its title and publisher. Without a
            // file type of its own it has no internal file info and the browser can only
            // show the file name.
            baseFileType = &GbFileType::sGbInstance;
        }
        else if (isNesExtension(extension))
        {
            baseFileType = &NesFileType::sInstance;
        }
        else if (isSnesExtension(extension))
        {
            baseFileType = !strcasecmp(extension, "smc")
                ? &SnesFileType::sSmcInstance
                : &SnesFileType::sSfcInstance;
        }
        _customFileTypes[i] = CustomFileType(&appSettings.fileAssociations[i], baseFileType,
            isNonGameExtension(appSettings.fileAssociations[i].extension)
                ? FileTypeClassification::Misc
                : FileTypeClassification::Game);
    }
}

const FileType* ExtensionFileTypeProvider::GetFileType(const TCHAR* path) const
{
    const char* extension = strrchr(path, '.');
    if (extension)
    {
        extension++; // skip over dot

        if (isNdsExtension(extension))
        {
            return &NdsFileType::sInstance;
        }

        for (u32 i = 0; i < _appSettings.numberOfFileAssociations; i++)
        {
            // the association's own extension: an agb one has the short name "gba"
            if (!strcasecmp(extension, _appSettings.fileAssociations[i].extension))
            {
                return &_customFileTypes[i];
            }
        }
    }

    return &UnknownFileType::sInstance;
}