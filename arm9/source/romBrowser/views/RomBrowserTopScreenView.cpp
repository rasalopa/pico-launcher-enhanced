#include "common.h"
#include "services/settings/Localization.h"
#include <algorithm>
#include <ctype.h>
#include <string.h>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxWindow.h>
#include <libtwl/dma/dmaNitro.h>
#include "core/mini-printf.h"
#include "../viewModels/RomBrowserViewModel.h"
#include "../IRomBrowserController.h"
#include "services/gamedata/IGameDataService.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "../Theme/IRomBrowserViewFactory.h"
#include "smallHeartIconFilled.h"
#include "checkIcon.h"
#include "stripChipBg.h"
#include "RomBrowserTopScreenView.h"

RomBrowserTopScreenView::RomBrowserTopScreenView(
    SharedPtr<RomBrowserViewModel> viewModel,
    const RomBrowserDisplayMode* displayMode,
    const IThemeFileIconFactory* themeFileIconFactory,
    const IRomBrowserViewFactory* romBrowserViewFactory,
    const IFontRepository* fontRepository,
    const MaterialColorScheme* materialColorScheme)
    : _viewModel(std::move(viewModel))
    , _themeFileIconFactory(themeFileIconFactory)
    , _fileInfoView(romBrowserViewFactory->CreateFileInfoView())
    , _showCover(displayMode->ShowCoverOnTopScreen())
    , _coverPosition(romBrowserViewFactory->GetTopCoverPosition())
{
    AddChildTail(_fileInfoView.GetPointer());

    // the strip is positioned by the theme (top-left corner for the game count,
    // top-right corner for the launch info) so a custom theme can move or hide
    // it away from its own top art; clamp to the screen so a malformed theme.json
    // can't place the OBJs at coordinates that wrap in OAM
    auto gameCountLayout = romBrowserViewFactory->GetTopGameCountLayout();
    _gameCountHidden = gameCountLayout.hidden;
    _gameCountPosition = Point(std::clamp(gameCountLayout.position.x, 0, 256),
        std::clamp(gameCountLayout.position.y, 0, 192));
    auto launchInfoLayout = romBrowserViewFactory->GetTopLaunchInfoLayout();
    _launchInfoHidden = launchInfoLayout.hidden;
    _launchInfoPosition = Point(std::clamp(launchInfoLayout.position.x, 0, 256),
        std::clamp(launchInfoLayout.position.y, 0, 192));

    u32 gameCount = _viewModel->GetFileInfoManager().GetGameCount();
    if (gameCount > 0 && !_gameCountHidden)
    {
        mini_snprintf(_gameCountText, sizeof(_gameCountText), Localization::GameCountFormat(), gameCount, Localization::GameWord(gameCount));
        _gameCountLabel = Label2DView::CreateShared(96, 16, 15, fontRepository->GetFont(FontType::Medium10));
        _gameCountLabel->SetText(_gameCountText);
        // Draw() puts a chip behind each strip cluster so the strip stays readable
        // over any theme art; the label sits 6px in from the pill's left edge and
        // 2px down from its top
        _gameCountLabel->SetPosition(_gameCountPosition.x + 6, _gameCountPosition.y + 2);
        _gameCountLabel->SetBackgroundColor(materialColorScheme->surfaceBright);
        _gameCountLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
        AddChildTail(_gameCountLabel.GetPointer());
    }

    _gameDataService = _viewModel->GetRomBrowserController()->GetGameDataService();
    _materialColorScheme = materialColorScheme;
    // launch info for the selected game ("3x 16/07"), packed right to left in
    // the top strip with the completed check + favorite heart; Draw() positions
    // it so the cluster's chip is sized to its content
    _launchInfoLabel = Label2DView::CreateShared(96, 16, 15, fontRepository->GetFont(FontType::Medium10));
    _launchInfoLabel->SetHorizontalAlignment(Alignment::End);
    _launchInfoLabel->SetPosition(_launchInfoPosition.x - 96, _launchInfoPosition.y + 2);
    _launchInfoLabel->SetBackgroundColor(materialColorScheme->surfaceBright);
    _launchInfoLabel->SetForegroundColor(materialColorScheme->onSurfaceVariant);
    if (!_launchInfoHidden)
        AddChildTail(_launchInfoLabel.GetPointer());
}

void RomBrowserTopScreenView::InitVram(const VramContext& vramContext)
{
    ViewContainer::InitVram(vramContext);
    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _heartVramOffset = objVramManager->Alloc(smallHeartIconFilledTilesLen);
        dma_ntrCopy32(3, smallHeartIconFilledTiles,
            objVramManager->GetVramAddress(_heartVramOffset), smallHeartIconFilledTilesLen);
        _checkVramOffset = objVramManager->Alloc(checkIconTilesLen);
        dma_ntrCopy32(3, checkIconTiles,
            objVramManager->GetVramAddress(_checkVramOffset), checkIconTilesLen);
        _chipVramOffset = objVramManager->Alloc(stripChipBgTilesLen);
        dma_ntrCopy32(3, stripChipBgTiles,
            objVramManager->GetVramAddress(_chipVramOffset), stripChipBgTilesLen);
    }
    int tileIndex = 0;
    vu16* mapPtr = (vu16*)((u8*)GFX_BG_SUB + 0x3800);
    for (int y = 0; y < 12; y++)
    {
        for (int x = 0; x < 14; x++)
        {
            *mapPtr++ = tileIndex;
            tileIndex++;
        }
        mapPtr += 2;
    }
}

void RomBrowserTopScreenView::Update()
{
    int selectedItem = _viewModel->GetSelectedItem();
    if (selectedItem != _lastSelectedItem && selectedItem >= 0)
    {
        auto& fileInfoManager = _viewModel->GetFileInfoManager();
        // GetInternalFileInfo() covers both game banners and custom icon overrides (the
        // latter apply to any file type, including folders), so check it directly instead
        // of branching on FileType::HasInternalFileInfo() - that's a static per-type property
        // and knows nothing about a per-item custom icon. IsFileInfoLoaded() distinguishes
        // "still loading" from "loaded, and there's legitimately nothing" so this waits for
        // the io thread instead of flashing the previous item's icon while undecided.
        if (fileInfoManager.IsFileInfoLoaded(selectedItem))
        {
            const auto& item = fileInfoManager.GetItem(selectedItem);
            auto info = fileInfoManager.GetInternalFileInfo(selectedItem);

            bool fileNameAsTitle = true;
            const char16_t* gameTitle = info ? info->GetGameTitle() : nullptr;
            if (gameTitle && gameTitle[0] != 0)
            {
                _fileInfoView->SetGameTitleAsync(_viewModel->GetBgTaskQueue(), gameTitle);
                fileNameAsTitle = false;
            }

            _selectedFileIcon = info ? info->CreateGameIcon() : nullptr;
            if (!_selectedFileIcon)
            {
                _selectedFileIcon = item.GetFileType()->CreateFileIcon("", _themeFileIconFactory);
            }
            if (_selectedFileIcon)
            {
                _selectedFileIcon->SetAnimFrame(_viewModel->GetIconFrameCounter());
                _iconGraphicsUploaded = false;
            }
            _fileInfoView->SetIcon(std::move(_selectedFileIcon));
            _fileInfoView->SetFileNameAsync(_viewModel->GetBgTaskQueue(), item.GetFileName(), fileNameAsTitle);

            _lastSelectedItem = selectedItem;

            auto cover = fileInfoManager.GetFileCover(selectedItem);
            if (cover.IsValid())
            {
                _selectedFileCover = std::move(cover);
                _coverGraphicsUploaded = false;
            }
        }
    }

    u32 gameDataVersion = _gameDataService->GetVersion();
    if (selectedItem != _lastGameDataItem || gameDataVersion != _lastGameDataVersion)
    {
        _selectedFavorite = false;
        _selectedCompleted = false;
        char info[24];
        info[0] = 0;
        if (selectedItem >= 0)
        {
            const auto& item = _viewModel->GetFileInfoManager().GetItem(selectedItem);
            // by file name, exactly like the browser filter resolves it. This
            // used to also try the loaded rom header's game code, which made the
            // heart appear for a file the filter could not match (issue #7), and
            // meant the strip had to wait for the header to load before it could
            // show anything.
            const auto* entry = _gameDataService->GetEntry(item.GetFileName());
            if (entry)
            {
                _selectedFavorite = entry->favorite;
                _selectedCompleted = entry->completed;
                if (entry->launchCount > 0)
                {
                    if (entry->playMinutes >= 60)
                    {
                        mini_snprintf(info, sizeof(info), "%ux %uh%02u", entry->launchCount,
                            entry->playMinutes / 60, entry->playMinutes % 60);
                    }
                    else if (entry->playMinutes > 0)
                    {
                        mini_snprintf(info, sizeof(info), "%ux %um", entry->launchCount,
                            entry->playMinutes);
                    }
                    else if (strlen(entry->lastPlayed.GetString()) >= 10)
                    {
                        // stored as "YYYY-MM-DD HH:MM", shown as "3x · 16 Jul" (a bare
                        // "16/07" reads like a fraction to new users). The separator is
                        // the middle dot U+00B7, which the Medium10 font provides.
                        const char* lastPlayed = entry->lastPlayed.GetString();
                        u32 month = (lastPlayed[5] - '0') * 10 + (lastPlayed[6] - '0');
                        u32 day = (lastPlayed[8] - '0') * 10 + (lastPlayed[9] - '0');
                        if (month >= 1 && month <= 12)
                        {
                            mini_snprintf(info, sizeof(info), "%ux · %u %s", entry->launchCount,
                                day, Localization::Month(month));
                        }
                        else
                        {
                            mini_snprintf(info, sizeof(info), "%ux", entry->launchCount);
                        }
                    }
                    else
                    {
                        mini_snprintf(info, sizeof(info), "%ux", entry->launchCount);
                    }
                }
            }
        }
        _launchInfoLabel->SetText(info);
        _lastGameDataItem = selectedItem;
        _lastGameDataVersion = gameDataVersion;
    }

    UpdateSortLetterChip(selectedItem);
    ViewContainer::Update();
}

// An L/R jump crosses the list by whole initials, which is easy to lose your place
// in, so the game-count chip does double duty: on a jump it flashes the letter you
// landed on for a moment, then settles back to the count. A plain d-pad step does
// not - the controller only flags L/R jumps, which happen only in name sort, so the
// letter is always meaningful. Reuses the chip that is already there, so there is
// nothing new to place and a theme that hides the count hides this too.
// kLetterHoldFrames is counted in Update() calls (one a frame), so ~2s at 60 fps.
static constexpr int kLetterHoldFrames = 120;

void RomBrowserTopScreenView::UpdateSortLetterChip(int selectedItem)
{
    // Drain the jump flag every frame, before the no-chip guard below. The flag
    // lives on the controller and outlives this view, so a jump made in a folder
    // with no game count (one holding only subfolders) must not leave it set for
    // the next folder's freshly created top screen to consume and flash a letter
    // on plain entry.
    bool jumped = _viewModel->GetRomBrowserController()->ConsumeBigStepJump();

    if (!_gameCountLabel)
        return;

    if (jumped && selectedItem >= 0)
    {
        char initial = (char)toupper((unsigned char)
            _viewModel->GetFileInfoManager().GetItem(selectedItem).GetFileName()[0]);
        char letter[2] = { initial, 0 };
        _gameCountLabel->SetText(letter);
        _letterHoldFrames = kLetterHoldFrames;
    }

    if (_letterHoldFrames > 0 && --_letterHoldFrames == 0)
        _gameCountLabel->SetText(_gameCountText);
}

void RomBrowserTopScreenView::Draw(GraphicsContext& graphicsContext)
{
    // widths follow the currently displayed string (updated at vblank), so the
    // chips always match the text on screen
    u32 gameCountWidth = _gameCountLabel ? _gameCountLabel->GetStringWidth() : 0;
    // a hidden launch info suppresses its text, heart and check together
    bool showFavorite = _selectedFavorite && !_launchInfoHidden;
    bool showCompleted = _selectedCompleted && !_launchInfoHidden;
    u32 launchInfoWidth = _launchInfoHidden ? 0 : _launchInfoLabel->GetStringWidth();
    int clusterWidth = 0;
    if (launchInfoWidth > 0)
        clusterWidth += launchInfoWidth + 2;
    if (showCompleted)
        clusterWidth += 16 + 2;
    if (showFavorite)
        clusterWidth += 16 + 2;
    if (clusterWidth > 0)
        clusterWidth -= 2;
    int heartX = 0;
    int checkX = 0;
    if (gameCountWidth > 0 || clusterWidth > 0)
    {
        // both chips share one palette row (SimplePaletteManager doesn't dedup)
        u32 chipPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(_materialColorScheme->outline, _materialColorScheme->surfaceBright), 0, 18);
        if (gameCountWidth > 0)
            DrawChip(graphicsContext, _gameCountPosition.x, _gameCountPosition.y,
                std::max((int)gameCountWidth + 12, 38), chipPaletteRow);
        if (clusterWidth > 0)
        {
            // right chip's right edge sits at the themed x, content centered with 6px padding
            int chipWidth = std::max(clusterWidth + 12, 38);
            int chipX = _launchInfoPosition.x - chipWidth;
            int x = chipX + (chipWidth - clusterWidth) / 2;
            if (launchInfoWidth > 0)
            {
                _launchInfoLabel->SetPosition(x + (int)launchInfoWidth - 96, _launchInfoPosition.y + 2);
                x += launchInfoWidth + 2;
            }
            if (showCompleted)
            {
                checkX = x;
                x += 16 + 2;
            }
            if (showFavorite)
                heartX = x;
            DrawChip(graphicsContext, chipX, _launchInfoPosition.y, chipWidth, chipPaletteRow);
        }
    }
    // the labels draw after the chips and therefore get lower oam indices,
    // which puts them in front
    ViewContainer::Draw(graphicsContext);
    if (showFavorite)
    {
        auto oams = graphicsContext.GetOamManager().AllocOams(1);
        u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(_materialColorScheme->surfaceBright, _materialColorScheme->primary), 1, 17);
        OamBuilder::OamWithSize<16, 16>(heartX, _launchInfoPosition.y + 1, _heartVramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(oams[0]);
    }
    if (showCompleted)
    {
        auto oams = graphicsContext.GetOamManager().AllocOams(1);
        u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(_materialColorScheme->surfaceBright, Rgb<8, 8, 8>(67, 160, 71)), 1, 17);
        OamBuilder::OamWithSize<16, 16>(checkX, _launchInfoPosition.y + 1, _checkVramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(oams[0]);
    }
}

// 18px tall pill behind a strip cluster, built from 32x32 pieces whose left 6
// columns are rounded: a left cap, middle pieces at a 26px stride and an h-flipped
// right cap. Oams come out of AllocOams front to back, so each piece hides the
// rounded columns of the piece to its right. Minimum width is 38 to keep the caps'
// flat columns out of each other's corners. Pieces are semi-transparent OBJs (see
// the blend setup in VBlank) so the theme art shows faintly through the pill.
void RomBrowserTopScreenView::DrawChip(GraphicsContext& graphicsContext, int x, int y, int width, u32 paletteRow)
{
    int middleCount = width > 64 ? (width - 64 + 25) / 26 : 0;
    auto oams = graphicsContext.GetOamManager().AllocOams(middleCount + 2);
    OamBuilder::OamWithSize<32, 32>(x, y, _chipVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .AsTranslucent()
        .Build(oams[0]);
    for (int i = 0; i < middleCount; i++)
    {
        OamBuilder::OamWithSize<32, 32>(x + 26 * (i + 1), y, _chipVramOffset >> 7)
            .WithPalette16(paletteRow)
            .WithPriority(graphicsContext.GetPriority())
            .AsTranslucent()
            .Build(oams[1 + i]);
    }
    OamBuilder::OamWithSize<32, 32>(x + width - 32, y, _chipVramOffset >> 7)
        .WithPalette16(paletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .WithHFlip()
        .AsTranslucent()
        .Build(oams[middleCount + 1]);
}

void RomBrowserTopScreenView::MirrorToMainEngine() const
{
    // Same numbers as VBlank puts on the sub engine, aimed at the main one.
    int x0 = std::clamp(_coverPosition.x, 0, 256);
    int x1 = std::clamp(_coverPosition.x + 106, 0, 256);
    int y0 = std::clamp(_coverPosition.y, 0, 192);
    int y1 = std::clamp(_coverPosition.y + 96, 0, 192);
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover() ||
        x0 >= x1 || y0 >= y1)
    {
        // No cover on screen, so nothing to restate: the mirrored display
        // control already has this background and its window switched off.
        return;
    }
    REG_BG3PA = 0x100;
    REG_BG3PB = 0;
    REG_BG3PC = 0;
    REG_BG3PD = -0x100;
    REG_BG3X = (-_coverPosition.x) << 8;
    REG_BG3Y = (96 + _coverPosition.y - 1) << 8;
    gfx_setWindow0(x0, y0, x1, y1);
}

void RomBrowserTopScreenView::VBlank()
{
    ViewContainer::VBlank();

    // the chip pieces are semi-transparent OBJs so the pills reveal a hint of the
    // theme art behind them (the labels/heart/check stay opaque normal OBJs in
    // front). Reuse the splash's REG_BLDCNT_SUB value: BG1 (its 1st target) is off
    // in the browser, so its only live effect is picking the theme backgrounds and
    // backdrop as 2nd targets, and matching the value keeps the startup fade - which
    // also drives these registers until it hands off - from glitching. 12/16 pill +
    // 4/16 backdrop keeps the labels readable over busy art.
    REG_BLDCNT_SUB = 0x3D42;
    REG_BLDALPHA_SUB = (4 << 8) | 12;

    if (!_coverGraphicsUploaded && _selectedFileCover.IsValid())
    {
        if (_showCover && _selectedFileCover->IsActualCover())
        {
            _selectedFileCover->Upload2DCoverBitmap((u8*)GFX_BG_SUB + 0x4000);
            mem_setVramHMapping(MEM_VRAM_H_LCDC);
            _selectedFileCover->Upload2DCoverPalette((void*)0x0689E000);
            GFX_PLTT_BG_SUB[0] = *(vu16*)0x0689E000;
            mem_setVramHMapping(MEM_VRAM_H_SUB_BG_EXT_PLTT_SLOT_0123);
        }
        _coverGraphicsUploaded = true;
    }
    int x0 = std::clamp(_coverPosition.x, 0, 256);
    int x1 = std::clamp(_coverPosition.x + 106, 0, 256);
    int y0 = std::clamp(_coverPosition.y, 0, 192);
    int y1 = std::clamp(_coverPosition.y + 96, 0, 192);
    if (!_showCover || !_selectedFileCover.IsValid() || !_selectedFileCover->IsActualCover() ||
        x0 >= x1 || y0 >= y1)
    {
        // hide cover
        REG_DISPCNT_SUB &= ~(((1 << 3) | (1 << 5)) << 8);
    }
    else
    {
        // display cover
        REG_BG3PA_SUB = 0x100;
        REG_BG3PB_SUB = 0;
        REG_BG3PC_SUB = 0;
        REG_BG3PD_SUB = -0x100;
        REG_BG3X_SUB = (-_coverPosition.x) << 8;
        REG_BG3Y_SUB = (96 + _coverPosition.y - 1) << 8;
        REG_BG3CNT_SUB = 0x0705;
        REG_DISPCNT_SUB |= ((1 << 3) | (1 << 5)) << 8;
        gfx_setSubWindow0(x0, y0, x1, y1);
        REG_WININ_SUB = 0x002A;
        REG_WINOUT_SUB = ~(1 << 3);
    }
    if (!_iconGraphicsUploaded)
    {
        _fileInfoView->UploadIconGraphics();
        _iconGraphicsUploaded = true;
    }
}
