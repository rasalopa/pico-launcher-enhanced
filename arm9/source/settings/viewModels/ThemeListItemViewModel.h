#pragma once
#include "core/SharedPtr.h"
#include "romBrowser/viewModels/IRomBrowserItemViewModel.h"
#include "settings/ThemeInfoManager.h"
#include "themes/ThemeInfo.h"

class ISettingsController;

class ThemeListItemViewModel : public IRomBrowserItemViewModel
{
public:
    explicit ThemeListItemViewModel(ISettingsController* settingsController)
        : _settingsController(settingsController) { }

    void Activate() override;

    void ShowGameInfo() override { }

    void ToggleFavorite() override { }

    void ToggleCompleted() override { }

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

    const ThemeInfoManager::ExtraThemeInfo* GetExtraThemeInfo() const
    {
        return _extraThemeInfo.GetPointer();
    }

    void SetExtraThemeInfo(SharedPtr<ThemeInfoManager::ExtraThemeInfo> extraThemeInfo)
    {
        _extraThemeInfo = std::move(extraThemeInfo);
    }

private:
    int _index = -1;
    QueueTask<void> _queueTask;
    SharedPtr<ThemeInfoManager::ExtraThemeInfo> _extraThemeInfo;

    ISettingsController* _settingsController;
};
