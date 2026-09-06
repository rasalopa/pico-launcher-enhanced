#pragma once
#include "core/SharedPtr.h"
#include "gui/views/ViewContainer.h"
#include "BannerView.h"
#include "gui/views/LabelView.h"
#include "gui/views/Label2DView.h"
#include "../FileType/FileIcon.h"
#include "../DisplayMode/RomBrowserDisplayMode.h"

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
    SharedPtr<Label2DView> _gameCountLabel;
    SharedPtr<Label2DView> _launchInfoLabel;
    IGameDataService* _gameDataService;
    std::unique_ptr<FileIcon> _selectedFileIcon;
    SharedPtr<FileCover> _selectedFileCover;
    int _lastSelectedItem = -1;
    bool _iconGraphicsUploaded = false;
    bool _coverGraphicsUploaded = false;
    bool _showCover;
    Point _coverPosition;
    Point _gameCountPosition;
    bool _gameCountHidden = false;
    Point _launchInfoPosition;
    bool _launchInfoHidden = false;
    u32 _heartVramOffset = 0;
    u32 _checkVramOffset = 0;
    u32 _chipVramOffset = 0;
    bool _selectedFavorite = false;
    bool _selectedCompleted = false;
    int _lastGameDataItem = -1;
    u32 _lastGameDataVersion = 0;
    const MaterialColorScheme* _materialColorScheme;
    // On an L/R jump the game-count chip briefly shows the letter landed on, so
    // the jump is not disorienting; _gameCountText is the count to put back after
    // the hold, and _letterHoldFrames counts it down.
    char _gameCountText[16] = {};
    int _letterHoldFrames = 0;

    RomBrowserTopScreenView(SharedPtr<RomBrowserViewModel> viewModel,
        const RomBrowserDisplayMode* displayMode,
        const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        const IFontRepository* fontRepository,
        const MaterialColorScheme* materialColorScheme);

    void DrawChip(GraphicsContext& graphicsContext, int x, int y, int width, u32 paletteRow);
    void UpdateSortLetterChip(int selectedItem);
};