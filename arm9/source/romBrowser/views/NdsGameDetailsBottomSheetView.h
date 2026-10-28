#pragma once
#include "core/SharedPtr.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/NdsGameDetailsViewModel.h"
#include "BottomSheetView.h"
#include "ChipView.h"
#include "gui/FocusManager.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Bottom sheet with the options for an nds game (Y in the browser): the save slot
///        to use, and the way to its cheats.
class NdsGameDetailsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(NdsGameDetailsBottomSheetView)

public:
    void SetGraphics(const ChipView::VramToken& chipVramToken)
    {
        _cheatsChip->SetGraphics(chipVramToken);
        _saveSlotChip->SetGraphics(chipVramToken);
    }

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(_cheatsChip);
    }

    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;

    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    SharedPtr<NdsGameDetailsViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _saveFileLabel;
    SharedPtr<Label2DView> _saveStatusLabel;
    SharedPtr<Label2DView> _hintLabel;
    SharedPtr<ChipView> _cheatsChip;
    SharedPtr<ChipView> _saveSlotChip;
    /// @brief Slot the labels show, -1 until the view model has loaded.
    int _displayedSaveSlot = -1;

    NdsGameDetailsBottomSheetView(
        SharedPtr<NdsGameDetailsViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme,
        const IFontRepository* fontRepository);

    void UpdateSaveSlotText();
};
