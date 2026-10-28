#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "NdsGameDetailsBottomSheetView.h"

#define TITLE_LABEL_X           20
#define TITLE_LABEL_Y           16

#define CHIPS_X                 20
#define CHIPS_Y                 40
#define CHIPS_SPACING           8

#define SAVE_FILE_LABEL_X       20
#define SAVE_FILE_LABEL_Y       68

#define SAVE_STATUS_LABEL_X     20
#define SAVE_STATUS_LABEL_Y     84

#define HINT_LABEL_X            16
#define HINT_LABEL_Y            147

NdsGameDetailsBottomSheetView::NdsGameDetailsBottomSheetView(
    SharedPtr<NdsGameDetailsViewModel> viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
    , _titleLabel(Label2DView::CreateShared(216, 16, 128, fontRepository->GetFont(FontType::Medium11)))
    , _saveFileLabel(Label2DView::CreateShared(216, 16, 160, fontRepository->GetFont(FontType::Regular10)))
    , _saveStatusLabel(Label2DView::CreateShared(216, 16, 64, fontRepository->GetFont(FontType::Medium7_5)))
    , _hintLabel(Label2DView::CreateShared(224, 16, 64, fontRepository->GetFont(FontType::Medium7_5)))
    , _cheatsChip(ChipView::CreateShared(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository))
    , _saveSlotChip(ChipView::CreateShared(md::sys::color::surfaceContainerLow, materialColorScheme, fontRepository))
{
    _titleLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _titleLabel->SetText(_viewModel->GetRomFileInfo().GetFileName());
    AddChildTail(_titleLabel.GetPointer());

    _saveFileLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _saveFileLabel->SetText("");
    AddChildTail(_saveFileLabel.GetPointer());

    _saveStatusLabel->SetText("");
    AddChildTail(_saveStatusLabel.GetPointer());

    _hintLabel->SetText("");
    AddChildTail(_hintLabel.GetPointer());

    _cheatsChip->SetText(u"Cheats");
    _cheatsChip->SetAction([] (ChipView*, void* arg)
    {
        ((NdsGameDetailsBottomSheetView*)arg)->_viewModel->ShowCheats();
    }, this);
    AddChildTail(_cheatsChip.GetPointer());

    _saveSlotChip->SetText(u"Save slot");
    _saveSlotChip->SetAction([] (ChipView*, void* arg)
    {
        ((NdsGameDetailsBottomSheetView*)arg)->_viewModel->NextSaveSlot();
    }, this);
    AddChildTail(_saveSlotChip.GetPointer());
}

void NdsGameDetailsBottomSheetView::Update()
{
    if (_viewModel->IsLoaded() && _displayedSaveSlot != (int)_viewModel->GetSaveSlot())
    {
        UpdateSaveSlotText();
    }

    BottomSheetView::Update();
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _cheatsChip->SetPosition(CHIPS_X, _position.y + CHIPS_Y);
    _saveSlotChip->SetPosition(CHIPS_X + _cheatsChip->GetWidth() + CHIPS_SPACING, _position.y + CHIPS_Y);
    _saveFileLabel->SetPosition(SAVE_FILE_LABEL_X, _position.y + SAVE_FILE_LABEL_Y);
    _saveStatusLabel->SetPosition(SAVE_STATUS_LABEL_X, _position.y + SAVE_STATUS_LABEL_Y);
    _hintLabel->SetPosition(HINT_LABEL_X, _position.y + HINT_LABEL_Y);
}

void NdsGameDetailsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _saveFileLabel->SetBackgroundColor(backColor);
        _saveFileLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _saveStatusLabel->SetBackgroundColor(backColor);
        // A slot without a save starts the game fresh, which is easy to miss when it is
        // picked by accident, so that one stands out.
        _saveStatusLabel->SetForegroundColor(
            _viewModel->HasSaveSlots() && !_viewModel->SaveFileExists()
                ? _materialColorScheme->primary
                : _materialColorScheme->onSurfaceVariant);
        _hintLabel->SetBackgroundColor(backColor);
        _hintLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

SharedPtr<View> NdsGameDetailsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (currentFocus.GetPointer() == _cheatsChip.GetPointer() && direction == FocusMoveDirection::Right)
        return _saveSlotChip;
    else if (currentFocus.GetPointer() == _saveSlotChip.GetPointer() && direction == FocusMoveDirection::Left)
        return _cheatsChip;
    return nullptr;
}

bool NdsGameDetailsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B) || inputProvider.Triggered(InputKey::Y))
    {
        _viewModel->Close();
        return true;
    }
    else if (inputProvider.Triggered(InputKey::L))
    {
        _viewModel->PreviousSaveSlot();
        return true;
    }
    else if (inputProvider.Triggered(InputKey::R))
    {
        _viewModel->NextSaveSlot();
        return true;
    }
    return false;
}

void NdsGameDetailsBottomSheetView::Close()
{
    _viewModel->Close();
}

void NdsGameDetailsBottomSheetView::UpdateSaveSlotText()
{
    u32 saveSlot = _viewModel->GetSaveSlot();
    _displayedSaveSlot = saveSlot;

    if (!_viewModel->HasSaveSlots())
    {
        _saveSlotChip->SetText(u"No save slots");
        _saveFileLabel->SetText("");
        _saveStatusLabel->SetText(u"Homebrew and DSiWare keep their own saves");
        _hintLabel->SetText("");
        return;
    }

    char16_t chipText[] = u"Save slot 0";
    chipText[sizeof(chipText) / sizeof(chipText[0]) - 2] = u'0' + saveSlot;
    _saveSlotChip->SetText(chipText);

    char saveFileName[6 + 256];
    if (!_viewModel->GetSaveFileName(saveFileName, sizeof(saveFileName)))
    {
        saveFileName[0] = 0;
    }
    _saveFileLabel->SetText(saveFileName);
    _saveStatusLabel->SetText(_viewModel->SaveFileExists()
        ? u"Existing save"
        : u"New save, created on first launch");
    _hintLabel->SetText(u"L/R: Change save slot");
}
