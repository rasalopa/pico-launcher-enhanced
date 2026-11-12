#pragma once
#include "gui/views/ViewContainer.h"
#include "../FileType/FileIcon.h"

class TaskQueueBase;

class BannerView : public ViewContainer
{
public:
    void InitVram(const VramContext& vramContext) override;

    void SetFileName(const TCHAR* fileName, bool useAsTitle)
    {
        SetFileNameAsync(nullptr, fileName, useAsTitle);
    }

    virtual void SetFileNameAsync(TaskQueueBase* taskQueue, const TCHAR* fileName, bool useAsTitle);

    void SetGameTitle(const char16_t* gameTitle)
    {
        SetGameTitleAsync(nullptr, gameTitle);
    }

    void SetGameTitleAsync(TaskQueueBase* taskQueue, const char16_t* gameTitle);

    /// @brief Shows the publisher from the header on the second line, under the
    ///        title. Does nothing when there is no publisher to show.
    void SetPublisher(const char16_t* publisher)
    {
        SetPublisherAsync(nullptr, publisher);
    }

    void SetPublisherAsync(TaskQueueBase* taskQueue, const char16_t* publisher)
    {
        if (!publisher || publisher[0] == 0)
            return;
        if (_lines < 2)
            _lines = 2;
        SetSecondLineAsync(taskQueue, publisher);
    }

    void SetIcon(std::unique_ptr<FileIcon> icon)
    {
        _icon = std::move(icon);
        if (_icon)
        {
            _icon->SetVramAddress(_iconVram, _iconVramOffset);
        }
    }

    void UploadIconGraphics() const
    {
        if (_icon)
        {
            _icon->UploadGraphics();
        }
    }

protected:
    std::unique_ptr<FileIcon> _icon = nullptr;
    vu16* _iconVram = nullptr;
    u32 _iconVramOffset = 0;
    u32 _lines = 3;

    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char* firstLine, bool ellipsis) = 0;
    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, bool ellipsis) = 0;
    virtual void SetFirstLineAsync(TaskQueueBase* taskQueue, const char16_t* firstLine, u32 length, bool ellipsis) = 0;
    virtual void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine) = 0;
    virtual void SetSecondLineAsync(TaskQueueBase* taskQueue, const char16_t* secondLine, u32 length) = 0;
    virtual void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine) = 0;
    virtual void SetThirdLineAsync(TaskQueueBase* taskQueue, const char16_t* thirdLine, u32 length) = 0;
};
