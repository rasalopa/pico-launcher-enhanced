#!/usr/bin/env python3
"""Download missing GBA covers from libretro-thumbnails (No-Intro) and install
them on the SD in Pico Launcher's format.

For each .gba in <SD>/Games/gba without a cover: read the gamecode (offset
0xAC), find the boxart by name (fuzzy, preferring the region indicated by the
gamecode's last letter), convert it with img2cover and drop it at
<SD>/_pico/covers/gba/<CODE>.bmp (or covers/user/<file>.bmp if there is no code).

Usage: python3 tools/fetch_covers_gba.py [/Volumes/DSPICO] [--dry-run]
       python3 tools/fetch_covers_gba.py --sd E:\\ [--dry-run]
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

REPO = "libretro-thumbnails/Nintendo_-_Game_Boy_Advance"
RAW = f"https://raw.githubusercontent.com/{REPO}/master/Named_Boxarts/"

REGION_PREF = {
    "E": ["(USA", "(World", "(Europe"],
    "P": ["(Europe", "(World", "(USA"],
    "S": ["(Spain", "(Europe", "(USA"],
    "F": ["(France", "(Europe", "(USA"],
    "D": ["(Germany", "(Europe", "(USA"],
    "I": ["(Italy", "(Europe", "(USA"],
    "J": ["(Japan", "(USA", "(Europe"],
}
DEFAULT_PREF = ["(Europe", "(USA", "(World"]


def norm(s: str) -> str:
    s = unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode()
    s = re.sub(r"\(.*?\)", "", s)            # strip (USA), (Rev 1), etc.
    s = re.sub(r"[^a-z0-9]+", " ", s.lower())
    s = re.sub(r"\b(the|a|an|el|la|los|las)\b", " ", s)
    return " ".join(s.split())


def fetch_catalog() -> list[str]:
    url = f"https://api.github.com/repos/{REPO}/git/trees/master?recursive=1"
    with urllib.request.urlopen(url, timeout=60) as r:
        tree = json.load(r)["tree"]
    return [
        e["path"].removeprefix("Named_Boxarts/")
        for e in tree
        if e["path"].startswith("Named_Boxarts/") and e["path"].endswith(".png")
    ]


def pick(title: str, code: str, catalog: list[str], by_norm: dict[str, list[str]]) -> str | None:
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
    prefs = REGION_PREF.get(code[3] if len(code) == 4 else "", DEFAULT_PREF)
    for p in prefs:
        for c in candidates:
            if p in c:
                return c
    return candidates[0]


def main() -> None:
    # --sd is accepted here too. It used to be ignored, so someone who learned
    # the flag from the other script silently wrote to whatever the default
    # happened to be instead of the card they named.
    argv = sys.argv[1:]
    dry = "--dry-run" in argv
    sd = "/Volumes/DSPICO"
    sd_flag: str | None = None
    positional: list[str] = []
    i = 0
    while i < len(argv):
        if argv[i] == "--sd":
            if i + 1 >= len(argv):
                sys.exit("--sd needs a path after it")
            sd_flag = argv[i + 1]
            i += 2
            continue
        if argv[i].startswith("--"):
            if argv[i] != "--dry-run":
                sys.exit(f"unknown option {argv[i]}")
        else:
            positional.append(argv[i])
        i += 1
    if len(positional) > 1:
        sys.exit(f"one card path at a time, got {len(positional)}")
    # Only as a fallback: a bare path used to win over the flag, which is the
    # very thing this parsing was added to stop - naming one card and writing
    # to another.
    if positional and sd_flag is None:
        sd = positional[0]
    elif sd_flag is not None:
        sd = sd_flag
    # A bare drive letter on windows passes isdir but joins without a
    # separator, so "E:" plus "_pico" resolves against whatever directory that
    # drive is sitting in rather than its root.
    sd = os.path.abspath(sd)
    if not os.path.isdir(sd):
        sys.exit(f"No card at {sd}")
    games_dir = os.path.join(sd, "Games", "gba")
    covers_gba = os.path.join(sd, "_pico", "covers", "gba")
    covers_user = os.path.join(sd, "_pico", "covers", "user")

    # The launcher only ever opens these folders, it never creates them, so on
    # a card that has not had a cover installed by hand they are simply absent.
    # Listing one blind ended the run with a raw traceback.
    have = ({f[:-4].upper() for f in os.listdir(covers_gba)
             if f.lower().endswith(".bmp") and not f.startswith("._")}
            if os.path.isdir(covers_gba) else set())
    have_user = {f for f in os.listdir(covers_user)} if os.path.isdir(covers_user) else set()

    if not os.path.isdir(games_dir):
        sys.exit(f"No games folder at {games_dir}")

    print("Downloading libretro-thumbnails catalog...")
    catalog = fetch_catalog()
    by_norm: dict[str, list[str]] = {}
    for name in catalog:
        by_norm.setdefault(norm(name[:-4]), []).append(name)
    print(f"{len(catalog)} boxarts in the catalog")

    ok, fail = [], []
    for f in sorted(os.listdir(games_dir)):
        if f.startswith("._") or not f.lower().endswith((".gba", ".agb")):
            continue
        with open(os.path.join(games_dir, f), "rb") as fh:
            fh.seek(0xAC)
            code = re.sub(r"[^A-Za-z0-9]", "", fh.read(4).decode("ascii", "replace"))
        if (code and code.upper() in have) or f + ".bmp" in have_user:
            continue

        title = f.rsplit(".", 1)[0]
        match = pick(title, code, catalog, by_norm)
        if not match:
            fail.append((f, code, "no catalog match"))
            continue
        if dry:
            ok.append((f, code, match))
            continue

        if code:
            os.makedirs(covers_gba, exist_ok=True)
            dst = os.path.join(covers_gba, f"{code.upper()}.bmp")
        else:
            os.makedirs(covers_user, exist_ok=True)
            dst = os.path.join(covers_user, f + ".bmp")
        try:
            url = RAW + urllib.parse.quote(match)
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
                convert(png, dst)
            finally:
                shutil.rmtree(tmpdir, ignore_errors=True)
            ok.append((f, code, match))
        except Exception as e:  # noqa: BLE001 — report and keep going with the rest
            fail.append((f, code, str(e)))

    print(f"\n✓ {len(ok)} covers{' (dry-run)' if dry else ' installed'}:")
    for f, code, m in ok:
        print(f"  [{code or '----'}] {f}  ←  {m}")
    if fail:
        print(f"\n✗ {len(fail)} unresolved:")
        for f, code, why in fail:
            print(f"  [{code or '----'}] {f}: {why}")


if __name__ == "__main__":
    main()
