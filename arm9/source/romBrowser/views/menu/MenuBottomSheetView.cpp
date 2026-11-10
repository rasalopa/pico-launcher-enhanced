#include "common.h"
#include <algorithm>
#include <iterator>
#include "core/mini-printf.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "core/math/RgbMixer.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "../MarkerColors.h"
#include "cheatSelector.h"
#include "recentIcon.h"
#include "smallHeartIconFilled.h"
#include "statsIcon.h"
#include "trashIcon.h"
#include "plusIcon.h"
#include "heartIcon.h"
#include "checkIcon.h"
#include "infoIcon.h"
#include "settingsIcon.h"
#include "MenuBottomSheetView.h"

// Placed from the sheet's top edge, which rests at y 32 once the sheet is open.
#define TITLE_X             20
#define TITLE_Y             16

// The about button shares the title row, where the display settings sheet
// keeps its theme button.
#define ABOUT_BUTTON_X      212
#define ABOUT_BUTTON_Y      (TITLE_Y - 7)

// Two cells per row over the 224 px the recents list uses, or one full row.
// Rows sit 24 apart so five 24 px items fit inside the sheet: New save, the
// badges filter and Parental controls each get a full row. When New save is
// not shown the rows below it move up.
#define ITEMS_X             16
#define CELL_WIDTH          112
#define ROW_WIDTH           224
#define ROWS_Y              34
#define ROW_SPACING         24

// Inside an item.
#define ICON_DX             4
#define ICON_DY             4
#define ICON_SIZE           16
#define NAME_DX             26
#define NAME_DY             4
#define STATE_WIDTH         40
#define STATE_DX            (ROW_WIDTH - 4 - STATE_WIDTH)
#define STATE_DY            7

// The recents list's highlight sprite: 64 px wide, laid end to end.
#define SELECTOR_WIDTH      64

namespace
{
    struct MenuEntry
    {
        const char* name;
        int row;
        /// 0 or 1 for a cell, -1 for a full row.
        int column;
    };
}

// In the order of MenuBottomSheetView::Item.
static constexpr MenuEntry kEntries[] =
{
    { "Recently played", 0, 0 },
    { "Favorites", 0, 1 },
    { "Statistics", 1, 0 },
    { "Delete game", 1, 1 },
    { "New save", 2, -1 },
    { "Only badges", 3, -1 },
    { "Parental controls", 4, -1 }
};

MenuItemView::MenuItemView(IMenuItemHost* sheet, int index, int width, const char* name,
    const char* stateText, const Rgb<8, 8, 8>& activeColor,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _sheet(sheet), _index(index), _width(width), _activeColor(activeColor)
    , _materialColorScheme(materialColorScheme)
{
    int nameWidth = width - NAME_DX - 4 - (stateText ? STATE_WIDTH : 0);
    _nameLabel = Label2DView::CreateShared(nameWidth, 16, 24, fontRepository->GetFont(FontType::Regular10));
    _nameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _nameLabel->SetText(name);
    AddChildTail(_nameLabel.GetPointer());
    if (stateText)
    {
        // "Save 9" is the longest
        _stateLabel = Label2DView::CreateShared(STATE_WIDTH, 16, 6, fontRepository->GetFont(FontType::Medium7_5));
        _stateLabel->SetHorizontalAlignment(Alignment::End);
        _stateLabel->SetText(stateText);
        AddChildTail(_stateLabel.GetPointer());
    }
}

void MenuItemView::Update()
{
    _nameLabel->SetPosition(_position.x + NAME_DX, _position.y + NAME_DY);
    if (_stateLabel)
        _stateLabel->SetPosition(_position.x + STATE_DX, _position.y + STATE_DY);
    ViewContainer::Update();
}

void MenuItemView::Draw(GraphicsContext& graphicsContext)
{
    // The sheet slides in from the bottom, and an oam y past the screen wraps.
    if (!graphicsContext.IsVisible(GetBounds()))
        return;

    auto back = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
    if (IsFocused())
    {
        // The recents list's highlight: a rounded bar a tenth of the way to the text colour.
        back = RgbMixer::Lerp(back, _materialColorScheme->onSurface, 10, 100);
        u32 selectorRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(back, back), _position.y, _position.y + HEIGHT);
        for (int dx = 0; dx < _width; dx += SELECTOR_WIDTH)
        {
            // the last sprite is pulled back to the edge, so the bar ends rounded
            auto oams = graphicsContext.GetOamManager().AllocOams(1);
            OamBuilder::OamWithSize<SELECTOR_WIDTH, 32>(
                    _position.x + std::min(dx, _width - SELECTOR_WIDTH), _position.y, _selectorVramOffset >> 7)
                .WithPalette16(selectorRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(oams[0]);
        }
    }

    // Faded when it cannot act, its own colour while a filter is on, plain
    // otherwise. Focus also takes the icon to the accent while the entry can
    // act, the same rule the icon buttons follow (see IconButtonView.h), on top
    // of the bar; a disabled entry keeps its faded icon. The name keeps its
    // colour, so an active filter still reads as on while focused.
    Rgb<8, 8, 8> tint = _materialColorScheme->onSurfaceVariant;
    Rgb<8, 8, 8> nameColor = _materialColorScheme->onSurface;
    if (!_enabled)
        tint = nameColor = _materialColorScheme->outline;
    else if (_active)
        tint = nameColor = _activeColor;
    if (_enabled && IsFocused())
        tint = _materialColorScheme->primary;

    _nameLabel->SetBackgroundColor(back);
    _nameLabel->SetForegroundColor(nameColor);
    if (_stateLabel)
    {
        _stateLabel->SetBackgroundColor(back);
        _stateLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
    }
    ViewContainer::Draw(graphicsContext);

    if (!_hasIcon)
        return;
    int iconY = _position.y + ICON_DY;
    u32 iconRow = graphicsContext.GetPaletteManager().AllocRow(
        GradientPalette(back, tint), iconY, iconY + ICON_SIZE);
    auto oams = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<ICON_SIZE, ICON_SIZE>(_position.x + ICON_DX, iconY, _iconVramOffset >> 7)
        .WithPalette16(iconRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(oams[0]);
}

bool MenuItemView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        _sheet->Activate(_index);
        return true;
    }
    // B goes up to the sheet
    return View::HandleInput(inputProvider, focusManager);
}

void MenuItemView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _penDown = GetBounds().Contains(touchPoint);
}

void MenuItemView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (!GetBounds().Contains(touchPoint))
        _penDown = false;
}

void MenuItemView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    // One tap runs the entry. A list row takes a first tap to select it, but a
    // menu is picked from, not browsed; the focus is only so the row lights up
    // for the frames the sheet takes to leave.
    if (_penDown && GetBounds().Contains(lastTouchPoint))
    {
        focusManager.Focus(SharedFromThis());
        _sheet->Activate(_index);
    }
    _penDown = false;
}

MenuBottomSheetView::MenuBottomSheetView(SharedPtr<MenuViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel = Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11));
    _titleLabel->SetText("Menu");
    AddChildTail(_titleLabel.GetPointer());

    _aboutButton = IconButton2DView::CreateShared(
        IconButtonView::Type::Standard,
        IconButtonView::State::NoToggle,
        md::sys::color::inverseOnSurface,
        materialColorScheme);
    _aboutButton->SetAction([] (IconButtonView*, void* arg)
    {
        ((MenuBottomSheetView*)arg)->_viewModel->ShowAbout();
    }, this);
    AddChildTail(_aboutButton.GetPointer());

    static_assert(std::size(kEntries) == ITEM_COUNT, "one entry per item");
    // the highlighted game can't change while the menu is open
    _newSaveShown = _viewModel->ShowsNewSave();
    u32 newSaveSlot = _viewModel->GetNewSaveSlot();
    char newSaveText[8];
    mini_snprintf(newSaveText, sizeof(newSaveText), "Save %u", newSaveSlot);
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        const char* stateText = i == ITEM_NEW_SAVE && newSaveSlot != 0 ? newSaveText : nullptr;
        _items[i] = MenuItemView::CreateShared(this, i, kEntries[i].column < 0 ? ROW_WIDTH : CELL_WIDTH,
            kEntries[i].name, stateText, kFavoriteRed,
            materialColorScheme, fontRepository);
        if (i != ITEM_NEW_SAVE || _newSaveShown)
            AddChildTail(_items[i].GetPointer());
    }
    _items[ITEM_NEW_SAVE]->SetEnabled(newSaveSlot != 0);
}

void MenuBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (!objVramManager)
        return;
    // In item order. The arrays are left unsized so the static_asserts below
    // catch an entry added to the enum without its icon.
    static const unsigned int* const kTiles[] =
        { recentIconTiles, smallHeartIconFilledTiles, statsIconTiles, trashIconTiles,
          plusIconTiles, checkIconTiles, settingsIconTiles };
    static const u32 kTilesLength[] =
        { recentIconTilesLen, smallHeartIconFilledTilesLen, statsIconTilesLen, trashIconTilesLen,
          plusIconTilesLen, checkIconTilesLen, settingsIconTilesLen };
    static_assert(std::size(kTiles) == ITEM_COUNT, "one icon per item");
    static_assert(std::size(kTilesLength) == ITEM_COUNT, "one icon length per item");
    _aboutButton->SetIconVramOffset(LoadSprite(*objVramManager, infoIconTiles, infoIconTilesLen));
    u32 selectorVramOffset = LoadSprite(*objVramManager, cheatSelectorTiles, cheatSelectorTilesLen);
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        _items[i]->SetIconVramOffset(LoadSprite(*objVramManager, kTiles[i], kTilesLength[i]));
        _items[i]->SetSelectorVramOffset(selectorVramOffset);
    }
}

u32 MenuBottomSheetView::LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}

int MenuBottomSheetView::GetRow(int index) const
{
    int row = kEntries[index].row;
    return !_newSaveShown && row > kEntries[ITEM_NEW_SAVE].row ? row - 1 : row;
}

int MenuBottomSheetView::FindEntry(int row, int column) const
{
    int firstCell = -1;
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        if (GetRow(i) != row || (i == ITEM_NEW_SAVE && !_newSaveShown))
            continue;
        if (kEntries[i].column == column || kEntries[i].column < 0)
            return i;
        if (firstCell < 0)
            firstCell = i;
    }
    return firstCell;
}

void MenuBottomSheetView::Update()
{
    int y = _position.y;
    _titleLabel->SetPosition(TITLE_X, y + TITLE_Y);
    _aboutButton->SetPosition(ABOUT_BUTTON_X, y + ABOUT_BUTTON_Y);
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        int x = ITEMS_X + std::max(kEntries[i].column, 0) * CELL_WIDTH;
        _items[i]->SetPosition(x, y + ROWS_Y + GetRow(i) * ROW_SPACING);
    }
    _items[ITEM_DELETE]->SetEnabled(_viewModel->CanDeleteSelected());
    _items[ITEM_BADGES_FILTER]->SetActive(_viewModel->IsBadgesFilterEnabled());
    BottomSheetView::Update();
}

void MenuBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        _titleLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool MenuBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

SharedPtr<View> MenuBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (currentFocus.GetPointer() == _aboutButton.GetPointer())
    {
        if (direction == FocusMoveDirection::Down)
            return _items[ITEM_FAVORITES];
        return nullptr;
    }
    int idx = -1;
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        if (currentFocus.GetPointer() == _items[i].GetPointer())
            idx = i;
    }
    if (idx < 0)
        return nullptr;

    int row = GetRow(idx);
    int column = kEntries[idx].column;
    int target = -1;
    switch (direction)
    {
        case FocusMoveDirection::Left:
        case FocusMoveDirection::Right:
            if (column >= 0)
                target = FindEntry(row, direction == FocusMoveDirection::Left ? 0 : 1);
            break;
        case FocusMoveDirection::Up:
            if (row == 0)
                return _aboutButton;
            target = FindEntry(row - 1, column);
            break;
        case FocusMoveDirection::Down:
            target = FindEntry(row + 1, column);
            break;
    }
    return target >= 0 && target != idx ? _items[target] : nullptr;
}

void MenuBottomSheetView::SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken)
{
    _aboutButton->SetGraphics(iconButtonVramToken);
}

void MenuBottomSheetView::Focus(FocusManager& focusManager)
{
    focusManager.Focus(_items[ITEM_RECENTS]);
}

void MenuBottomSheetView::Activate(int index)
{
    switch (index)
    {
        case ITEM_RECENTS:
            _viewModel->ShowRecents();
            break;
        case ITEM_FAVORITES:
            _viewModel->ShowFavorites();
            break;
        case ITEM_STATISTICS:
            _viewModel->ShowStatistics();
            break;
        case ITEM_DELETE:
            // faded on a folder and inert with it: the controller checks too
            _viewModel->RequestDeleteSelected();
            break;
        case ITEM_NEW_SAVE:
            // faded and inert without a free save: the view model checks
            _viewModel->StartNewSave();
            break;
        case ITEM_BADGES_FILTER:
            _viewModel->ToggleBadgesFilter();
            break;
        case ITEM_PARENTAL_CONTROLS:
            _viewModel->ShowParentalControls();
            break;
    }
}

void MenuBottomSheetView::Close()
{
    _viewModel->Close();
}
