#include "common.h"
#include "romBrowser/FileInfoManager.h"
#include "MaterialCoverView.h"
#include "romBrowser/Theme/IRomBrowserViewFactory.h"
#include "romBrowser/FileType/UnknownFileCover.h"
#include "romBrowser/ICoverRepository.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "MaterialCoverFlowFileRecyclerAdapter.h"

void MaterialCoverFlowFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = COVER_WIDTH;
    height = COVER_HEIGHT;
}

SharedPtr<View> MaterialCoverFlowFileRecyclerAdapter::CreateView() const
{
    return MaterialCoverView::CreateShared(
        std::make_unique<RomBrowserItemViewModel>(_romBrowserController), _vblankTextureLoader);
}

void MaterialCoverFlowFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo) const
{
    auto coverView = static_cast<MaterialCoverView*>(view.GetPointer());
    coverView->SetCover(_fileInfoManager->GetFileCover(index));
    coverView->UploadCoverGraphics();
}

IRomBrowserItemViewModel& MaterialCoverFlowFileRecyclerAdapter::GetViewModel(const SharedPtr<View>& view) const
{
    return static_cast<MaterialCoverView*>(view.GetPointer())->GetViewModel();
}

void MaterialCoverFlowFileRecyclerAdapter::UnbindView(const SharedPtr<View>& view, int index) const
{
    auto coverView = static_cast<MaterialCoverView*>(view.GetPointer());
    coverView->ClearCover();
    _fileInfoManager->ReleaseFileInfo(index);
}