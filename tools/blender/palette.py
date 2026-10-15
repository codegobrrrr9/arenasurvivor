"""Shared colour palette for the Arena Survivor art pipeline.

The palette is a 4x4 grid of flat colours baked into a 64x64 PNG. Each model
face gets UVs collapsed to the centre of one cell, so a single URP/Lit material
with this texture colours every model.

This module has no dependency on Blender so it can also be run on its own:
    python3 -I tools/blender/palette.py Assets/Art/Textures/palette.png
"""
import struct
import sys
import zlib

GRID = 4
CELL_PX = 16
SIZE_PX = GRID * CELL_PX

# name -> (r, g, b) in sRGB 0-255. Index = row * GRID + col.
PALETTE = [
    ("player_blue", (52, 120, 246)),
    ("player_dark", (28, 44, 80)),
    ("skin", (240, 196, 160)),
    ("gun_metal", (70, 74, 82)),
    ("blob_green", (96, 200, 90)),
    ("spiky_purple", (160, 80, 220)),
    ("tank_rust", (176, 92, 48)),
    ("eye_white", (245, 245, 245)),
    ("eye_black", (20, 20, 24)),
    ("gem_cyan", (70, 230, 240)),
    ("heart_red", (230, 60, 70)),
    ("floor_light", (120, 124, 136)),
    ("floor_dark", (88, 92, 104)),
    ("pillar_stone", (150, 150, 158)),
    ("crate_wood", (190, 140, 80)),
    ("projectile_yellow", (255, 220, 80)),
]

NAMES = [n for n, _ in PALETTE]
INDEX = {n: i for i, n in enumerate(NAMES)}


def uv_for(name):
    """UV centre of the palette cell for a colour name (Blender UV, V up)."""
    i = INDEX[name]
    col, row = i % GRID, i // GRID
    u = (col + 0.5) / GRID
    v = 1.0 - (row + 0.5) / GRID
    return (u, v)


def _png_chunk(tag, data):
    body = tag + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def write_png(path):
    rows = []
    for y in range(SIZE_PX):
        row = bytearray([0])  # filter type 0
        cell_row = y // CELL_PX
        for x in range(SIZE_PX):
            cell_col = x // CELL_PX
            r, g, b = PALETTE[cell_row * GRID + cell_col][1]
            row += bytes((r, g, b))
        rows.append(bytes(row))
    raw = b"".join(rows)
    ihdr = struct.pack(">IIBBBBB", SIZE_PX, SIZE_PX, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + _png_chunk(b"IHDR", ihdr)
    png += _png_chunk(b"IDAT", zlib.compress(raw, 9)) + _png_chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "palette.png"
    write_png(out)
    print(f"wrote {out} ({SIZE_PX}x{SIZE_PX}, {len(PALETTE)} colours)")
