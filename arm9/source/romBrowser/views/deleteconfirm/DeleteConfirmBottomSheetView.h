#pragma once
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/IDeleteConfirmViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief Confirmation sheet before deleting something from the SD card: a game
///        and its save, or a theme. X confirms; A and B cancel - A is the launch
///        button and muscle memory must not delete anything.
class DeleteConfirmBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(DeleteConfirmBottomSheetView)

public:
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    SharedPtr<IDeleteConfirmViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _fileNameLabel;
    SharedPtr<Label2DView> _saveLabel;
    SharedPtr<Label2DView> _saveEndLabel;
    SharedPtr<Label2DView> _hintLabel;
    const MaterialColorScheme* _materialColorScheme;
    const char* _shownStatus = nullptr;
    bool _hasDetail = false;
    bool _hasDetailEnd = false;
    bool _confirmed = false;

    DeleteConfirmBottomSheetView(SharedPtr<IDeleteConfirmViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
