#pragma once
#include "core/task/TaskQueue.h"
#include "gui/views/RecyclerAdapter.h"

class FileInfoManager;
class IVramManager;
class InternalFileInfo;
class IThemeFileIconFactory;
class VramContext;
class IRomBrowserController;
class IRomBrowserItemViewModel;

class FileRecyclerAdapter : public RecyclerAdapter
{
public:
    u32 GetItemCount() const override;
    void BindView(SharedPtr<View> view, int index) const override;
    void ReleaseView(SharedPtr<View> view, int index) const override;
    void ViewsReleased() const override;

    /// @brief Jumps to where the next initial starts, so a long folder can be
    ///        crossed in a few presses instead of a page at a time. Only when
    ///        the list is sorted by name, since that is the order the jump
    ///        follows. Otherwise paging is left in place.
    int GetBigStepTarget(int fromIdx, int direction) const override;

    /// @brief Tells the controller an L/R jump happened, so the top screen shows
    ///        the letter chip for it (and not for plain d-pad moves).
    void OnBigStepJump() const override;

    void SetIconFrameCounter(u32 iconFrameCounter)
    {
        _iconFrameCounter = iconFrameCounter;
    }

    virtual void InitVram(const VramContext& vramContext) { }

protected:
    IRomBrowserController* _romBrowserController;
    FileInfoManager* _fileInfoManager;
    TaskQueueBase* _taskQueue;
    u32 _iconFrameCounter;
    const IThemeFileIconFactory* _themeFileIconFactory;

    FileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory)
        : _romBrowserController(romBrowserController), _fileInfoManager(fileInfoManager), _taskQueue(taskQueue)
        , _iconFrameCounter(0), _themeFileIconFactory(themeFileIconFactory) { }

    virtual void BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo) const = 0;
    /// @brief Undoes BindView and releases the file info of the item.
    virtual void UnbindView(const SharedPtr<View>& view, int index) const = 0;
    virtual IRomBrowserItemViewModel& GetViewModel(const SharedPtr<View>& view) const = 0;

private:
    // what the io bind task is using: ReleaseView leaves both to it
    mutable const View* volatile _bindingView = nullptr;
    mutable volatile int _bindingIndex = -1;
};
