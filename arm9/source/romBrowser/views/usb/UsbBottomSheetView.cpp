#include "common.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "romBrowser/IRomBrowserController.h"
#include "UsbBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define LINE_X              20
#define FIRST_LINE_Y        44
#define LINE_SPACING        16

#define LINE_WIDTH          216

/// What the sheet says once the card is on the PC. Short lines rather than a
/// paragraph: a sheet label does not wrap, and this is read once, at the
/// moment the user is about to pull the cable. The middle lines answer the
/// two things a first mount leaves unclear: the drive the PC lists while the
/// card is still starting up carries a generic name and only later the
/// card's own, and there is no way back to the launcher from here, which is
/// deliberate rather than a freeze.
static const char* const kMountedLines[] =
{
    "The microSD card is on your computer.",
    "The PC lists it as a plain \"USB Drive\"",
    "at first, then by its real name.",
    "That is normal. You cannot go back to",
    "the launcher; that is intended, so eject",
    "on the PC, power off or go to the HOME",
    "menu, then take the flashcart out."
};

/// The retry instructions, in the order the user has to do them: unplug, plug
/// the cable into the flashcart, start the launcher again.
static const char* const kNoHostLines[] =
{
    "No USB host found.",
    "Take the flashcart out and plug in",
    "its USB cable, then start the launcher",
    "and pick this again."
};

UsbBottomSheetView::UsbBottomSheetView(bool mounted, IRomBrowserController* controller,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _controller(controller)
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel = Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11));
    _titleLabel->SetText("USB card reader");
    AddChildTail(_titleLabel.GetPointer());

    const char* const* lines = mounted ? kMountedLines : kNoHostLines;
    for (int i = 0; i < LINE_COUNT; i++)
    {
        _lines[i] = Label2DView::CreateShared(LINE_WIDTH, 16, 48, fontRepository->GetFont(FontType::Regular10));
        _lines[i]->SetText(lines[i]);
        AddChildTail(_lines[i].GetPointer());
    }
}

void UsbBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    for (int i = 0; i < LINE_COUNT; i++)
        _lines[i]->SetPosition(LINE_X, _position.y + FIRST_LINE_Y + i * LINE_SPACING);
    BottomSheetView::Update();
}

void UsbBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);

        for (int i = 0; i < LINE_COUNT; i++)
        {
            _lines[i]->SetBackgroundColor(backColor);
            _lines[i]->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _lines[i]->Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool UsbBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A) || inputProvider.Triggered(InputKey::B))
    {
        _controller->HideUsb();
        return true;
    }
    return false;
}

void UsbBottomSheetView::Focus(FocusManager& focusManager)
{
    // focus a CHILD of the sheet: FocusManager::Update skips parent-less
    // focused views, so keys would never arrive if the sheet focused itself
    focusManager.Focus(_titleLabel->SharedFromThis());
}

void UsbBottomSheetView::Close()
{
    // pen tap outside the sheet
    _controller->HideUsb();
}
