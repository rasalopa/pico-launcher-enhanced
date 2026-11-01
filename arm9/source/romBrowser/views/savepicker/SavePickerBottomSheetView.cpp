#include "common.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "cheatSelector.h"
#include "SavePickerBottomSheetView.h"

// Placed from the sheet's top edge, which rests at y 32 once the sheet is open.
#define TITLE_X             20
#define TITLE_Y             16

// After the title, where the cheats sheet names its category.
#define GAME_NAME_X         (TITLE_X + 43)
#define GAME_NAME_Y         (TITLE_Y + 1)

// Two cells per row over the 224 px the recents list uses, rows as tall as its rows:
// the five rows ten saves take end at the bottom of the screen.
#define ENTRIES_X           16
#define CELL_WIDTH          112
#define ROWS_Y              40

SavePickerBottomSheetView::SavePickerBottomSheetView(SharedPtr<SavePickerViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
    , _entryCount(_viewModel->GetEntryCount())
{
    _titleLabel = Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Medium11));
    _titleLabel->SetText("Saves");
    AddChildTail(_titleLabel.GetPointer());

    // a random pick opens this too, so the game it is for has to show
    _gameNameLabel = Label2DView::CreateShared(153, 16, 128, fontRepository->GetFont(FontType::Regular10));
    _gameNameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _gameNameLabel->SetText(_viewModel->GetGameName());
    AddChildTail(_gameNameLabel.GetPointer());

    for (u32 i = 0; i < _entryCount; i++)
    {
        char name[16];
        _viewModel->FormatEntry(i, name, sizeof(name));
        _entries[i] = MenuItemView::CreateShared(this, i, CELL_WIDTH, name, false,
            materialColorScheme->primary, materialColorScheme, fontRepository);
        AddChildTail(_entries[i].GetPointer());
    }
}

void SavePickerBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (!objVramManager)
        return;
    u32 selectorVramOffset = objVramManager->Alloc(cheatSelectorTilesLen);
    dma_ntrCopy32(3, cheatSelectorTiles, objVramManager->GetVramAddress(selectorVramOffset),
        cheatSelectorTilesLen);
    for (u32 i = 0; i < _entryCount; i++)
        _entries[i]->SetSelectorVramOffset(selectorVramOffset);
}

void SavePickerBottomSheetView::Update()
{
    int y = _position.y;
    _titleLabel->SetPosition(TITLE_X, y + TITLE_Y);
    _gameNameLabel->SetPosition(GAME_NAME_X, y + GAME_NAME_Y);
    for (u32 i = 0; i < _entryCount; i++)
        _entries[i]->SetPosition(ENTRIES_X + (i % 2) * CELL_WIDTH, y + ROWS_Y + (i / 2) * MenuItemView::HEIGHT);
    BottomSheetView::Update();
}

void SavePickerBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _gameNameLabel->SetBackgroundColor(backColor);
        _gameNameLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool SavePickerBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

SharedPtr<View> SavePickerBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    u32 idx = 0;
    while (idx < _entryCount && currentFocus.GetPointer() != _entries[idx].GetPointer())
        idx++;
    if (idx == _entryCount)
        return nullptr;

    switch (direction)
    {
        case FocusMoveDirection::Left:
            if (idx % 2 == 1)
                return _entries[idx - 1];
            break;
        case FocusMoveDirection::Right:
            if (idx % 2 == 0 && idx + 1 < _entryCount)
                return _entries[idx + 1];
            break;
        case FocusMoveDirection::Up:
            if (idx >= 2)
                return _entries[idx - 2];
            break;
        case FocusMoveDirection::Down:
            if (idx + 2 < _entryCount)
                return _entries[idx + 2];
            // from the right column onto a last row with a single entry
            if (idx % 2 == 1 && idx + 1 < _entryCount)
                return _entries[idx + 1];
            break;
    }
    return nullptr;
}

void SavePickerBottomSheetView::Focus(FocusManager& focusManager)
{
    focusManager.Focus(_entries[_viewModel->GetInitialEntry()]);
}

void SavePickerBottomSheetView::Activate(int index)
{
    _viewModel->ActivateEntry(index);
}

void SavePickerBottomSheetView::Close()
{
    _viewModel->Close();
}
