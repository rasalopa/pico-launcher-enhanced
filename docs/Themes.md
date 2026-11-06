# Themes
Using themes, the look and feel of Pico Launcher can be customized. Themes are placed in subfolders of the `/_pico/themes` folder. For example `/_pico/themes/my_theme`.

## JSON file
Each theme has a `theme.json` file with information about the theme.

- **type** - Type of theme. Currently `material` and `custom` are supported. See below for information about each type.
- **name** - The name of the theme.
- **description** - Description of the theme.
- **author** - Author of the theme.
- **primaryColor** - Material Design 3 primary color to use. `r`, `g` and `b` are provided in range 0-255.
- **darkTheme** - When `true`, a dark Material Design 3 palette will be used.

### Example
```json
{
    "type": "material",
    "name": "Theme name",
    "description": "Theme description here.",
    "author": "Author Name",
    "primaryColor": {
        "r": 149,
        "g": 143,
        "b": 237
    },
    "darkTheme": true
}
```

## Material type
![Horizontal display mode with custom theme](images/Horizontal.png)

The `material` type theme is a pure Material Design 3 theme. It will be fully themed based on the `primaryColor` and `darkTheme` settings from the `theme.json`. In coverflow mode, this theme type uses a Material Design 3 style carousel.

## Custom type
![Horizontal display mode with custom theme](images/HorizontalCustom.png)

The `custom` type theme is much more customizable, compared to the `material` type theme.
Note that the `primaryColor` and `darkTheme` settings from the `theme.json` are still used to color some parts of the UI.

The following additional files are needed:
| Files                                                        | Size                 | Format                   | Description                                                    |
|--------------------------------------------------------------|----------------------|--------------------------|----------------------------------------------------------------|
| bannerListCell.bin<br>bannerListCellPltt.bin                 | 256x49 (209x49 used) | A3I5<br>32 color palette | Unselected item background for banner list mode.               |
| bannerListCellSelected.bin<br>bannerListCellSelectedPltt.bin | 256x49 (209x49 used) | A3I5<br>32 color palette | Selected item background for banner list mode.                 |
| bottombg.bin                                                 | 256x192              | 15 bpp bitmap            | Bottom screen background.                                      |
| gridcell.bin<br>gridcellPltt.bin                             | 64x48 (48x48 used)   | A3I5<br>32 color palette | Unselected item background for grid modes.                     |
| gridcellSelected.bin<br>gridcellPlttSelected.bin             | 64x48 (48x48 used)   | A3I5<br>32 color palette | Selected item background for grid modes.                       |
| scrim.bin<br>scrimPltt.bin                                   | 8x42                 | A5I3<br>8 color palette  | Background for the toolbar. Intended to be a translucent fade. |
| topbg.bin                                                    | 256x192              | 15 bpp bitmap            | Top screen background.                                         |
| statusBar.bin<br>(optional)                                  | 32x16                | 4 bpp tiles, 256 bytes   | Piece of the top bar background, repeated 8 times across.      |
| statusBarBattery.bin<br>(optional)                           | 16x96                | 4 bpp tiles, 768 bytes   | Battery frames of 16x16: empty, 1 to 4 bars, charging.         |

These files can be created, for example, using [NitroPaint](https://github.com/Garhoogin/NitroPaint).

The top screen background should include a box in which the banner text and icon of the selected item will be shown.

### Additional JSON properties
Custom themes support additional properties in the `theme.json` file to allow for more customization.

- **topIcon** - Properties of the icon displayed on the top screen.
- **topBannerTextLine0** - Properties of the first banner text line displayed on the top screen.
- **topBannerTextLine1** - Properties of the second banner text line displayed on the top screen.
- **topBannerTextLine2** - Properties of the third banner text line displayed on the top screen.
- **topFileNameText** - Properties of the file name text displayed on the top screen.
- **topCover** - Properties of the cover image displayed on the top screen.
- **topGameCount** - No longer used: the game count moved to the statistics panel. A theme that still sets it keeps loading; the key is ignored.
- **topLaunchInfo** - Where the game's markers go: a star on the most launched game, a check on a completed one and a heart on a favorite. Left out, they sit above `topIcon` with nothing under them: the row is centred on the icon (`topIcon.x + 16`) and its top edge is 18 px above it (`topIcon.y - 18`), which in the themes the launcher ships with puts the 16 px markers astride the card's top edge; one marker or three, the row stays centred. Set it to put them on a pill instead: `position` is the top-right corner of that pill, which grows to the left. Set `hidden` to `true` to remove them entirely. The launch count and play time that used to sit on the pill are switched off in this version.
- **topStatusBar** - The bar with the clock and the battery meter along the top of the top screen. It is 16 px tall and full width. Themes made before the bar existed do not get it: it is **hidden unless the theme sets `"hidden": false`**. Everything else is optional and falls back to the launcher's own look:
  - `background` - `hidden`, `fromColor` and `toColor` (the bar art is a gradient between them), `translucent`. The art comes from `statusBar.bin` when the theme has one.
  - `clock` - `position`, `hidden`, `textColor`, `blendColor`.
  - `battery` - `position`, `hidden`, `color`, `lowColor` (shown at the lowest level), `chargingColor`. The art comes from `statusBarBattery.bin` when the theme has one.
  
  A `.bin` file of the wrong size is ignored.
- **gridIcon** - Properties of the icons displayed on the bottom screen in grid display modes.
- **bannerListIcon** - Properties of the icons displayed on the bottom screen in banner list display mode.
- **bannerListTextLine0** - Properties of the first banner text line displayed on the bottom screen in banner list display mode.
- **bannerListTextLine1** - Properties of the second banner text line displayed on the bottom screen in banner list display mode.
- **bannerListTextLine2** - Properties of the third banner text line displayed on the bottom screen in banner list display mode.

Blend colors are used to fake translucency. They should be set to an approximation of the background color.

```json
{
    "type": "custom",
    "name": "Raspberry",
    "description": "Theme based on raspberries.",
    "author": "Gericom",
    "primaryColor": { "r": 138, "g": 217, "b": 255 },
    "darkTheme": false,
    "topIcon": {
        "position": { "x": 24, "y": 132 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine0": {
        "position": { "x": 70, "y": 126 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine1": {
        "position": { "x": 70, "y": 141 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine2": {
        "position": { "x": 70, "y": 155 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topFileNameText": {
        "position": { "x": 18, "y": 170 },
        "width": 220,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topCover": {
        "position": { "x": 75, "y": 18 }
    },
    "topLaunchInfo": {
        "position": { "x": 252, "y": 2 },
        "hidden": false
    },
    "topStatusBar": {
        "hidden": false,
        "background": { "fromColor": { "r": 120, "g": 120, "b": 120 }, "toColor": { "r": 240, "g": 240, "b": 240 }, "translucent": true },
        "clock": { "position": { "x": 6, "y": 1 }, "textColor": { "r": 30, "g": 30, "b": 30 } },
        "battery": { "position": { "x": 236, "y": 0 }, "color": { "r": 30, "g": 30, "b": 30 } }
    },
    "gridIcon": {
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "bannerListIcon": {
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "bannerListTextLine0": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    },
    "bannerListTextLine1": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    },
    "bannerListTextLine2": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    }
}
```

## Background music
All themes support background music by placing DSP-ADPCM encoded `.bcstm` files in a `bgm` folder inside the theme folder. Looping is supported. When multiple `.bcstm` files are provided, the background music will be selected at random each time Pico Launcher is started.

## Theme selector icon
A theme can have an `icon.bmp` file that is shown in the theme list when selecting a theme. It must be **32×32 pixels, 4 bpp (16 colors), uncompressed `.bmp`** file, with the first palette color treated as transparent.

## Theme selector preview image
A theme can have a `preview.bin` file that is shown on the top screen in the theme selection screen. It must be a 256x192 pixels 15 bpp bitmap (same format as `topbg.bin` and `bottombg.bin`). When it is not provided, `topbg.bin` is displayed instead. If that does not exist either, nothing is shown on the top screen.

## Theme selector look
The theme selector is always drawn as a `material` theme with your theme's `primaryColor` and `darkTheme`, whatever the theme's type, so its list stays readable. Your theme shows through its icon in the list and its preview on the top screen.

The selector can also delete a theme's folder (see [Deleting themes](Enhanced.md#deleting-themes)). The theme in use and the two that come with the launcher, `material` and `raspberry`, can't be deleted there.
