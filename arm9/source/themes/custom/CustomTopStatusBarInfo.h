#pragma once
#include <optional>
#include "core/math/Point.h"
#include "core/math/Rgb.h"

/// @brief What a custom theme says about the bar along the top of the top
///        screen ("topStatusBar"). Anything left unset keeps the launcher's
///        own value, which for the colors depends on the theme's color scheme,
///        so they are optional rather than defaulted here.
struct CustomTopStatusBarInfo
{
    using Rgb888 = Rgb<8, 8, 8>;

    /// Hidden unless the theme asks for the bar with "hidden": false, so that
    /// themes made before the bar existed look as they always did.
    bool hidden = true;

    bool backgroundHidden = false;
    std::optional<Rgb888> backgroundFromColor;
    std::optional<Rgb888> backgroundToColor;
    std::optional<bool> backgroundTranslucent;

    bool clockHidden = false;
    std::optional<Point> clockPosition;
    std::optional<Rgb888> clockTextColor;
    std::optional<Rgb888> clockBlendColor;

    bool batteryHidden = false;
    std::optional<Point> batteryPosition;
    std::optional<Rgb888> batteryColor;
    std::optional<Rgb888> batteryLowColor;
    std::optional<Rgb888> batteryChargingColor;
};
