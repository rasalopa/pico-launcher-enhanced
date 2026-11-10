#pragma once
#include <memory>
#include "fat/Directory.h"
#include "SdFolder.h"
#include "FileType/IFileTypeProvider.h"

class SdFolderFactory
{
public:
    SdFolderFactory(const IFileTypeProvider* fileTypeProvider)
        : _fileTypeProvider(fileTypeProvider) { }

    std::unique_ptr<SdFolder> CreateFromPath(const char* path) const;

    /// @brief Starting value callers should give \p readBudget in
    ///        HasVisibleContent. Pass the SAME variable to every folder probed
    ///        during one navigation (see RomBrowserController) so the allowance
    ///        is shared across all of them, not handed out fresh per folder.
    ///        Generous on purpose: this is a backstop against a pathological
    ///        card, not the normal operating limit. Running out makes folders
    ///        fail open, i.e. empty ones become visible again, so a tight
    ///        ceiling produced exactly the bug it was meant to prevent - the
    ///        earlier value of 300 was already ~45% spent by one navigation of
    ///        a real card carrying DSi and macOS metadata folders. Reading
    ///        this many directory entries is a few tens of KB, far less than
    ///        the icons and covers the same navigation loads anyway.
    static constexpr int kInitialReadBudget = 2000;

    /// @brief Cap on what a SINGLE folder may spend, so one pathological
    ///        folder cannot starve every sibling after it. Deliberately much
    ///        larger than any real game folder needs.
    static constexpr int kPerFolderReadBudget = 512;

    /// @brief Cheap, allocation-free check for whether the folder at \p path
    ///        has any visible entry at all: not hidden, not dot-named, not
    ///        FileTypeClassification::Unknown. A subfolder only counts as
    ///        content if IT has visible content (recurses, bounded by a small
    ///        depth cap - see the .cpp for why it's kept small: this runs on
    ///        the IO task's small fixed stack).
    ///
    ///        NOT aware of the favorites/completed filters, deliberately. A
    ///        filter-aware answer cannot stop at the first game it finds, so a
    ///        folder of unmarked roms cost one read per rom and drained the
    ///        shared budget; and the answer is cached per navigation, so it
    ///        went stale the moment a filter was toggled (that only rebuilds
    ///        the view model, it does not reload the folder). "Empty" therefore
    ///        means "nothing in it", and with a filter on a listed folder may
    ///        hold nothing that matches.
    ///
    ///        \p readBudget is consumed for every directory entry actually
    ///        read; once it reaches zero (here, or already from an earlier
    ///        sibling folder sharing the same variable), this and every
    ///        following call fails open with no further I/O at all. \p path
    ///        itself, and any folder named starting with '_' found along the
    ///        way, is the launcher's own support data (matches upstream issue
    ///        #45) and is skipped for free without spending any budget on it.
    ///        Stops at the first match instead of reading the whole directory
    ///        like CreateFromPath does, so probing a folder full of games
    ///        stays cheap - only an actually-empty chain pays for the extra
    ///        reads. On failure to open/read a folder, once the budget runs
    ///        out, or past the depth cap, returns true (fail open: never hide
    ///        something we couldn't actually inspect) and logs why.
    bool HasVisibleContent(const char* path, int& readBudget) const;

private:
    bool HasVisibleContentBounded(char* pathBuffer, size_t pathBufferSize, size_t pathLength,
        FILINFO& fileInfo, int& readBudget, int& folderBudget, int depth) const;

    const IFileTypeProvider* _fileTypeProvider;
};