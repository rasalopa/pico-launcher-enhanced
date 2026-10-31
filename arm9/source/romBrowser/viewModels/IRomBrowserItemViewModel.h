#pragma once
#include "core/task/TaskQueue.h"

class IRomBrowserItemViewModel
{
public:
    virtual ~IRomBrowserItemViewModel() = default;

    virtual void Activate() = 0;
    virtual void ShowGameInfo() = 0;
    virtual void ToggleFavorite() = 0;
    virtual void ToggleCompleted() = 0;
    virtual void SetIndex(int index) = 0;
    /// @brief Item index this view model is currently bound to, -1 when
    ///        unbound. Views are pooled: input state armed on one item must
    ///        be revalidated against this after any rebind.
    virtual int GetIndex() const = 0;
    virtual void SetQueueTask(QueueTask<void> queueTask) = 0;
    /// @brief Cancels the bound load task. Returns false when the task is already
    ///        running on the io thread and may still be using the item's data.
    virtual bool CancelQueueTask() = 0;
    virtual void DisposeQueueTaskWhenComplete() = 0;

protected:
    IRomBrowserItemViewModel() = default;
};
