#pragma once
#include "core/SharedPtr.h"
#include "core/String.h"
#include "gui/views/ViewContainer.h"
#include "BannerView.h"
#include "gui/views/LabelView.h"
#include "gui/views/Label2DView.h"
#include "../FileType/FileIcon.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"
#include "../Theme/TopStatusBarStyle.h"

class RomBrowserViewModel;
class IRomBrowserViewFactory;
class IFontRepository;
class IGameDataService;
struct MaterialColorScheme;

class RomBrowserTopScreenView : public ViewContainer
{
    SHARED_ONLY(RomBrowserTopScreenView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    void VBlank() override;

    /// @brief Writes the cover's affine matrix and clip window to the MAIN
    ///        engine, for the one frame a screenshot borrows that engine to draw
    ///        this screen. Those registers cannot be read back, so they cannot
    ///        be copied across - only the view that computes them can restate
    ///        them.
    void MirrorToMainEngine() const;

    Rectangle GetBounds() const override
    {
        return Rectangle(0, 0, 256, 192);
    }

private:
    SharedPtr<RomBrowserViewModel> _viewModel;
    const IThemeFileIconFactory* _themeFileIconFactory;
    SharedPtr<BannerView> _fileInfoView;
    SharedPtr<Label2DView> _launchInfoLabel;
    IGameDataService* _gameDataService;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;
    bool _iconGraphicsUploaded = false;
    bool _coverGraphicsUploaded = false;
    bool _showCover;
    Point _coverPosition;
    Point _launchInfoPosition;
    bool _launchInfoHidden = false;
    u32 _heartVramOffset = 0;
    u32 _checkVramOffset = 0;
    u32 _starVramOffset = 0;
    u32 _chipVramOffset = 0;
    bool _selectedFavorite = false;
    bool _selectedCompleted = false;
    bool _selectedStarred = false;
    bool _launchInfoCentered = false;
    bool _launchInfoBare = false;
    // The most launched game's file name, refreshed when the game data changes.
    String<char, 96> _mostPlayedFileName;
    u32 _mostPlayedVersion = 0;
    bool _mostPlayedKnown = false;
    int _lastGameDataItem = -1;
    u32 _lastGameDataVersion = 0;
    const MaterialColorScheme* _materialColorScheme;
    // The bar along the top edge: the time at the left, the battery at the right.
    SharedPtr<Label2DView> _clockLabel;
    const TopStatusBarStyle* _statusBarStyle;
    bool _statusBarHidden = false;
    bool _backgroundHidden = false;
    bool _clockHidden = false;
    bool _batteryHidden = false;
    u32 _statusBarVramOffset = 0;
    u32 _batteryVramOffset = 0;
    // frames until the rtc is read again, and the hour and minute on show
    u32 _clockFramesLeft = 0;
    u32 _clockShownMinute = 0xFFFF;

    RomBrowserTopScreenView(SharedPtr<RomBrowserViewModel> viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        const IFontRepository* fontRepository,
        const MaterialColorScheme* materialColorScheme);

    void DrawChip(GraphicsContext& graphicsContext, int x, int y, int width, u32 paletteRow);
    void RefreshMostPlayed(u32 gameDataVersion);
    void UpdateClock();
    void DrawStatusBar(GraphicsContext& graphicsContext);
};