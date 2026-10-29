#include "common.h"
#include "core/mini-printf.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "gui/OamBuilder.h"
#include "gui/palette/GradientPalette.h"
#include "gui/palette/DirectPalette.h"
#include "gui/input/InputProvider.h"
#include "gui/FocusManager.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "lnhChipLogo.h"
#include "rasalopaAvatar.h"
#include "Version.h"
#include "AboutBottomSheetView.h"

// Placed from the sheet's top edge, which rests at y 32 once the sheet is
// open. No title: the two columns say what the sheet is.
#define COLUMN_WIDTH        128
#define LEFT_X              0
#define RIGHT_X             128

// The chip logo is the boot splash's own, its 40 px of art brought to 32.
#define LOGO_ART            32
#define LOGO_X              (LEFT_X + (COLUMN_WIDTH - LOGO_ART) / 2)
#define LOGO_Y              10

// The maker's avatar, head and collar from a pixel portrait at its own grid,
// 20 by 24 centred on a 32 by 64 sprite, its own colours, its bottom on the
// same line as the chip's lowest pins.
#define AVATAR_WIDTH        32
#define AVATAR_SPRITE       64
#define AVATAR_ART          24
#define AVATAR_X            (RIGHT_X + (COLUMN_WIDTH - AVATAR_WIDTH) / 2)
#define AVATAR_Y            (LOGO_Y + LOGO_ART - AVATAR_ART)

#define NAME_Y              46
#define BY_Y                58

// Where the build came from, small and faint, with the list's position at the
// end of the same line so the rows below can start lower.
#define BUILD_X             16
#define BUILD_WIDTH         180
#define BUILD_Y             74
#define RANGE_X             200
#define RANGE_WIDTH         40

// The controls: the button on the left in the heavier face, what it does
// beside it, three rows at a time; the last one ends at 148, screen y 180.
#define COMMANDS_Y          94
#define COMMAND_STEP        18
#define BUTTON_X            20
#define BUTTON_WIDTH        72
#define DESCRIPTION_X       96
#define DESCRIPTION_WIDTH   144

// The controls that have no button on screen, in the order a new player meets
// them. Kept in step with docs/Enhanced.md's table.
static const struct { const char* button; const char* action; } kCommands[] =
{
    { "X",        "favorite. hold: completed" },
    { "Y",        "cheats for this game" },
    { "L R",      "previous or next initial" },
    { "SELECT+A", "random game in this folder" },
    { "START",    "hold: save a screenshot" },
    { "B",        "folder up, or close a panel" },
    { "X",        "in cheats: every cheat off" },
};
static const int kCommandCount = sizeof(kCommands) / sizeof(kCommands[0]);

AboutBottomSheetView::AboutBottomSheetView(SharedPtr<AboutViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
    : _viewModel(std::move(viewModel))
    , _materialColorScheme(materialColorScheme)
{
    char text[96];
    _upstreamName = AddLabel(fontRepository, FontType::Medium10, COLUMN_WIDTH, 16, "Pico Launcher", Alignment::Center);
    _upstreamBy = AddLabel(fontRepository, FontType::Regular10, COLUMN_WIDTH, 24, "by the LNH team", Alignment::Center);
    char version[16];
    FormatLauncherVersion(version, sizeof(version), false);
    mini_snprintf(text, sizeof(text), "Enhanced %s", version);
    _forkName = AddLabel(fontRepository, FontType::Medium10, COLUMN_WIDTH, 24, text, Alignment::Center);
    _forkBy = AddLabel(fontRepository, FontType::Regular10, COLUMN_WIDTH, 24, "by rasalopa", Alignment::Center);

    // "rasalopa/pico-launcher-enhanced @ 2173910": where the build came from
    // and which commit, as far as the build knows. The version is in the
    // fork's name above.
    if (kLauncherRepo[0] != 0 && kLauncherBuild[0] != 0)
        mini_snprintf(text, sizeof(text), "%s @ %s", kLauncherRepo, kLauncherBuild);
    else if (kLauncherRepo[0] != 0)
        mini_snprintf(text, sizeof(text), "%s", kLauncherRepo);
    else if (kLauncherBuild[0] != 0)
        mini_snprintf(text, sizeof(text), "build %s", kLauncherBuild);
    else
        text[0] = 0;
    _buildLabel = AddLabel(fontRepository, FontType::Medium7_5, BUILD_WIDTH, 80, text, Alignment::Start);

    for (int i = 0; i < VISIBLE_COMMANDS; i++)
    {
        _commandButtons[i] = AddLabel(fontRepository, FontType::Medium10, BUTTON_WIDTH, 10, "", Alignment::Start);
        _commandLabels[i] = AddLabel(fontRepository, FontType::Regular10, DESCRIPTION_WIDTH, 40, "", Alignment::Start);
    }
    _rangeLabel = AddLabel(fontRepository, FontType::Medium7_5, RANGE_WIDTH, 16, "", Alignment::End);
    ShowCommandsFrom(0);
}

SharedPtr<Label2DView> AboutBottomSheetView::AddLabel(const IFontRepository* fontRepository,
    FontType fontType, u32 width, u32 maxChars, const char* text, Alignment alignment)
{
    auto label = Label2DView::CreateShared(width, 16, maxChars, fontRepository->GetFont(fontType));
    label->SetHorizontalAlignment(alignment);
    label->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    label->SetText(text);
    AddChildTail(label.GetPointer());
    return label;
}

void AboutBottomSheetView::ShowCommandsFrom(int scroll)
{
    int maxScroll = kCommandCount - VISIBLE_COMMANDS;
    if (scroll > maxScroll)
        scroll = maxScroll;
    if (scroll < 0)
        scroll = 0;
    if (scroll == _scroll)
        return;
    _scroll = scroll;
    for (int i = 0; i < VISIBLE_COMMANDS; i++)
    {
        int idx = scroll + i;
        _commandButtons[i]->SetText(idx < kCommandCount ? kCommands[idx].button : "");
        _commandLabels[i]->SetText(idx < kCommandCount ? kCommands[idx].action : "");
    }
    char text[16];
    mini_snprintf(text, sizeof(text), "%d-%d of %d", scroll + 1, scroll + VISIBLE_COMMANDS, kCommandCount);
    _rangeLabel->SetText(text);
}

void AboutBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (!objVramManager)
        return;
    _logoVramOffset = LoadSprite(*objVramManager, lnhChipLogoTiles, lnhChipLogoTilesLen);
    _avatarVramOffset = LoadSprite(*objVramManager, rasalopaAvatarTiles, rasalopaAvatarTilesLen);
}

u32 AboutBottomSheetView::LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}

void AboutBottomSheetView::Update()
{
    int y = _position.y;
    _upstreamName->SetPosition(LEFT_X, y + NAME_Y);
    _upstreamBy->SetPosition(LEFT_X, y + BY_Y);
    _forkName->SetPosition(RIGHT_X, y + NAME_Y);
    _forkBy->SetPosition(RIGHT_X, y + BY_Y);
    _buildLabel->SetPosition(BUILD_X, y + BUILD_Y);
    _rangeLabel->SetPosition(RANGE_X, y + BUILD_Y);
    for (int i = 0; i < VISIBLE_COMMANDS; i++)
    {
        int rowY = y + COMMANDS_Y + i * COMMAND_STEP;
        _commandButtons[i]->SetPosition(BUTTON_X, rowY);
        _commandLabels[i]->SetPosition(DESCRIPTION_X, rowY);
    }
    BottomSheetView::Update();
}

void AboutBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        const auto back = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);
        const auto& bright = _materialColorScheme->onSurface;
        const auto& body = _materialColorScheme->onSurfaceVariant;
        const auto& faint = _materialColorScheme->outline;

        auto paint = [&](const SharedPtr<Label2DView>& label, const Rgb<8, 8, 8>& color)
        {
            label->SetBackgroundColor(back);
            label->SetForegroundColor(color);
        };
        paint(_upstreamName, bright);
        paint(_upstreamBy, body);
        paint(_forkName, bright);
        paint(_forkBy, body);
        paint(_buildLabel, faint);
        for (int i = 0; i < VISIBLE_COMMANDS; i++)
        {
            paint(_commandButtons[i], bright);
            paint(_commandLabels[i], body);
        }
        paint(_rangeLabel, faint);
        BottomSheetView::Draw(graphicsContext);

        // The sheet slides in from the bottom, and an oam y past the screen wraps.
        int logoY = _position.y + LOGO_Y;
        if (graphicsContext.IsVisible(Rectangle(LOGO_X, logoY, LOGO_ART, LOGO_ART)))
        {
            // The logo is indexed by how dark its greys are, so it is drawn in
            // the text colour and blends with the sheet on any theme.
            u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(back, body), logoY, logoY + LOGO_ART);
            auto oams = graphicsContext.GetOamManager().AllocOams(1);
            OamBuilder::OamWithSize<LOGO_ART, LOGO_ART>(LOGO_X, logoY, _logoVramOffset >> 7)
                .WithPalette16(paletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(oams[0]);
        }

        // The avatar keeps its own colours, so its palette is the sprite's.
        int avatarY = _position.y + AVATAR_Y;
        if (graphicsContext.IsVisible(Rectangle(AVATAR_X, avatarY, AVATAR_WIDTH, AVATAR_ART)))
        {
            u32 paletteRow = graphicsContext.GetPaletteManager().AllocRow(
                DirectPalette(rasalopaAvatarPal), avatarY, avatarY + AVATAR_ART);
            auto oams = graphicsContext.GetOamManager().AllocOams(1);
            OamBuilder::OamWithSize<AVATAR_WIDTH, AVATAR_SPRITE>(AVATAR_X, avatarY, _avatarVramOffset >> 7)
                .WithPalette16(paletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(oams[0]);
        }
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

bool AboutBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

SharedPtr<View> AboutBottomSheetView::MoveFocus(const SharedPtr<View>& currentFocus,
    FocusMoveDirection direction, View* source)
{
    // Nothing here takes focus, so the d-pad arrives as focus moves; up and
    // down scroll the controls instead, and the focus stays where it is.
    if (direction == FocusMoveDirection::Up)
        ShowCommandsFrom(_scroll - 1);
    else if (direction == FocusMoveDirection::Down)
        ShowCommandsFrom(_scroll + 1);
    return nullptr;
}

void AboutBottomSheetView::Focus(FocusManager& focusManager)
{
    // a child of the sheet, so keys reach it and bubble up to HandleInput
    focusManager.Focus(_upstreamName->SharedFromThis());
}

void AboutBottomSheetView::Close()
{
    _viewModel->Close();
}
