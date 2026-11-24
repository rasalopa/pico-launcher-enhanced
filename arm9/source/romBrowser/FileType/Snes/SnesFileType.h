#pragma once
#include "../FileType.h"
#include "SnesInternalFileInfo.h"
#include "../../Theme/IThemeFileIconFactory.h"

/// @brief File type for SNES roms.
class SnesFileType : public FileType
{
public:
    static const SnesFileType sSfcInstance;
    static const SnesFileType sSmcInstance;

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        return themeFileIconFactory->CreateGenericFileIcon(fileName);
    }

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return new SnesInternalFileInfo(fastFileRef);
    }

private:
    constexpr SnesFileType(const char* shortName)
        : FileType(shortName, FileTypeClassification::Game) { }
};
