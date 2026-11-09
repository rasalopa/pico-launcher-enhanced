#pragma once
#include "core/SharedPtr.h"
#include "gui/Alignment.h"
#include "gui/views/KeyboardView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/views/BottomSheetView.h"
#include "romBrowser/viewModels/GameInfoEditViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Edits the title, publisher and developer shown for a game, reached
///        from the menu. The title replaces the automatic one and the two
///        credits fill the lines under it.
///
/// The sheet has two modes. It opens on the three fields, picked with the
/// d-pad, where A starts editing and B or Select saves and closes. While
/// editing, the shared KeyboardView owns the d-pad and A, B, X and Y; Select
/// returns to the fields. A game with no values keeps showing its automatic
/// title and no credits.
class GameInfoEditBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(GameInfoEditBottomSheetView)

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
    static constexpr int kFieldCount = 3;

    SharedPtr<GameInfoEditViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _fieldLabels[kFieldCount];
    SharedPtr<Label2DView> _valueLabels[kFieldCount];
    SharedPtr<KeyboardView> _keyboard;
    SharedPtr<Label2DView> _hintLabel;
    char _values[kFieldCount][GameDataEntry::kMaxTitleLength + 1] = {};
    int _field = 0;
    bool _editing = false;

    GameInfoEditBottomSheetView(SharedPtr<GameInfoEditViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    SharedPtr<Label2DView> AddLabel(const IFontRepository* fontRepository, FontType fontType,
        u32 width, u32 height, u32 maxChars, const char* text, Alignment alignment);
    void BeginEditing(FocusManager& focusManager);
    void EndEditing(FocusManager& focusManager);
    void SaveAndClose();
    void Refresh();
    static void OnKeyboardTextChanged(void* arg);
};
