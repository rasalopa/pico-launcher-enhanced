#pragma once
#include <array>
#include "BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "IconButton2DView.h"
#include "../viewModels/DisplaySettingsViewModel.h"

class IRomBrowserController;
class MaterialColorScheme;
class IFontRepository;

class DisplaySettingsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(DisplaySettingsBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus,
        FocusMoveDirection direction, View* source) override;

    void SetGraphics(const IconButton2DView::VramToken& iconButtonVramToken);

    void Focus(FocusManager& focusManager) override
    {
        focusManager.Focus(_layoutOptions[0]);
    }

protected:
    void Close() override;

private:
    DisplaySettingsViewModel* _viewModel;

    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<IconButton2DView> _themeButton;
    /// @brief Toggles RomBrowserDisplaySettings::hideEmptyFolders. Lives in
    ///        the title row (icon-only, like the theme button) - the layout/
    ///        sorting/brightness rows below already fill the sheet's height.
    SharedPtr<IconButton2DView> _hideEmptyFoldersButton;
    SharedPtr<Label2DView> _layoutLabel;
    SharedPtr<Label2DView> _sortingLabel;
    SharedPtr<Label2DView> _brightnessLabel;
    SharedPtr<Label2DView> _launcherLabel;
    SharedPtr<Label2DView> _picoLauncherLabel;
    SharedPtr<Label2DView> _bootstrapLauncherLabel;
    SharedPtr<Label2DView> _languageLabel;

    std::array<SharedPtr<IconButton2DView>, 4> _layoutOptions;
    std::array<SharedPtr<IconButton2DView>, /*3*/2> _sortOptions;
    std::array<SharedPtr<IconButton2DView>, 4> _brightnessOptions;
    std::array<SharedPtr<IconButton2DView>, 2> _launcherOptions;
    std::array<SharedPtr<IconButton2DView>, 6> _languageOptions;

    const MaterialColorScheme* _materialColorScheme;

    SharedPtr<IconButton2DView> CreateLayoutOptionIconButton();
    SharedPtr<IconButton2DView> CreateSortOptionIconButton();
    SharedPtr<IconButton2DView> CreateBrightnessOptionIconButton();
    SharedPtr<IconButton2DView> CreateLauncherOptionIconButton();
    SharedPtr<IconButton2DView> CreateLanguageOptionIconButton();

    DisplaySettingsBottomSheetView(DisplaySettingsViewModel* viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    void UpdateLabels();
    void UpdateScroll();

    int _scrollOffset = 0;
    int _penDownY = 0;
    bool _touchScrolling = false;

    u32 LoadIcon(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};
