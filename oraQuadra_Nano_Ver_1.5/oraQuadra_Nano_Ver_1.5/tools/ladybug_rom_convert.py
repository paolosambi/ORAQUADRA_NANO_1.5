#!/usr/bin/env python3
"""
Lady Bug (Universal, 1981) ROM Converter for oraQuadra Nano Arcade Emulator

Supports BOTH the "ladybug" parent MAME romset and the "ladybugb" bootleg romset.
Auto-detects which set is present based on folder name and file availability.

MAME ladybugb ROM mapping (bootleg variant):
  CPU: lb1a.cpu, lb2a.cpu, lb3a.cpu replace lb1/2/3.cpu; lb6a.cpu replaces lb6.cpu
       lb4.cpu -> l4.h4, lb5.cpu -> l5.j4 (shared, IC position names)
  GFX: lb9.vid -> l9.f7, lb10.vid -> l0.h7, lb8.cpu -> l8.l7, lb7.cpu -> l7.m7
  PROMs: 10-2.vid -> 10-2.k1, 10-1.vid -> 10-1.f4, 10-3.vid -> 10-3.c4
  All GFX and PROMs have identical CRCs (shared content, different names).

Generates:
  1. rom1.bin              - 24KB CPU ROM for SD card (/ARCADE/LADYBUG/)
  2. ladybug_tilemap.h     - 512 decoded characters (8x8, 2bpp, ROT270)
  3. ladybug_spritemap.h   - 128 decoded sprites (16x16, 2bpp, 4 flip variants, ROT270)
  4. ladybug_cmap.h        - Color palettes (RGB565, from PROMs)

Usage:
  python tools/ladybug_rom_convert.py
  (run from project root)
"""

import os
import sys

# Paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
ROM_DST = os.path.join(PROJECT_ROOT, "ARCADE_ROMS_SD", "ARCADE", "LADYBUG")
HDR_DST = os.path.join(PROJECT_ROOT, "arcade", "machines", "ladybug")

# Auto-detect ROM source folder
ROM_SRC_LADYBUG = os.path.join(PROJECT_ROOT, "ARCADE_ROMS_SD", "ladybug")
ROM_SRC_LADYBUGB = os.path.join(PROJECT_ROOT, "ARCADE_ROMS_SD", "ladybugb")
ROM_SRC = None
ROMSET = None

# ============================================================================
# ROM file definitions: (parent_name, ladybugb_name)
# ============================================================================

# CPU ROMs: 6 x 4KB = 24KB
CPU_DEFS = [
    ("lb1.cpu", "lb1a.cpu"),    # 0x0000: bootleg-specific
    ("lb2.cpu", "lb2a.cpu"),    # 0x1000: bootleg-specific
    ("lb3.cpu", "lb3a.cpu"),    # 0x2000: bootleg-specific
    ("lb4.cpu", "l4.h4"),       # 0x3000: shared (CRC ffc424d7)
    ("lb5.cpu", "l5.j4"),       # 0x4000: shared (CRC ad6af809)
    ("lb6.cpu", "lb6a.cpu"),    # 0x5000: bootleg-specific
]

# GFX1 characters: 2 planes x 4KB = 512 chars
GFX1_PLANE0_DEFS = ("lb9.vid", "l9.f7")    # CRC 77b1da1e (shared)
GFX1_PLANE1_DEFS = ("lb10.vid", "l0.h7")   # CRC aa82e00b (shared)

# GFX2 sprites: 2 planes x 4KB = 128 sprites
GFX2_PLANE0_DEFS = ("lb8.cpu", "l8.l7")    # CRC 8b99910b (shared)
GFX2_PLANE1_DEFS = ("lb7.cpu", "l7.m7")    # CRC 86a5b448 (shared)

# Color PROMs: 32 bytes each
PROM_PALETTE_DEFS = ("10-2.vid", "10-2.k1")  # CRC df091e52 (shared)
PROM_SPRITE_DEFS = ("10-1.vid", "10-1.f4")   # CRC 40640d8f (shared)
PROM_CHAR_DEFS = ("10-3.vid", "10-3.c4")     # CRC 27fa3a50 (shared)


def resolve_rom(defs):
    """Resolve a (parent_name, alt_name) tuple to the actual filename in ROM_SRC."""
    parent_name, alt_name = defs
    if os.path.exists(os.path.join(ROM_SRC, parent_name)):
        return parent_name
    if alt_name and os.path.exists(os.path.join(ROM_SRC, alt_name)):
        return alt_name
    return parent_name


def read_rom(filename):
    """Read a ROM file and return its contents."""
    path = os.path.join(ROM_SRC, filename)
    if not os.path.exists(path):
        print(f"ERROR: ROM file not found: {path}")
        sys.exit(1)
    with open(path, "rb") as f:
        data = f.read()
    print(f"  Read: {filename} ({len(data)} bytes)")
    return data


def read_rom_resolved(defs):
    """Read a ROM using auto-resolved filename."""
    return read_rom(resolve_rom(defs))


# ============================================================================
# Rotation and flip helpers
# ============================================================================

def rotate_tile_270(rows_16bit):
    """Rotate 8x8 tile 270 CW for portrait display (ROT270).
    new_pixel(r, c) = old_pixel(c, 7 - r)"""
    rotated = []
    for out_row in range(8):
        val = 0
        for out_col in range(8):
            pixel = (rows_16bit[out_col] >> ((7 - out_row) * 2)) & 3
            val |= pixel << (out_col * 2)
        rotated.append(val)
    return rotated


def flip_tile_h(rows_16bit):
    """Flip 8x8 tile horizontally (reverse pixel order in each row)."""
    flipped = []
    for val in rows_16bit:
        new_val = 0
        for px in range(8):
            color = (val >> (px * 2)) & 3
            new_val |= color << ((7 - px) * 2)
        flipped.append(new_val)
    return flipped


def rotate_sprite_270(rows_32bit):
    """Rotate 16x16 sprite 270 CW for portrait display (ROT270).
    new_pixel(r, c) = old_pixel(c, 15 - r)"""
    rotated = []
    for out_row in range(16):
        val = 0
        for out_col in range(16):
            pixel = (rows_32bit[out_col] >> ((15 - out_row) * 2)) & 3
            val |= pixel << (out_col * 2)
        rotated.append(val)
    return rotated


def flip_sprite_h(rows_32bit):
    """Flip 16x16 sprite horizontally (reverse pixel order in each 32-bit row)."""
    flipped = []
    for val in rows_32bit:
        new_val = 0
        for px in range(16):
            color = (val >> (px * 2)) & 3
            new_val |= (color << ((15 - px) * 2))
        flipped.append(new_val)
    return flipped


def flip_sprite_v(rows_32bit):
    """Flip 16x16 sprite vertically (reverse row order)."""
    return list(reversed(rows_32bit))


# ============================================================================
# CPU ROM merging
# ============================================================================

def make_cpu_rom():
    """Concatenate 6 CPU ROMs into rom1.bin (24KB)."""
    os.makedirs(ROM_DST, exist_ok=True)

    print("CPU ROMs (24KB):")
    rom = bytearray()
    for parent_name, alt_name in CPU_DEFS:
        name = resolve_rom((parent_name, alt_name))
        data = read_rom(name)
        if len(data) != 4096:
            print(f"  WARNING: {name} is {len(data)} bytes (expected 4096)")
        rom.extend(data)

    out_path = os.path.join(ROM_DST, "rom1.bin")
    with open(out_path, "wb") as f:
        f.write(rom)
    print(f"  -> Created {out_path} ({len(rom)} bytes)")
    return rom


# ============================================================================
# Character tiles (GFX1): 512 chars, 8x8, 2bpp
# ============================================================================

def decode_char_tile(plane0_data, plane1_data, char_index):
    """
    Decode one 8x8 character from Lady Bug GFX1 ROMs.

    512 characters total. Each ROM is 4KB = 512 chars x 8 bytes/char.
    Plane 0: l9.f7 (lb9.vid), Plane 1: l0.h7 (lb10.vid)
    Bit layout per row: MSB-first (bit 7 = leftmost pixel).

    Returns 8 rows of 16-bit packed 2bpp values.
    Each pixel = 2 bits, packed LSB-first (pixel 0 in bits 0-1, etc).
    """
    rows = []
    base = char_index * 8

    for row in range(8):
        b0 = plane0_data[base + row]  # l9.f7 = MAME plane 0 = MSB (bit 1)
        b1 = plane1_data[base + row]  # l0.h7 = MAME plane 1 = LSB (bit 0)

        val = 0
        for px in range(8):
            bit0 = (b0 >> (7 - px)) & 1  # plane 0 = MSB
            bit1 = (b1 >> (7 - px)) & 1  # plane 1 = LSB
            color = (bit0 << 1) | bit1    # MAME: plane 0 is MSB
            val |= (color << (px * 2))
        rows.append(val)

    return rows


def make_tilemap():
    """Generate ladybug_tilemap.h with 512 decoded characters."""
    p0 = read_rom_resolved(GFX1_PLANE0_DEFS)
    p1 = read_rom_resolved(GFX1_PLANE1_DEFS)

    num_chars = min(len(p0), len(p1)) // 8  # 512

    lines = []
    lines.append("// Lady Bug character tiles - 512 chars, 8x8, 2bpp packed")
    lines.append("// Generated by ladybug_rom_convert.py")
    lines.append(f"const unsigned short ladybug_tilemap[{num_chars}][8] = {{")

    for ch in range(num_chars):
        rows = decode_char_tile(p0, p1, ch)
        rows = rotate_tile_270(rows)
        row_str = ",".join(f"0x{r:x}" for r in rows)
        comma = "," if ch < num_chars - 1 else ""
        lines.append(f" {{ {row_str} }}{comma}")

    lines.append("};")

    out_path = os.path.join(HDR_DST, "ladybug_tilemap.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path} ({num_chars} characters)")


# ============================================================================
# Sprites (GFX2): 128 sprites, 16x16, 2bpp packed
#
# MAME spritelayout (from mamedev/mame ladybug.cpp):
#   planes: { 1, 0 }  - packed 2-bit pixels (NOT separate ROMs!)
#   xoffset: { 0,2,4,6,8,10,12,14, 128,130,132,134,136,138,140,142 }
#   yoffset: { 23*16,22*16,...,16*16, 7*16,6*16,...,0*16 }  (rows reversed)
#   charincrement: 64*8 (64 bytes per sprite)
#
# GFX2 region = lb8.cpu (4KB at 0x0000) + lb7.cpu (4KB at 0x1000) = 8KB
# 128 sprites x 64 bytes = 8192 bytes
#
# 64-byte sprite block layout (4 quadrants, rows reversed within each):
#   Bytes  0-15: bottom-left  (rows 15-8, cols 0-7, reversed)
#   Bytes 16-31: bottom-right (rows 15-8, cols 8-15, reversed)
#   Bytes 32-47: top-left     (rows 7-0, cols 0-7, reversed)
#   Bytes 48-63: top-right    (rows 7-0, cols 8-15, reversed)
#
# Each row = 2 bytes (16 bits) for 8 pixels x 2 bits.
# Pixel format: bit at even position = MSB (plane 1), bit at odd = LSB (plane 0).
# ============================================================================

# MAME yoffset values (bit offsets)
_SPRITE_YOFF = [23*16, 22*16, 21*16, 20*16, 19*16, 18*16, 17*16, 16*16,
                7*16,  6*16,  5*16,  4*16,  3*16,  2*16,  1*16,  0*16]


def decode_sprite(combined_data, sprite_index):
    """
    Decode one 16x16 sprite from combined GFX2 data (lb8+lb7, 8KB).

    Uses the exact MAME spritelayout: packed 2-bit pixels, reversed rows,
    4-quadrant byte layout, 64 bytes per sprite.

    Returns 16 rows of 32-bit packed 2bpp values (same format as other games).
    """
    rows = []
    base = sprite_index * 64  # 64 bytes per sprite

    for row in range(16):
        y_off = _SPRITE_YOFF[row]  # bit offset for this row

        # Left half (cols 0-7): xoffset starts at 0
        left_byte = base + y_off // 8
        word_left = combined_data[left_byte] | (combined_data[left_byte + 1] << 8)

        # Right half (cols 8-15): xoffset starts at 128 (16 bytes ahead)
        right_byte = base + (y_off + 128) // 8
        word_right = combined_data[right_byte] | (combined_data[right_byte + 1] << 8)

        val = 0
        for px in range(8):
            # MAME planeoffset {1, 0}: bit at even pos = MSB, bit at odd pos = LSB
            msb = (word_left >> (px * 2)) & 1       # plane 1 (MSB)
            lsb = (word_left >> (px * 2 + 1)) & 1   # plane 0 (LSB)
            color = lsb | (msb << 1)
            val |= color << (px * 2)

            msb = (word_right >> (px * 2)) & 1
            lsb = (word_right >> (px * 2 + 1)) & 1
            color = lsb | (msb << 1)
            val |= color << ((px + 8) * 2)

        rows.append(val)

    return rows


def make_spritemap():
    """Generate ladybug_spritemap.h with 128 sprites x 4 flip variants."""
    # Combine both GFX2 ROMs into one 8KB block (MAME region layout)
    p0 = read_rom_resolved(GFX2_PLANE0_DEFS)  # lb8.cpu at 0x0000
    p1 = read_rom_resolved(GFX2_PLANE1_DEFS)  # lb7.cpu at 0x1000
    combined = bytearray(p0) + bytearray(p1)   # 8KB combined

    num_sprites = len(combined) // 64  # 128

    lines = []
    lines.append("// Lady Bug sprites - 128 sprites, 16x16, 2bpp packed")
    lines.append("// 4 flip variants: [0]=normal, [1]=flipX, [2]=flipY, [3]=flipXY")
    lines.append("// Generated by ladybug_rom_convert.py")
    lines.append(f"const unsigned long ladybug_sprites[4][{num_sprites}][16] = {{")

    for flip in range(4):
        lines.append(" {")
        for sp in range(num_sprites):
            rows = decode_sprite(combined, sp)

            if flip == 1:
                rows = flip_sprite_h(rows)
            elif flip == 2:
                rows = flip_sprite_v(rows)
            elif flip == 3:
                rows = flip_sprite_h(flip_sprite_v(rows))

            rows = rotate_sprite_270(rows)

            row_str = ",".join(f"0x{r:x}" for r in rows)
            comma = "," if sp < num_sprites - 1 else ""
            lines.append(f"  {{ {row_str} }}{comma}")

        comma = "," if flip < 3 else ""
        lines.append(f" }}{comma}")

    lines.append("};")

    out_path = os.path.join(HDR_DST, "ladybug_spritemap.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path} ({num_sprites} sprites x 4 flips)")


# ============================================================================
# Color palettes (from PROMs)
# ============================================================================

def prom_to_rgb565(prom_byte):
    """
    Decode a Lady Bug color PROM byte to RGB565.

    From MAME ladybug.cpp ladybug_palette():
    - Bits are INVERTED (~color_prom[i])
    - Resistor weights via compute_resistor_weights():
      resistances = {470, 220}, pulldown = 470 ohm
      -> 470 ohm = weight 81, 220 ohm = weight 174
    - Bit 0: Red   470 ohm (81)
    - Bit 5: Red   220 ohm (174)
    - Bit 2: Green 470 ohm (81)
    - Bit 6: Green 220 ohm (174)
    - Bit 4: Blue  470 ohm (81)
    - Bit 7: Blue  220 ohm (174)
    """
    inv = (~prom_byte) & 0xFF

    r8 = ((inv >> 0) & 1) * 81 + ((inv >> 5) & 1) * 174
    g8 = ((inv >> 2) & 1) * 81 + ((inv >> 6) & 1) * 174
    b8 = ((inv >> 4) & 1) * 81 + ((inv >> 7) & 1) * 174

    r5 = (r8 >> 3) & 0x1F
    g6 = (g8 >> 2) & 0x3F
    b5 = (b8 >> 3) & 0x1F

    rgb565 = (r5 << 11) | (g6 << 5) | b5
    # Byte-swap to big-endian (pre-swapped for SPI displays, matching other games)
    return ((rgb565 >> 8) & 0xFF) | ((rgb565 & 0xFF) << 8)


def bitswap4(val):
    """Reverse 4 bits - MAME bitswap<4>(val, 0,1,2,3)."""
    b0 = (val >> 0) & 1
    b1 = (val >> 1) & 1
    b2 = (val >> 2) & 1
    b3 = (val >> 3) & 1
    return (b0 << 3) | (b1 << 2) | (b2 << 1) | (b3 << 0)


def make_colormap():
    """
    Generate ladybug_cmap.h with character and sprite color palettes.

    PROM layout (0x60 bytes total from 3 PROMs):
      0x00-0x1F: 10-2.vid/k1 (palette: 32 RGB colors)
      0x20-0x3F: 10-1.vid/f4 (sprite color lookup)
      0x40-0x5F: 10-3.vid/c4 (unused by palette init)

    Characters: COMPUTED mapping (no PROM lookup!)
      ctabentry = ((i << 3) & 0x18) | ((i >> 2) & 0x07)
      = pixel_value * 8 + color_group
      So group g, pixel p -> palette[p*8 + g]

    Sprites: from sprite PROM with bitswap<4> on lower nibble
      ctabentry = bitswap<4>(sprite_prom[i] & 0x0F, 0,1,2,3)
    """
    palette_prom = read_rom_resolved(PROM_PALETTE_DEFS)  # 32 entries
    sprite_prom = read_rom_resolved(PROM_SPRITE_DEFS)    # 32 entries

    # Decode all 32 palette colors to RGB565
    palette = [prom_to_rgb565(palette_prom[i]) for i in range(32)]

    lines = []
    lines.append("// Lady Bug color maps - RGB565")
    lines.append("// Generated by ladybug_rom_convert.py")
    lines.append("")

    # Character colors: 8 groups x 4 colors (COMPUTED, no PROM)
    lines.append("// Character color palette: 8 groups x 4 colors")
    lines.append("const unsigned short ladybug_colormap[8][4] = {")
    for group in range(8):
        colors = []
        for pixel in range(4):
            if pixel == 0:
                colors.append(0)  # transparent (black)
            else:
                palette_idx = pixel * 8 + group
                colors.append(palette[palette_idx])
        color_str = ",".join(f"0x{c:x}" for c in colors)
        comma = "," if group < 7 else ""
        lines.append(f"{{{color_str}}}{comma}")
    lines.append("};")
    lines.append("")

    # Sprite colors: 8 groups x 4 colors (from sprite PROM with bitswap)
    lines.append("// Sprite color palette: 8 groups x 4 colors")
    lines.append("const unsigned short ladybug_sprite_colormap[8][4] = {")
    for group in range(8):
        colors = []
        for pixel in range(4):
            if pixel == 0:
                colors.append(0)  # transparent
            else:
                idx = group * 4 + pixel
                palette_idx = bitswap4(sprite_prom[idx] & 0x0F)
                colors.append(palette[palette_idx])
        color_str = ",".join(f"0x{c:x}" for c in colors)
        comma = "," if group < 7 else ""
        lines.append(f"{{{color_str}}}{comma}")
    lines.append("};")

    out_path = os.path.join(HDR_DST, "ladybug_cmap.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path}")


# ============================================================================
# Main
# ============================================================================

def main():
    global ROM_SRC, ROMSET

    print("=== Lady Bug ROM Converter (ladybug + ladybugb support) ===")

    # Auto-detect ROM source folder: try parent first, then bootleg
    if os.path.isdir(ROM_SRC_LADYBUG):
        ROM_SRC = ROM_SRC_LADYBUG
        ROMSET = "ladybug"
    elif os.path.isdir(ROM_SRC_LADYBUGB):
        ROM_SRC = ROM_SRC_LADYBUGB
        ROMSET = "ladybugb"
    else:
        print(f"ERROR: No ROM folder found!")
        print(f"  Tried: {ROM_SRC_LADYBUG}")
        print(f"  Tried: {ROM_SRC_LADYBUGB}")
        print(f"  Place MAME 'ladybug' or 'ladybugb' ROM files in one of these folders.")
        sys.exit(1)

    print(f"Detected romset: {ROMSET}")
    print(f"Source: {ROM_SRC}")
    print(f"CPU output: {ROM_DST}")
    print(f"Header output: {HDR_DST}")
    print()

    # List all files in source folder
    files = sorted(os.listdir(ROM_SRC))
    print(f"Files in {ROMSET}/ ({len(files)} files):")
    for f in files:
        size = os.path.getsize(os.path.join(ROM_SRC, f))
        print(f"  {f:20s} {size:6d} bytes")
    print()

    # Verify critical ROMs exist (try both naming conventions)
    all_defs = (
        CPU_DEFS +
        [GFX1_PLANE0_DEFS, GFX1_PLANE1_DEFS,
         GFX2_PLANE0_DEFS, GFX2_PLANE1_DEFS,
         PROM_PALETTE_DEFS, PROM_SPRITE_DEFS, PROM_CHAR_DEFS]
    )

    missing = []
    for defs in all_defs:
        parent_name, alt_name = defs
        if not os.path.exists(os.path.join(ROM_SRC, parent_name)):
            if not alt_name or not os.path.exists(os.path.join(ROM_SRC, alt_name)):
                missing.append(f"{parent_name} (alt: {alt_name})")

    if missing:
        print(f"ERROR: Missing ROM files ({len(missing)}):")
        for m in missing:
            print(f"  - {m}")
        sys.exit(1)

    print("All required ROMs found!")
    print()

    os.makedirs(HDR_DST, exist_ok=True)

    make_cpu_rom()
    print()
    make_tilemap()
    make_spritemap()
    make_colormap()

    print()
    print("=" * 60)
    print("Done! Copy rom1.bin to SD card at /ARCADE/LADYBUG/rom1.bin")
    print(f"Romset used: {ROMSET}")
    if ROMSET == "ladybugb":
        print("NOTE: Using ladybugb (bootleg) CPU ROMs.")
        print("GFX and PROMs are identical to parent set.")
    print("=" * 60)


if __name__ == "__main__":
    main()
