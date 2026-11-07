#pragma once
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"

class IRomBrowserController;
class MaterialColorScheme;
class IFontRepository;

/// @brief The USB card reader sheet, behind the menu's "Mount microSD".
///
/// It carries the answer to the mount attempt and nothing else. When a host
/// was found it says the card is on the PC, that the drive is listed as a
/// plain "USB Drive" until the card finishes starting up and only then under
/// its own name, that there is no way back to the launcher from here, and
/// spells out the way out: eject on the PC, then power off (DS, DS Lite) or
/// leave to the HOME menu before taking the flashcart out. The absence of a
/// close hint is the point, not an oversight. When
/// no host was found it says how to retry, which is the same dance the card
/// reader always needed: the DSpico cannot report a cable on its own, so the
/// launcher only finds a host by asking the USB device stack to appear on the
/// bus.
class UsbBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(UsbBottomSheetView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    static constexpr int LINE_COUNT = 7;

    IRomBrowserController* _controller;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _lines[LINE_COUNT];

    UsbBottomSheetView(bool mounted, IRomBrowserController* controller,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
