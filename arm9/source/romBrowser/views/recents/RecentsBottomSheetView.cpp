#include "common.h"
#include "services/settings/Localization.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "cheatSelector.h"
#include "smallHeartIconFilled.h"
#include "RecentsBottomSheetView.h"

#define TITLE_LABEL_X               20
#define TITLE_LABEL_Y               16

#define EMPTY_LABEL_X               20
#define EMPTY_LABEL_Y               36

#define LIST_X                      16
#define LIST_Y                      40
#define LIST_WIDTH                  224
#define LIST_HEIGHT                 120

RecentsBottomSheetView::RecentsBottomSheetView(SharedPtr<RecentsViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titleLabel(Label2DView::CreateShared(128, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _emptyLabel(Label2DView::CreateShared(192, 16, 32, fontRepository->GetFont(FontType::Regular10)))
    , _recentsRecycler(RecyclerView::CreateShared(
        LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT, RecyclerView::Mode::VerticalList))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    if (_viewModel->GetKind() == GameListKind::Recents)
    {
        _titleLabel->SetText(Localization::RecentGames());
        _emptyLabel->SetText(Localization::NothingPlayedYet());
    }
    else
    {
        _titleLabel->SetText(Localization::FavoriteGames());
        _emptyLabel->SetText(Localization::NoFavoritesYet());
    }
    AddChildTail(_titleLabel.GetPointer());
    if (_viewModel->GetItemCount() == 0)
        AddChildTail(_emptyLabel.GetPointer());
    else
        AddChildTail(_recentsRecycler.GetPointer());
}

void RecentsBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _vramOffsets.selectorVramOffset
            = LoadSprite(*objVramManager, cheatSelectorTiles, cheatSelectorTilesLen);
        _vramOffsets.heartIconVramOffset
            = LoadSprite(*objVramManager, smallHeartIconFilledTiles, smallHeartIconFilledTilesLen);
    }

    _objVramManager = vramContext.GetObjVramManager();
}

void RecentsBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _emptyLabel->SetPosition(EMPTY_LABEL_X, _position.y + EMPTY_LABEL_Y);
    _recentsRecycler->SetPosition(LIST_X, _position.y + LIST_Y);
    if (_viewModel->GetItemCount() > 0 && !_recentsAdapter && _objVramManager != nullptr)
    {
        _recentsAdapter = SharedPtr<RecentsAdapter>::MakeShared(
            _viewModel, _materialColorScheme, _fontRepository, _vramOffsets);
        _recentsRecycler->SetAdapter(_recentsAdapter);
        _recentsRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
        _recentsRecycler->Focus(*_focusManager);
    }
    BottomSheetView::Update();
    _viewModel->SetSelectedItem(_recentsRecycler->GetSelectedItem());
}

void RecentsBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        if (_recentsAdapter)
        {
            graphicsContext.SetClipArea(_recentsRecycler->GetBounds());
            _recentsRecycler->Draw(graphicsContext);
            graphicsContext.SetClipArea(GetBounds());

            // mask strip above the list so scrolled-out rows don't bleed into the title
            auto maskOam = graphicsContext.GetOamManager().AllocOams(4);
            u32 maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(backColor, backColor),
                _position.y + LIST_Y - 24, _position.y + LIST_Y);
            for (int i = 0; i < 4; i++)
            {
                int x = LIST_X + (i < 3 ? i * 64 : 2 * 64 + 32);
                OamBuilder::OamWithSize<64, 32>(x, _position.y + LIST_Y - 24, _vramOffsets.selectorVramOffset >> 7)
                    .WithPalette16(maskPaletteRow)
                    .WithPriority(graphicsContext.GetPriority())
                    .Build(maskOam[i]);
            }
        }

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);

        if (_viewModel->GetItemCount() == 0)
        {
            _emptyLabel->SetBackgroundColor(backColor);
            _emptyLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _emptyLabel->Draw(graphicsContext);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

void RecentsBottomSheetView::Focus(FocusManager& focusManager)
{
    if (_viewModel->GetItemCount() > 0)
    {
        _recentsRecycler->Focus(focusManager);
    }
    else
    {
        // an empty sheet must still capture key input (B to close). Focus a
        // CHILD of the sheet: FocusManager::Update skips parent-less focused
        // views, so focusing the sheet itself would never deliver keys.
        focusManager.Focus(_titleLabel->SharedFromThis());
    }
}

bool RecentsBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

void RecentsBottomSheetView::Close()
{
    _viewModel->Close();
}

u32 RecentsBottomSheetView::LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}
