#include "common.h"
#include "core/StringUtil.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "GameInfoEditBottomSheetView.h"

// Placed from the sheet's top edge, which rests at y 32 once the sheet is open.
#define TITLE_X             20
#define TITLE_Y             10

#define FIELD_Y             30
#define FIELD_STEP          17
#define FIELD_LABEL_X       20
#define FIELD_LABEL_WIDTH   54
#define FIELD_VALUE_X       76
#define FIELD_VALUE_WIDTH   164

// The shared keyboard's top edge, below the three fields.
#define KEYBOARD_Y          86

#define HINT_X              20
#define HINT_Y              138
#define HINT_WIDTH          224
#define HINT_MAX_CHARS      64

// A edits, B or Select saves and leaves.
#define ENTRY_HINT          "A edit  B back  Select back"
// A types, B deletes, X spaces and Y swaps case; the keyboard takes those.
#define KEY_HINT            "A type  B del  X space  Y case  Select done"

GameInfoEditBottomSheetView::GameInfoEditBottomSheetView(
    SharedPtr<GameInfoEditViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
{
    StringUtil::Copy(_values[0], _viewModel->GetTitle(), sizeof(_values[0]));
    StringUtil::Copy(_values[1], _viewModel->GetPublisher(), sizeof(_values[1]));
    StringUtil::Copy(_values[2], _viewModel->GetDeveloper(), sizeof(_values[2]));

    _titleLabel = AddLabel(fontRepository, FontType::Medium11,
        160, 16, 20, "Game info", Alignment::Start);
    static const char* const kFieldNames[kFieldCount] = { "Title", "Publisher", "Developer" };
    for (int i = 0; i < kFieldCount; i++)
    {
        _fieldLabels[i] = AddLabel(fontRepository, FontType::Medium7_5,
            FIELD_LABEL_WIDTH, 16, 12, kFieldNames[i], Alignment::Start);
        _valueLabels[i] = AddLabel(fontRepository, FontType::Medium7_5,
            FIELD_VALUE_WIDTH, 16, GameDataEntry::kMaxTitleLength, "", Alignment::Start);
    }

    // The text is typed on the shared keyboard, pointed at the field being
    // edited. It reports changes back so that field's line keeps up.
    _keyboard = KeyboardView::CreateShared(fontRepository->GetFont(FontType::Medium11));
    _keyboard->SetTextChangedCallback(&GameInfoEditBottomSheetView::OnKeyboardTextChanged, this);
    _keyboard->SetBuffer(_values[_field], GameDataEntry::kMaxTitleLength);
    _keyboard->SetVisible(false);
    AddChildTail(_keyboard.GetPointer());

    _hintLabel = AddLabel(fontRepository, FontType::Medium7_5, HINT_WIDTH, 16, HINT_MAX_CHARS,
        ENTRY_HINT, Alignment::Start);
    Refresh();
}

SharedPtr<Label2DView> GameInfoEditBottomSheetView::AddLabel(
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

void GameInfoEditBottomSheetView::Refresh()
{
    for (int i = 0; i < kFieldCount; i++)
        _valueLabels[i]->SetText(_values[i]);
}

void GameInfoEditBottomSheetView::OnKeyboardTextChanged(void* arg)
{
    ((GameInfoEditBottomSheetView*)arg)->Refresh();
}

void GameInfoEditBottomSheetView::BeginEditing(FocusManager& focusManager)
{
    static constexpr u32 kFieldLimits[kFieldCount] =
    {
        GameDataEntry::kMaxTitleLength,
        GameDataEntry::kMaxPublisherLength,
        GameDataEntry::kMaxDeveloperLength
    };

    _editing = true;
    _hintLabel->SetText(KEY_HINT);
    _keyboard->SetBuffer(_values[_field], kFieldLimits[_field]);
    _keyboard->SetVisible(true);
    _keyboard->Focus(focusManager);
}

void GameInfoEditBottomSheetView::EndEditing(FocusManager& focusManager)
{
    _editing = false;
    _hintLabel->SetText(ENTRY_HINT);
    _keyboard->SetVisible(false);
    focusManager.Focus(_fieldLabels[_field]);
}

void GameInfoEditBottomSheetView::SaveAndClose()
{
    _viewModel->Save(_values[0], _values[1], _values[2]);
}

void GameInfoEditBottomSheetView::Update()
{
    int y = _position.y;
    _titleLabel->SetPosition(TITLE_X, y + TITLE_Y);
    for (int i = 0; i < kFieldCount; i++)
    {
        int rowY = y + FIELD_Y + i * FIELD_STEP;
        _fieldLabels[i]->SetPosition(FIELD_LABEL_X, rowY);
        _valueLabels[i]->SetPosition(FIELD_VALUE_X, rowY);
    }
    _keyboard->SetPosition(0, y + KEYBOARD_Y);
    _hintLabel->SetPosition(HINT_X, y + HINT_Y);
    BottomSheetView::Update();
}

void GameInfoEditBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto back = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        _titleLabel->SetBackgroundColor(back);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        for (int i = 0; i < kFieldCount; i++)
        {
            bool selected = i == _field;
            _fieldLabels[i]->SetBackgroundColor(back);
            _fieldLabels[i]->SetForegroundColor(selected
                ? _materialColorScheme->primary
                : _materialColorScheme->onSurfaceVariant);
            _valueLabels[i]->SetBackgroundColor(back);
            _valueLabels[i]->SetForegroundColor(selected
                ? _materialColorScheme->primary
                : _materialColorScheme->onSurface);
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

bool GameInfoEditBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (_editing)
    {
        // Select finishes the field. A, B, X and Y are the keyboard's, and it
        // saw them first.
        if (inputProvider.Triggered(InputKey::Select))
        {
            EndEditing(focusManager);
            return true;
        }
        return false;
    }

    if (inputProvider.Triggered(InputKey::A))
    {
        BeginEditing(focusManager);
        return true;
    }
    // B and Select both store the fields and leave.
    if (inputProvider.Triggered(InputKey::B) || inputProvider.Triggered(InputKey::Select))
    {
        SaveAndClose();
        return true;
    }
    return false;
}

SharedPtr<View> GameInfoEditBottomSheetView::MoveFocus(
    const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (_editing)
        return nullptr;

    int field = -1;
    for (int i = 0; i < kFieldCount; i++)
    {
        if (currentFocus.GetPointer() == _fieldLabels[i].GetPointer())
            field = i;
    }
    if (field < 0)
        return nullptr;

    if (direction == FocusMoveDirection::Up && field > 0)
        field--;
    else if (direction == FocusMoveDirection::Down && field < kFieldCount - 1)
        field++;
    else
        return nullptr;

    _field = field;
    return _fieldLabels[_field];
}

void GameInfoEditBottomSheetView::Focus(FocusManager& focusManager)
{
    _editing = false;
    _hintLabel->SetText(ENTRY_HINT);
    _keyboard->SetVisible(false);
    focusManager.Focus(_fieldLabels[_field]);
}

void GameInfoEditBottomSheetView::Close()
{
    SaveAndClose();
}
