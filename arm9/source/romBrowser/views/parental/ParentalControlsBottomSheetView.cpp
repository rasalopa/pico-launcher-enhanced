#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "romBrowser/IRomBrowserController.h"
#include "ParentalControlsBottomSheetView.h"

// Placed from the sheet's top edge, which rests at y 32 once the sheet is open.
#define TITLE_X             20
#define TITLE_Y             10

#define PROMPT_X            20
#define PROMPT_Y            30
#define PROMPT_WIDTH        224
#define PROMPT_MAX_CHARS    48

// The PIN is masked; this line sits under the prompt.
#define PIN_X               20
#define PIN_Y               48
#define PIN_WIDTH           224

// Eight compact rows: the master switch, six permissions and Change PIN.
#define ROW_X               20
#define ROW_Y               36
#define ROW_STEP            14
#define ROW_WIDTH           224
#define ROW_NAME_WIDTH      150
#define ROW_STATE_X         174
#define ROW_STATE_WIDTH     50

// The shared keyboard, below the prompt while a PIN is being typed.
#define KEYBOARD_Y          76

#define HINT_X              20
#define HINT_Y              138
#define HINT_WIDTH          224
#define HINT_MAX_CHARS      64

static const char* const kRowNames[ParentalControlsBottomSheetView::kRowCount] =
{
    "Restricted mode",
    "Allow deleting games",
    "Allow cheats",
    "Allow save slots",
    "Allow favourites",
    "Allow completion",
    "Allow game info",
    "Change PIN"
};

static const ParentalFeature kFeatures[ParentalControlsBottomSheetView::kFeatureCount] =
{
    ParentalFeature::Delete,
    ParentalFeature::Cheats,
    ParentalFeature::SaveSlots,
    ParentalFeature::Favorites,
    ParentalFeature::Completion,
    ParentalFeature::GameInfo
};

#define SETTINGS_HINT       "A change  B back  Start close"
#define PIN_HINT            "A type  B del  Select next  Start cancel"

ParentalControlsBottomSheetView::ParentalControlsBottomSheetView(
    IRomBrowserController* controller, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _controller(controller)
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel = AddLabel(fontRepository, FontType::Medium11,
        180, 16, 20, "Parental controls", Alignment::Start);
    _promptLabel = AddLabel(fontRepository, FontType::Regular10,
        PROMPT_WIDTH, 16, PROMPT_MAX_CHARS, "", Alignment::Start);
    _pinLabel = AddLabel(fontRepository, FontType::Medium11,
        PIN_WIDTH, 16, kMaxPinLength, "", Alignment::Start);

    for (int i = 0; i < kRowCount; i++)
    {
        _rowLabels[i] = AddLabel(fontRepository, FontType::Regular10,
            ROW_NAME_WIDTH, 14, 24, kRowNames[i], Alignment::Start);
        _stateLabels[i] = AddLabel(fontRepository, FontType::Medium7_5,
            ROW_STATE_WIDTH, 14, 8, "", Alignment::End);
    }

    _keyboard = KeyboardView::CreateShared(fontRepository->GetFont(FontType::Medium11));
    _keyboard->SetBuffer(_pin, kMaxPinLength);
    _keyboard->SetTextChangedCallback(&ParentalControlsBottomSheetView::OnKeyboardTextChanged, this);
    _keyboard->SetVisible(false);
    AddChildTail(_keyboard.GetPointer());

    _hintLabel = AddLabel(fontRepository, FontType::Medium7_5,
        HINT_WIDTH, 16, HINT_MAX_CHARS, SETTINGS_HINT, Alignment::Start);

    const auto& parentalControls = _controller->GetParentalControls();
    if (!parentalControls.HasPin())
        _mode = Mode::SetupPin;
    else if (parentalControls.enabled)
        _mode = Mode::Unlock;
    else
        _mode = Mode::Settings;
    ApplyMode(_mode, nullptr);
}

SharedPtr<Label2DView> ParentalControlsBottomSheetView::AddLabel(
    const IFontRepository* fontRepository, FontType fontType,
    u32 width, u32 height, u32 maxChars, const char* text, Alignment alignment)
{
    auto label = Label2DView::CreateShared(width, height, maxChars, fontRepository->GetFont(fontType));
    label->SetHorizontalAlignment(alignment);
    label->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    label->SetText(text);
    AddChildTail(label.GetPointer());
    return label;
}

void ParentalControlsBottomSheetView::ApplyMode(Mode mode, FocusManager* focusManager)
{
    _mode = mode;
    _message[0] = 0;
    _keyboard->SetVisible(IsTyping());
    if (IsTyping())
    {
        _pin[0] = 0;
        _maskedPin[0] = 0;
        _hintLabel->SetText(PIN_HINT);
        _promptLabel->SetText(_mode == Mode::SetupPin ? "Create a 4-8 character PIN"
            : _mode == Mode::ConfirmPin ? "Re-enter the PIN"
            : _mode == Mode::ChangePin ? "Enter the new PIN"
            : _mode == Mode::ConfirmChangePin ? "Re-enter the new PIN"
            : "Enter the PIN");
        _keyboard->SetBuffer(_pin, kMaxPinLength);
        if (focusManager)
            _keyboard->Focus(*focusManager);
    }
    else
    {
        _hintLabel->SetText(SETTINGS_HINT);
        _promptLabel->SetText("");
        _pinLabel->SetText("");
        if (focusManager)
            focusManager->Focus(_rowLabels[_row]);
    }
    Refresh();
}

void ParentalControlsBottomSheetView::RefreshPin()
{
    u32 length = strlen(_pin);
    for (u32 i = 0; i < length; i++)
        _maskedPin[i] = '*';
    _maskedPin[length] = 0;
    _pinLabel->SetText(_maskedPin);
}

void ParentalControlsBottomSheetView::Refresh()
{
    const auto& parentalControls = _controller->GetParentalControls();
    if (IsTyping())
    {
        RefreshPin();
        return;
    }

    _stateLabels[0]->SetText(parentalControls.enabled ? "On" : "Off");
    for (int i = 0; i < kFeatureCount; i++)
        _stateLabels[i + 1]->SetText(parentalControls.IsAllowed(kFeatures[i]) ? "On" : "Off");
    _stateLabels[kRowCount - 1]->SetText("");
}

void ParentalControlsBottomSheetView::OnKeyboardTextChanged(void* arg)
{
    ((ParentalControlsBottomSheetView*)arg)->RefreshPin();
}

void ParentalControlsBottomSheetView::SubmitPin(FocusManager& focusManager)
{
    u32 length = strlen(_pin);
    if (length < kMinPinLength)
    {
        StringUtil::Copy(_message, "PIN must be 4 to 8 characters", sizeof(_message));
        _promptLabel->SetText(_message);
        return;
    }

    if (_mode == Mode::Unlock)
    {
        if (_controller->VerifyParentalPin(_pin))
        {
            _row = 0;
            ApplyMode(Mode::Settings, &focusManager);
        }
        else
        {
            StringUtil::Copy(_message, "Wrong PIN", sizeof(_message));
            _promptLabel->SetText(_message);
            _pin[0] = 0;
            RefreshPin();
        }
        return;
    }

    if (_mode == Mode::SetupPin || _mode == Mode::ChangePin)
    {
        StringUtil::Copy(_firstPin, _pin, sizeof(_firstPin));
        ApplyMode(_mode == Mode::SetupPin ? Mode::ConfirmPin : Mode::ConfirmChangePin, &focusManager);
        return;
    }

    if (strcmp(_pin, _firstPin) != 0)
    {
        StringUtil::Copy(_message, "PINs did not match", sizeof(_message));
        _promptLabel->SetText(_message);
        _mode = _mode == Mode::ConfirmPin ? Mode::SetupPin : Mode::ChangePin;
        _firstPin[0] = 0;
        _pin[0] = 0;
        RefreshPin();
        return;
    }

    _controller->SetParentalPin(_pin);
    if (_mode == Mode::ConfirmPin)
    {
        _controller->SetParentalEnabled(true);
        _row = 0;
        ApplyMode(Mode::Settings, &focusManager);
    }
    else
    {
        _row = kRowCount - 1;
        ApplyMode(Mode::Settings, &focusManager);
    }
}

void ParentalControlsBottomSheetView::ActivateRow(FocusManager& focusManager)
{
    const auto& parentalControls = _controller->GetParentalControls();

    if (_row == 0)
    {
        if (!parentalControls.enabled && !parentalControls.HasPin())
        {
            ApplyMode(Mode::SetupPin, &focusManager);
            return;
        }
        _controller->SetParentalEnabled(!parentalControls.enabled);
    }
    else if (_row >= 1 && _row <= kFeatureCount)
    {
        ParentalFeature feature = kFeatures[_row - 1];
        _controller->SetParentalFeatureAllowed(feature, !parentalControls.IsAllowed(feature));
    }
    else
    {
        _firstPin[0] = 0;
        ApplyMode(Mode::ChangePin, &focusManager);
        return;
    }
    Refresh();
}

void ParentalControlsBottomSheetView::Update()
{
    int y = _position.y;
    _titleLabel->SetPosition(TITLE_X, y + TITLE_Y);
    _promptLabel->SetPosition(PROMPT_X, y + PROMPT_Y);
    _pinLabel->SetPosition(PIN_X, y + PIN_Y);
    for (int i = 0; i < kRowCount; i++)
    {
        _rowLabels[i]->SetPosition(ROW_X, y + ROW_Y + i * ROW_STEP);
        _stateLabels[i]->SetPosition(ROW_X + ROW_STATE_X, y + ROW_Y + i * ROW_STEP);
    }
    _keyboard->SetPosition(0, y + KEYBOARD_Y);
    _hintLabel->SetPosition(HINT_X, y + HINT_Y);
    BottomSheetView::Update();
}

void ParentalControlsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto back = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        _titleLabel->SetBackgroundColor(back);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _promptLabel->SetBackgroundColor(back);
        _promptLabel->SetForegroundColor(_message[0] ? _materialColorScheme->primary
            : _materialColorScheme->onSurfaceVariant);
        _pinLabel->SetBackgroundColor(back);
        _pinLabel->SetForegroundColor(_materialColorScheme->primary);

        for (int i = 0; i < kRowCount; i++)
        {
            bool selected = !IsTyping() && i == _row;
            _rowLabels[i]->SetBackgroundColor(selected ? _materialColorScheme->primary : back);
            _rowLabels[i]->SetForegroundColor(selected ? _materialColorScheme->onPrimary
                : _materialColorScheme->onSurface);
            _stateLabels[i]->SetBackgroundColor(selected ? _materialColorScheme->primary : back);
            _stateLabels[i]->SetForegroundColor(selected ? _materialColorScheme->onPrimary
                : _materialColorScheme->onSurfaceVariant);
            if (IsTyping())
            {
                _rowLabels[i]->SetText("");
                _stateLabels[i]->SetText("");
            }
            else
            {
                _rowLabels[i]->SetText(kRowNames[i]);
            }
        }

        KeyboardView::Colors keyColors;
        keyColors.background = back;
        keyColors.text = _materialColorScheme->onSurface;
        keyColors.specialText = _materialColorScheme->onSurfaceVariant;
        keyColors.focusedBackground = _materialColorScheme->primary;
        keyColors.focusedText = _materialColorScheme->onPrimary;
        _keyboard->SetColors(keyColors);

        _hintLabel->SetBackgroundColor(back);
        _hintLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool ParentalControlsBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (IsTyping())
    {
        if (inputProvider.Triggered(InputKey::Start))
        {
            Close();
            return true;
        }
        if (inputProvider.Triggered(InputKey::Select))
        {
            SubmitPin(focusManager);
            return true;
        }
        return false;
    }

    if (inputProvider.Triggered(InputKey::B))
    {
        Close();
        return true;
    }
    if (inputProvider.Triggered(InputKey::A))
    {
        ActivateRow(focusManager);
        return true;
    }
    return false;
}

SharedPtr<View> ParentalControlsBottomSheetView::MoveFocus(
    const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (IsTyping())
        return nullptr;

    int row = -1;
    for (int i = 0; i < kRowCount; i++)
    {
        if (currentFocus.GetPointer() == _rowLabels[i].GetPointer() ||
            currentFocus.GetPointer() == _stateLabels[i].GetPointer())
            row = i;
    }
    if (row < 0)
        return nullptr;

    if (direction == FocusMoveDirection::Up)
        row = (row + kRowCount - 1) % kRowCount;
    else if (direction == FocusMoveDirection::Down)
        row = (row + 1) % kRowCount;
    else
        return nullptr;

    _row = row;
    return _rowLabels[row];
}

void ParentalControlsBottomSheetView::Focus(FocusManager& focusManager)
{
    if (IsTyping())
        _keyboard->Focus(focusManager);
    else
        focusManager.Focus(_rowLabels[_row]);
}

void ParentalControlsBottomSheetView::Close()
{
    _controller->HideParentalControls();
}
