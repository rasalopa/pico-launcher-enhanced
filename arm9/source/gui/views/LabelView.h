#pragma once
#include "common.h"
#include <memory>
#include "gui/font/nitroFont2.h"
#include "core/task/TaskQueue.h"
#include "gui/Alignment.h"
#include "gui/materialDesign.h"
#include "gui/views/View.h"
#include "core/math/Rgb.h"

class StackVramManager;
class MaterialGraphicsContext;

class LabelView : public View
{
public:
    enum class EllipsisStyle
    {
        None,
        Ellipsis,
        Marquee
    };

    void Update() override;

    void SetText(const char* text);
    void SetText(const char16_t* text);
    void SetText(const char16_t* text, u32 length);
    QueueTask<void> SetTextAsync(TaskQueueBase* taskQueue, const char* text);
    QueueTask<void> SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text);
    QueueTask<void> SetTextAsync(TaskQueueBase* taskQueue, const char16_t* text, u32 length);

    void SetHorizontalAlignment(Alignment alignment)
    {
        _hAlign = alignment;
    }

    u32 GetStringWidth() const { return _stringWidth; }

    void SetBackgroundColor(const Rgb<8, 8, 8>& backgroundColor)
    {
        _backgroundColor = backgroundColor;
    }

    void SetForegroundColor(const Rgb<8, 8, 8>& foregroundColor)
    {
        _foregroundColor = foregroundColor;
    }

    Rectangle GetBounds() const override
    {
        return Rectangle(_position, _width, _height);
    }

    void SetEllipsisStyle(EllipsisStyle ellipsisStyle)
    {
        if (_ellipsisStyle != ellipsisStyle)
        {
            _ellipsisStyle = ellipsisStyle;
            _ellipsisStyleChanged = true;
        }
    }

protected:
    u32 _width;
    u32 _height;
    u32 _actualWidth;
    u32 _actualHeight;
    u32 _maxStringLength;
    std::unique_ptr<char16_t[]> _textBuffer;
    u32 _tileBufferSize;
    std::unique_ptr<u8[]> _tileBuffer;
    const nft2_header_t* _font;
    Alignment _hAlign;
    u32 _stringWidth;
    u32 _newStringWidth;
    Rgb<8, 8, 8> _backgroundColor;
    Rgb<8, 8, 8> _foregroundColor;
    int _paletteRow = -1;
    EllipsisStyle _ellipsisStyle = EllipsisStyle::None;
    bool _a5i3;
    /// Set while a synchronous SetText() is rebuilding the buffers. The main
    /// thread preempts the io thread, so its Update() and VBlank() must not
    /// touch the label until this is clear.
    volatile bool _ioBusy = false;

    LabelView(u32 width, u32 height, u32 maxStringLength, const nft2_header_t* font, bool a5i3);

    void SetTextBuffer(const char* text);
    void SetTextBuffer(const char16_t* text);
    void SetTextBuffer(const char16_t* text, u32 length);
    virtual void UpdateTileBuffer();
    QueueTask<void> UpdateTileBufferAsync(TaskQueueBase* taskQueue);

private:
    enum class MarqueeState
    {
        StartWait,
        Moving,
        EndWait
    };

    MarqueeState _marqueeState = MarqueeState::StartWait;
    int _marqueeOffset = 0;
    int _marqueeCounter = 0;
    bool _ellipsisStyleChanged = false;

    void RestartMarquee();
    void UpdateMarquee();
};