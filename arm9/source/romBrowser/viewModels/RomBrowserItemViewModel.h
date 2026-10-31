#pragma once
#include "IRomBrowserItemViewModel.h"

class IRomBrowserController;
class FileInfoManager;

class RomBrowserItemViewModel : public IRomBrowserItemViewModel
{
public:
    explicit RomBrowserItemViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController) { }

    void Activate() override;
    void ShowGameInfo() override;
    void ToggleFavorite() override;
    void ToggleCompleted() override;

    void SetIndex(int index) override
    {
        _index = index;
    }

    int GetIndex() const override
    {
        return _index;
    }

    void SetQueueTask(QueueTask<void> queueTask) override
    {
        _queueTask = std::move(queueTask);
    }

    bool CancelQueueTask() override
    {
        return _queueTask.CancelTask();
    }

    void DisposeQueueTaskWhenComplete() override
    {
        if (_queueTask.GetTask().IsCompleted())
        {
            _queueTask.Dispose();
        }
    }

private:
    int _index = -1;
    QueueTask<void> _queueTask;

    IRomBrowserController* _romBrowserController;

    const char* GetGameCode(FileInfoManager& fileInfoManager) const;
};
