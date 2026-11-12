#pragma once
#include "../FileRecyclerAdapter.h"
#include "../views/IconGridItemView.h"

class IRomBrowserViewFactory;

class IconGridFileRecyclerAdapter : public FileRecyclerAdapter
{
public:
    IconGridFileRecyclerAdapter(IRomBrowserController* romBrowserController, FileInfoManager* fileInfoManager,
        TaskQueueBase* taskQueue, const IThemeFileIconFactory* themeFileIconFactory,
        const IRomBrowserViewFactory* romBrowserViewFactory)
        : FileRecyclerAdapter(romBrowserController, fileInfoManager, taskQueue, themeFileIconFactory)
        , _romBrowserViewFactory(romBrowserViewFactory) { }

    void GetViewSize(int& width, int& height) const override;
    SharedPtr<View> CreateView() const override;
    void BindView(SharedPtr<View> view, int index) const override;

    void InitVram(const VramContext& vramContext) override;

private:
    const IRomBrowserViewFactory* _romBrowserViewFactory;
    IconGridItemView::VramToken _iconGridItemViewGraphics;

    void BindView(SharedPtr<View> view, int index,
        const InternalFileInfo* internalFileInfo) const override;
    IRomBrowserItemViewModel& GetViewModel(const SharedPtr<View>& view) const override;
    void UnbindView(const SharedPtr<View>& view, int index) const override;
};
