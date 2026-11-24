#pragma once
#include "../FileType.h"
#include "GbInternalFileInfo.h"
#include "../../Theme/IThemeFileIconFactory.h"

/// @brief File type for game boy and game boy color roms.
class GbFileType : public FileType
{
public:
    static const GbFileType sGbInstance;
    static const GbFileType sGbcInstance;

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        return themeFileIconFactory->CreateGenericFileIcon(fileName);
    }

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return new GbInternalFileInfo(fastFileRef);
    }

private:
    constexpr GbFileType(const char* shortName)
        : FileType(shortName, FileTypeClassification::Game) { }
};
