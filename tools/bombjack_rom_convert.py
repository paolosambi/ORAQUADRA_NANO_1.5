#!/usr/bin/env python3
"""
Bomb Jack ROM Converter for oraQuadra Nano Arcade Emulator

Converts original Bomb Jack (1984 Tehkan) ROM files to the format needed by the emulator.

ROM Files (from MAME bombjack set):
  01_h03t.bin  - Audio CPU Z80 (8KB)  [not used - audio CPU not emulated]
  02_p04t.bin  - BG map data (4KB)    -> bombjack_bgmap.h
  03_e08t.bin  - FG tile bitplane 0 (4KB)  |
  04_h08t.bin  - FG tile bitplane 1 (4KB)  |-> bombjack_tilemap.h
  05_k08t.bin  - FG tile bitplane 2 (4KB)  |
  06_l08t.bin  - BG tile bitplane 0 (8KB)  |
  07_n08t.bin  - BG tile bitplane 1 (8KB)  |-> bombjack_bgtiles.h
  08_r08t.bin  - BG tile bitplane 2 (8KB)  |
  09_j01b.bin  - Main CPU ROM bank 0 (8KB) |
  10_l01b.bin  - Main CPU ROM bank 1 (8KB) |
  11_m01b.bin  - Main CPU ROM bank 2 (8KB) |-> rom1.bin (40KB for SD card)
  12_n01b.bin  - Main CPU ROM bank 3 (8KB) |
  13.1r        - Main CPU ROM bank 4 (8KB) |
  14_j07b.bin  - Sprite bitplane 0 (8KB)   |
  15_l07b.bin  - Sprite bitplane 1 (8KB)   |-> bombjack_spritemap.h
  16_m07b.bin  - Sprite bitplane 2 (8KB)   |

Graphics format: 3 bitplanes per pixel (8 colors per palette group)
  Packed into unsigned long (32-bit): 8 pixels x 4 bits each
  Bits 31-28 = pixel 0 (left), bits 3-0 = pixel 7 (right)
  Pixel value 0-7 (3 bitplanes), upper bit always 0

  MAME plane convention: PlaneOffset = { 0, RGN_FRAC(1,3), RGN_FRAC(2,3) }
  In gfx_element::decode(): planebit = 1 << (planes - 1 - plane)
  So plane 0 (first ROM) = bit 2 (MSB), plane 2 (third ROM) = bit 0 (LSB)

Usage:
  python bombjack_rom_convert.py [rom_dir] [output_dir] [sd_output_dir]
"""

import os
import sys
import struct

# --- Configuration ---

ROM_FILES = {
    'audio':   '01_h03t.bin',
    'bgmap':   '02_p04t.bin',
    'fg_bp0':  '03_e08t.bin',
    'fg_bp1':  '04_h08t.bin',
    'fg_bp2':  '05_k08t.bin',
    'bg_bp0':  '06_l08t.bin',
    'bg_bp1':  '07_n08t.bin',
    'bg_bp2':  '08_r08t.bin',
    'cpu_0':   '09_j01b.bin',
    'cpu_1':   '10_l01b.bin',
    'cpu_2':   '11_m01b.bin',
    'cpu_3':   '12_n01b.bin',
    'cpu_4':   '13.1r',
    'spr_bp0': '14_j07b.bin',  # MAME RGN_FRAC(0,3) = plane 0
    'spr_bp1': '15_l07b.bin',  # MAME RGN_FRAC(1,3) = plane 1
    'spr_bp2': '16_m07b.bin',  # MAME RGN_FRAC(2,3) = plane 2
}

EXPECTED_SIZES = {
    'audio': 8192, 'bgmap': 4096,
    'fg_bp0': 4096, 'fg_bp1': 4096, 'fg_bp2': 4096,
    'bg_bp0': 8192, 'bg_bp1': 8192, 'bg_bp2': 8192,
    'cpu_0': 8192, 'cpu_1': 8192, 'cpu_2': 8192, 'cpu_3': 8192, 'cpu_4': 8192,
    'spr_bp2': 8192, 'spr_bp1': 8192, 'spr_bp0': 8192,
}


def read_rom(rom_dir, key):
    """Read a ROM file and verify its size."""
    path = os.path.join(rom_dir, ROM_FILES[key])
    if not os.path.exists(path):
        print(f"  ERROR: {ROM_FILES[key]} not found!")
        return None
    data = open(path, 'rb').read()
    expected = EXPECTED_SIZES[key]
    if len(data) != expected:
        print(f"  WARNING: {ROM_FILES[key]} is {len(data)} bytes (expected {expected})")
    return data


def decode_8x8_tiles(bp0, bp1, bp2):
    """
    Decode 8x8 foreground tiles from 3 bitplane ROMs.
    MAME layout_8x8:
      PlaneOffset: { 0, RGN_FRAC(1,3), RGN_FRAC(2,3) }
      XOffset: { 0,1,2,3,4,5,6,7 }
      YOffset: { 0,8,16,24,32,40,48,56 }
      CharIncrement: 64 bits (8 bytes per tile per plane)

    Returns list of tiles, each tile = list of 8 unsigned longs (one per row).
    """
    num_tiles = len(bp0) // 8  # 512 tiles (4096 / 8)
    tiles = []
    for t in range(num_tiles):
        tile = []
        for y in range(8):
            offset = t * 8 + y
            b0 = bp0[offset]
            b1 = bp1[offset]
            b2 = bp2[offset]
            row_val = 0
            for x in range(8):
                bit = 7 - x
                # MAME: plane 0 → MSB (bit 2), plane 1 → bit 1, plane 2 → LSB (bit 0)
                px = ((b0 >> bit) & 1) << 2 | ((b1 >> bit) & 1) << 1 | ((b2 >> bit) & 1)
                row_val |= (px << (28 - x * 4))
            tile.append(row_val)
        tiles.append(tile)
    return tiles


def decode_16x16_tiles(bp0, bp1, bp2):
    """
    Decode 16x16 tiles from 3 bitplane ROMs.
    MAME layout_16x16:
      PlaneOffset: { 0, RGN_FRAC(1,3), RGN_FRAC(2,3) }
      XOffset: { 0,1,2,3,4,5,6,7, 64,65,66,67,68,69,70,71 }
      YOffset: { 0,8,16,24,32,40,48,56, 128,136,144,152,160,168,176,184 }
      CharIncrement: 256 bits (32 bytes per tile per plane)

    Quadrant layout within 32 bytes:
      Bytes 0-7:   top-left 8x8 (rows 0-7, pixels 0-7)
      Bytes 8-15:  top-right 8x8 (rows 0-7, pixels 8-15)
      Bytes 16-23: bottom-left 8x8 (rows 8-15, pixels 0-7)
      Bytes 24-31: bottom-right 8x8 (rows 8-15, pixels 8-15)

    Returns list of tiles, each tile = list of 32 unsigned longs
    (16 rows x 2 longs per row: [left_half, right_half]).
    """
    num_tiles = len(bp0) // 32
    tiles = []
    for t in range(num_tiles):
        tile = []
        base = t * 32
        for y in range(16):
            if y < 8:
                byte_left = base + y
                byte_right = base + y + 8
            else:
                byte_left = base + 16 + (y - 8)
                byte_right = base + 24 + (y - 8)

            # Left half (pixels 0-7)
            b0 = bp0[byte_left]
            b1 = bp1[byte_left]
            b2 = bp2[byte_left]
            left_val = 0
            for x in range(8):
                bit = 7 - x
                # MAME: plane 0 → MSB (bit 2), plane 1 → bit 1, plane 2 → LSB (bit 0)
                px = ((b0 >> bit) & 1) << 2 | ((b1 >> bit) & 1) << 1 | ((b2 >> bit) & 1)
                left_val |= (px << (28 - x * 4))

            # Right half (pixels 8-15)
            b0 = bp0[byte_right]
            b1 = bp1[byte_right]
            b2 = bp2[byte_right]
            right_val = 0
            for x in range(8):
                bit = 7 - x
                # MAME: plane 0 → MSB (bit 2), plane 1 → bit 1, plane 2 → LSB (bit 0)
                px = ((b0 >> bit) & 1) << 2 | ((b1 >> bit) & 1) << 1 | ((b2 >> bit) & 1)
                right_val |= (px << (28 - x * 4))

            tile.append(left_val)
            tile.append(right_val)
        tiles.append(tile)
    return tiles


def write_fg_tiles(tiles, output_path):
    """Write foreground tiles as C header: bombjack_fgtiles[512][8] PROGMEM."""
    with open(output_path, 'w') as f:
        f.write("// Bomb Jack foreground tiles (8x8, 3bpp packed as 4bpp)\n")
        f.write("// Generated by bombjack_rom_convert.py\n")
        f.write("// 512 tiles x 8 rows, each row = unsigned long (8 pixels x 4 bits)\n\n")
        f.write(f"#define FGTILES_ARR bombjack_fgtiles\n\n")
        f.write(f"const unsigned long bombjack_fgtiles[{len(tiles)}][8] PROGMEM = {{\n")
        for i, tile in enumerate(tiles):
            f.write("  {")
            for j, row in enumerate(tile):
                f.write(f"0x{row:08X}")
                if j < 7:
                    f.write(",")
            f.write("}")
            if i < len(tiles) - 1:
                f.write(",")
            if (i + 1) % 4 == 0:
                f.write(f"  // tiles {i-3}-{i}")
            f.write("\n")
        f.write("};\n")
    print(f"  FG tiles: {len(tiles)} tiles -> {output_path}")


def write_16x16_tiles(tiles, array_name, macro_name, description, output_path):
    """Write 16x16 tiles as C header: name[N][32] PROGMEM."""
    with open(output_path, 'w') as f:
        f.write(f"// Bomb Jack {description} (16x16, 3bpp packed as 4bpp)\n")
        f.write("// Generated by bombjack_rom_convert.py\n")
        f.write(f"// {len(tiles)} tiles x 32 longs (16 rows x 2 longs/row)\n\n")
        f.write(f"#define {macro_name} {array_name}\n\n")
        f.write(f"const unsigned long {array_name}[{len(tiles)}][32] PROGMEM = {{\n")
        for i, tile in enumerate(tiles):
            f.write("  {\n    ")
            for j, val in enumerate(tile):
                f.write(f"0x{val:08X}")
                if j < 31:
                    f.write(",")
                if (j + 1) % 8 == 0 and j < 31:
                    f.write("\n    ")
            f.write("\n  }")
            if i < len(tiles) - 1:
                f.write(",")
            f.write(f"  // tile {i}\n")
        f.write("};\n")
    print(f"  {description}: {len(tiles)} tiles -> {output_path}")


def write_bgmap(data, output_path):
    """Write background map ROM as raw byte array."""
    with open(output_path, 'w') as f:
        f.write("// Bomb Jack background map data\n")
        f.write("// Generated by bombjack_rom_convert.py\n")
        f.write(f"// {len(data)} bytes - 5 backgrounds x 512 bytes each\n")
        f.write("// Layout: bgmap[bg_image * 0x200 + tile_index] = tile code\n")
        f.write("//         bgmap[bg_image * 0x200 + 0x100 + tile_index] = attribute\n\n")
        f.write(f"#define BGMAP_ARR bombjack_bgmap\n\n")
        f.write(f"const unsigned char bombjack_bgmap[{len(data)}] PROGMEM = {{\n")
        for i in range(0, len(data), 16):
            f.write("  ")
            chunk = data[i:i+16]
            for j, b in enumerate(chunk):
                f.write(f"0x{b:02X}")
                if i + j < len(data) - 1:
                    f.write(",")
            f.write("\n")
        f.write("};\n")
    print(f"  BG map: {len(data)} bytes -> {output_path}")


def write_cpu_rom(rom_data, output_path):
    """Write concatenated CPU ROM binary for SD card."""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(rom_data)
    print(f"  CPU ROM: {len(rom_data)} bytes -> {output_path}")


def main():
    # Determine paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)

    rom_dir = os.path.join(project_dir, "ARCADE_ROMS_SD", "bombjack")
    output_dir = os.path.join(project_dir, "arcade", "machines", "bombjack")
    sd_output_dir = os.path.join(project_dir, "ARCADE_ROMS_SD", "bombjack")

    if len(sys.argv) > 1:
        rom_dir = sys.argv[1]
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
    if len(sys.argv) > 3:
        sd_output_dir = sys.argv[3]

    print(f"Bomb Jack ROM Converter")
    print(f"  ROM dir:    {rom_dir}")
    print(f"  Output dir: {output_dir}")
    print(f"  SD dir:     {sd_output_dir}")
    print()

    os.makedirs(output_dir, exist_ok=True)

    # --- Read ROM files ---
    print("Reading ROM files...")
    roms = {}
    missing = False
    for key in ROM_FILES:
        data = read_rom(rom_dir, key)
        if data is None:
            missing = True
        else:
            roms[key] = data
            print(f"  {ROM_FILES[key]:20s} {len(data):6d} bytes  OK")

    if missing:
        print("\nERROR: Some ROM files are missing!")
        sys.exit(1)

    # --- Generate FG tiles (8x8) ---
    print("\nDecoding FG tiles (8x8, 512 tiles)...")
    fg_tiles = decode_8x8_tiles(roms['fg_bp0'], roms['fg_bp1'], roms['fg_bp2'])
    write_fg_tiles(fg_tiles, os.path.join(output_dir, "bombjack_tilemap.h"))

    # --- Generate BG tiles (16x16) ---
    print("Decoding BG tiles (16x16, 256 tiles)...")
    bg_tiles = decode_16x16_tiles(roms['bg_bp0'], roms['bg_bp1'], roms['bg_bp2'])
    write_16x16_tiles(bg_tiles, "bombjack_bgtiles", "BGTILES_ARR",
                      "background tiles", os.path.join(output_dir, "bombjack_bgtiles.h"))

    # --- Generate sprites (16x16) ---
    print("Decoding sprites (16x16, 256 sprites)...")
    spr_tiles = decode_16x16_tiles(roms['spr_bp0'], roms['spr_bp1'], roms['spr_bp2'])
    write_16x16_tiles(spr_tiles, "bombjack_sprites", "SPRITES_ARR",
                      "sprites", os.path.join(output_dir, "bombjack_spritemap.h"))

    # --- Generate BG map ---
    print("Writing BG map...")
    write_bgmap(roms['bgmap'], os.path.join(output_dir, "bombjack_bgmap.h"))

    # --- Generate CPU ROM for SD card ---
    # Main CPU: 09+10+11+12 at 0x0000-0x7FFF, 13 at 0xC000-0xDFFF
    # Store as: 32KB (banks 0-3) + 8KB (bank 4) = 40KB total
    print("\nBuilding CPU ROM for SD card...")
    cpu_rom = bytearray()
    cpu_rom += roms['cpu_0']  # 0x0000-0x1FFF
    cpu_rom += roms['cpu_1']  # 0x2000-0x3FFF
    cpu_rom += roms['cpu_2']  # 0x4000-0x5FFF
    cpu_rom += roms['cpu_3']  # 0x6000-0x7FFF
    cpu_rom += roms['cpu_4']  # maps to 0xC000-0xDFFF (stored at offset 0x8000 in file)

    rom1_path = os.path.join(sd_output_dir, "rom1.bin")
    write_cpu_rom(cpu_rom, rom1_path)

    # --- Verify ROM contents ---
    print("\nVerification:")
    # Check reset vector (first bytes of CPU ROM bank 0)
    rst = roms['cpu_0'][:4]
    print(f"  CPU ROM reset vector: {rst[0]:02X} {rst[1]:02X} {rst[2]:02X} {rst[3]:02X}", end="")
    if rst[0] == 0xC3:
        addr = rst[1] | (rst[2] << 8)
        print(f"  -> JP 0x{addr:04X}")
    elif rst[0] == 0x31:
        sp = rst[1] | (rst[2] << 8)
        print(f"  -> LD SP, 0x{sp:04X}")
    else:
        print(f"  (opcode 0x{rst[0]:02X})")

    # Count non-zero tiles
    nz_fg = sum(1 for t in fg_tiles if any(r != 0 for r in t))
    nz_bg = sum(1 for t in bg_tiles if any(r != 0 for r in t))
    nz_spr = sum(1 for t in spr_tiles if any(r != 0 for r in t))
    print(f"  Non-zero FG tiles: {nz_fg}/{len(fg_tiles)}")
    print(f"  Non-zero BG tiles: {nz_bg}/{len(bg_tiles)}")
    print(f"  Non-zero sprites:  {nz_spr}/{len(spr_tiles)}")

    print(f"\nDone! Files generated:")
    print(f"  {os.path.join(output_dir, 'bombjack_tilemap.h')}")
    print(f"  {os.path.join(output_dir, 'bombjack_bgtiles.h')}")
    print(f"  {os.path.join(output_dir, 'bombjack_spritemap.h')}")
    print(f"  {os.path.join(output_dir, 'bombjack_bgmap.h')}")
    print(f"  {rom1_path}")
    print(f"\nCopy rom1.bin to /ARCADE/BOMBJACK/ on SD card")


if __name__ == '__main__':
    main()
