#include "common.h"
#include <string.h>
#include "core/mini-printf.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "DeleteConfirmBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define LINE_X              20
#define FILE_NAME_Y         44
#define SAVE_Y              62
#define SAVE_END_Y          78
#define HINT_Y              96

#define LINE_WIDTH          216

static const char* const kHint = "X: delete    A/B: cancel";

DeleteConfirmBottomSheetView::DeleteConfirmBottomSheetView(SharedPtr<IDeleteConfirmViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _fileNameLabel(Label2DView::CreateShared(LINE_WIDTH, 16, 256, fontRepository->GetFont(FontType::Regular10)))
    , _saveLabel(Label2DView::CreateShared(LINE_WIDTH, 16, 270, fontRepository->GetFont(FontType::Medium7_5)))
    , _saveEndLabel(Label2DView::CreateShared(LINE_WIDTH, 16, 40, fontRepository->GetFont(FontType::Medium7_5)))
    , _hintLabel(Label2DView::CreateShared(LINE_WIDTH, 16, 40, fontRepository->GetFont(FontType::Medium7_5)))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel->SetText(_viewModel->GetTitle());
    _fileNameLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Marquee);
    if (const char16_t* name16 = _viewModel->GetNameLine16())
        _fileNameLabel->SetText(name16);
    else
        _fileNameLabel->SetText(_viewModel->GetNameLine());
    const char* detailLine = _viewModel->GetDetailLine();
    _hasDetail = detailLine != nullptr && detailLine[0] != 0;
    if (_hasDetail)
    {
        _saveLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
        _saveLabel->SetText(detailLine);
    }
    const char* detailEndLine = _viewModel->GetDetailEndLine();
    _hasDetailEnd = _hasDetail && detailEndLine != nullptr && detailEndLine[0] != 0;
    if (_hasDetailEnd)
    {
        _saveEndLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
        _saveEndLabel->SetText(detailEndLine);
    }
    _hintLabel->SetText(kHint);
    AddChildTail(_titleLabel.GetPointer());
    AddChildTail(_fileNameLabel.GetPointer());
    if (_hasDetail)
        AddChildTail(_saveLabel.GetPointer());
    if (_hasDetailEnd)
        AddChildTail(_saveEndLabel.GetPointer());
    AddChildTail(_hintLabel.GetPointer());
}

void DeleteConfirmBottomSheetView::Update()
{
    // The view model can replace the hint, e.g. to say how a delete went. Once
    // there is an answer it stays on screen until the sheet is gone: the status
    // clears as the sheet starts to close, and "X: delete" must not come back.
    const char* status = _viewModel->GetStatusLine();
    if (status != nullptr && status != _shownStatus)
    {
        _shownStatus = status;
        _hintLabel->SetText(status);
    }
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _fileNameLabel->SetPosition(LINE_X, _position.y + FILE_NAME_Y);
    _saveLabel->SetPosition(LINE_X, _position.y + SAVE_Y);
    _saveEndLabel->SetPosition(LINE_X, _position.y + SAVE_END_Y);
    _hintLabel->SetPosition(LINE_X, _position.y + HINT_Y);
    BottomSheetView::Update();
}

void DeleteConfirmBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);

        _fileNameLabel->SetBackgroundColor(backColor);
        _fileNameLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _fileNameLabel->Draw(graphicsContext);

        if (_hasDetail)
        {
            _saveLabel->SetBackgroundColor(backColor);
            _saveLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _saveLabel->Draw(graphicsContext);
        }

        if (_hasDetailEnd)
        {
            _saveEndLabel->SetBackgroundColor(backColor);
            _saveEndLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _saveEndLabel->Draw(graphicsContext);
        }

        _hintLabel->SetBackgroundColor(backColor);
        _hintLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
        _hintLabel->Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool DeleteConfirmBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::X))
    {
        // ConfirmDelete queues the actual deletion; guard against repeats
        // while the folder reload is pending
        if (!_confirmed)
        {
            _confirmed = true;
            _viewModel->Confirm();
        }
        return true;
    }
    if (inputProvider.Triggered(InputKey::A) || inputProvider.Triggered(InputKey::B))
    {
        if (!_confirmed)
            _viewModel->Cancel();
        return true;
    }
    return false;
}

void DeleteConfirmBottomSheetView::Focus(FocusManager& focusManager)
{
    // focus a CHILD of the sheet: FocusManager::Update skips parent-less
    // focused views (keys would never arrive if the sheet focused itself)
    focusManager.Focus(_titleLabel->SharedFromThis());
}

void DeleteConfirmBottomSheetView::Close()
{
    // pen tap outside the sheet
    if (!_confirmed)
        _viewModel->Cancel();
}
