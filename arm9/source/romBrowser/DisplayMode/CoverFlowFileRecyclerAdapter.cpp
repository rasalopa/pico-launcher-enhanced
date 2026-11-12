#include "common.h"
#include "../FileInfoManager.h"
#include "../views/CoverView.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "../FileType/UnknownFileCover.h"
#include "romBrowser/ICoverRepository.h"
#include "romBrowser/viewModels/RomBrowserItemViewModel.h"
#include "CoverFlowFileRecyclerAdapter.h"

void CoverFlowFileRecyclerAdapter::GetViewSize(int& width, int& height) const
{
    width = 44;
    height = 44;
}

SharedPtr<View> CoverFlowFileRecyclerAdapter::CreateView() const
{
    return CoverView::CreateShared(
        std::make_unique<RomBrowserItemViewModel>(_romBrowserController), _vblankTextureLoader);
}

void CoverFlowFileRecyclerAdapter::BindView(SharedPtr<View> view, int index,
    const InternalFileInfo* internalFileInfo) const
{
    auto coverView = static_cast<CoverView*>(view.GetPointer());
    coverView->SetCover(_fileInfoManager->GetFileCover(index));
    coverView->UploadCoverGraphics();
}

IRomBrowserItemViewModel& CoverFlowFileRecyclerAdapter::GetViewModel(const SharedPtr<View>& view) const
{
    return static_cast<CoverView*>(view.GetPointer())->GetViewModel();
}

void CoverFlowFileRecyclerAdapter::UnbindView(const SharedPtr<View>& view, int index) const
{
    auto coverView = static_cast<CoverView*>(view.GetPointer());
    coverView->ClearCover();
    _fileInfoManager->ReleaseFileInfo(index);
}

void CoverFlowFileRecyclerAdapter::InitVram(const VramContext& vramContext)
{
    // _iconGridItemViewGraphics = _romBrowserViewFactory->UploadIconGridItemViewGraphics(vramManager);
}
