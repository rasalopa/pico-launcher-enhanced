#include "common.h"
#include <memory>
#include <optional>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include "json/ArduinoJson.h"
#include "core/mini-printf.h"
#include "CustomTopBackgroundType.h"
#include "gui/VramContext.h"
#include "../material/MaterialColorSchemeFactory.h"
#include "romBrowser/views/IconButton3DView.h"
#include "CustomTheme.h"

#define JSON_RESERVED_SIZE  4096

#define KEY_COLOR_R     "r"
#define KEY_COLOR_G     "g"
#define KEY_COLOR_B     "b"

#define KEY_POINT_X     "x"
#define KEY_POINT_Y     "y"

#define KEY_TOP_ICON                    "topIcon"
#define KEY_TOP_BANNER_TEXT_LINE_0      "topBannerTextLine0"
#define KEY_TOP_BANNER_TEXT_LINE_1      "topBannerTextLine1"
#define KEY_TOP_BANNER_TEXT_LINE_2      "topBannerTextLine2"
#define KEY_TOP_FILE_NAME_TEXT          "topFileNameText"
#define KEY_TOP_COVER                   "topCover"
#define KEY_TOP_LAUNCH_INFO             "topLaunchInfo"
#define KEY_TOP_STATUS_BAR              "topStatusBar"
#define KEY_GRID_ICON                   "gridIcon"
#define KEY_BANNER_LIST_ICON            "bannerListIcon"
#define KEY_BANNER_LIST_TEXT_LINE_0     "bannerListTextLine0"
#define KEY_BANNER_LIST_TEXT_LINE_1     "bannerListTextLine1"
#define KEY_BANNER_LIST_TEXT_LINE_2     "bannerListTextLine2"

#define KEY_ELEMENT_POSITION        "position"
#define KEY_ELEMENT_WIDTH           "width"
#define KEY_ELEMENT_TEXT_COLOR      "textColor"
#define KEY_ELEMENT_BLEND_COLOR     "blendColor"
#define KEY_ELEMENT_HIDDEN          "hidden"

#define KEY_STATUS_BAR_BACKGROUND   "background"
#define KEY_STATUS_BAR_CLOCK        "clock"
#define KEY_STATUS_BAR_BATTERY      "battery"
#define KEY_FROM_COLOR              "fromColor"
#define KEY_TO_COLOR                "toColor"
#define KEY_TRANSLUCENT             "translucent"
#define KEY_COLOR                   "color"
#define KEY_LOW_COLOR               "lowColor"
#define KEY_CHARGING_COLOR          "chargingColor"

static const CustomThemeInfo sDefaultCustomThemeInfo
{
    .topIconInfo = CustomTopIconInfo(Point(24, 132), Rgb8(200, 200, 200)),
    .topBannerTextLine0Info = CustomTopTextElementInfo(Point(70, 126), 176, Rgb8(30, 30, 30), Rgb8(200, 200, 200)),
    .topBannerTextLine1Info = CustomTopTextElementInfo(Point(70, 141), 176, Rgb8(30, 30, 30), Rgb8(200, 200, 200)),
    .topBannerTextLine2Info = CustomTopTextElementInfo(Point(70, 155), 176, Rgb8(30, 30, 30), Rgb8(200, 200, 200)),
    .topFileNameTextInfo = CustomTopTextElementInfo(Point(18, 170), 220, Rgb8(30, 30, 30), Rgb8(200, 200, 200)),
    .topCoverInfo = CustomTopCoverInfo(Point(75, 18)),
    // Only reached when the theme sets "topLaunchInfo" itself: then it is the
    // top-right corner of the markers' pill. Unset, the view factory puts the
    // markers at the theme's icon instead. A "topGameCount" key in an older
    // theme is simply not looked at any more.
    .topLaunchInfoInfo = CustomTopStripElementInfo(Point(252, 18), false, false),
    .topStatusBarInfo = CustomTopStatusBarInfo(),

    .gridIconInfo = CustomBottomIconInfo(Rgb8(200, 200, 200)),

    .bannerListIconInfo = CustomBottomIconInfo(Rgb8(200, 200, 200)),
    .bannerListTextLine0Info = CustomBannerListTextElementInfo(Rgb8(30, 30, 30)),
    .bannerListTextLine1Info = CustomBannerListTextElementInfo(Rgb8(30, 30, 30)),
    .bannerListTextLine2Info = CustomBannerListTextElementInfo(Rgb8(30, 30, 30))
};

static CustomTopBackgroundType parseTopBackgroundType(const char* topBackgroundTypeString)
{
    return CustomTopBackgroundType::Bitmap;
}

static Rgb8 parseColor(const JsonObjectConst& json, const Rgb8& defaultColor)
{
    if (json.isNull())
    {
        return defaultColor;
    }

    return Rgb8(
        json[KEY_COLOR_R] | 0,
        json[KEY_COLOR_G] | 0,
        json[KEY_COLOR_B] | 0
    );
}

static Point parsePoint(const JsonObjectConst& json, const Point& defaultPoint)
{
    if (json.isNull())
    {
        return defaultPoint;
    }

    return Point(
        json[KEY_POINT_X] | 0,
        json[KEY_POINT_Y] | 0
    );
}

static CustomBannerListTextElementInfo parseCustomBannerListTextElementInfo(
    const JsonObjectConst& json, const CustomBannerListTextElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomBannerListTextElementInfo(
        parseColor(json[KEY_ELEMENT_TEXT_COLOR], defaultInfo.GetTextColor())
    );
}

static CustomBottomIconInfo parseCustomBottomIconInfo(const JsonObjectConst& json, const CustomBottomIconInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomBottomIconInfo(
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor())
    );
}

static CustomTopIconInfo parseCustomTopIconInfo(const JsonObjectConst& json, const CustomTopIconInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopIconInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor())
    );
}

static CustomTopCoverInfo parseCustomTopCoverInfo(const JsonObjectConst& json, const CustomTopCoverInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopCoverInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition())
    );
}

static CustomTopStripElementInfo parseCustomTopStripElementInfo(
    const JsonObjectConst& json, const CustomTopStripElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopStripElementInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        json[KEY_ELEMENT_HIDDEN] | defaultInfo.GetIsHidden(),
        true
    );
}

static std::optional<Rgb8> parseOptionalColor(const JsonObjectConst& json)
{
    if (json.isNull())
        return std::nullopt;
    return parseColor(json, Rgb8(0, 0, 0));
}

static std::optional<Point> parseOptionalPoint(const JsonObjectConst& json)
{
    if (json.isNull())
        return std::nullopt;
    return parsePoint(json, Point(0, 0));
}

static CustomTopStatusBarInfo parseCustomTopStatusBarInfo(const JsonObjectConst& json)
{
    CustomTopStatusBarInfo info;
    if (json.isNull())
        return info;

    info.hidden = json[KEY_ELEMENT_HIDDEN] | true;

    const JsonObjectConst background = json[KEY_STATUS_BAR_BACKGROUND];
    info.backgroundHidden = background[KEY_ELEMENT_HIDDEN] | false;
    info.backgroundFromColor = parseOptionalColor(background[KEY_FROM_COLOR]);
    info.backgroundToColor = parseOptionalColor(background[KEY_TO_COLOR]);
    if (background.containsKey(KEY_TRANSLUCENT))
        info.backgroundTranslucent = background[KEY_TRANSLUCENT] | true;

    const JsonObjectConst clock = json[KEY_STATUS_BAR_CLOCK];
    info.clockHidden = clock[KEY_ELEMENT_HIDDEN] | false;
    info.clockPosition = parseOptionalPoint(clock[KEY_ELEMENT_POSITION]);
    info.clockTextColor = parseOptionalColor(clock[KEY_ELEMENT_TEXT_COLOR]);
    info.clockBlendColor = parseOptionalColor(clock[KEY_ELEMENT_BLEND_COLOR]);

    const JsonObjectConst battery = json[KEY_STATUS_BAR_BATTERY];
    info.batteryHidden = battery[KEY_ELEMENT_HIDDEN] | false;
    info.batteryPosition = parseOptionalPoint(battery[KEY_ELEMENT_POSITION]);
    info.batteryColor = parseOptionalColor(battery[KEY_COLOR]);
    info.batteryLowColor = parseOptionalColor(battery[KEY_LOW_COLOR]);
    info.batteryChargingColor = parseOptionalColor(battery[KEY_CHARGING_COLOR]);
    return info;
}

static CustomTopTextElementInfo parseCustomTextElementInfo(
    const JsonObjectConst& json, const CustomTopTextElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopTextElementInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        json[KEY_ELEMENT_WIDTH] | defaultInfo.GetWidth(),
        parseColor(json[KEY_ELEMENT_TEXT_COLOR], defaultInfo.GetTextColor()),
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor())
    );
}

static CustomThemeInfo parseCustomThemeInfo(const JsonDocument& json)
{
    return CustomThemeInfo
    {
        .topIconInfo = parseCustomTopIconInfo(json[KEY_TOP_ICON], sDefaultCustomThemeInfo.topIconInfo),
        .topBannerTextLine0Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_0], sDefaultCustomThemeInfo.topBannerTextLine0Info),
        .topBannerTextLine1Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_1], sDefaultCustomThemeInfo.topBannerTextLine1Info),
        .topBannerTextLine2Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_2], sDefaultCustomThemeInfo.topBannerTextLine2Info),
        .topFileNameTextInfo = parseCustomTextElementInfo(
            json[KEY_TOP_FILE_NAME_TEXT], sDefaultCustomThemeInfo.topFileNameTextInfo),
        .topCoverInfo = parseCustomTopCoverInfo(json[KEY_TOP_COVER], sDefaultCustomThemeInfo.topCoverInfo),
        .topLaunchInfoInfo = parseCustomTopStripElementInfo(
            json[KEY_TOP_LAUNCH_INFO], sDefaultCustomThemeInfo.topLaunchInfoInfo),
        .topStatusBarInfo = parseCustomTopStatusBarInfo(json[KEY_TOP_STATUS_BAR]),

        .gridIconInfo = parseCustomBottomIconInfo(json[KEY_GRID_ICON], sDefaultCustomThemeInfo.gridIconInfo),

        .bannerListIconInfo = parseCustomBottomIconInfo(json[KEY_BANNER_LIST_ICON], sDefaultCustomThemeInfo.bannerListIconInfo),
        .bannerListTextLine0Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_0], sDefaultCustomThemeInfo.bannerListTextLine0Info),
        .bannerListTextLine1Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_1], sDefaultCustomThemeInfo.bannerListTextLine1Info),
        .bannerListTextLine2Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_2], sDefaultCustomThemeInfo.bannerListTextLine2Info)
    };
}

CustomTheme::CustomTheme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode)
    : Theme(folderName, primaryColor, darkMode)
    , _customThemeInfo(sDefaultCustomThemeInfo)
    , _romBrowserViewFactory(&_customThemeInfo, &_materialColorScheme, &_fontRepository)
    , _themeFileIconFactory(&_materialColorScheme, &_fontRepository) { }

void CustomTheme::LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext)
{
    const auto file = std::make_unique<File>();
    OpenThemeFile(*file, "theme.json");

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
        return;

    _topBackgroundType = parseTopBackgroundType(json["topBackgroundType"].as<const char*>());
    _customThemeInfo = parseCustomThemeInfo(json);

    mem_setVramDMapping(MEM_VRAM_D_LCDC);
    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    IconButton3DView::UploadGraphics(mainVramContext);
    mem_setVramDMapping(MEM_VRAM_D_TEX_SLOT_0);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);
    _romBrowserViewFactory.LoadResources(*this, mainVramContext);
}
