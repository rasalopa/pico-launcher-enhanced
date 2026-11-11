#pragma once
#include "core/SharedPtr.h"
#include "core/math/Rgb.h"
#include "gui/Alignment.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/StatisticsViewModel.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

/// @brief Bottom sheet with library statistics.
///
/// A row of four figures, each with an icon: games in this folder, played,
/// favorites, completed. Under it the three most launched games with their
/// counts and play time against the right edge, the library's total play time
/// on the heading's line, and the last game played next to the clock the panel
/// is opened with. The launcher's version sits faint at the top-right; the build
/// hash is on the boot page and in the about sheet.
class StatisticsBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(StatisticsBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void Focus(FocusManager& focusManager) override;

protected:
    void Close() override;

private:
    static constexpr u32 TILE_COUNT = 4;
    static constexpr u32 TOP_COUNT = STATISTICS_TOP_COUNT;

    SharedPtr<StatisticsViewModel> _viewModel;
    const MaterialColorScheme* _materialColorScheme;

    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _versionLabel;
    SharedPtr<Label2DView> _tileNumbers[TILE_COUNT];
    SharedPtr<Label2DView> _tileCaptions[TILE_COUNT];
    u32 _tileIconVramOffsets[TILE_COUNT] = {};
    /// "Most played" over the list, or the reason there is no list.
    SharedPtr<Label2DView> _headingLabel;
    /// The library's total play time, absent when nothing was measured.
    SharedPtr<Label2DView> _totalLabel;
    SharedPtr<Label2DView> _rankLabels[TOP_COUNT];
    SharedPtr<Label2DView> _nameLabels[TOP_COUNT];
    SharedPtr<Label2DView> _countLabels[TOP_COUNT];
    u32 _topCount = 0;
    SharedPtr<Label2DView> _lastLabel;
    SharedPtr<Label2DView> _dateLabel;
    u32 _clockIconVramOffset = 0;
    bool _hasLast = false;

    StatisticsBottomSheetView(SharedPtr<StatisticsViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);

    SharedPtr<Label2DView> AddLabel(const IFontRepository* fontRepository, FontType fontType,
        u32 width, u32 maxChars, const char* text, Alignment alignment = Alignment::Start);
    static void CopyNameWithoutExtension(char* dst, u32 dstSize, const char* fileName);
    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
    void DrawIcon(GraphicsContext& graphicsContext, int x, int y, u32 vramOffset,
        const Rgb<8, 8, 8>& backColor, const Rgb<8, 8, 8>& tint) const;
};
