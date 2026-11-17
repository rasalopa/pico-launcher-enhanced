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

/// @brief One entry of the menu: an icon, its name and, for New save and Language, the
///        save it would start or the language the game starts in. A tap or A runs it; the
///        sheet decides what that means. The save picker lists its saves with these too,
///        without an icon and with the date each save was last started on.
class MenuItemView : public ViewContainer
{
    SHARED_ONLY(MenuItemView)

public:
    static constexpr int HEIGHT = 24;

    /// @brief The menu's entries have an icon before the name and room for a
    ///        language at the end. The save picker's have no icon, so the name
    ///        starts at the left, and a date at the end, which is narrower.
    enum class Layout
    {
        Menu,
        SavePicker
    };

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

    /// @brief Changes the text at the end, for an item made with one.
    void SetStateText(const char* stateText) { _stateLabel->SetText(stateText); }

private:
    IMenuItemHost* _sheet;
    int _index;
    int _width;
    int _nameX;
    int _stateWidth;
    Rgb<8, 8, 8> _activeColor;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _nameLabel;
    /// Small gray text at the end of the item: in the menu only New save and Language
    /// have one, in the save picker the saves with a date.
    SharedPtr<Label2DView> _stateLabel;
    u32 _iconVramOffset = 0;
    u32 _selectorVramOffset = 0;
    bool _hasIcon = false;
    bool _active = false;
    bool _enabled = true;
    bool _penDown = false;

    /// @param width A cell of the two columns (see the .cpp).
    /// @param stateText Text at the end of the item, or nullptr for none.
    MenuItemView(IMenuItemHost* sheet, int index, int width, const char* name,
        const char* stateText, const Rgb<8, 8, 8>& activeColor,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        Layout layout = Layout::Menu);
};

/// @brief The sheet behind the app bar's "more" button: what the bar used to
///        hold as icons, and behind long presses, written out.
///
/// Recently played, favorites, statistics and delete in two columns, then New save,
/// which names the save it would start, next to Language, which names the language
/// the game starts in, then the two filters, in their color while on. Picking a
/// panel replaces this sheet with it; a filter closes the sheet through the display
/// mode change it triggers; New save launches the highlighted game; Language moves
/// it to its next language and the sheet stays. Delete is faded while a folder is
/// highlighted. New save and Language are only there for a DS game, the filters
/// moving up into their row otherwise. New save is faded when all its saves are
/// taken, and Language without a game code or with a loader that can't set a
/// language. A small button at the right of the title opens the about sheet.
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
        ITEM_LANGUAGE,
        ITEM_FAVORITES_FILTER,
        ITEM_COMPLETED_FILTER,
        ITEM_COUNT
    };

    SharedPtr<MenuViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<IconButton2DView> _aboutButton;
    SharedPtr<MenuItemView> _items[ITEM_COUNT];
    /// @brief Whether New save and Language are in the sheet: when they aren't, their items
    ///        are made but never added, so they are neither drawn nor reached.
    bool _isDsGame = false;

    MenuBottomSheetView(SharedPtr<MenuViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
    bool IsShown(int index) const;
    /// @brief The row an entry sits on: the ones under New save move up when it isn't shown.
    int GetRow(int index) const;
    /// @brief The shown entry of a row in a column, or -1 when there is none.
    int FindEntry(int row, int column) const;
};
