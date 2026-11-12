#include "common.h"
#include "../FileInfoManager.h"
#include "../views/IconGridItemView.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "IconGridFileRecyclerAdapter.h"

void IconGridFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = 44;
    height = 44;
}

SharedPtr<View> IconGridFileRecyclerAdapter::CreateView() const
{
    return _romBrowserViewFactory->CreateIconGridItemView(std::make_unique<RomBrowserItemViewModel>(_romBrowserController));
}

void IconGridFileRecyclerAdapter::BindView(SharedPtr<View> view, int index) const
{
    auto iconGridItemView = static_cast<IconGridItemView*>(view.GetPointer());
    iconGridItemView->SetGraphics(_iconGridItemViewGraphics);
    FileRecyclerAdapter::BindView(view, index);
}

void IconGridFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo) const
{
    auto iconGridItemView = static_cast<IconGridItemView*>(view.GetPointer());
    auto icon = internalFileInfo ? internalFileInfo->CreateGameIcon() : nullptr;
    if (!icon)
    {
        const auto& fileInfo = _fileInfoManager->GetItem(index);
        icon = fileInfo.GetFileType()->CreateFileIcon(fileInfo.GetFileName(), _themeFileIconFactory);
    }
    if (icon != nullptr)
    {
        icon->SetAnimFrame(_iconFrameCounter);
        iconGridItemView->SetIcon(std::move(icon));
        iconGridItemView->UploadIconGraphics();
    }
}

IRomBrowserItemViewModel& IconGridFileRecyclerAdapter::GetViewModel(const SharedPtr<View>& view) const
{
    return static_cast<IconGridItemView*>(view.GetPointer())->GetViewModel();
}

void IconGridFileRecyclerAdapter::UnbindView(const SharedPtr<View>& view, int index) const
{
    auto iconGridItemView = static_cast<IconGridItemView*>(view.GetPointer());
    iconGridItemView->SetIcon(nullptr);
    _fileInfoManager->ReleaseFileInfo(index);
}

void IconGridFileRecyclerAdapter::InitVram(const VramContext& vramContext)
{
    _iconGridItemViewGraphics = _romBrowserViewFactory->UploadIconGridItemViewGraphics(vramContext);
}
