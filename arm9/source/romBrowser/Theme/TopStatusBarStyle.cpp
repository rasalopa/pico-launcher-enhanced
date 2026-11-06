#include "common.h"
#include "themes/material/MaterialColorScheme.h"
#include "../views/MarkerColors.h"
#include "statusBarBg.h"
#include "batteryIcon.h"
#include "TopStatusBarStyle.h"

static_assert(statusBarBgTilesLen == TOP_STATUS_BAR_TILES_SIZE);
static_assert(batteryIconTilesLen == TOP_STATUS_BAR_BATTERY_TILES_SIZE);

TopStatusBarStyle MakeDefaultTopStatusBarStyle(const MaterialColorScheme& colorScheme)
{
    return TopStatusBarStyle
    {
        .hidden = false,

        .backgroundHidden = false,
        .backgroundTiles = statusBarBgTiles,
        .backgroundFromColor = colorScheme.outline,
        .backgroundToColor = colorScheme.surfaceBright,
        .backgroundTranslucent = true,

        .clockHidden = false,
        .clockPosition = Point(6, 1),
        .clockTextColor = colorScheme.onSurface,
        .clockBlendColor = colorScheme.surfaceBright,

        .batteryHidden = false,
        .batteryTiles = batteryIconTiles,
        .batteryPosition = Point(236, 0),
        .batteryColor = colorScheme.onSurface,
        .batteryLowColor = kFavoriteRed,
        .batteryChargingColor = kCompletedGreen
    };
}
