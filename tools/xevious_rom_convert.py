#!/usr/bin/env python3
"""
Xevious (Namco, 1982) ROM Converter for oraQuadra Nano Arcade Emulator

Supports BOTH the "xevious" parent MAME romset and the "xevios" bootleg romset.
Auto-detects which set is present based on folder name and file availability.

Reads raw MAME ROM dumps from ARCADE_ROMS_SD/xevious/ (or xevios/) and generates:
  1. rom1.bin, rom2.bin, rom3.bin  - Merged CPU ROMs for SD card
  2. xevious_tilemap.h             - 512 FG characters (8x8, 1bpp expanded to 2bpp)
  3. xevious_bgtiles.h             - 512 BG tiles (8x8, 2bpp)
  4. xevious_spritemap.h           - Sprites (16x16, 2bpp, 4 flip variants)
  5. xevious_cmap.h                - Color palettes (RGB565)
  6. xevious_wavetable.h           - Namco WSG waveforms
  7. xevious_bgmap.h               - Background map PROM data
  8. xevious_scrolltable.h         - Scroll lookup PROMs

Usage:
  python tools/xevious_rom_convert.py
  (run from project root)

MAME xevios ROM mapping (bootleg variant):
  CPU1: 4.7h, 5.6h, xvi_3.2m(=6.5h), w7.4h(=7.4h)  instead of xvi_1..4
  Sprites: 16.8d instead of xvi_18.4r (xe-4r.bin)
  BGmap: 10.1d instead of xvi_9.2a, 12.3d instead of xvi_11.2c
  All other ROMs (GFX, PROMs, wave, scroll) are shared with parent.
"""

import os
import sys

# Paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
ROM_DST = os.path.join(PROJECT_ROOT, "ARCADE_ROMS_SD", "ARCADE", "XEVIOUS")
HDR_DST = os.path.join(PROJECT_ROOT, "arcade", "machines", "xevious")

# Auto-detect ROM source folder: try "xevious" first, then "xevios"
ROM_SRC_XEVIOUS = os.path.join(PROJECT_ROOT, "ARCADE_ROMS_SD", "xevious")
ROM_SRC_XEVIOS = os.path.join(PROJECT_ROOT, "ARCADE_ROMS_SD", "xevios")
ROM_SRC = None  # set in main()
ROMSET = None   # "xevious" or "xevios"

# === ROM file definitions with xevios alternatives ===
# Format: { "role": (xevious_name, xevios_alternative_name) }
# If xevios_alternative is None, the file is shared (same name in both sets)

# CPU1: 4 x 4KB = 16KB
CPU1_DEFS = [
    ("xvi_1.3p", "4.7h"),      # bank 0: 0x0000-0x0FFF
    ("xvi_2.3m", "5.6h"),      # bank 1: 0x1000-0x1FFF
    ("xvi_3.2m", "xvi_3.2m"),  # bank 2: 0x2000-0x2FFF (shared, also called 6.5h)
    ("xvi_4.2l", "w7.4h"),     # bank 3: 0x3000-0x3FFF (xevios: 7.4h or w7.4h)
]

# CPU2: 2 x 4KB = 8KB (shared between parent and bootleg)
CPU2_DEFS = [
    ("xvi_5.3f", "xvi_5.3f"),
    ("xvi_6.3j", "xvi_6.3j"),
]

# CPU3: 1 x 4KB = 4KB (shared)
CPU3_DEFS = [
    ("xvi_7.2c", "xvi_7.2c"),
]

# GFX ROMs (shared between parent and bootleg)
FG_DEFS = ("xvi_12.3b", "xvi_12.3b")
BG_PLANE0_DEFS = ("xvi_13.3c", "xvi_13.3c")
BG_PLANE1_DEFS = ("xvi_14.3d", "xvi_14.3d")

# Sprites (plane0_A and plane0_B shared, plane1_A shared, plane1_B differs)
SPR_P0_A_DEFS = ("xvi_15.4m", "xvi_15.4m")
SPR_P0_B_DEFS = ("xvi_16.4n", "xvi_16.4n")
SPR_P1_A_DEFS = ("xvi_17.4p", "xvi_17.4p")
SPR_P1_B_DEFS = ("xvi_18.4r", "16.8d")  # xevios uses 16.8d

# Color PROMs (all shared)
PROM_PALETTE_DEFS = ("xvi-8.6a", "xvi-8.6a")
PROM_FG_CLR_DEFS = ("xvi-9.6d", "xvi-9.6d")
PROM_SPR_CLR_DEFS = ("xvi-10.6e", "xvi-10.6e")
PROM_BG_CLR1_DEFS = ("xvi-1.5n", "xvi-1.5n")
PROM_BG_CLR2_DEFS = ("xvi-2.7n", "xvi-2.7n")

# Audio waveform ROMs (shared)
WAVE_ROM1_DEFS = ("xvi-4.3l", "xvi-4.3l")
WAVE_ROM2_DEFS = ("xvi-5.3m", "xvi-5.3m")

# Background map PROMs (ROM1 and ROM3 differ in xevios)
BGMAP_ROM1_DEFS = ("xvi_9.2a", "10.1d")   # xevios uses 10.1d
BGMAP_ROM2_DEFS = ("xvi_10.2b", "xvi_10.2b")  # shared
BGMAP_ROM3_DEFS = ("xvi_11.2c", "12.3d")   # xevios uses 12.3d

# Scroll lookup PROMs (shared)
SCROLL_X_DEFS = ("xvi-6.4f", "xvi-6.4f")
SCROLL_Y_DEFS = ("xvi-7.4h", "xvi-7.4h")


def resolve_rom(defs):
    """Resolve a ROM definition tuple to the actual filename.
    defs = (xevious_name, xevios_name)
    Returns the filename that exists in ROM_SRC.
    """
    xevious_name, xevios_name = defs
    # Try primary name first
    if os.path.exists(os.path.join(ROM_SRC, xevious_name)):
        return xevious_name
    # Try alternative name
    if xevios_name and os.path.exists(os.path.join(ROM_SRC, xevios_name)):
        return xevios_name
    # Return primary name (will fail in read_rom with clear error)
    return xevious_name


def read_rom(filename):
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


# === Rotation and flip helpers (same as ladybug_rom_convert.py) ===

def rotate_tile_270(rows_16bit):
    """Rotate 8x8 tile 270 CW for portrait display."""
    rotated = []
    for out_row in range(8):
        val = 0
        for out_col in range(8):
            pixel = (rows_16bit[out_col] >> ((7 - out_row) * 2)) & 3
            val |= pixel << (out_col * 2)
        rotated.append(val)
    return rotated


def rotate_sprite_270(rows, bpp=3):
    """Rotate 16x16 sprite 270 CW for portrait display."""
    mask = (1 << bpp) - 1  # 7 for 3bpp, 3 for 2bpp
    rotated = []
    for out_row in range(16):
        val = 0
        for out_col in range(16):
            pixel = (rows[out_col] >> ((15 - out_row) * bpp)) & mask
            val |= pixel << (out_col * bpp)
        rotated.append(val)
    return rotated


def flip_sprite_h(rows, bpp=3):
    """Flip 16x16 sprite horizontally."""
    mask = (1 << bpp) - 1
    flipped = []
    for val in rows:
        new_val = 0
        for px in range(16):
            color = (val >> (px * bpp)) & mask
            new_val |= (color << ((15 - px) * bpp))
        flipped.append(new_val)
    return flipped


def flip_sprite_v(rows):
    """Flip 16x16 sprite vertically."""
    return list(reversed(rows))


# === Namco resistor-weighted RGB decode ===

def namco_prom_to_rgb565(byte_val):
    """
    Decode a Xevious/Namco color PROM byte to RGB565.
    Namco standard resistor DAC:
      Bits 0-2: Red   (3 bits: 1K, 470, 220 ohm)
      Bits 3-5: Green (3 bits: 1K, 470, 220 ohm)
      Bits 6-7: Blue  (2 bits: 470, 220 ohm)

    Resistor weights (standard Namco):
      1K ohm   -> weight ~34  (bit 0 for R/G)
      470 ohm  -> weight ~71  (bit 1 for R/G, bit 0 for B)
      220 ohm  -> weight ~150 (bit 2 for R/G, bit 1 for B)
    """
    r_bit0 = (byte_val >> 0) & 1  # 1K   -> 34
    r_bit1 = (byte_val >> 1) & 1  # 470  -> 71
    r_bit2 = (byte_val >> 2) & 1  # 220  -> 150

    g_bit0 = (byte_val >> 3) & 1  # 1K   -> 34
    g_bit1 = (byte_val >> 4) & 1  # 470  -> 71
    g_bit2 = (byte_val >> 5) & 1  # 220  -> 150

    b_bit0 = (byte_val >> 6) & 1  # 470  -> 71
    b_bit1 = (byte_val >> 7) & 1  # 220  -> 150

    r8 = r_bit0 * 34 + r_bit1 * 71 + r_bit2 * 150
    g8 = g_bit0 * 34 + g_bit1 * 71 + g_bit2 * 150
    b8 = b_bit0 * 71 + b_bit1 * 150

    # Clamp to 255
    r8 = min(r8, 255)
    g8 = min(g8, 255)
    b8 = min(b8, 255)

    r5 = (r8 >> 3) & 0x1F
    g6 = (g8 >> 2) & 0x3F
    b5 = (b8 >> 3) & 0x1F

    rgb565 = (r5 << 11) | (g6 << 5) | b5
    # Byte-swap to big-endian (pre-swapped for SPI displays, matching other games)
    return ((rgb565 >> 8) & 0xFF) | ((rgb565 & 0xFF) << 8)


# === CPU ROM merging ===

def make_cpu_roms():
    """Merge CPU ROMs into rom1.bin (16KB), rom2.bin (8KB), rom3.bin (4KB)"""
    os.makedirs(ROM_DST, exist_ok=True)

    # CPU1: 4 x 4KB = 16KB
    print("CPU1 ROMs (16KB):")
    rom1 = bytearray()
    for xevious_name, xevios_name in CPU1_DEFS:
        name = resolve_rom((xevious_name, xevios_name))
        data = read_rom(name)
        if len(data) != 4096:
            print(f"  WARNING: {name} is {len(data)} bytes, expected 4096!")
        rom1.extend(data[:4096])  # truncate if larger
    out1 = os.path.join(ROM_DST, "rom1.bin")
    with open(out1, "wb") as f:
        f.write(rom1)
    print(f"  -> Created {out1} ({len(rom1)} bytes)")

    # CPU2: 2 x 4KB = 8KB
    print("CPU2 ROMs (8KB):")
    rom2 = bytearray()
    for xevious_name, xevios_name in CPU2_DEFS:
        name = resolve_rom((xevious_name, xevios_name))
        data = read_rom(name)
        rom2.extend(data)
    out2 = os.path.join(ROM_DST, "rom2.bin")
    with open(out2, "wb") as f:
        f.write(rom2)
    print(f"  -> Created {out2} ({len(rom2)} bytes)")

    # CPU3: 1 x 4KB
    print("CPU3 ROM (4KB):")
    name = resolve_rom(CPU3_DEFS[0])
    rom3 = read_rom(name)
    out3 = os.path.join(ROM_DST, "rom3.bin")
    with open(out3, "wb") as f:
        f.write(rom3)
    print(f"  -> Created {out3} ({len(rom3)} bytes)")


# === FG Characters (1bpp expanded to 2bpp) ===

def decode_fg_char(data, char_index):
    """
    Decode one 8x8 FG character from Xevious GFX ROM.
    MAME xoffset={0,1,2,3,4,5,6,7}: bit0=leftmost pixel (LSB-first).
    We expand to 2bpp: 0=transparent, 3=colored (matching engine format).
    Returns 8 rows of 16-bit packed 2bpp values.
    """
    rows = []
    base = char_index * 8

    for row in range(8):
        b = data[base + row]
        val = 0
        for px in range(8):
            bit = (b >> px) & 1  # LSB-first (MAME xoffset={0,1,...,7})
            color = 3 if bit else 0  # 1bpp: 0=transparent, 3=opaque
            val |= (color << (px * 2))
        rows.append(val)

    return rows


def make_tilemap():
    """Generate xevious_tilemap.h with 512 FG characters (1bpp expanded to 2bpp)"""
    data = read_rom_resolved(FG_DEFS)
    num_chars = len(data) // 8  # 512

    lines = []
    lines.append("// Xevious FG character tiles - 512 chars, 8x8, 1bpp expanded to 2bpp")
    lines.append("// 0=transparent, 3=colored (single color per char from attr)")
    lines.append("// Generated by xevious_rom_convert.py")
    lines.append(f"const unsigned short xevious_tilemap[{num_chars}][8] = {{")

    for ch in range(num_chars):
        rows = decode_fg_char(data, ch)
        rows = rotate_tile_270(rows)
        row_str = ",".join(f"0x{r:x}" for r in rows)
        comma = "," if ch < num_chars - 1 else ""
        lines.append(f" {{ {row_str} }}{comma}")

    lines.append("};")

    out_path = os.path.join(HDR_DST, "xevious_tilemap.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path} ({num_chars} FG characters)")


# === BG Tiles (2bpp standard Namco) ===

def decode_bg_tile(plane0, plane1, tile_index):
    """
    Decode one 8x8 BG tile from Xevious GFX ROMs.
    MAME xoffset={0,1,...,7}: LSB-first. plane0=bit0, plane1=bit1.
    8 bytes per tile per plane.
    Returns 8 rows of 16-bit packed 2bpp values.
    """
    rows = []
    base = tile_index * 8

    for row in range(8):
        b0 = plane0[base + row]
        b1 = plane1[base + row]
        val = 0
        for px in range(8):
            bit0 = (b0 >> px) & 1  # LSB-first
            bit1 = (b1 >> px) & 1
            color = bit0 | (bit1 << 1)
            val |= (color << (px * 2))
        rows.append(val)

    return rows


def make_bgtiles():
    """Generate xevious_bgtiles.h with 512 BG tiles (2bpp)"""
    p0 = read_rom_resolved(BG_PLANE0_DEFS)
    p1 = read_rom_resolved(BG_PLANE1_DEFS)
    num_tiles = min(len(p0), len(p1)) // 8  # 512

    lines = []
    lines.append("// Xevious BG tiles - 512 tiles, 8x8, 2bpp")
    lines.append("// Generated by xevious_rom_convert.py")
    lines.append(f"const unsigned short xevious_bgtiles[{num_tiles}][8] = {{")

    for t in range(num_tiles):
        rows = decode_bg_tile(p0, p1, t)
        rows = rotate_tile_270(rows)
        row_str = ",".join(f"0x{r:x}" for r in rows)
        comma = "," if t < num_tiles - 1 else ""
        lines.append(f" {{ {row_str} }}{comma}")

    lines.append("};")

    out_path = os.path.join(HDR_DST, "xevious_bgtiles.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path} ({num_tiles} BG tiles)")


# === Sprites (16x16, 3bpp - MAME spritelayout_xevious) ===

def build_sprite_region():
    """
    Build the gfx3 region (0xa000 bytes) matching MAME's ROM layout for Xevious.

    ROM_REGION( 0xa000, "gfx3", 0 )
    ROM_LOAD( "xvi_15.4m", 0x0000, 0x2000 )  // Set #1, planes 0/1
    ROM_LOAD( "xvi_17.4p", 0x2000, 0x2000 )  // Set #2, planes 0/1
    ROM_LOAD( "xvi_16.4n", 0x4000, 0x1000 )  // Set #3, planes 0/1
    ROM_LOAD( "xvi_18.4r", 0x5000, 0x2000 )  // Plane 2 packed (xevios: 16.8d)
    ROM_FILL(              0x9000, 0x1000 )   // Set #3 plane 2 = zeros

    init_xevious(): rom[0x7000+i] = rom[0x5000+i] >> 4  (unpack plane 2 high nibbles)
    """
    region = bytearray(0xa000)

    rom_15 = bytearray(read_rom_resolved(SPR_P0_A_DEFS))  # xvi_15.4m -> 0x0000
    rom_17 = bytearray(read_rom_resolved(SPR_P1_A_DEFS))  # xvi_17.4p -> 0x2000
    rom_16 = bytearray(read_rom_resolved(SPR_P0_B_DEFS))  # xvi_16.4n -> 0x4000
    rom_18 = bytearray(read_rom_resolved(SPR_P1_B_DEFS))  # xvi_18.4r/16.8d -> 0x5000

    region[0x0000:0x0000+len(rom_15)] = rom_15
    region[0x2000:0x2000+len(rom_17)] = rom_17
    region[0x4000:0x4000+len(rom_16)] = rom_16
    region[0x5000:0x5000+len(rom_18)] = rom_18

    # init_xevious() unpack: copy plane 2 data with >> 4
    for i in range(0x2000):
        region[0x7000 + i] = region[0x5000 + i] >> 4

    print(f"  Built gfx3 region: 0x{len(region):04x} bytes")
    return region


def decode_sprite_3bpp(region, sprite_index):
    """
    Decode one 16x16 3bpp sprite from the gfx3 region.

    MAME spritelayout_xevious:
      planeoffset = { RGN_FRAC(1,2)+4, 0, 4 }
        plane[0] = MSB (weight 4): second half, high nibble
        plane[1] = MID (weight 2): first half, low nibble
        plane[2] = LSB (weight 1): first half, high nibble
      xoffset = 4 groups of 4: {0,1,2,3}, {64,65,66,67}, {128,...}, {192,...}
      yoffset = {0,8,...,56, 256,264,...,312}
      charincrement = 512 bits = 64 bytes

    Returns 16 rows of packed 3bpp values (48 bits per row, LSB-first).
    """
    half_bits = 0x5000 * 8   # RGN_FRAC(1,2) in bits
    sprite_bits = sprite_index * 512  # charincrement

    yoffsets = [y * 8 for y in range(8)] + [256 + y * 8 for y in range(8)]

    rows = []
    for y in range(16):
        val = 0
        y_off = yoffsets[y]
        px_idx = 0

        for col_group in range(4):
            x_base = col_group * 64
            for px in range(4):
                bit_off = y_off + x_base + px

                # Plane 1 (MID, weight 2): planeoffset = 0
                p1_bit = sprite_bits + bit_off
                mid = (region[p1_bit // 8] >> (p1_bit % 8)) & 1

                # Plane 2 (LSB, weight 1): planeoffset = 4
                p2_bit = 4 + sprite_bits + bit_off
                lsb = (region[p2_bit // 8] >> (p2_bit % 8)) & 1

                # Plane 0 (MSB, weight 4): planeoffset = RGN_FRAC(1,2) + 4
                p0_bit = half_bits + 4 + sprite_bits + bit_off
                msb = (region[p0_bit // 8] >> (p0_bit % 8)) & 1

                pixel = (msb << 2) | (mid << 1) | lsb
                val |= (pixel << (px_idx * 3))
                px_idx += 1

        rows.append(val)
    return rows


def make_spritemap():
    """Generate xevious_spritemap.h with 3bpp sprites x 4 flip variants"""
    region = build_sprite_region()

    # Total sprites from first half: 0x5000 / 64 bytes = 320
    # Pad to 324 for safety (multi-tile sprites can offset code by +3)
    num_decoded = 0x5000 // 64  # 320 actual sprites
    num_sprites = 324  # padded for multi-tile sprite code offsets
    print(f"  Total sprites: {num_decoded} decoded + {num_sprites - num_decoded} padding = {num_sprites}")

    lines = []
    lines.append("// Xevious sprites - 16x16, 3bpp packed (MAME spritelayout_xevious)")
    lines.append("// 4 flip variants: [0]=normal, [1]=flipX, [2]=flipY, [3]=flipXY")
    lines.append("// Each row: 16 pixels x 3 bits = 48 bits, stored in unsigned long long")
    lines.append("// Pixel 0 at bits 0-2 (LSB), pixel 15 at bits 45-47")
    lines.append(f"// {num_decoded} actual sprites + {num_sprites - num_decoded} padding")
    lines.append("// Generated by xevious_rom_convert.py")
    lines.append(f"const unsigned long long xevious_sprites[4][{num_sprites}][16] = {{")

    for flip in range(4):
        lines.append(" {")
        for sp in range(num_sprites):
            if sp < num_decoded:
                rows = decode_sprite_3bpp(region, sp)
            else:
                rows = [0] * 16  # padding: empty sprite

            if flip == 1:
                rows = flip_sprite_h(rows, bpp=3)
            elif flip == 2:
                rows = flip_sprite_v(rows)
            elif flip == 3:
                rows = flip_sprite_h(flip_sprite_v(rows), bpp=3)

            rows = rotate_sprite_270(rows, bpp=3)

            row_str = ",".join(f"0x{r:x}" for r in rows)
            comma = "," if sp < num_sprites - 1 else ""
            lines.append(f"  {{ {row_str} }}{comma}")

        comma = "," if flip < 3 else ""
        lines.append(f" }}{comma}")

    lines.append("};")

    out_path = os.path.join(HDR_DST, "xevious_spritemap.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path} ({num_sprites} sprites x 4 flips, 3bpp)")


# === Color Palettes ===

def make_colormap():
    """
    Generate xevious_cmap.h with all color palettes.

    Xevious color system:
    - xvi-8.6a (256B): Master RGB palette PROM -> 256 RGB565 colors
    - xvi-9.6d (256B): FG char color lookup -> maps (char_color*4 + pixel) -> palette index
    - xvi-10.6e (256B): Sprite color lookup -> maps (sprite_color*4 + pixel) -> palette index
    - xvi-1.5n + xvi-2.7n (256+256B): BG color lookup -> maps (bg_color*4 + pixel) -> palette index
    """
    palette_prom = read_rom_resolved(PROM_PALETTE_DEFS)
    fg_clr_prom = read_rom_resolved(PROM_FG_CLR_DEFS)
    spr_clr_prom = read_rom_resolved(PROM_SPR_CLR_DEFS)
    bg_clr1_prom = read_rom_resolved(PROM_BG_CLR1_DEFS)
    bg_clr2_prom = read_rom_resolved(PROM_BG_CLR2_DEFS)

    # Decode master palette
    palette = [namco_prom_to_rgb565(palette_prom[i]) for i in range(256)]

    lines = []
    lines.append("// Xevious color palettes - RGB565")
    lines.append("// Generated by xevious_rom_convert.py")
    lines.append("")

    # Master palette (for reference/direct use)
    lines.append(f"const unsigned short xevious_palette[256] = {{")
    for i in range(0, 256, 16):
        chunk = palette[i:i+16]
        lines.append("  " + ",".join(f"0x{c:04x}" for c in chunk) + ",")
    lines.append("};")
    lines.append("")

    # FG character colors: 64 groups x 4 colors
    num_fg_groups = 256 // 4
    lines.append(f"// FG char colors: {num_fg_groups} groups x 4 colors")
    lines.append(f"// For 1bpp chars: only [0]=transparent and [3]=color are meaningful")
    lines.append(f"const unsigned short xevious_cmap_fg[{num_fg_groups}][4] = {{")
    for group in range(num_fg_groups):
        colors = []
        for pixel in range(4):
            idx = group * 4 + pixel
            if idx < len(fg_clr_prom):
                pal_idx = fg_clr_prom[idx] & 0xFF
                if pixel == 0:
                    colors.append(0)  # transparent
                else:
                    colors.append(palette[pal_idx] if pal_idx < 256 else 0)
            else:
                colors.append(0)
        color_str = ",".join(f"0x{c:04x}" for c in colors)
        comma = "," if group < num_fg_groups - 1 else ""
        lines.append(f"  {{{color_str}}}{comma}")
    lines.append("};")
    lines.append("")

    # Sprite colors: 64 groups x 8 colors (3bpp)
    num_spr_groups = 512 // 8  # 64 groups, 8 entries each
    lines.append(f"// Sprite colors: {num_spr_groups} groups x 8 colors (3bpp)")
    lines.append(f"const unsigned short xevious_cmap_sprites[{num_spr_groups}][8] = {{")
    for group in range(num_spr_groups):
        colors = []
        for pixel in range(8):
            idx = group * 8 + pixel
            if idx < len(spr_clr_prom):
                pal_idx = spr_clr_prom[idx] & 0xFF
                if pixel == 0:
                    colors.append(0)  # transparent
                else:
                    colors.append(palette[pal_idx] if pal_idx < 256 else 0)
            else:
                colors.append(0)
        color_str = ",".join(f"0x{c:04x}" for c in colors)
        comma = "," if group < num_spr_groups - 1 else ""
        lines.append(f"  {{{color_str}}}{comma}")
    lines.append("};")
    lines.append("")

    # BG tile colors: 128 groups x 4 colors (from two 256-byte PROMs)
    total_bg_entries = len(bg_clr1_prom) + len(bg_clr2_prom)
    num_bg_groups = total_bg_entries // 4
    bg_combined = bytearray(bg_clr1_prom) + bytearray(bg_clr2_prom)

    lines.append(f"// BG tile colors: {num_bg_groups} groups x 4 colors")
    lines.append(f"const unsigned short xevious_cmap_bg[{num_bg_groups}][4] = {{")
    for group in range(num_bg_groups):
        colors = []
        for pixel in range(4):
            idx = group * 4 + pixel
            if idx < len(bg_combined):
                pal_idx = bg_combined[idx] & 0xFF
                if pixel == 0:
                    colors.append(0)  # transparent
                else:
                    colors.append(palette[pal_idx] if pal_idx < 256 else 0)
            else:
                colors.append(0)
        color_str = ",".join(f"0x{c:04x}" for c in colors)
        comma = "," if group < num_bg_groups - 1 else ""
        lines.append(f"  {{{color_str}}}{comma}")
    lines.append("};")

    out_path = os.path.join(HDR_DST, "xevious_cmap.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path}")


# === Waveforms ===

def make_wavetable():
    """Generate xevious_wavetable.h with Namco WSG waveforms"""
    wave1 = read_rom_resolved(WAVE_ROM1_DEFS)
    wave2 = read_rom_resolved(WAVE_ROM2_DEFS)
    wave_data = bytearray(wave1) + bytearray(wave2)

    num_waves = len(wave_data) // 32

    lines = []
    lines.append("// Xevious Namco WSG waveforms - 32 samples per waveform, signed 4-bit")
    lines.append("// Generated by xevious_rom_convert.py")
    lines.append(f"const signed char xevious_wavetable[{num_waves}][32] = {{")

    for w in range(num_waves):
        samples = []
        for s in range(32):
            raw = wave_data[w * 32 + s] & 0x0F
            if raw >= 8:
                signed_val = raw - 16
            else:
                signed_val = raw
            samples.append(signed_val)
        sample_str = ",".join(f"{s:d}" for s in samples)
        comma = "," if w < num_waves - 1 else ""
        lines.append(f" {{ {sample_str} }}{comma}")

    lines.append("};")

    out_path = os.path.join(HDR_DST, "xevious_wavetable.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path} ({num_waves} waveforms)")


# === Background Map PROM ===

def make_bgmap():
    """Generate xevious_bgmap.h from background map PROMs (16KB total)"""
    data1 = read_rom_resolved(BGMAP_ROM1_DEFS)
    data2 = read_rom_resolved(BGMAP_ROM2_DEFS)
    data3 = read_rom_resolved(BGMAP_ROM3_DEFS)
    bgmap = bytearray(data1) + bytearray(data2) + bytearray(data3)

    lines = []
    lines.append("// Xevious background map PROM data")
    lines.append(f"// ROM1 ({len(data1)}B) + ROM2 ({len(data2)}B) + ROM3 ({len(data3)}B) = {len(bgmap)}B")
    lines.append("// Generated by xevious_rom_convert.py")
    lines.append(f"const unsigned char xevious_bgmap[{len(bgmap)}] PROGMEM = {{")

    for i in range(0, len(bgmap), 16):
        chunk = bgmap[i:i+16]
        hex_str = ",".join(f"0x{b:02x}" for b in chunk)
        comma = "," if i + 16 < len(bgmap) else ""
        lines.append(f"  {hex_str}{comma}")

    lines.append("};")

    out_path = os.path.join(HDR_DST, "xevious_bgmap.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path} ({len(bgmap)} bytes)")


# === Scroll Lookup Tables ===

def make_scrolltable():
    """Generate xevious_scrolltable.h from scroll PROMs"""
    scroll_x = read_rom_resolved(SCROLL_X_DEFS)
    scroll_y = read_rom_resolved(SCROLL_Y_DEFS)

    lines = []
    lines.append("// Xevious scroll lookup PROMs")
    lines.append("// Generated by xevious_rom_convert.py")
    lines.append("")

    lines.append(f"const unsigned char xevious_scroll_x[{len(scroll_x)}] PROGMEM = {{")
    for i in range(0, len(scroll_x), 16):
        chunk = scroll_x[i:i+16]
        hex_str = ",".join(f"0x{b:02x}" for b in chunk)
        comma = "," if i + 16 < len(scroll_x) else ""
        lines.append(f"  {hex_str}{comma}")
    lines.append("};")
    lines.append("")

    lines.append(f"const unsigned char xevious_scroll_y[{len(scroll_y)}] PROGMEM = {{")
    for i in range(0, len(scroll_y), 16):
        chunk = scroll_y[i:i+16]
        hex_str = ",".join(f"0x{b:02x}" for b in chunk)
        comma = "," if i + 16 < len(scroll_y) else ""
        lines.append(f"  {hex_str}{comma}")
    lines.append("};")

    out_path = os.path.join(HDR_DST, "xevious_scrolltable.h")
    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Created {out_path}")


# === Main ===

def main():
    global ROM_SRC, ROMSET

    print("=== Xevious ROM Converter (xevious + xevios support) ===")

    # Auto-detect ROM source folder
    if os.path.isdir(ROM_SRC_XEVIOUS):
        ROM_SRC = ROM_SRC_XEVIOUS
        ROMSET = "xevious"
    elif os.path.isdir(ROM_SRC_XEVIOS):
        ROM_SRC = ROM_SRC_XEVIOS
        ROMSET = "xevios"
    else:
        print(f"ERROR: No ROM folder found!")
        print(f"  Tried: {ROM_SRC_XEVIOUS}")
        print(f"  Tried: {ROM_SRC_XEVIOS}")
        print(f"  Place MAME 'xevious' or 'xevios' ROM files in one of these folders.")
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
        CPU1_DEFS + CPU2_DEFS + CPU3_DEFS +
        [FG_DEFS, BG_PLANE0_DEFS, BG_PLANE1_DEFS,
         SPR_P0_A_DEFS, SPR_P0_B_DEFS, SPR_P1_A_DEFS, SPR_P1_B_DEFS,
         PROM_PALETTE_DEFS, PROM_FG_CLR_DEFS, PROM_SPR_CLR_DEFS,
         PROM_BG_CLR1_DEFS, PROM_BG_CLR2_DEFS,
         WAVE_ROM1_DEFS, WAVE_ROM2_DEFS,
         BGMAP_ROM1_DEFS, BGMAP_ROM2_DEFS, BGMAP_ROM3_DEFS,
         SCROLL_X_DEFS, SCROLL_Y_DEFS]
    )

    missing = []
    for defs in all_defs:
        xev_name, alt_name = defs
        if not os.path.exists(os.path.join(ROM_SRC, xev_name)):
            if not alt_name or not os.path.exists(os.path.join(ROM_SRC, alt_name)):
                missing.append(f"{xev_name} (alt: {alt_name})")

    if missing:
        print(f"ERROR: Missing ROM files ({len(missing)}):")
        for m in missing:
            print(f"  - {m}")
        sys.exit(1)

    print("All required ROMs found!")
    print()

    os.makedirs(HDR_DST, exist_ok=True)

    make_cpu_roms()
    print()
    make_tilemap()
    make_bgtiles()
    make_spritemap()
    make_colormap()
    make_wavetable()
    make_bgmap()
    make_scrolltable()

    print()
    print("=" * 60)
    print("Done! Copy rom1.bin, rom2.bin, rom3.bin to SD card at /ARCADE/XEVIOUS/")
    print(f"Romset used: {ROMSET}")
    if ROMSET == "xevios":
        print("NOTE: Using xevios (bootleg) ROMs. The emulator's ROM test")
        print("bypass ensures compatibility with bootleg CPU code.")
    print("=" * 60)


if __name__ == "__main__":
    main()
