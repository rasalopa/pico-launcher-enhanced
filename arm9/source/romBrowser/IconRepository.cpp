#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "fat/Directory.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileIconData.h"
#include "SdFolderFactory.h"
#include "IconRepository.h"

/// @brief Loads a BMP icon.
/// @return The icon, or \c nullptr when the file is not a valid icon, so the caller can fall back.
static SharedPtr<BmpFileIconData> LoadBmpIcon(const FastFileRef& iconFileRef)
{
    auto iconData = SharedPtr<BmpFileIconData>::MakeShared(iconFileRef);
    return iconData->IsLoaded() ? iconData : nullptr;
}

void IconRepository::Initialize()
{
    InitializeFolders("/_pico/icons/");
}

SharedPtr<BmpFileIconData> IconRepository::GetIconForFile(const FileInfo& fileInfo, const char* gameCode) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() == FileTypeClassification::Folder)
    {
        // Look for icon.bmp inside the folder (path relative to FatFs CWD = current browse dir).
        // Scan with the already-open directory handle so the match can be turned directly into a
        // FastFileRef, instead of stat'ing then re-opening the same path by name.
        auto folderDir = std::make_unique<Directory>();
        if (folderDir->Open(fileInfo.GetFileName()) == FR_OK)
        {
            FILINFO folderFileInfo;
            while (folderDir->Read(&folderFileInfo) == FR_OK && folderFileInfo.fname[0] != 0)
            {
                if (!(folderFileInfo.fattrib & AM_DIR) && !strcasecmp(folderFileInfo.fname, "icon.bmp"))
                {
                    return LoadBmpIcon(FastFileRef(folderDir->GetFatFsDirectory(), &folderFileInfo));
                }
            }
        }

        return nullptr;
    }

    // Try to get an icon based on the filename in the user folder
    if (_userFolder)
    {
        mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", fileInfo.GetFileName());
        const auto* iconFile = _userFolder->BinarySearch(nameBuffer);
        if (iconFile)
        {
            auto iconData = LoadBmpIcon(iconFile->GetFastFileRef());
            if (iconData)
            {
                return iconData;
            }
        }
    }

    // Try to get an icon based on an internal game code
    if (gameCode)
    {
        const auto* iconFolder = GetFileTypeFolder(fileType->GetShortName());
        if (iconFolder)
        {
            mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", gameCode);
            const auto* iconFile = iconFolder->BinarySearch(nameBuffer);
            if (iconFile)
            {
                return LoadBmpIcon(iconFile->GetFastFileRef());
            }
        }
    }

    return nullptr;
}
