# Tools
The `tools/` folder contains helper scripts that run on a PC. They require Python 3, and all except `make_night_bg.py` also need [Pillow](https://pypi.org/project/pillow/) (`pip install pillow`). Run them from the repository root.

## png2icon.py
Prepares the cartridge banner icon used at build time (`GAME_ICON`, embedded by `ndstool -b`): fits any image into a 32x32 PNG with at most 15 opaque colors plus alpha transparency, the limits ndstool accepts.

```
python3 tools/png2icon.py input.png [output.png]
```

## png2iconbmp.py
Converts an image to a custom launcher icon (see [Customization](Customization.md)): 32x32, 4 bpp, uncompressed BMP with the first palette color transparent. Place the result under `/_pico/icons/` or as a folder's `icon.bmp`.

```
python3 tools/png2iconbmp.py input.png output.bmp
```

## img2cover.py
Converts an image to a cover (see [Covers](Covers.md)): 128x96, 8 bpp, uncompressed BMP, with the art fitted to the visible 106x96 area.

```
python3 tools/img2cover.py input.png output.bmp
```

## fetch_covers_gba.py
Downloads missing GBA covers from libretro-thumbnails: reads the game code from each `.gba` in `<SD>/Games/gba`, finds the matching boxart, converts it and installs it as `<SD>/_pico/covers/gba/<CODE>.bmp`. The SD path defaults to `/Volumes/DSPICO`; pass it as the first argument or with `--sd`, which is the form to use on Windows and Linux. `--dry-run` shows what would be downloaded without writing anything.

```
python3 tools/fetch_covers_gba.py [/Volumes/DSPICO] [--dry-run]
python3 tools/fetch_covers_gba.py --sd E:\ [--dry-run]
```

## fetch_covers.py
Same idea for systems without game codes: downloads missing covers matched by file name and installs them into `<SD>/_pico/covers/user/`. Each system is scanned in its own `<SD>/Games/<system>` folder. Supported systems: `gb`, `gbc`, `gen`, `sms`, `gg`, `nes`, `snes`, `ws`, `wsc`, `ngp`, `ngc`.

```
python3 tools/fetch_covers.py gb gbc gen [--sd /Volumes/DSPICO] [--dry-run]
```

Matching for both fetchers is exact name first, then prefix, then fuzzy — check the output for wrong guesses on numbered series.

## make_banner.py
Builds a folder `banner.bnr` (see [Customization](Customization.md)): a custom title plus a 32x32 icon, taken losslessly from another ROM's banner or quantized from an image.

```
python3 tools/make_banner.py --from-nds emulator.nds "Title" banner.bnr
python3 tools/make_banner.py --from-image logo.png "Title" banner.bnr
```

## make_night_bg.py
Generates the night variants (`topbg_night.bin`, `bottombg_night.bin`) of a custom theme's backgrounds by darkening and cooling its existing `topbg.bin`/`bottombg.bin` (see the time-of-day section in [Enhanced.md](Enhanced.md)).

```
python3 tools/make_night_bg.py "/Volumes/DSPICO/_pico/themes/My Theme"
```
