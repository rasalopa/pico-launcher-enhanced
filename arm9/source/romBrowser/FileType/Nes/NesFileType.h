#pragma once
#include "../FileType.h"
#include "NesInternalFileInfo.h"
#include "../../Theme/IThemeFileIconFactory.h"

/// @brief File type for NES roms.
class NesFileType : public FileType
{
public:
    static const NesFileType sInstance;

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        return themeFileIconFactory->CreateGenericFileIcon(fileName);
    }

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return new NesInternalFileInfo(fastFileRef);
    }

private:
    constexpr NesFileType()
        : FileType("nes", FileTypeClassification::Game) { }
};
