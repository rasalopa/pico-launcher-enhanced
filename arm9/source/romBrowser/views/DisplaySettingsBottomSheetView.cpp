#include "common.h"
#include "services/settings/Localization.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/IVramManager.h"
#include "hGridIcon.h"
#include "vGridIcon.h"
#include "bannerListIcon.h"
#include "listIcon.h"
#include "sortNameAscendingIcon.h"
#include "sortNameDescendingIcon.h"
#include "brightness1Icon.h"
#include "brightness2Icon.h"
#include "brightness3Icon.h"
#include "brightness4Icon.h"
#include "recentIcon.h"
#include "gamesIcon.h"
#include "picturesIcon.h"
#include "musicIcon.h"
#include "moviesIcon.h"
#include "unknownIcon.h"
#include "coverflowIcon.h"
#include "themeIcon.h"
#include "gamesIcon.h"
#include "languageENIcon.h"
#include "languageESIcon.h"
#include "languageFRIcon.h"
#include "languageDEIcon.h"
#include "languageITIcon.h"
#include "languagePTIcon.h"
#include "hideEmptyFoldersIcon.h"
#include "../IRomBrowserController.h"
#include "gui/input/InputProvider.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "DisplaySettingsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define THEME_BUTTON_X      212
#define THEME_BUTTON_Y      (TITLE_LABEL_Y - 7)

// Shares the title row with the theme button - the layout/sorting/brightness
// rows below already run edge to edge with no vertical room for a 4th row.
#define HIDE_EMPTY_FOLDERS_BUTTON_X     176
#define HIDE_EMPTY_FOLDERS_BUTTON_Y     (TITLE_LABEL_Y - 7)

#define LAYOUT_LABEL_X      20
#define LAYOUT_LABEL_Y      46

#define SORTING_LABEL_X     20
#define SORTING_LABEL_Y     78

#define BRIGHTNESS_LABEL_X  20
#define BRIGHTNESS_LABEL_Y  110
#define LAUNCHER_LABEL_X    20
#define LAUNCHER_LABEL_Y    134
#define LAUNCHER_PICO_X     92
#define LAUNCHER_BOOTSTRAP_X 148
#define LAUNCHER_BUTTON_Y   126
#define LAUNCHER_PICO_LABEL_X  86
#define LAUNCHER_BOOTSTRAP_LABEL_X 136
#define LAUNCHER_LABEL_Y_TEXT 158
#define LANGUAGE_LABEL_X    20
#define LANGUAGE_LABEL_Y    182
#define LANGUAGE_BUTTON_X   84
#define LANGUAGE_BUTTON_Y   174
#define LANGUAGE_BUTTON_STEP 28
#define LANGUAGE_MAX_SCROLL 46

#define FILTERS_LABEL_X     20
#define FILTERS_LABEL_Y     112

static RomBrowserLayout sRomBrowserDisplayModes[4] =
{
    [0] = RomBrowserLayout::HorizontalIconGrid,
    [1] = RomBrowserLayout::VerticalIconGrid,
    [2] = RomBrowserLayout::BannerList,
    [3] = RomBrowserLayout::CoverFlow
};

static const char* sLanguages[6] = { "english", "spanish", "french", "german", "italian", "portuguese" };

static RomBrowserSortMode sRomBrowserSortModes[4] =
{
    [0] = RomBrowserSortMode::NameAscending,
    [1] = RomBrowserSortMode::NameDescending,
    [2] = RomBrowserSortMode::LastModified
};

DisplaySettingsBottomSheetView::DisplaySettingsBottomSheetView(
    DisplaySettingsViewModel* viewModel, const MaterialColorScheme* materialColorScheme,
    const IFontRepository* fontRepository)
    : _viewModel(viewModel)
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _themeButton(IconButton2DView::CreateShared(
        IconButtonView::Type::Standard,
        IconButtonView::State::NoToggle,
        md::sys::color::inverseOnSurface,
        materialColorScheme))
    , _hideEmptyFoldersButton(IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        materialColorScheme))
    , _layoutLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _sortingLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _brightnessLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _launcherLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _picoLauncherLabel(Label2DView::CreateShared(48, 16, 16, fontRepository->GetFont(FontType::Medium7_5)))
    , _bootstrapLauncherLabel(Label2DView::CreateShared(72, 16, 16, fontRepository->GetFont(FontType::Medium7_5)))
    , _languageLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Regular10)))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel->SetText(Localization::DisplaySettings());
    AddChildTail(_titleLabel.GetPointer());

    _themeButton->SetAction([] (IconButtonView*, void* arg)
    {
        ((DisplaySettingsBottomSheetView*)arg)->_viewModel->GotoSettingsScreen();
    }, this);
    AddChildTail(_themeButton.GetPointer());

    _hideEmptyFoldersButton->SetAction([] (IconButtonView*, void* arg)
    {
        auto self = (DisplaySettingsBottomSheetView*)arg;
        self->_viewModel->SetHideEmptyFolders(!self->_viewModel->GetHideEmptyFolders());
    }, this);
    AddChildTail(_hideEmptyFoldersButton.GetPointer());

    _layoutLabel->SetText(Localization::Layout());
    AddChildTail(_layoutLabel.GetPointer());
    _sortingLabel->SetText(Localization::Sorting());
    AddChildTail(_sortingLabel.GetPointer());
    _brightnessLabel->SetText(Localization::Light());
    AddChildTail(_brightnessLabel.GetPointer());
    _launcherLabel->SetText(Localization::Launcher());
    AddChildTail(_launcherLabel.GetPointer());
    _picoLauncherLabel->SetText(Localization::PicoLauncher());
    AddChildTail(_picoLauncherLabel.GetPointer());
    _bootstrapLauncherLabel->SetText(Localization::BootstrapLauncher());
    AddChildTail(_bootstrapLauncherLabel.GetPointer());
    _languageLabel->SetText(Localization::Language());
    AddChildTail(_languageLabel.GetPointer());

    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption = CreateLayoutOptionIconButton();
        AddChildTail(layoutOption.GetPointer());
    }

    for (auto& sortOption : _sortOptions)
    {
        sortOption = CreateSortOptionIconButton();
        AddChildTail(sortOption.GetPointer());
    }

    for (auto& brightnessOption : _brightnessOptions)
    {
        brightnessOption = CreateBrightnessOptionIconButton();
        AddChildTail(brightnessOption.GetPointer());
    }

    for (auto& launcherOption : _launcherOptions)
    {
        launcherOption = CreateLauncherOptionIconButton();
        AddChildTail(launcherOption.GetPointer());
    }
    for (u32 i = 0; i < _languageOptions.size(); i++)
    {
        _languageOptions[i] = CreateLanguageOptionIconButton();
        AddChildTail(_languageOptions[i].GetPointer());
    }

}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateLayoutOptionIconButton()
{
    auto layoutOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    layoutOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_layoutOptions.size(); i++)
        {
            if (self->_layoutOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetRomBrowserDisplayMode(sRomBrowserDisplayModes[i]);
                break;
            }
        }
    }, this);
    return layoutOption;
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateSortOptionIconButton()
{
    auto sortOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    sortOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_sortOptions.size(); i++)
        {
            if (self->_sortOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetRomBrowserSortMode(sRomBrowserSortModes[i]);
                break;
            }
        }
    }, this);
    return sortOption;
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateBrightnessOptionIconButton()
{
    auto brightnessOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    brightnessOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_brightnessOptions.size(); i++)
        {
            if (self->_brightnessOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetBacklightLevel(i);
                break;
            }
        }
    }, this);
    return brightnessOption;
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateLauncherOptionIconButton()
{
    auto launcherOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    launcherOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_launcherOptions.size(); i++)
        {
            if (self->_launcherOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetLauncher(i == 0 ? "pico" : "bootstrap");
                break;
            }
        }
    }, this);
    return launcherOption;
}

SharedPtr<IconButton2DView> DisplaySettingsBottomSheetView::CreateLanguageOptionIconButton()
{
    auto languageOption = IconButton2DView::CreateShared(
        IconButtonView::Type::Tonal,
        IconButtonView::State::ToggleUnselected,
        md::sys::color::surfaceContainerLow,
        _materialColorScheme
    );
    languageOption->SetAction([] (IconButtonView* sender, void* arg)
    {
        auto self = reinterpret_cast<DisplaySettingsBottomSheetView*>(arg);
        for (u32 i = 0; i < self->_languageOptions.size(); i++)
        {
            if (self->_languageOptions[i].GetPointer() == sender)
            {
                self->_viewModel->SetLanguage(sLanguages[i]);
                break;
            }
        }
    }, this);
    return languageOption;
}

void DisplaySettingsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _themeButton->SetIconVramOffset(LoadIcon(*objVramManager, themeIconTiles, themeIconTilesLen));
        _hideEmptyFoldersButton->SetIconVramOffset(LoadIcon(*objVramManager, hideEmptyFoldersIconTiles, hideEmptyFoldersIconTilesLen));

        // layout options
        _layoutOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, hGridIconTiles, hGridIconTilesLen));
        _layoutOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, vGridIconTiles, vGridIconTilesLen));
        _layoutOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, bannerListIconTiles, bannerListIconTilesLen));
        _layoutOptions[3]->SetIconVramOffset(LoadIcon(*objVramManager, coverflowIconTiles, coverflowIconTilesLen));

        // sort options
        _sortOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameAscendingIconTiles, sortNameAscendingIconTilesLen));
        _sortOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, sortNameDescendingIconTiles, sortNameDescendingIconTilesLen));

        // brightness options (DS Lite backlight levels)
        _brightnessOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, brightness1IconTiles, brightness1IconTilesLen));
        _brightnessOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, brightness2IconTiles, brightness2IconTilesLen));
        _brightnessOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, brightness3IconTiles, brightness3IconTilesLen));
        _brightnessOptions[3]->SetIconVramOffset(LoadIcon(*objVramManager, brightness4IconTiles, brightness4IconTilesLen));

        _launcherOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, gamesIconTiles, gamesIconTilesLen));
        _launcherOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, gamesIconTiles, gamesIconTilesLen));

        _languageOptions[0]->SetIconVramOffset(LoadIcon(*objVramManager, languageENIconTiles, languageENIconTilesLen));
        _languageOptions[1]->SetIconVramOffset(LoadIcon(*objVramManager, languageESIconTiles, languageESIconTilesLen));
        _languageOptions[2]->SetIconVramOffset(LoadIcon(*objVramManager, languageFRIconTiles, languageFRIconTilesLen));
        _languageOptions[3]->SetIconVramOffset(LoadIcon(*objVramManager, languageDEIconTiles, languageDEIconTilesLen));
        _languageOptions[4]->SetIconVramOffset(LoadIcon(*objVramManager, languageITIconTiles, languageITIconTilesLen));
        _languageOptions[5]->SetIconVramOffset(LoadIcon(*objVramManager, languagePTIconTiles, languagePTIconTilesLen));
    }
}

void DisplaySettingsBottomSheetView::UpdateLabels()
{
    _titleLabel->SetText(Localization::DisplaySettings());
    _layoutLabel->SetText(Localization::Layout());
    _sortingLabel->SetText(Localization::Sorting());
    _brightnessLabel->SetText(Localization::Light());
    _launcherLabel->SetText(Localization::Launcher());
    _languageLabel->SetText(Localization::Language());

    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y - _scrollOffset);
    _layoutLabel->SetPosition(LAYOUT_LABEL_X, _position.y + LAYOUT_LABEL_Y - _scrollOffset);
    _sortingLabel->SetPosition(SORTING_LABEL_X, _position.y + SORTING_LABEL_Y - _scrollOffset);
    _brightnessLabel->SetPosition(BRIGHTNESS_LABEL_X, _position.y + BRIGHTNESS_LABEL_Y - _scrollOffset);
    _launcherLabel->SetPosition(LAUNCHER_LABEL_X, _position.y + LAUNCHER_LABEL_Y - _scrollOffset);
    _picoLauncherLabel->SetPosition(LAUNCHER_PICO_LABEL_X, _position.y + LAUNCHER_LABEL_Y_TEXT - _scrollOffset);
    _bootstrapLauncherLabel->SetPosition(LAUNCHER_BOOTSTRAP_LABEL_X, _position.y + LAUNCHER_LABEL_Y_TEXT - _scrollOffset);
    _languageLabel->SetPosition(LANGUAGE_LABEL_X, _position.y + LANGUAGE_LABEL_Y - _scrollOffset);
}

void DisplaySettingsBottomSheetView::UpdateScroll()
{
    const int maxScroll = LANGUAGE_MAX_SCROLL;
    if (_scrollOffset < 0) _scrollOffset = 0;
    if (_scrollOffset > maxScroll) _scrollOffset = maxScroll;

    int x = 70;
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetPosition(x, _position.y + 38 - _scrollOffset);
        x += 32;
    }
    x = 70;
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetPosition(x, _position.y + 70 - _scrollOffset);
        x += 32;
    }
    x = 70;
    for (auto& brightnessOption : _brightnessOptions)
    {
        brightnessOption->SetPosition(x, _position.y + 102 - _scrollOffset);
        x += 32;
    }
    _launcherOptions[0]->SetPosition(LAUNCHER_PICO_X, _position.y + LAUNCHER_BUTTON_Y - _scrollOffset);
    _launcherOptions[1]->SetPosition(LAUNCHER_BOOTSTRAP_X, _position.y + LAUNCHER_BUTTON_Y - _scrollOffset);
    for (u32 i = 0; i < _languageOptions.size(); i++)
        _languageOptions[i]->SetPosition(LANGUAGE_BUTTON_X + i * LANGUAGE_BUTTON_STEP, _position.y + LANGUAGE_BUTTON_Y - _scrollOffset);
}

void DisplaySettingsBottomSheetView::Update()
{
    BottomSheetView::Update();
    _themeButton->SetPosition(THEME_BUTTON_X, _position.y + THEME_BUTTON_Y);
    _hideEmptyFoldersButton->SetPosition(HIDE_EMPTY_FOLDERS_BUTTON_X, _position.y + HIDE_EMPTY_FOLDERS_BUTTON_Y);
    _hideEmptyFoldersButton->SetState(_viewModel->GetHideEmptyFolders()
        ? IconButtonView::State::ToggleSelected
        : IconButtonView::State::ToggleUnselected);

    UpdateScroll();
    UpdateLabels();

    auto selectedDisplayMode = _viewModel->GetRomBrowserDisplayMode();
    u32 idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetState(sRomBrowserDisplayModes[idx] == selectedDisplayMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        idx++;
    }
    auto selectedSortMode = _viewModel->GetRomBrowserSortMode();
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetState(sRomBrowserSortModes[idx] == selectedSortMode
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        idx++;
    }
    int backlightLevel = _viewModel->GetBacklightLevel();
    idx = 0;
    for (auto& brightnessOption : _brightnessOptions)
    {
        brightnessOption->SetState((int)idx == backlightLevel
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
        idx++;
    }

    const bool useBootstrap = !strcasecmp(_viewModel->GetLauncher(), "bootstrap");
    _launcherOptions[0]->SetState(!useBootstrap ? IconButtonView::State::ToggleSelected : IconButtonView::State::ToggleUnselected);
    _launcherOptions[1]->SetState(useBootstrap ? IconButtonView::State::ToggleSelected : IconButtonView::State::ToggleUnselected);

    const char* currentLanguage = _viewModel->GetLanguage();
    for (u32 i = 0; i < _languageOptions.size(); i++)
    {
        _languageOptions[i]->SetState(!strcasecmp(currentLanguage, sLanguages[i])
            ? IconButtonView::State::ToggleSelected
            : IconButtonView::State::ToggleUnselected);
    }
}

void DisplaySettingsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        _titleLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _layoutLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _layoutLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _sortingLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _sortingLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _brightnessLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _brightnessLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _launcherLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _launcherLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _picoLauncherLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _picoLauncherLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _bootstrapLauncherLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _bootstrapLauncherLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _languageLabel->SetBackgroundColor(_materialColorScheme->GetColor(md::sys::color::surfaceContainerLow));
        _languageLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        BottomSheetView::Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void DisplaySettingsBottomSheetView::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    _penDownY = touchPoint.y;
    _touchScrolling = false;
    BottomSheetView::HandlePenDown(touchPoint, focusManager);
}

void DisplaySettingsBottomSheetView::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    const int dy = _penDownY - touchPoint.y;
    if (std::abs(dy) >= 4 && GetBounds().Contains(touchPoint))
        _touchScrolling = true;

    if (_touchScrolling)
    {
        _scrollOffset += dy;
        _penDownY = touchPoint.y;
        if (_scrollOffset < 0) _scrollOffset = 0;
        if (_scrollOffset > LANGUAGE_MAX_SCROLL) _scrollOffset = LANGUAGE_MAX_SCROLL;
        UpdateScroll();
        UpdateLabels();
        return;
    }

    BottomSheetView::HandlePenMove(touchPoint, focusManager);
}

void DisplaySettingsBottomSheetView::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_touchScrolling)
    {
        _touchScrolling = false;
        return;
    }
    BottomSheetView::HandlePenUp(lastTouchPoint, focusManager);
}

bool DisplaySettingsBottomSheetView::HandleInput(
    const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

SharedPtr<View> DisplaySettingsBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    if (currentFocus.GetPointer() == _themeButton.GetPointer())
    {
        if (direction == FocusMoveDirection::Down)
        {
            return _layoutOptions[0];
        }
        else if (direction == FocusMoveDirection::Left)
        {
            return _hideEmptyFoldersButton;
        }
        else
        {
            return nullptr;
        }
    }

    if (currentFocus.GetPointer() == _hideEmptyFoldersButton.GetPointer())
    {
        if (direction == FocusMoveDirection::Down)
        {
            return _layoutOptions[0];
        }
        else if (direction == FocusMoveDirection::Right)
        {
            return _themeButton;
        }
        else
        {
            return nullptr;
        }
    }

    int idx = 0;
    for (auto& layoutOption : _layoutOptions)
    {
        if (currentFocus.GetPointer() == layoutOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _layoutOptions.size();
                return _layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_layoutOptions.size())
                    idx = 0;
                return _layoutOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                return _themeButton;
            }
            else //if (direction == FocusMoveDirection::Down)
            {
                if (idx >= (int)_sortOptions.size())
                    idx = _sortOptions.size() - 1;
                return _sortOptions[idx];
            }
        }
        idx++;
    }
    idx = 0;
    for (auto& sortOption : _sortOptions)
    {
        if (currentFocus.GetPointer() == sortOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _sortOptions.size();
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_sortOptions.size())
                    idx = 0;
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                if (idx >= (int)_layoutOptions.size())
                    idx = _layoutOptions.size() - 1;
                return _layoutOptions[idx];
            }
            else //if (direction == FocusMoveDirection::Down)
            {
                if (idx >= (int)_brightnessOptions.size())
                    idx = _brightnessOptions.size() - 1;
                return _brightnessOptions[idx];
            }
        }
        idx++;
    }
    idx = 0;
    for (auto& launcherOption : _launcherOptions)
    {
        if (currentFocus.GetPointer() == launcherOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left || direction == FocusMoveDirection::Right)
                return _launcherOptions[idx == 0 ? 1 : 0];
            else if (direction == FocusMoveDirection::Up)
                return _brightnessOptions[0];
            else if (direction == FocusMoveDirection::Down)
                return _languageOptions[0];
        }
        idx++;
    }

    idx = 0;
    for (auto& languageOption : _languageOptions)
    {
        if (currentFocus.GetPointer() == languageOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0) idx += _languageOptions.size();
                return _languageOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_languageOptions.size()) idx = 0;
                return _languageOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
                return _launcherOptions[0];
        }
        idx++;
    }

    idx = 0;
    for (auto& brightnessOption : _brightnessOptions)
    {
        if (currentFocus.GetPointer() == brightnessOption.GetPointer())
        {
            if (direction == FocusMoveDirection::Left)
            {
                if (--idx < 0)
                    idx += _brightnessOptions.size();
                return _brightnessOptions[idx];
            }
            else if (direction == FocusMoveDirection::Right)
            {
                if (++idx >= (int)_brightnessOptions.size())
                    idx = 0;
                return _brightnessOptions[idx];
            }
            else if (direction == FocusMoveDirection::Up)
            {
                if (idx >= (int)_sortOptions.size())
                    idx = _sortOptions.size() - 1;
                return _sortOptions[idx];
            }
            else if (direction == FocusMoveDirection::Down)
            {
                return _launcherOptions[0];
            }
        }
        idx++;
    }
    return nullptr;
}

void DisplaySettingsBottomSheetView::SetGraphics(
    const IconButton2DView::VramToken& iconButtonVramToken)
{
    _themeButton->SetGraphics(iconButtonVramToken);
    _hideEmptyFoldersButton->SetGraphics(iconButtonVramToken);
    for (auto& layoutOption : _layoutOptions)
    {
        layoutOption->SetGraphics(iconButtonVramToken);
    }
    for (auto& sortOption : _sortOptions)
    {
        sortOption->SetGraphics(iconButtonVramToken);
    }
    for (auto& brightnessOption : _brightnessOptions)
    {
        brightnessOption->SetGraphics(iconButtonVramToken);
    }
    for (auto& launcherOption : _launcherOptions)
    {
        launcherOption->SetGraphics(iconButtonVramToken);
    }
    for (auto& languageOption : _languageOptions)
    {
        languageOption->SetGraphics(iconButtonVramToken);
    }
}

void DisplaySettingsBottomSheetView::Close()
{
    _viewModel->Close();
}

u32 DisplaySettingsBottomSheetView::LoadIcon(IVramManager& vramManager,
    const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}