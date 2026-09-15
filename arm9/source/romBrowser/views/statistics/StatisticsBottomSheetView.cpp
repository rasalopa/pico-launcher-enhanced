#include "common.h"
#include "services/settings/Localization.h"
#include <string.h>
#include "core/mini-printf.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "StatisticsBottomSheetView.h"

#define TITLE_LABEL_X       20
#define TITLE_LABEL_Y       16

#define LINE_X              20
#define LINE_FIRST_Y        40
#define LINE_SPACING        16

#define LINE_WIDTH          216
#define LINE_MAX_CHARS      120

StatisticsBottomSheetView::StatisticsBottomSheetView(SharedPtr<StatisticsViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _materialColorScheme(materialColorScheme)
{
    _titleLabel->SetText(Localization::Statistics());
    AddChildTail(_titleLabel.GetPointer());

    char text[144];
    if (_viewModel->GetPlayedCount() == 0 && _viewModel->GetFavoriteCount() == 0 &&
        _viewModel->GetCompletedCount() == 0)
    {
        AddLine(fontRepository, FontType::Regular10, Localization::NothingPlayedYet());
    }
    else
    {
        if (_viewModel->GetCompletedCount() > 0)
        {
            mini_snprintf(text, sizeof(text), Localization::PlayedFavoritesCompletedFormat(),
                _viewModel->GetPlayedCount(), _viewModel->GetFavoriteCount(),
                _viewModel->GetCompletedCount());
        }
        else
        {
            mini_snprintf(text, sizeof(text), Localization::PlayedFavoritesFormat(),
                _viewModel->GetPlayedCount(), _viewModel->GetFavoriteCount());
        }
        AddLine(fontRepository, FontType::Regular10, text);

        u32 playMinutes = _viewModel->GetTotalPlayMinutes();
        if (playMinutes > 0)
        {
            mini_snprintf(text, sizeof(text), Localization::LaunchesPlayedFormat(),
                _viewModel->GetTotalLaunches(), playMinutes / 60, playMinutes % 60);
        }
        else
        {
            mini_snprintf(text, sizeof(text), Localization::LaunchesTotalFormat(),
                _viewModel->GetTotalLaunches());
        }
        AddLine(fontRepository, FontType::Regular10, text);

        for (u32 t = 0; t < _viewModel->GetTopCount(); t++)
        {
            const auto& entry = _viewModel->GetTopEntry(t);
            mini_snprintf(text, sizeof(text), "%u. %s (%ux)",
                t + 1, entry.fileName.GetString(), entry.launchCount);
            AddLine(fontRepository, FontType::Medium7_5, text);
        }

        const char* lastPlayed = _viewModel->GetLastPlayed().lastPlayed.GetString();
        if (strlen(lastPlayed) >= 16)
        {
            mini_snprintf(text, sizeof(text), Localization::LastPlayedFormat(),
                _viewModel->GetLastPlayed().fileName.GetString(),
                lastPlayed[8], lastPlayed[9], lastPlayed[5], lastPlayed[6],
                lastPlayed[11], lastPlayed[12], lastPlayed[14], lastPlayed[15]);
            AddLine(fontRepository, FontType::Medium7_5, text);
        }
    }
}

void StatisticsBottomSheetView::AddLine(const IFontRepository* fontRepository, FontType fontType, const char* text)
{
    if (_lineCount >= MAX_LINES)
        return;
    auto label = Label2DView::CreateShared(LINE_WIDTH, 16, LINE_MAX_CHARS, fontRepository->GetFont(fontType));
    label->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    label->SetText(text);
    AddChildTail(label.GetPointer());
    _lines[_lineCount++] = std::move(label);
}

void StatisticsBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    for (u32 i = 0; i < _lineCount; i++)
        _lines[i]->SetPosition(LINE_X, _position.y + LINE_FIRST_Y + i * LINE_SPACING);
    BottomSheetView::Update();
}

void StatisticsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);

        for (u32 i = 0; i < _lineCount; i++)
        {
            _lines[i]->SetBackgroundColor(backColor);
            _lines[i]->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _lines[i]->Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool StatisticsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

void StatisticsBottomSheetView::Focus(FocusManager& focusManager)
{
    // focus a CHILD of the sheet: FocusManager::Update skips parent-less
    // focused views, so focusing the sheet itself would never deliver keys.
    // Input bubbles from the label up to this sheet's HandleInput (B).
    focusManager.Focus(_titleLabel->SharedFromThis());
}

void StatisticsBottomSheetView::Close()
{
    _viewModel->Close();
}
