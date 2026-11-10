#pragma once
#include "core/SharedPtr.h"
#include "gui/Alignment.h"
#include "gui/views/KeyboardView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/views/BottomSheetView.h"

class IRomBrowserController;
class MaterialColorScheme;
class IFontRepository;

/// @brief The parental controls sheet reached from the menu.
///
/// With no PIN it opens on setup; with a PIN and the lock on it asks for that
/// PIN first. Once unlocked it lists the master switch, one permission for each
/// action, Kid saves, and Change PIN. The PIN is typed on the shared KeyboardView and must
/// be entered twice when it is created or changed. The sheet owns only its
/// working buffers; the stored PIN and switches live in AppSettings.
class ParentalControlsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(ParentalControlsBottomSheetView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;
    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    enum class Mode
    {
        SetupPin,
        ConfirmPin,
        Unlock,
        Settings,
        ChangePin,
        ConfirmChangePin
    };

public:
    static constexpr int kRowCount = 9;
    static constexpr int kFeatureCount = 6;

private:
    static constexpr u32 kMaxPinLength = 8;
    static constexpr u32 kMinPinLength = 4;

    IRomBrowserController* _controller;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _promptLabel;
    SharedPtr<Label2DView> _pinLabel;
    SharedPtr<Label2DView> _rowLabels[kRowCount];
    SharedPtr<Label2DView> _stateLabels[kRowCount];
    SharedPtr<KeyboardView> _keyboard;
    SharedPtr<Label2DView> _hintLabel;
    Mode _mode;
    int _row = 0;
    char _pin[kMaxPinLength + 1] = {};
    char _firstPin[kMaxPinLength + 1] = {};
    char _maskedPin[kMaxPinLength + 1] = {};
    char _message[48] = {};

    ParentalControlsBottomSheetView(IRomBrowserController* controller,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    bool IsTyping() const
    {
        return _mode != Mode::Settings;
    }

    void ApplyMode(Mode mode, FocusManager* focusManager);
    void SubmitPin(FocusManager& focusManager);
    void ActivateRow(FocusManager& focusManager);
    void Refresh();
    void RefreshPin();
    static void OnKeyboardTextChanged(void* arg);

    SharedPtr<Label2DView> AddLabel(const IFontRepository* fontRepository, FontType fontType,
        u32 width, u32 height, u32 maxChars, const char* text, Alignment alignment);
};
