"""
Convert Gyruss arcade ROMs to galagino header files.

Uses the MAME-documented graphics and memory layout for Gyruss (Konami, 1983).

ROM set (from gyruss.zip):
  Main CPU Z80:   gyrussk.1 + gyrussk.2 + gyrussk.3  (3 x 8KB = 24KB)
  Sub CPU M6809:  gyrussk.9  (8KB, Konami-1 encrypted opcodes)
  Audio CPU Z80:  gyrussk.1a + gyrussk.2a  (2 x 8KB = 16KB)
  GFX1 (chars):   gyrussk.4  (8KB, 2bpp interleaved)
  GFX2 (sprites): gyrussk.6 + gyrussk.5 + gyrussk.8 + gyrussk.7  (4 x 8KB = 32KB, 4bpp)
  PROMs:          gyrussk.pr1 (sprite CLUT, 256B)
                  gyrussk.pr2 (char CLUT, 256B)
                  gyrussk.pr3 (color palette, 32B)

Usage:
    python convert_roms.py [path_to_gyruss.zip]
    (defaults to ../gyruss.zip)
"""

import sys, os, struct, zipfile

# ============================================================================
# Configuration
# ============================================================================

ZIP_PATH = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "gyruss.zip")
OUTPUT_DIR = os.path.dirname(__file__) or "."

EXPECTED_SIZES = {
    'gyrussk.1': 8192, 'gyrussk.2': 8192, 'gyrussk.3': 8192,
    'gyrussk.9': 8192,
    'gyrussk.1a': 8192, 'gyrussk.2a': 8192,
    'gyrussk.4': 8192,
    'gyrussk.5': 8192, 'gyrussk.6': 8192, 'gyrussk.7': 8192, 'gyrussk.8': 8192,
    'gyrussk.pr1': 256, 'gyrussk.pr2': 256, 'gyrussk.pr3': 32,
}


def load_roms():
    """Extract all ROMs from zip file."""
    print(f"Reading ROMs from: {ZIP_PATH}")
    roms = {}
    with zipfile.ZipFile(ZIP_PATH) as z:
        for name in z.namelist():
            roms[name] = z.read(name)

    for name, expected_size in EXPECTED_SIZES.items():
        if name not in roms:
            raise FileNotFoundError(f"Missing ROM: {name}")
        if len(roms[name]) != expected_size:
            raise ValueError(f"{name}: expected {expected_size} bytes, got {len(roms[name])}")
        print(f"  {name}: {len(roms[name])} bytes OK")

    return roms


# ============================================================================
# Utility: byte array to C header
# ============================================================================

def byte_array_to_c(data, name):
    """Convert byte array to C PROGMEM array."""
    lines = [f"const unsigned char {name}[] PROGMEM = {{"]
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_vals = ','.join(f'0x{b:02X}' for b in chunk)
        lines.append(f"  {hex_vals},")
    lines.append("};")
    return '\n'.join(lines)


# ============================================================================
# RGB565 conversion (same convention as ladybug/convert_roms.py)
# ============================================================================

def rgb_to_rgb565(r, g, b):
    """Convert 8-bit RGB to RGB565 with byte-swap for ESP32 little-endian TFT."""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    rgb565 = (r5 << 11) | (g6 << 5) | b5
    # Byte-swap for little-endian storage (SPI sends low byte first)
    lo = rgb565 & 0xFF
    hi = (rgb565 >> 8) & 0xFF
    return (lo << 8) | hi


# ============================================================================
# Main CPU ROM (Z80, 24KB)
# ============================================================================

def gen_rom_main(roms):
    data = roms['gyrussk.1'] + roms['gyrussk.2'] + roms['gyrussk.3']
    content = "// Gyruss main Z80 CPU ROM (24KB: 0x0000-0x5FFF)\n"
    content += "// Generated from gyrussk.1 + gyrussk.2 + gyrussk.3\n\n"
    content += byte_array_to_c(data, "gyruss_rom_main") + '\n'

    path = os.path.join(OUTPUT_DIR, "gyruss_rom_main.h")
    with open(path, 'w') as f:
        f.write(content)
    print(f"Generated {path} ({len(data)} bytes)")


# ============================================================================
# Sub CPU ROM (M6809, 8KB with Konami-1 decryption)
# ============================================================================

def konami1_decrypt(data, base_addr):
    """Konami-1 opcode decryption: swap bits based on address lines.
    - If addr bit 1 set: swap data bits 6 and 7
    - If addr bit 3 set: swap data bits 4 and 5
    """
    result = bytearray(len(data))
    for i in range(len(data)):
        addr = base_addr + i
        val = data[i]

        if addr & 0x02:
            bit6 = (val >> 6) & 1
            bit7 = (val >> 7) & 1
            val = (val & 0x3F) | (bit6 << 7) | (bit7 << 6)

        if addr & 0x08:
            bit4 = (val >> 4) & 1
            bit5 = (val >> 5) & 1
            val = (val & 0xCF) | (bit4 << 5) | (bit5 << 4)

        result[i] = val
    return bytes(result)


def gen_rom_sub(roms):
    raw = roms['gyrussk.9']
    decrypted = konami1_decrypt(raw, 0xE000)

    content = "// Gyruss M6809 sub-CPU ROM (8KB: mapped at 0xE000-0xFFFF)\n"
    content += "// Generated from gyrussk.9\n"
    content += "// Raw data for operand reads, decrypted for opcode fetches (Konami-1)\n\n"
    content += byte_array_to_c(raw, "gyruss_rom_sub_raw")
    content += "\n\n"
    content += byte_array_to_c(decrypted, "gyruss_rom_sub_decrypt") + '\n'

    path = os.path.join(OUTPUT_DIR, "gyruss_rom_sub.h")
    with open(path, 'w') as f:
        f.write(content)
    print(f"Generated {path} (2 x {len(raw)} bytes)")


# ============================================================================
# Audio CPU ROM (Z80, 16KB)
# ============================================================================

def gen_rom_audio(roms):
    data = roms['gyrussk.1a'] + roms['gyrussk.2a']
    content = "// Gyruss audio Z80 CPU ROM (16KB: 0x0000-0x3FFF)\n"
    content += "// Generated from gyrussk.1a + gyrussk.2a\n\n"
    content += byte_array_to_c(data, "gyruss_rom_audio") + '\n'

    path = os.path.join(OUTPUT_DIR, "gyruss_rom_audio.h")
    with open(path, 'w') as f:
        f.write(content)
    print(f"Generated {path} ({len(data)} bytes)")


# ============================================================================
# Palette and color lookup tables
#
# MAME Gyruss palette (from gyruss.cpp):
#   pr3 (32 bytes): master palette definition
#     - 3 bits R (resistors 1000/470/220 ohm), 3 bits G, 2 bits B (470/220 ohm)
#   pr1 (256 bytes): sprite color lookup table
#     - pen i -> pr1[i] & 0x0f -> master palette entry 0-15
#   pr2 (256 bytes): character color lookup table
#     - pen i -> (pr2[i] & 0x0f) + 16 -> master palette entry 16-31
#
# Gyruss gfxdecode:
#   Characters: palette_start = 32*4 = 128, 32 groups
#     -> pens 128-255 (within sprite CLUT pr1 range)
#   Sprites: palette_start = 0, 16 groups
#     -> pens 0-255
#
# So characters use pr1[128 + group*4 + pixel] & 0x0f -> palette[0-15]
# And sprites use pr1[group*16 + pixel] & 0x0f -> palette[0-15]
# ============================================================================

def gen_palette(roms):
    pr1 = roms['gyrussk.pr1']  # sprite/char CLUT
    pr2 = roms['gyrussk.pr2']  # (char CLUT, for pen 256+)
    pr3 = roms['gyrussk.pr3']  # master palette

    # Resistor weights (globally normalized across all channels)
    # All channels use the same normalization: 255 / Gsum_rg
    # where Gsum_rg = 1/1000 + 1/470 + 1/220 (the 3-resistor network)
    # R,G: 3 resistors (1000, 470, 220 ohm) -> weights [33, 71, 151], sum=255
    # B: 2 resistors (470, 220 ohm) -> weights [71, 151], sum=222 (global scaling)
    rw = [33, 71, 151]  # weights for 1000, 470, 220 ohm
    bw = [71, 151]       # weights for 470, 220 ohm (same scale as rw)

    # Decode 32-color master palette from pr3
    master_rgb = []
    for i in range(32):
        byte = pr3[i]
        r = ((byte >> 0) & 1) * rw[0] + ((byte >> 1) & 1) * rw[1] + ((byte >> 2) & 1) * rw[2]
        g = ((byte >> 3) & 1) * rw[0] + ((byte >> 4) & 1) * rw[1] + ((byte >> 5) & 1) * rw[2]
        b = ((byte >> 6) & 1) * bw[0] + ((byte >> 7) & 1) * bw[1]
        master_rgb.append((min(r, 255), min(g, 255), min(b, 255)))

    master_565 = [rgb_to_rgb565(r, g, b) for r, g, b in master_rgb]

    print("\n  Master palette (32 colors):")
    for i in range(32):
        r, g, b = master_rgb[i]
        print(f"    [{i:2d}] RGB({r:3d},{g:3d},{b:3d}) -> 0x{master_565[i]:04x}")

    # Sprite color lookup: 16 groups x 16 colors
    # pen = group*16 + pixel -> pr1[pen] & 0x0f -> palette[0-15]
    sprite_colormap = []
    for group in range(16):
        colors = []
        for pixel in range(16):
            pen = group * 16 + pixel
            pal_idx = pr1[pen] & 0x0f
            r, g, b = master_rgb[pal_idx]
            colors.append(rgb_to_rgb565(r, g, b))
        sprite_colormap.append(colors)

    # Character color lookup: 16 groups x 4 colors
    # Characters use pr2 CLUT, mapping to upper 16 palette entries (16-31)
    # pen i -> (pr2[i] & 0x0f) + 16 -> master palette entry 16-31
    char_colormap = []
    for group in range(16):
        colors = []
        for pixel in range(4):
            pen = group * 4 + pixel
            pal_idx = (pr2[pen] & 0x0f) + 16
            r, g, b = master_rgb[pal_idx]
            colors.append(rgb_to_rgb565(r, g, b))
        char_colormap.append(colors)

    print("\n  Char colormap (from pr2, palette 16-31):")
    for g in range(16):
        vals = [f"0x{v:04x}" for v in char_colormap[g]]
        print(f"    group {g:2d}: {vals}")

    # Write palette header
    lines = []
    lines.append("// Gyruss color data")
    lines.append("// Generated from gyrussk.pr1 + gyrussk.pr2 + gyrussk.pr3")
    lines.append("")
    lines.append("// Master palette (32 entries, RGB565 byte-swapped)")
    lines.append("const unsigned short gyruss_palette[] PROGMEM = {")
    pal_str = ','.join(f'0x{v:04x}' for v in master_565)
    lines.append(f"  {pal_str}")
    lines.append("};")
    lines.append("")

    # Sprite colormap
    lines.append("// Sprite color lookup (16 groups x 16 colors, RGB565)")
    lines.append("const unsigned short gyruss_sprite_colormap[][16] PROGMEM = {")
    for group in range(16):
        colors_str = ','.join(f'0x{v:04x}' for v in sprite_colormap[group])
        lines.append(f"  {{ {colors_str} }},")
    lines.append("};")
    lines.append("")

    # Char colormap - using pr2 (upper 16 master palette entries)
    lines.append("// Character color lookup (16 groups x 4 colors, RGB565)")
    lines.append("// Using pr2 CLUT -> master palette entries 16-31")
    lines.append("const unsigned short gyruss_char_colormap[][4] PROGMEM = {")
    for group in range(16):
        colors_str = ','.join(f'0x{v:04x}' for v in char_colormap[group])
        lines.append(f"  {{ {colors_str} }},")
    lines.append("};")

    path = os.path.join(OUTPUT_DIR, "gyruss_palette.h")
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print(f"\nGenerated {path}")

    return master_rgb, master_565


# ============================================================================
# Character tiles (512 tiles, 8x8, 2bpp interleaved)
#
# MAME Gyruss charlayout:
#   planes: { 4, 0 }  (MSB-first: plane[0]=4 is MSB, plane[1]=0 is LSB)
#   xoffset: { 0,1,2,3, 64,65,66,67 }  (left 4 px from byte N, right 4 from byte N+8)
#   yoffset: { 0*8, 1*8, ..., 7*8 }
#   charsize: 16*8 = 16 bytes per tile
#
# MAME bit convention: bit N in byte B is tested as B & (0x80 >> N)
# So bit 0 = MSB (0x80), bit 7 = LSB (0x01)
# ============================================================================

def gen_tilemap(roms):
    rom = roms['gyrussk.4']
    tiles = []

    for tile_idx in range(512):
        base = tile_idx * 16
        rows = []
        for row in range(8):
            row_val = 0
            for col in range(8):
                if col < 4:
                    byte = rom[base + row]
                    mame_bit = col
                else:
                    byte = rom[base + 8 + row]
                    mame_bit = col - 4

                # MAME readbit: byte & (0x80 >> mame_bit)
                # planes {4, 0}: MSB plane at bit offset +4, LSB plane at +0
                p_lsb = 1 if (byte & (0x80 >> mame_bit)) else 0
                p_msb = 1 if (byte & (0x80 >> (mame_bit + 4))) else 0
                pixel = (p_msb << 1) | p_lsb
                row_val |= (pixel << (col * 2))
            rows.append(row_val)
        tiles.append(rows)

    lines = []
    lines.append("// Gyruss character tiles (512 tiles, 8x8, 2bpp)")
    lines.append("// Generated from gyrussk.4")
    lines.append("")
    lines.append("const unsigned short gyruss_tilemap[][8] PROGMEM = {")
    for t in range(512):
        row_str = ','.join(f'0x{v:04x}' for v in tiles[t])
        lines.append(f"  {{ {row_str} }},")
    lines.append("};")

    path = os.path.join(OUTPUT_DIR, "gyruss_tilemap.h")
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print(f"Generated {path} (512 tiles)")


# ============================================================================
# Sprites (512 sprites: 256 per bank, 8x16, 4bpp, 4 flip variants)
#
# MAME Gyruss spritelayout:
#   8x16 pixels, 256 sprites per bank, 4bpp
#   planes: { 0x4000*8+4, 0x4000*8+0, 4, 0 }  (MSB-first)
#   MAME bit convention: bit N = byte & (0x80 >> N)
#   xoffset: { 0,1,2,3, 64,65,66,67 }
#   yoffset: { 0*8,...,7*8, 32*8,...,39*8 }
#   charsize: 64*8 = 64 bytes per sprite
#
# GFXDECODE has two entries:
#   Bank 0 at offset 0x0000: "upper half" sprites
#   Bank 1 at offset 0x0010: "lower half" sprites (+16 bytes)
# gfx_bank = code_byte & 1 selects which bank
#
# ROM layout:
#   Low half  (0x0000-0x3FFF): gyrussk.6 + gyrussk.5 (planes 0,1)
#   High half (0x4000-0x7FFF): gyrussk.8 + gyrussk.7 (planes 2,3)
# ============================================================================

def decode_sprite(rom_low, rom_high, code, bank_offset):
    """Decode one 8x16 sprite from ROM data."""
    spr_base = code * 64 + bank_offset
    rows = []
    for row in range(16):
        if row < 8:
            row_off = row
        else:
            row_off = 24 + row  # rows 8-15 at bytes 32-39

        row_val = 0
        for col in range(8):
            if col < 4:
                byte_off = row_off
                mame_bit = col
            else:
                byte_off = row_off + 8
                mame_bit = col - 4

            addr = spr_base + byte_off
            low_byte = rom_low[addr]
            high_byte = rom_high[addr]

            p0 = 1 if (low_byte & (0x80 >> mame_bit)) else 0
            p1 = 1 if (low_byte & (0x80 >> (mame_bit + 4))) else 0
            p2 = 1 if (high_byte & (0x80 >> mame_bit)) else 0
            p3 = 1 if (high_byte & (0x80 >> (mame_bit + 4))) else 0

            pixel = p0 | (p1 << 1) | (p2 << 2) | (p3 << 3)
            row_val |= (pixel << (col * 4))
        rows.append(row_val)
    return rows


def gen_spritemap(roms):
    rom_low  = roms['gyrussk.6'] + roms['gyrussk.5']   # planes 0,1
    rom_high = roms['gyrussk.8'] + roms['gyrussk.7']   # planes 2,3

    # Decode 512 sprites: 0-255 = bank 0, 256-511 = bank 1
    sprites_normal = []
    for bank in range(2):
        bank_offset = bank * 16  # 0x0000 or 0x0010
        for code in range(256):
            sprites_normal.append(decode_sprite(rom_low, rom_high, code, bank_offset))

    # Generate 4 flip variants
    def flip_row_x(row_val):
        """Reverse 8 nibbles (4-bit pixels) in a 32-bit value."""
        result = 0
        for i in range(8):
            nibble = (row_val >> (i * 4)) & 0xf
            result |= nibble << ((7 - i) * 4)
        return result

    variants = [None] * 4
    variants[0] = sprites_normal
    variants[1] = [list(reversed(s)) for s in sprites_normal]
    variants[2] = [[flip_row_x(r) for r in s] for s in sprites_normal]
    variants[3] = [[flip_row_x(r) for r in reversed(s)] for s in sprites_normal]

    lines = []
    lines.append("// Gyruss sprites (512 sprites: bank0 0-255 + bank1 256-511)")
    lines.append("// 8x16, 4bpp, 4 flip variants")
    lines.append("// Generated from gyrussk.6 + gyrussk.5 + gyrussk.8 + gyrussk.7")
    lines.append("// Bank 0 = GFXDECODE offset 0x0000, Bank 1 = offset 0x0010")
    lines.append("// Variant 0=normal, 1=Y-flip, 2=X-flip, 3=XY-flip")
    lines.append("")
    lines.append("const unsigned long gyruss_sprites[][512][16] PROGMEM = {")

    for v in range(4):
        lines.append("  {")
        for idx in range(512):
            row_str = ','.join(f'0x{r:08x}' for r in variants[v][idx])
            lines.append(f"    {{ {row_str} }},")
        lines.append("  },")
    lines.append("};")

    path = os.path.join(OUTPUT_DIR, "gyruss_spritemap.h")
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print(f"Generated {path} (512 sprites x 4 variants)")


# ============================================================================
# Main
# ============================================================================

if __name__ == "__main__":
    roms = load_roms()

    print("\n--- Generating CPU ROMs ---")
    gen_rom_main(roms)
    gen_rom_sub(roms)
    gen_rom_audio(roms)

    print("\n--- Generating palette ---")
    gen_palette(roms)

    print("\n--- Generating character tiles ---")
    gen_tilemap(roms)

    print("\n--- Generating sprites ---")
    gen_spritemap(roms)

    print("\nAll files generated!")
