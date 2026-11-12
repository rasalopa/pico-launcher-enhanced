#pragma once
#include "romBrowser/FileRecyclerAdapter.h"

class IRomBrowserViewFactory;
class VBlankTextureLoader;
class ICoverRepository;

class MaterialCoverFlowFileRecyclerAdapter : public FileRecyclerAdapter
{
public:
    MaterialCoverFlowFileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory,
        VBlankTextureLoader* vblankTextureLoader,
        const ICoverRepository* coverRepository)
        : FileRecyclerAdapter(romBrowserController, fileInfoManager, taskQueue, themeFileIconFactory)
        , _romBrowserViewFactory(romBrowserViewFactory)
        , _vblankTextureLoader(vblankTextureLoader)
        , _coverRepository(coverRepository) { }

    void GetViewSize(int& width, int& height) const override;
    SharedPtr<View> CreateView() const override;

private:
    const IRomBrowserViewFactory* _romBrowserViewFactory;
    VBlankTextureLoader* _vblankTextureLoader;
    const ICoverRepository* _coverRepository;

    void BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo) const override;
    IRomBrowserItemViewModel& GetViewModel(const SharedPtr<View>& view) const override;
    void UnbindView(const SharedPtr<View>& view, int index) const override;
};
