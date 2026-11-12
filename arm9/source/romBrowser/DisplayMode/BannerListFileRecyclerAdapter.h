#pragma once
#include "../FileRecyclerAdapter.h"
#include "../views/BannerListItemView.h"

class TaskQueueBase;
class IRomBrowserViewFactory;

class BannerListFileRecyclerAdapter : public FileRecyclerAdapter
{
public:
    BannerListFileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        VBlankTextureLoader* vblankTextureLoader)
        : FileRecyclerAdapter(romBrowserController, fileInfoManager, taskQueue, themeFileIconFactory)
        , _romBrowserViewFactory(romBrowserViewFactory)
        , _vblankTextureLoader(vblankTextureLoader) { }

    void GetViewSize(int& width, int& height) const override;
    SharedPtr<View> CreateView() const override;
    void BindView(SharedPtr<View> view, int index) const override;

    void InitVram(const VramContext& vramContext) override;

private:
    const IRomBrowserViewFactory* _romBrowserViewFactory;
    BannerListItemView::VramToken _bannerListItemViewGraphics;
    VBlankTextureLoader* _vblankTextureLoader;

    void BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo) const override;
    IRomBrowserItemViewModel& GetViewModel(const SharedPtr<View>& view) const override;
    void UnbindView(const SharedPtr<View>& view, int index) const override;
};
