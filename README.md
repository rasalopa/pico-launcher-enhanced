# Pico Launcher Enhanced

[![Latest release](https://img.shields.io/github/v/release/rasalopa/pico-launcher-enhanced?display_name=tag&sort=semver&label=release)](../../releases/latest)
[![Downloads](https://img.shields.io/github/downloads/rasalopa/pico-launcher-enhanced/total?label=downloads)](../../releases)
[![License](https://img.shields.io/github/license/rasalopa/pico-launcher-enhanced)](LICENSE.txt)

A feature fork of [Pico Launcher](https://github.com/LNH-team/pico-launcher) by the LNH team, adding library features inspired by modern consoles, like favorites, play stats and recently played, while staying fully compatible with stock SD cards. It is a drop-in `_picoboot.nds` replacement, and all upstream features remain intact.

![Cover flow in the Material theme: the most launched game, a favorite and completed, with its star, heart and check above the icon](docs/images/enhanced/Coverflow.png)
![Icon grid in the Material theme: the most launched game, a favorite and completed, with its star, heart and check above the icon](docs/images/enhanced/Grid.png)
![A custom theme: the game markers sit above the game's icon, drawn to read on any art](docs/images/enhanced/CustomTheme.png)
![The favorites panel, every favorite across all folders with its play time](docs/images/enhanced/Favorites.png)
![The statistics panel: totals, most launched games and the last one played](docs/images/enhanced/Statistics.png)
![The menu: recently played, favorites, statistics, delete game, and the two filters with their state](docs/images/enhanced/Menu.png)
![The about sheet: Pico Launcher by the LNH team, Enhanced by rasalopa, where the build came from, and a cheat sheet of the controls](docs/images/enhanced/About.png)

*Taken on the console with the launcher's own screenshot key (hold START).*

## Features

Everything upstream Pico Launcher offers (display modes, [custom icons, banners & covers](docs/Customization.md) for games *and folders*, [themes](docs/Themes.md), [cheats](docs/Cheats.md), [file associations](docs/FileAssociations.md); see [Usage](docs/Usage.md)), plus:

- **A menu instead of a row of icons**: the app bar keeps back and display settings and gains a three-dot button; recently played, favorites, statistics, deletion and the two filters live in the sheet it opens, each with its name
- **About sheet**: from the menu's title row: who made what, the exact build and the repository it came from, and a cheat sheet of the controls that have no button
- **Jump by initial**: press L or R to jump to the next initial letter in a folder sorted by name; the letter you land on shows for a moment at the bottom of the touch screen
- **Game count** of the current folder, at the top of the statistics panel
- **Random game launch** with SELECT + A
- **Favorites**: press X on a game; a heart shows on the top screen
- **Completed games**: hold X on a game; a green check shows on the top screen
- **Favorites and completed filters**: Only favorites and Only completed in the menu, each saying when it is on
- **Favorites panel**: every favorite across all folders, from the menu; tapping one jumps to it
- **Recently played panel**: from the menu; tapping an entry jumps to the game
- **Statistics panel**: totals, most-played games and the launcher version, from the menu
- **Screenshots**: hold START for about half a second to save both screens to `/_pico/screenshots`
- **Per-game launch tracking**: launch count and last-played date, kept per game; the recently played panel shows each game's date, and the statistics panel the counts of the three most launched games; can be switched off in `settings.json`
- **Approximate play time**: per game, in the favorites panel
- **Game deletion**: from the menu, with confirmation; removes the ROM and its save
- **Theme deletion** (next release): from the theme selector, with confirmation; the theme in use and the two that come with the launcher are kept
- **A saves folder**: set `"saveLocation": "saves"` in `settings.json` and DS saves live in a `saves` folder next to the games, the layout TWiLight Menu++ uses, so both launchers can share one card; see [Usage.md](docs/Usage.md#settings)
- **Brightness control**: set the DS Lite's backlight level from display settings
- **Hide empty folders**: optional; folders with nothing playable inside are left out of the listing
- **Cheats list that reads better**: the list wraps around at both ends, and X turns every cheat off at once
- **Game markers on the top screen**: a gold star for the most launched game, a heart for a favorite and a check for a completed one, above the game's icon; a [custom theme](docs/Themes.md) can move or hide them
- **Per-folder background music**: drop a `bgm.bcstm` inside a folder
- **Time-of-day theme backgrounds**: optional night variants shown from 20:00 to 6:59
- **Per-file game data**: favorites, completed marks and play stats belong to the ROM file, so two copies of a game never share them

See [Enhanced features](docs/Enhanced.md) for details on each feature.

## Installation

1. Download `LAUNCHER.nds` from the [Releases](../../releases) page.
2. Rename it to `_picoboot.nds` and place it in the root of your SD card, replacing the existing one.

No other changes to your SD card are needed: themes, covers and the `_pico` folder from a stock setup keep working as-is.

> [!NOTE]
> To use Pico Launcher, the Pico Loader files (`aplist.bin`, `savelist.bin`, `picoLoader7.bin` and `picoLoader9.bin`) must also be present in the `/_pico` folder on your SD card.

## Setup & Configuration
We recommend using WSL (Windows Subsystem for Linux), or MSYS2 to compile this repository.
The steps provided will assume you already have one of those environments set up.

1. Install [BlocksDS](https://blocksds.skylyrac.net/docs/setup/)
2. Fetch the submodules: `git submodule update --init`

## Compiling

1. Run `make`

Alternatively, build with Docker (the same image used by CI) without installing BlocksDS locally:

```sh
docker run --rm -v "$PWD":/work -w /work skylyrac/blocksds:slim-v1.16.0 make
```

The launcher can be found in the root directory under the name `LAUNCHER.nds`.

2. Copy `LAUNCHER.nds` to your SD card.
    - If you are using DSpico, rename to `_picoboot.nds` and place it in the root of your SD card.
3. Copy the `_pico` pico folder to the root of your SD card.

For DSpico the final directory structure will look like this:
```
.
├── _pico
│   ├── themes
│   │   ├── material
│   │   │   └── theme.json
│   │   └── raspberry
│   │       ├── bannerListCell.bin
│   │       ├── bannerListCellPltt.bin
│   │       ├── bannerListCellSelected.bin
│   │       ├── bannerListCellSelectedPltt.bin
│   │       ├── bottombg.bin
│   │       ├── gridcell.bin
│   │       ├── gridcellPltt.bin
│   │       ├── gridcellSelected.bin
│   │       ├── gridcellSelectedPltt.bin
│   │       ├── scrim.bin
│   │       ├── scrimPltt.bin
│   │       ├── theme.json
│   │       └── topbg.bin
│   ├── aplist.bin
│   ├── savelist.bin
│   ├── picoLoader7.bin
│   └── picoLoader9.bin
└── _picoboot.nds
```
Note: If you want to play DSiWare on the DSpico, additional files are required. See the [Pico Loader](https://github.com/LNH-team/pico-loader) readme for more information.

## Extra tools

The [`tools/`](tools/) directory contains desktop helper scripts for preparing SD card content: cover art converters and fetchers, banner and icon generators, and night background makers. See [Tools](docs/Tools.md).

## Data formats

The fork stores per-game data (favorites, launch counts, play time) in `/_pico/gamedata.json`. Each entry belongs to one ROM file. If a favorite or a play count is not where you expect it, [Data storage](docs/Enhanced.md#data-storage) explains why in a table. The file format itself is documented in [Game data](docs/GameData.md) for tool authors.

## Contributing

Bug reports, ideas and pull requests are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

Every change the fork makes on top of upstream is mapped in [The fork against upstream](docs/Upstream.md), with its commits and the upstream files it touches, so any of it can be taken on its own.

## License

Icons by [icons8](https://icons8.com/)

This project is licensed under the Zlib license. For details, see `LICENSE.txt`.

Additional licenses may apply to the project. For details, see the `license` directory.

## Contributors
- [@Gericom](https://github.com/Gericom)
- [@XLuma](https://github.com/XLuma)
- [@Dartz150](https://github.com/Dartz150)
- [@lifehackerhansol](https://github.com/lifehackerhansol)

All credit for the launcher's foundation goes to the LNH team. This fork only builds on their excellent work.
