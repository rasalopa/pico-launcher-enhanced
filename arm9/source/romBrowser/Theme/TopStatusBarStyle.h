#pragma once
#include "core/math/Point.h"
#include "core/math/Rgb.h"

struct MaterialColorScheme;

// Everything the bar along the top of the top screen takes from the theme: the
// art, where the clock and the battery go, and their colors. Material uses
// MakeDefaultTopStatusBarStyle as is; a custom theme overrides parts of it.
struct TopStatusBarStyle
{
    using Rgb888 = Rgb<8, 8, 8>;

    // the whole bar: background, clock and battery
    bool hidden;

    // 32x16 4 bpp tiles (256 bytes) repeated across the screen, drawn as a
    // gradient from backgroundFromColor to backgroundToColor
    bool backgroundHidden;
    const void* backgroundTiles;
    Rgb888 backgroundFromColor;
    Rgb888 backgroundToColor;
    bool backgroundTranslucent;

    bool clockHidden;
    Point clockPosition;
    Rgb888 clockTextColor;
    Rgb888 clockBlendColor;

    // 16x96 4 bpp sheet (768 bytes) of 16x16 frames: empty, 1 to 4 bars, charging
    bool batteryHidden;
    const void* batteryTiles;
    Point batteryPosition;
    Rgb888 batteryColor;
    Rgb888 batteryLowColor;
    Rgb888 batteryChargingColor;
};

#define TOP_STATUS_BAR_TILES_SIZE           256
#define TOP_STATUS_BAR_BATTERY_TILES_SIZE   768

/// @brief The look the bar has when the theme says nothing about it, with the
///        art compiled into the launcher.
TopStatusBarStyle MakeDefaultTopStatusBarStyle(const MaterialColorScheme& colorScheme);
