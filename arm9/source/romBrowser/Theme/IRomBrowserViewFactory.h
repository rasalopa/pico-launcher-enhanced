#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "core/math/Point.h"
#include "TopStatusBarStyle.h"
#include "../views/IconGridItemView.h"
#include "../views/BannerListItemView.h"
#include "../views/AppBarView.h"
#include "../views/BannerView.h"
#include "gui/views/RecyclerViewBase.h"

class VramContext;
class VBlankTextureLoader;
class RomBrowserViewModel;
class IThemeFileIconFactory;
class FileRecyclerAdapter;
class IRomBrowserItemViewModel;

// Where the selected game's markers (star, check, heart, and the launch text
// when it is switched on) go. Unless centered, position is the top-right corner
// of their pill, which grows to the left; centered, it is the middle of the
// row's top edge. hidden suppresses the pill, its text and its markers entirely.
struct TopStripElementLayout
{
    Point position;
    bool hidden;
    // Centred on position.x when set - one marker or three, the row keeps its
    // middle there - and laid out to the left of position otherwise, which is
    // then the pill's top-right corner.
    bool centered = false;
    // Drawn straight on the screen, with no pill under them, as two-tone
    // outlined sprites that read on any art. Material does this above the icon
    // of its card, and so does a custom theme that does not place the markers
    // itself; one that does keeps the pill where it put it.
    bool bare = false;
};

class IRomBrowserViewFactory
{
public:
    virtual ~IRomBrowserViewFactory() = 0;

    virtual SharedPtr<IconGridItemView> CreateIconGridItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel) const = 0;
    virtual IconGridItemView::VramToken UploadIconGridItemViewGraphics(
        const VramContext& vramContext) const { return IconGridItemView::VramToken(0); }

    virtual SharedPtr<BannerListItemView> CreateBannerListItemView(std::unique_ptr<IRomBrowserItemViewModel> viewModel,
        VBlankTextureLoader* vblankTextureLoader) const = 0;
    virtual BannerListItemView::VramToken UploadBannerListItemViewGraphics(
        const VramContext& vramContext) const { return BannerListItemView::VramToken(0); }

    virtual SharedPtr<AppBarView> CreateAppBarView(int x, int y, AppBarView::Orientation orientation,
        int startButtonCount, int endButtonCount) const = 0;

    virtual SharedPtr<BannerView> CreateFileInfoView() const = 0;

    virtual SharedPtr<RecyclerViewBase> CreateCoverFlowRecyclerView() const = 0;

    virtual SharedPtr<FileRecyclerAdapter> CreateCoverFlowRecyclerAdapter(
        RomBrowserViewModel* viewModel, const IThemeFileIconFactory* themeFileIconFactory,
        VBlankTextureLoader* vblankTextureLoader) const = 0;

    virtual Point GetTopCoverPosition() const = 0;

    virtual TopStripElementLayout GetTopLaunchInfoLayout() const = 0;

    /// @brief The art, positions and colors of the clock and battery bar along
    ///        the top of the top screen. Valid as long as the factory is.
    virtual const TopStatusBarStyle& GetTopStatusBarStyle() const = 0;
};

inline IRomBrowserViewFactory::~IRomBrowserViewFactory() { }
