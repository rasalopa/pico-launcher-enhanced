#pragma once
#include "SdFolderSortType.h"
#include "SdFolderSortDirection.h"

class IGameDataService;

class SdFolderFilterSortParams
{
public:
    SdFolderSortType sortType = SdFolderSortType::Name;
    SdFolderSortDirection sortDirection = SdFolderSortDirection::Ascending;
    bool includeHiddenFiles = false;
    /// @brief When true only folders and favorite games are kept.
    ///        Requires gameDataService.
    bool favoritesOnly = false;
    /// @brief When true only folders and completed games are kept.
    ///        Requires gameDataService.
    bool completedOnly = false;
    /// @brief When true, folders with no visible content are dropped. Relies
    ///        on FileInfo::IsEmptyFolder() already being populated (see
    ///        SdFolderFactory::HasVisibleContent) - this flag itself does no SD
    ///        access. That answer ignores favoritesOnly/completedOnly on
    ///        purpose, so it stays valid when a filter is toggled without
    ///        reloading the folder.
    bool hideEmptyFolders = false;
    /// @brief When true, folders named "saves" are dropped: with the saves folder setting
    ///        on they hold save files the browser has nothing to show for.
    bool hideSavesFolders = false;
    const IGameDataService* gameDataService = nullptr;

    SdFolderFilterSortParams() { }

    SdFolderFilterSortParams(SdFolderSortType sortType, SdFolderSortDirection sortDirection, bool includeHiddenFiles)
        : sortType(sortType), sortDirection(sortDirection), includeHiddenFiles(includeHiddenFiles) { }
};
