#pragma once
#include "core/SharedPtr.h"
#include "core/math/Rgb.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/views/IconButton2DView.h"
#include "romBrowser/viewModels/MenuViewModel.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

/// @brief A sheet made of MenuItemViews, which call it with their index on A or a tap.
class IMenuItemHost
{
public:
    virtual void Activate(int index) = 0;

protected:
    ~IMenuItemHost() = default;
};

/// @brief One entry of the menu: an icon, its name and, for New save, the save
///        it would start. A tap or A runs it; the sheet decides what that means.
///        The save picker lists its saves with these too, without an icon.
class MenuItemView : public ViewContainer
{
    SHARED_ONLY(MenuItemView)

public:
    static constexpr int HEIGHT = 24;

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, _width, HEIGHT);
    }

    void SetIconVramOffset(u32 vramOffset)
    {
        _iconVramOffset = vramOffset;
        _hasIcon = true;
    }
    void SetSelectorVramOffset(u32 vramOffset) { _selectorVramOffset = vramOffset; }

    /// @brief A filter that is on draws its name and icon in its color.
    void SetActive(bool active) { _active = active; }

    /// @brief Off is faded and inert, like the delete button was on a folder.
    void SetEnabled(bool enabled) { _enabled = enabled; }

private:
    IMenuItemHost* _sheet;
    int _index;
    int _width;
    Rgb<8, 8, 8> _activeColor;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _nameLabel;
    /// Small grey text at the end of the row; only New save has one.
    SharedPtr<Label2DView> _stateLabel;
    u32 _iconVramOffset = 0;
    u32 _selectorVramOffset = 0;
    bool _hasIcon = false;
    bool _active = false;
    bool _enabled = true;
    bool _penDown = false;

    /// @param width A cell of the two-column part or a full row (see the .cpp).
    /// @param stateText Text at the end of the row, or nullptr for none.
    MenuItemView(IMenuItemHost* sheet, int index, int width, const char* name,
        const char* stateText, const Rgb<8, 8, 8>& activeColor,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};

/// @brief The sheet behind the app bar's "more" button: what the bar used to
///        hold as icons, and behind long presses, written out.
///
/// Recently played, favorites, statistics and delete in two columns, New save
/// as a full row that names the save it would start, then the two filters side
/// by side, in their color while on. Picking a panel replaces this sheet with
/// it; a filter closes the sheet through the display mode change it triggers;
/// New save launches the highlighted game. Delete is faded while a folder is
/// highlighted. New save is only there for a DS game, the filters moving up into
/// its row otherwise, and is faded when all its saves are taken. A small button
/// at the right of the title opens the about sheet.
class MenuBottomSheetView : public BottomSheetView, public IMenuItemHost
{
    SHARED_ONLY(MenuBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;
    void Focus(FocusManager& focusManager) override;

    /// @brief Runs the entry at index; the items call this on A or a tap.
    void Activate(int index) override;

    void SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken);

protected:
    void Close() override;

private:
    enum Item
    {
        ITEM_RECENTS = 0,
        ITEM_FAVORITES,
        ITEM_STATISTICS,
        ITEM_DELETE,
        ITEM_NEW_SAVE,
        ITEM_FAVORITES_FILTER,
        ITEM_COMPLETED_FILTER,
        ITEM_COUNT
    };

    SharedPtr<MenuViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<IconButton2DView> _aboutButton;
    SharedPtr<MenuItemView> _items[ITEM_COUNT];
    /// @brief Whether New save is in the sheet: when it isn't, its item is made but never
    ///        added, so it is neither drawn nor reached.
    bool _newSaveShown = false;

    MenuBottomSheetView(SharedPtr<MenuViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
    /// @brief The row an entry sits on: the ones under New save move up when it isn't shown.
    int GetRow(int index) const;
    /// @brief The shown entry of a row in a column, or -1 when there is none. A full row
    ///        answers for either column, and a row of cells answers for a full row with its
    ///        first cell.
    int FindEntry(int row, int column) const;
};
