# Enhanced Features
This fork adds a number of quality-of-life features on top of Pico Launcher. This document describes each of them.

## Controls
These controls are available in the rom browser, on top of the standard ones (see [Usage](Usage.md)):

| Input | Action |
|---|---|
| L / R | Jump to the previous or next initial (see [Jumping by initial](#jumping-by-initial)) |
| X (short press) | Toggle favorite for the highlighted game |
| X (hold ~half a second) | Toggle completed for the highlighted game |
| SELECT + A | Launch a random game from the current folder |
| Heart button (app bar) | Toggle the favorites filter (the heart turns red while active) |
| Heart button (hold ~half a second) | Open the favorites panel: all favorites from every folder |
| Check button (app bar) | Toggle the completed filter (the check turns green while active) |
| Light row (display settings) | Set the DS Lite backlight level (4 levels) |
| Folder button (display settings) | Toggle hiding empty folders |
| Clock button (app bar) | Open the recently played panel |
| Clock button (hold ~half a second) | Open the statistics panel |
| Trash button (app bar) | Delete the highlighted game (X confirms, A or B cancels) |

## Jumping by initial
In a folder with hundreds of games, paging through the list a screen at a time takes a
while. L and R now jump to where the previous or next initial starts, so crossing a large
folder takes a few presses instead of dozens.

R moves to the first game filed under the next initial. L moves to the top of the current
initial, and again to the top of the one before it, so getting back to the start of a long
run of games sharing an initial does not need a detour.

As you jump, the game count on the top screen briefly shows the initial you landed on, then
settles back to the count, so it stays easy to see where you are without watching the list.

Folders and games are stepped through separately, since folders are always listed first.
Games are grouped by the first character of their file name, which is what the list is
sorted by, so the jump always follows the order on screen. This means a game whose file
name starts with an article or a number is filed under that, not under its title.

L and R keep paging in the cheats, favorites and recently played lists.

## Game count
The top-left of the top screen shows how many games the current folder contains (e.g. `12 games`). Only games are counted, not folders or other files.

## Favorites
Press X on a highlighted game to mark it as a favorite (press again to unmark). Favorites show a small heart on the top screen when highlighted. The mark belongs to that ROM **file**: moving it to another folder keeps it, renaming it starts over, and a second copy of the same game is marked separately — see [Data storage](#data-storage) if a mark is not where you expect it.

## Favorites panel
Hold the heart button in the app bar for about half a second to open a panel listing your favorites from **every** folder, alphabetically, each with its total play time — handy when the collection is spread across many folders. Tap an entry (or highlight it and press A) to jump to that game's folder with the game preselected; press B to close.

Favorites marked before this feature existed appear in the panel after you toggle them again or launch them once (the panel needs the game's stored path). An entry whose file has moved or is gone still appears, but selecting it does nothing instead of jumping to the card root; re-mark or launch the game from its new location to update it.

## Completed games
Hold X on a highlighted game for about half a second to mark it as completed (hold again to unmark). Completed games show a small green check on the top screen when highlighted, next to the heart. Like favorites, the mark belongs to the ROM file.

## Favorites and completed filters
The heart button in the app bar filters the browser down to favorites; the heart is drawn red while the filter is active. The check button next to it filters down to completed games and turns green while active. With both filters on, only games that are favorite *and* completed remain. The filters apply per folder — folders themselves always stay visible.

Marks, play counts and play time all belong to the ROM file, so what the top screen shows and what the filter matches are always the same thing (see [GameData.md](GameData.md)). Two copies of a game are marked separately, and a ROM hack no longer inherits its base game's mark. Renaming a ROM outside the launcher starts it over, and games whose file name is longer than 96 bytes cannot be marked at all (accented characters count double).

## Random game
Hold SELECT and press A to launch a random game from the folder you are currently viewing. With the favorites filter active, it picks a random favorite. SELECT on its own does nothing, so the DSi brightness shortcut (SELECT + volume) stays free.

## Launch tracking and play time
Every launch is recorded automatically. The top-right of the top screen shows the highlighted game's launch count together with its total play time (`3x 2h05`), or with the date it was last played (`3x 16/07`, day/month) when no play time has been recorded yet.

Play time is approximate: a session starts when a game is launched and ends the next time the launcher boots. Because of that:
- Sessions longer than 6 hours are discarded — that was a power-off, not a play session.
- Time spent in sleep mode counts as play time.
- A session is lost if the console is powered off without booting back into the launcher.

## Recently played
The clock button in the app bar opens a list of up to 20 recently played games, most recent first, each with the date and time it was last played. Tap an entry (or highlight it and press A) to jump to that game's folder with the game preselected. Press B to close the panel.

## Statistics
Hold the clock button in the app bar for about half a second to open a summary panel: how many games you have played, favorited and completed, total launches and total play time, your top 3 most launched games, and the last game you played. Press B to close it. A short tap on the clock opens the recently played panel instead.

## Deleting games
The trash button in the app bar deletes the highlighted game. A confirmation sheet opens first: press **X** to confirm, or A or B to cancel. Only games can be deleted, not folders.

Deleting a game also deletes its save file (same name with a `.sav` extension, next to the ROM) and removes the game's entry from `gamedata.json`. Note that saves are matched by name without the extension: if `Game.gba` and `Game.nds` sit in the same folder, they share `Game.sav`, and deleting either game deletes it.

## Cheats
The cheat list wraps around at both ends: pressing up on the first entry jumps to the last
one, and pressing down on the last entry comes back to the first, so the bottom of a long
cheat database is one press away from the top. Inside a sub-category, pressing up from the
first entry still moves to the back button, as before; the wrap happens where there is
nothing above the list to move to.

The sheet also shows a small `X: all off` hint next to the cheat description while cheats
are listed. Pressing X disables every cheat at once — the launcher supported this already,
but nothing on screen said so. Handy to make sure no code is active before going online or
starting a speedrun.

## Screen brightness (DS Lite)
The display settings sheet (gear button in the app bar) has a **Light** row with the DS Lite's four backlight levels. Tapping a level applies it immediately, and the choice is remembered and restored on every boot — it also stays active inside the game you launch, until the console powers off.

Until you pick a level the launcher leaves the firmware's brightness untouched. The original DS has no brightness levels (the setting does nothing there), and the DSi manages brightness through its own system menu.

## Hide empty folders
The display settings sheet has a folder toggle that hides folders containing no visible games, homebrew or media of their own (banner, BGM and other system files don't count as content). It's off by default; toggling it refreshes the folder you're currently viewing immediately.

Subfolders are followed a few levels deep, so a folder containing only other empty folders is hidden too. Launcher support folders (names starting with `_`) are always kept.

Emptiness means "has nothing in it", independently of the favorites and completed filters. With one of those filters on you can therefore still see a folder that turns out to hold nothing matching it — checking the filters here meant reading every ROM in every folder on each navigation, which was slow enough that folders started reappearing.

## Per-folder music
Place a `bgm.bcstm` file directly inside a folder to give it its own background music. It uses the same DSP-ADPCM `.bcstm` format as theme music (see [Themes](Themes.md)) and supports looping. The music starts when you enter the folder and switches back to the theme music when you leave. Each folder is checked independently — subfolders do not inherit their parent's music.

The `bgm.bcstm` file itself is not shown in the rom browser (file extensions without an association are hidden).

## Time-of-day theme backgrounds
Custom themes can provide night variants of their backgrounds: place `topbg_night.bin` and/or `bottombg_night.bin` next to `topbg.bin` and `bottombg.bin` in the theme folder (same 256x192, 15 bpp format). Between 20:00 and 6:59 the night variants are used when present. The time is checked when the launcher starts. Delete the `_night` files to disable the effect.

`tools/make_night_bg.py` can generate night variants from a theme's existing backgrounds — see [Tools.md](Tools.md).

## Data storage
Favorites, completed marks, launch counts, play time and the recents list are all stored in a single file, `/_pico/gamedata.json`, written by the launcher itself. Saves are atomic, and if the file ever fails to parse the launcher refuses to overwrite it rather than starting over. Deleting the file resets all favorites and statistics.

**Each entry belongs to one ROM file, identified by its file name.** That single rule explains most surprises:

| What you see | Why |
|---|---|
| A game lost its heart and its play time | The file was renamed outside the launcher. The launcher sees a different file, so it starts over. The old entry stays in the file, unused |
| The same game in two folders has separate favorites | They are two files. Marking one does not mark the other |
| A ROM hack does not inherit the base game's marks | Same reason — different files, even though they share an internal game code |
| Two copies with the *same* file name share one entry | The name is the identity, so same name means same entry. Deleting one through the launcher removes the entry both were using |
| Pressing X does nothing on some game | Its file name is longer than 96 bytes, which cannot be stored (accented characters count as two). The launcher logs it |
| A favorite is missing from the favorites panel | The panel only lists entries with a stored path. Marks made before that existed get one the next time you launch or re-mark the game. Entries whose file has moved or is gone are still listed, but selecting them does nothing |

Earlier versions identified a game by its internal game code instead, which made a ROM hack and its base game share one entry, and could leave the browser filter and the top screen disagreeing about the same game.

The file format is documented in [GameData.md](GameData.md) for anyone writing external tools.
