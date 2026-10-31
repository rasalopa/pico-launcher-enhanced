#include "common.h"
#include <algorithm>
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
#include "heartIcon.h"
#include "checkIcon.h"
#include "infoIcon.h"
#include "MenuBottomSheetView.h"

// Placed from the sheet's top edge, which rests at y 32 once the sheet is open.
#define TITLE_X             20
#define TITLE_Y             16

// The about button shares the title row, where the display settings sheet
// keeps its theme button.
#define ABOUT_BUTTON_X      212
#define ABOUT_BUTTON_Y      (TITLE_Y - 7)

// Two cells per row over the 224 px the recents list uses, then two full rows.
// Rows sit 26 apart so the 24 px items keep a hairline between them; the last
// one ends at 142, screen y 174, with the sheet's rounded edge below it.
#define ITEMS_X             16
#define CELL_WIDTH          112
#define ROW_WIDTH           224
#define ROWS_Y              40
#define ROW_SPACING         26

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

MenuItemView::MenuItemView(IMenuItemHost* sheet, int index, int width, const char* name,
    bool hasState, const Rgb<8, 8, 8>& activeColor,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _sheet(sheet), _index(index), _width(width), _activeColor(activeColor)
    , _materialColorScheme(materialColorScheme)
{
    int nameWidth = width - NAME_DX - 4 - (hasState ? STATE_WIDTH : 0);
    _nameLabel = Label2DView::CreateShared(nameWidth, 16, 24, fontRepository->GetFont(FontType::Regular10));
    _nameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    _nameLabel->SetText(name);
    AddChildTail(_nameLabel.GetPointer());
    if (hasState)
    {
        _stateLabel = Label2DView::CreateShared(STATE_WIDTH, 16, 4, fontRepository->GetFont(FontType::Medium7_5));
        _stateLabel->SetHorizontalAlignment(Alignment::End);
        _stateLabel->SetText("off");
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
        _stateLabel->SetForegroundColor(_active ? _activeColor : _materialColorScheme->outline);
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

    static const struct { const char* name; bool filter; } kEntries[ITEM_COUNT] =
    {
        { "Recently played", false },
        { "Favorites", false },
        { "Statistics", false },
        { "Delete game", false },
        { "Only favorites", true },
        { "Only completed", true }
    };
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        _items[i] = MenuItemView::CreateShared(this, i, kEntries[i].filter ? ROW_WIDTH : CELL_WIDTH,
            kEntries[i].name, kEntries[i].filter, i == ITEM_COMPLETED_FILTER ? kCompletedGreen : kFavoriteRed,
            materialColorScheme, fontRepository);
        AddChildTail(_items[i].GetPointer());
    }
}

void MenuBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (!objVramManager)
        return;
    static const unsigned int* const kTiles[ITEM_COUNT] =
        { recentIconTiles, smallHeartIconFilledTiles, statsIconTiles, trashIconTiles, heartIconTiles, checkIconTiles };
    static const u32 kTilesLength[ITEM_COUNT] =
        { recentIconTilesLen, smallHeartIconFilledTilesLen, statsIconTilesLen, trashIconTilesLen, heartIconTilesLen, checkIconTilesLen };
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

void MenuBottomSheetView::Update()
{
    int y = _position.y;
    _titleLabel->SetPosition(TITLE_X, y + TITLE_Y);
    _aboutButton->SetPosition(ABOUT_BUTTON_X, y + ABOUT_BUTTON_Y);
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        // two rows of two cells, then the filters as the third and fourth row
        bool cell = i < ITEM_FAVORITES_FILTER;
        int row = cell ? i / 2 : i - 2;
        int x = ITEMS_X + (cell ? (i % 2) * CELL_WIDTH : 0);
        _items[i]->SetPosition(x, y + ROWS_Y + row * ROW_SPACING);
    }
    _items[ITEM_DELETE]->SetEnabled(_viewModel->CanDeleteSelected());
    _items[ITEM_FAVORITES_FILTER]->SetActive(_viewModel->IsFavoritesFilterEnabled());
    _items[ITEM_COMPLETED_FILTER]->SetActive(_viewModel->IsCompletedFilterEnabled());
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

    // 0 1 and 2 3 are the two-by-two grid, 4 and 5 the full rows under it.
    bool cell = idx < ITEM_FAVORITES_FILTER;
    switch (direction)
    {
        case FocusMoveDirection::Left:
            if (cell && (idx % 2) == 1)
                return _items[idx - 1];
            break;
        case FocusMoveDirection::Right:
            if (cell && (idx % 2) == 0)
                return _items[idx + 1];
            break;
        case FocusMoveDirection::Up:
            if (idx == ITEM_COMPLETED_FILTER)
                return _items[ITEM_FAVORITES_FILTER];
            if (idx == ITEM_FAVORITES_FILTER)
                return _items[ITEM_STATISTICS];
            if (idx >= 2)
                return _items[idx - 2];
            return _aboutButton;
        case FocusMoveDirection::Down:
            if (idx == ITEM_FAVORITES_FILTER)
                return _items[ITEM_COMPLETED_FILTER];
            if (idx < 2)
                return _items[idx + 2];
            if (cell)
                return _items[ITEM_FAVORITES_FILTER];
            break;
    }
    return nullptr;
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
        case ITEM_FAVORITES_FILTER:
            _viewModel->ToggleFavoritesFilter();
            break;
        case ITEM_COMPLETED_FILTER:
            _viewModel->ToggleCompletedFilter();
            break;
    }
}

void MenuBottomSheetView::Close()
{
    _viewModel->Close();
}
