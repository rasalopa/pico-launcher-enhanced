#include "common.h"
#include <ctype.h>
#include "core/task/TaskQueue.h"
#include "IRomBrowserController.h"
#include "FileType/FileType.h"
#include "services/settings/RomBrowserDisplaySettings.h"
#include "FileInfoManager.h"
#include "FileRecyclerAdapter.h"

/// @brief The character a name is filed under. Folded the same way the sort
///        compares names, so the groups match the order on screen.
static char GetInitial(const FileInfo& fileInfo)
{
    return toupper((unsigned char)fileInfo.GetFileName()[0]);
}

/// @brief Folders are listed before files whatever the sort is, so the two
///        blocks are stepped through separately even when a folder and a file
///        share an initial.
static bool IsFolder(const FileInfo& fileInfo)
{
    return fileInfo.GetFileType()->GetClassification() == FileTypeClassification::Folder;
}

u32 FileRecyclerAdapter::GetItemCount() const
{
    return _fileInfoManager->GetItemCount();
}

int FileRecyclerAdapter::GetBigStepTarget(int fromIdx, int direction) const
{
    auto sortMode = _romBrowserController->GetRomBrowserDisplaySettings().sortMode;
    if (sortMode != RomBrowserSortMode::NameAscending && sortMode != RomBrowserSortMode::NameDescending)
    {
        return -1; // sorted by date, where initials are in no particular order
    }

    int itemCount = (int)_fileInfoManager->GetItemCount();
    if (fromIdx < 0 || fromIdx >= itemCount)
    {
        return -1;
    }

    auto sameGroup = [this] (int a, int b)
    {
        const auto& itemA = _fileInfoManager->GetItem(a);
        const auto& itemB = _fileInfoManager->GetItem(b);
        return GetInitial(itemA) == GetInitial(itemB) && IsFolder(itemA) == IsFolder(itemB);
    };

    if (direction > 0)
    {
        int next = fromIdx;
        while (next + 1 < itemCount && sameGroup(next + 1, fromIdx))
        {
            next++;
        }
        if (next + 1 < itemCount)
        {
            return next + 1;
        }
        // Nothing filed after this: settle on the last item so the end of a long
        // folder is still one press away.
        return fromIdx == itemCount - 1 ? -1 : itemCount - 1;
    }

    // Going back lands on the top of the group first, so getting to the start of
    // a long run of the same initial does not need a detour.
    int groupStart = fromIdx;
    while (groupStart > 0 && sameGroup(groupStart - 1, fromIdx))
    {
        groupStart--;
    }

    if (groupStart != fromIdx)
    {
        return groupStart;
    }
    if (groupStart == 0)
    {
        return -1; // already at the very start
    }

    // At the top of a group: go to the top of the one before it.
    int previousStart = groupStart - 1;
    while (previousStart > 0 && sameGroup(previousStart - 1, groupStart - 1))
    {
        previousStart--;
    }
    return previousStart;
}

void FileRecyclerAdapter::OnBigStepJump() const
{
    _romBrowserController->NotifyBigStepJump();
}

void FileRecyclerAdapter::BindView(SharedPtr<View> view, int index) const
{
    LOG_DEBUG("Binding %d\n", index);
    auto queueTask = _taskQueue->Enqueue([=, this] (const vu8& cancelRequested)
    {
        if (cancelRequested)
        {
            LOG_DEBUG("Task to load %d was canceled\n", index);
            return TaskResult<void>::Canceled();
        }

        LOG_DEBUG("Started task to load %d\n", index);
        _fileInfoManager->LoadFileInfo(index);
        auto internalFileInfo = _fileInfoManager->GetInternalFileInfo(index);
        if (cancelRequested)
        {
            _fileInfoManager->ReleaseFileInfo(index);
            return TaskResult<void>::Canceled();
        }
        auto result = BindView(view, index, internalFileInfo, cancelRequested);
        if (cancelRequested)
        {
            // ReleaseView left the release to this task because it was running
            _fileInfoManager->ReleaseFileInfo(index);
            return TaskResult<void>::Canceled();
        }
        return result;
    });
    SetQueueTask(view, std::move(queueTask));
}
