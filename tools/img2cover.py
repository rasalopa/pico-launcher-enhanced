#!/usr/bin/env python3
"""Convert any image to a Pico Launcher cover.

Format (see arm9/source/romBrowser/FileType/BmpHeader.h and BmpFileCover.cpp):
128x96 BMP, 8bpp indexed, uncompressed, 40-byte DIB, clrUsed=256.
The launcher only shows the leftmost 106x96, so the art is stretched to
exactly 106x96 (same convention as the community cover packs: no visible
black bars; the slight distortion is unnoticeable on screen) and columns
106-127 stay black (invisible area).

Usage: python3 tools/img2cover.py input.png output.bmp
"""
import struct
import sys

from PIL import Image

W, H = 128, 96
VISIBLE_W = 106


def convert(src_path: str, dst_path: str) -> None:
    im = Image.open(src_path).convert("RGB")
    art = im.resize((VISIBLE_W, H), Image.LANCZOS)

    canvas = Image.new("RGB", (W, H), (0, 0, 0))
    canvas.paste(art, (0, 0))

    quant = canvas.quantize(colors=256, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG)
    # An image with few colours quantizes to fewer than 256 entries, and the
    # packing below indexes all 256 unconditionally - a flat boxart died with
    # "list index out of range", which both fetch scripts then reported as if
    # the download had failed.
    pal = (quant.getpalette() or [])[: 256 * 3]
    pal += [0] * (256 * 3 - len(pal))
    pixels = quant.tobytes()

    pal_bytes = b"".join(
        struct.pack("<BBBB", pal[i * 3 + 2], pal[i * 3 + 1], pal[i * 3], 0) for i in range(256)
    )
    pixel_offset = 14 + 40 + len(pal_bytes)
    # rows bottom-up, 128 bytes/row (multiple of 4)
    pixel_data = b"".join(pixels[y * W : (y + 1) * W] for y in range(H - 1, -1, -1))

    header = struct.pack("<2sIHHI", b"BM", pixel_offset + len(pixel_data), 0, 0, pixel_offset)
    dib = struct.pack("<IiiHHIIiiII", 40, W, H, 1, 8, 0, len(pixel_data), 2835, 2835, 256, 0)

    with open(dst_path, "wb") as f:
        f.write(header + dib + pal_bytes + pixel_data)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    convert(sys.argv[1], sys.argv[2])
    print(f"{sys.argv[2]}: {W}x{H}, 8bpp, 256 colors")
