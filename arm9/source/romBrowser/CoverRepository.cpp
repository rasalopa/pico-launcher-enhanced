#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "fat/Directory.h"
#include "FileType/NullFileTypeProvider.h"
#include "FileType/BmpFileCover.h"
#include "FileType/InternalFileInfo.h"
#include "SdFolderFactory.h"
#include "CoverRepository.h"

/// @brief Loads a BMP cover.
/// @return The cover, or \c nullptr when the file is not a valid cover, so the caller can fall back.
static FileCover* LoadBmpCover(const FastFileRef& coverFileRef)
{
    auto cover = std::make_unique<BmpFileCover>(coverFileRef);
    return cover->IsLoaded() ? cover.release() : nullptr;
}

void CoverRepository::Initialize()
{
    InitializeFolders("/_pico/covers/");
}

FileCover* CoverRepository::GetCoverForFile(const FileInfo& fileInfo, const InternalFileInfo* internalFileInfo) const
{
    char nameBuffer[256];
    const auto& fileType = fileInfo.GetFileType();

    if (fileType->GetClassification() == FileTypeClassification::Folder)
    {
        // Look for cover.bmp inside the folder (path relative to FatFs CWD = current browse dir).
        // Scan with the already-open directory handle so the match can be turned directly into a
        // FastFileRef, instead of stat'ing then re-opening the same path by name.
        auto folderDir = std::make_unique<Directory>();
        if (folderDir->Open(fileInfo.GetFileName()) == FR_OK)
        {
            FILINFO folderFileInfo;
            while (folderDir->Read(&folderFileInfo) == FR_OK && folderFileInfo.fname[0] != 0)
            {
                if (!(folderFileInfo.fattrib & AM_DIR) && !strcasecmp(folderFileInfo.fname, "cover.bmp"))
                {
                    auto cover = LoadBmpCover(FastFileRef(folderDir->GetFatFsDirectory(), &folderFileInfo));
                    if (cover)
                    {
                        return cover;
                    }

                    break;
                }
            }
        }

        return fileType->CreateFileCover(fileInfo.GetFileName());
    }

    // Try to get a cover based on the filename in the user folder
    if (_userFolder)
    {
        mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", fileInfo.GetFileName());
        const auto* coverFile = _userFolder->BinarySearch(nameBuffer);
        if (coverFile)
        {
            auto cover = LoadBmpCover(coverFile->GetFastFileRef());
            if (cover)
            {
                return cover;
            }
        }
    }

    // Try to get a cover based on an internal game code
    if (internalFileInfo)
    {
        const auto* coverFolder = GetFileTypeFolder(fileType->GetShortName());
        if (coverFolder)
        {
            const char* gameCode = internalFileInfo->GetGameCode();
            if (gameCode)
            {
                mini_snprintf(nameBuffer, sizeof(nameBuffer), "%s.bmp", gameCode);
                const auto* coverFile = coverFolder->BinarySearch(nameBuffer);
                if (coverFile)
                {
                    auto cover = LoadBmpCover(coverFile->GetFastFileRef());
                    if (cover)
                    {
                        return cover;
                    }
                }
            }
        }
    }

    if (internalFileInfo)
    {
        auto cover = internalFileInfo->CreateGameCover();
        if (cover)
        {
            return cover;
        }
    }

    return fileType->CreateFileCover(fileInfo.GetFileName());
}
