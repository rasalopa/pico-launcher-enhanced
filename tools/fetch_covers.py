#!/usr/bin/env python3
"""Download missing covers from libretro-thumbnails (No-Intro) for the systems
WITHOUT a gamecode (GB, GBC, Mega Drive, etc.) and install them into
<SD>/_pico/covers/user/<file>.bmp — the folder Pico Launcher checks by
filename for any associated type.

(For GBA use fetch_covers_gba.py, which leverages the gamecode in the header.)

Usage: python3 tools/fetch_covers.py <system...> [--sd /Volumes/DSPICO] [--dry-run]
       python3 tools/fetch_covers.py gb gbc gen
"""
from __future__ import annotations

import difflib
import json
import os
import re
import shutil
import sys
import tempfile
import unicodedata
import urllib.parse
import urllib.request

from img2cover import convert

# gamesDir relative to the SD root; exts in lowercase
SYSTEMS = {
    "gb":   {"repo": "Nintendo_-_Game_Boy",                          "gamesDir": "Games/gb",   "exts": (".gb",)},
    "gbc":  {"repo": "Nintendo_-_Game_Boy_Color",                    "gamesDir": "Games/gb",   "exts": (".gbc",)},
    "gen":  {"repo": "Sega_-_Mega_Drive_-_Genesis",                  "gamesDir": "Games/gen",  "exts": (".md", ".gen")},
    "sms":  {"repo": "Sega_-_Master_System_-_Mark_III",              "gamesDir": "Games/sms",  "exts": (".sms",)},
    "gg":   {"repo": "Sega_-_Game_Gear",                             "gamesDir": "Games/gg",   "exts": (".gg",)},
    "nes":  {"repo": "Nintendo_-_Nintendo_Entertainment_System",     "gamesDir": "Games/nes",  "exts": (".nes",)},
    "snes": {"repo": "Nintendo_-_Super_Nintendo_Entertainment_System", "gamesDir": "Games/snes", "exts": (".sfc", ".smc")},
    "ws":   {"repo": "Bandai_-_WonderSwan",                          "gamesDir": "Games/ws",   "exts": (".ws",)},
    "wsc":  {"repo": "Bandai_-_WonderSwan_Color",                    "gamesDir": "Games/ws",   "exts": (".wsc",)},
    "ngp":  {"repo": "SNK_-_Neo_Geo_Pocket",                         "gamesDir": "Games/ngp",  "exts": (".ngp",)},
    "ngc":  {"repo": "SNK_-_Neo_Geo_Pocket_Color",                   "gamesDir": "Games/ngp",  "exts": (".ngc",)},
}

REGION_PREF = ["(Europe", "(USA", "(World", "(Spain", "(Japan"]


def norm(s: str) -> str:
    s = unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode()
    s = re.sub(r"\(.*?\)", "", s)
    s = re.sub(r"[^a-z0-9]+", " ", s.lower())
    s = re.sub(r"\b(the|a|an|el|la|los|las)\b", " ", s)
    return " ".join(s.split())


def fetch_catalog(repo: str) -> list[str]:
    url = f"https://api.github.com/repos/libretro-thumbnails/{repo}/git/trees/master?recursive=1"
    with urllib.request.urlopen(url, timeout=60) as r:
        tree = json.load(r)["tree"]
    return [
        e["path"].removeprefix("Named_Boxarts/")
        for e in tree
        if e["path"].startswith("Named_Boxarts/") and e["path"].endswith(".png")
    ]


def pick(title: str, by_norm: dict[str, list[str]]) -> str | None:
    key = norm(title)
    candidates = by_norm.get(key)
    if not candidates:
        # prefix BEFORE fuzzy: difflib mixes up numbered entries (Zero 1 vs Zero 4)
        prefixes = [k for k in by_norm if key.startswith(k + " ") or k.startswith(key + " ")]
        if prefixes:
            candidates = by_norm[max(prefixes, key=len)]
    if not candidates:
        close = difflib.get_close_matches(key, by_norm.keys(), n=1, cutoff=0.85)
        if close:
            candidates = by_norm[close[0]]
    if not candidates:
        return None
    for p in REGION_PREF:
        for c in candidates:
            if p in c:
                return c
    return candidates[0]


def main() -> None:
    # --sd takes a value, so it cannot be filtered by prefix: doing that left
    # the path itself in the system list and the next check rejected it, which
    # made the documented option always print the usage and exit. Since the
    # default only exists on a mac, that made this script unusable anywhere
    # else - a worse bug than the one reported in issue #12, found reviewing it.
    argv = sys.argv[1:]
    sd = "/Volumes/DSPICO"
    dry = "--dry-run" in argv
    args: list[str] = []
    i = 0
    while i < len(argv):
        if argv[i] == "--sd":
            if i + 1 >= len(argv):
                sys.exit("--sd needs a path after it")
            sd = argv[i + 1]
            i += 2
            continue
        if argv[i].startswith("--"):
            # Not swallowed: a mistyped --dry-run would otherwise drop through
            # and install for real, writing files the user asked not to write.
            if argv[i] != "--dry-run":
                sys.exit(f"unknown option {argv[i]}")
        else:
            args.append(argv[i])
        i += 1
    if not args or any(a not in SYSTEMS for a in args):
        sys.exit(__doc__ + "\nSystems: " + ", ".join(SYSTEMS))

    # Checked before anything is created: with --sd working, a typo used to be
    # silently materialized on the wrong volume and the run still said it
    # succeeded. And nothing is created at all under --dry-run.
    # A bare drive letter on windows passes isdir but joins without a
    # separator, so "E:" plus "_pico" resolves against whatever directory that
    # drive is sitting in rather than its root.
    sd = os.path.abspath(sd)
    if not os.path.isdir(sd):
        sys.exit(f"No card at {sd}")

    covers_user = os.path.join(sd, "_pico", "covers", "user")
    if not dry:
        os.makedirs(covers_user, exist_ok=True)
    have = ({f for f in os.listdir(covers_user) if not f.startswith("._")}
            if os.path.isdir(covers_user) else set())

    ok, fail = [], []
    for system in args:
        cfg = SYSTEMS[system]
        games_dir = os.path.join(sd, cfg["gamesDir"])
        if not os.path.isdir(games_dir):
            continue
        pending = [
            f for f in sorted(os.listdir(games_dir))
            if not f.startswith("._") and f.lower().endswith(cfg["exts"]) and f + ".bmp" not in have
        ]
        if not pending:
            print(f"{system}: nothing to do")
            continue

        print(f"{system}: downloading catalog {cfg['repo']}...")
        catalog = fetch_catalog(cfg["repo"])
        by_norm: dict[str, list[str]] = {}
        for name in catalog:
            by_norm.setdefault(norm(name[:-4]), []).append(name)
        print(f"{system}: {len(catalog)} boxarts, {len(pending)} games without a cover")

        raw_base = f"https://raw.githubusercontent.com/libretro-thumbnails/{cfg['repo']}/master/Named_Boxarts/"
        for f in pending:
            match = pick(f.rsplit(".", 1)[0], by_norm)
            if not match:
                fail.append((system, f, "no catalog match"))
                continue
            if dry:
                ok.append((system, f, match))
                continue
            try:
                url = raw_base + urllib.parse.quote(match)
                # A temporary directory rather than a NamedTemporaryFile:
                # windows holds that one with an exclusive handle, and convert()
                # opens the file by name (issue #12). Torn down with
                # ignore_errors because a cleanup that fails - an indexer still
                # holding the png, again windows - must not turn a cover that is
                # already on the card into a reported failure.
                tmpdir = tempfile.mkdtemp()
                try:
                    png = os.path.join(tmpdir, "cover.png")
                    with urllib.request.urlopen(url, timeout=60) as r, open(png, "wb") as fh:
                        fh.write(r.read())
                    convert(png, os.path.join(covers_user, f + ".bmp"))
                finally:
                    shutil.rmtree(tmpdir, ignore_errors=True)
                ok.append((system, f, match))
            except Exception as e:  # noqa: BLE001 — report and keep going
                fail.append((system, f, str(e)))

    print(f"\n✓ {len(ok)} covers{' (dry-run)' if dry else ' installed'}:")
    for s, f, m in ok:
        print(f"  [{s}] {f}  ←  {m}")
    if fail:
        print(f"\n✗ {len(fail)} unresolved:")
        for s, f, why in fail:
            print(f"  [{s}] {f}: {why}")


if __name__ == "__main__":
    main()
