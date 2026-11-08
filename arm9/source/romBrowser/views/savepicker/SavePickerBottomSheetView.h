#pragma once
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/views/menu/MenuBottomSheetView.h"
#include "romBrowser/viewModels/SavePickerViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Bottom sheet that asks which save to start a game with, its saves in two
///        columns of the menu's entries. A or a tap on one launches the game with it,
///        B goes back.
class SavePickerBottomSheetView : public BottomSheetView, public IMenuItemHost
{
    SHARED_ONLY(SavePickerBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;
    void Focus(FocusManager& focusManager) override;

    void Activate(int index) override;

protected:
    void Close() override;

private:
    SharedPtr<SavePickerViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _gameNameLabel;
    SharedPtr<MenuItemView> _entries[SavePickerViewModel::kMaxEntryCount];
    u32 _entryCount;

    SavePickerBottomSheetView(SharedPtr<SavePickerViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
