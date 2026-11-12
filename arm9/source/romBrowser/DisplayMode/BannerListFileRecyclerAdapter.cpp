#include "common.h"
#include "../FileInfoManager.h"
#include "../views/BannerListItemView.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "BannerListFileRecyclerAdapter.h"

void BannerListFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = 203;
    height = 44;
}

SharedPtr<View> BannerListFileRecyclerAdapter::CreateView() const
{
    return _romBrowserViewFactory->CreateBannerListItemView(
        std::make_unique<RomBrowserItemViewModel>(_romBrowserController), _vblankTextureLoader);
}

void BannerListFileRecyclerAdapter::BindView(SharedPtr<View> view, int index) const
{
    auto listItemView = static_cast<BannerListItemView*>(view.GetPointer());
    listItemView->SetGraphics(_bannerListItemViewGraphics);
    FileRecyclerAdapter::BindView(view, index);
}

void BannerListFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo) const
{
    auto listItemView = static_cast<BannerListItemView*>(view.GetPointer());
    const auto& fileInfo = _fileInfoManager->GetItem(index);
    bool fileNameAsTitle = true;
    if (internalFileInfo)
    {
        const char16_t* gameTitle = internalFileInfo->GetGameTitle();
        if (gameTitle && gameTitle[0] != 0)
        {
            listItemView->SetGameTitle(gameTitle);
            fileNameAsTitle = false;
        }
    }
    listItemView->SetFileName(fileInfo.GetFileName(), fileNameAsTitle);
    auto icon = internalFileInfo ? internalFileInfo->CreateGameIcon() : nullptr;
    if (!icon)
    {
        icon = fileInfo.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
    }
    if (icon != nullptr)
    {
        icon->SetAnimFrame(_iconFrameCounter);
        listItemView->SetIcon(std::move(icon));
        listItemView->UploadIconGraphics();
    }
}

IRomBrowserItemViewModel& BannerListFileRecyclerAdapter::GetViewModel(const SharedPtr<View>& view) const
{
    return static_cast<BannerListItemView*>(view.GetPointer())->GetViewModel();
}

void BannerListFileRecyclerAdapter::UnbindView(const SharedPtr<View>& view, int index) const
{
    auto listItemView = static_cast<BannerListItemView*>(view.GetPointer());
    listItemView->SetIcon(nullptr);
    listItemView->SetGameTitle(u"");
    _fileInfoManager->ReleaseFileInfo(index);
}

void BannerListFileRecyclerAdapter::InitVram(const VramContext& vramContext)
{
    _bannerListItemViewGraphics = _romBrowserViewFactory->UploadBannerListItemViewGraphics(vramContext);
}
