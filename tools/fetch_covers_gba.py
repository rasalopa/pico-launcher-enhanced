#!/usr/bin/env python3
"""Download missing GBA covers from libretro-thumbnails (No-Intro) and install
them on the SD in Pico Launcher's format.

For each .gba in <SD>/Games/gba without a cover: read the gamecode (offset
0xAC), find the boxart by name (fuzzy, preferring the region indicated by the
gamecode's last letter), convert it with img2cover and drop it at
<SD>/_pico/covers/gba/<CODE>.bmp (or covers/user/<file>.bmp if there is no code).

Usage: python3 tools/fetch_covers_gba.py [/Volumes/DSPICO] [--dry-run]
"""
from __future__ import annotations

import difflib
import json
import os
import re
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
    sd = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("--") else "/Volumes/DSPICO"
    dry = "--dry-run" in sys.argv
    games_dir = os.path.join(sd, "Games", "gba")
    covers_gba = os.path.join(sd, "_pico", "covers", "gba")
    covers_user = os.path.join(sd, "_pico", "covers", "user")

    have = {f[:-4].upper() for f in os.listdir(covers_gba) if f.lower().endswith(".bmp") and not f.startswith("._")}
    have_user = {f for f in os.listdir(covers_user)} if os.path.isdir(covers_user) else set()

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
            dst = os.path.join(covers_gba, f"{code.upper()}.bmp")
        else:
            os.makedirs(covers_user, exist_ok=True)
            dst = os.path.join(covers_user, f + ".bmp")
        try:
            url = RAW + urllib.parse.quote(match)
            with tempfile.TemporaryDirectory() as tmpdir:
                # Windows refuses a second handle on a NamedTemporaryFile while
                # the first is still open, and convert() opens it by name - so
                # the download goes to a plain file inside a temporary directory
                # that is closed before converting. Reported as issue #12.
                png = os.path.join(tmpdir, "cover.png")
                with urllib.request.urlopen(url, timeout=60) as r, open(png, "wb") as fh:
                    fh.write(r.read())
                convert(png, dst)
            ok.append((f, code, match))
        except Exception as e:  # noqa: BLE001 — report and keep going with the rest
            fail.append((f, code, str(e)))

    print(f"\n✓ {len(ok)} covers installed:")
    for f, code, m in ok:
        print(f"  [{code or '----'}] {f}  ←  {m}")
    if fail:
        print(f"\n✗ {len(fail)} unresolved:")
        for f, code, why in fail:
            print(f"  [{code or '----'}] {f}: {why}")


if __name__ == "__main__":
    main()
