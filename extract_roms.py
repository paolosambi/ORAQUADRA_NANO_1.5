#!/usr/bin/env python3
"""
Extract ROM data from galag/galagino .h files and create ARCADE_ROMS.zip
with the folder structure expected by oraQuadra Nano SD card.

Searches for ROM data in multiple paths (galag project first, then galagino).
"""

import re
import os
import zipfile

# Search paths for ROM source files (first found wins)
ROM_SEARCH_PATHS = [
    r"C:\Users\Paolo\Desktop\galag\machines",
    r"C:\Users\Paolo\Desktop\PROGETTO_ORAQUADRA_NANO\SURVIVAL_HACKING\galagino-main\galagino-main\galagino_arduino\machines",
    r"C:\Users\Paolo\Desktop\PROGETTO_ORAQUADRA_NANO\SURVIVAL_HACKING\galagino_arduino_BT\machines",
]

# Mapping: (subfolder, h_file) -> (ARCADE_folder, bin_name)
ROM_MAP = [
    ("pacman",    "pacman_rom.h",     "PACMAN",    "rom1.bin"),
    ("galaga",    "galaga_rom1.h",    "GALAGA",    "rom1.bin"),
    ("galaga",    "galaga_rom2.h",    "GALAGA",    "rom2.bin"),
    ("galaga",    "galaga_rom3.h",    "GALAGA",    "rom3.bin"),
    ("dkong",     "dkong_rom1.h",     "DKONG",     "rom1.bin"),
    ("dkong",     "dkong_rom2.h",     "DKONG",     "rom2.bin"),
    ("frogger",   "frogger_rom1.h",   "FROGGER",   "rom1.bin"),
    ("frogger",   "frogger_rom2.h",   "FROGGER",   "rom2.bin"),
    ("digdug",    "digdug_rom1.h",    "DIGDUG",    "rom1.bin"),
    ("digdug",    "digdug_rom2.h",    "DIGDUG",    "rom2.bin"),
    ("digdug",    "digdug_rom3.h",    "DIGDUG",    "rom3.bin"),
    ("1942",      "1942_rom1.h",      "1942",      "rom1.bin"),
    ("1942",      "1942_rom2.h",      "1942",      "rom2.bin"),
    ("1942",      "1942_rom1_b0.h",   "1942",      "rom1_b0.bin"),
    ("1942",      "1942_rom1_b1.h",   "1942",      "rom1_b1.bin"),
    ("1942",      "1942_rom1_b2.h",   "1942",      "rom1_b2.bin"),
    ("eyes",      "eyes_rom.h",       "EYES",      "rom1.bin"),
    ("mrtnt",     "mrtnt_rom.h",      "MRTNT",     "rom1.bin"),
    ("lizwiz",    "lizwiz_rom.h",     "LIZWIZ",    "rom1.bin"),
    ("theglob",   "theglob_rom.h",    "THEGLOB",   "rom1.bin"),
    ("crush",     "crush_rom.h",      "CRUSH",     "rom1.bin"),
    ("anteater",  "anteater_rom1.h",  "ANTEATER",  "rom1.bin"),
    ("anteater",  "anteater_rom2.h",  "ANTEATER",  "rom2.bin"),
    ("ladybug",   "ladybug_rom.h",    "LADYBUG",   "rom1.bin"),
    ("gyruss",    "gyruss_rom_main.h",  "GYRUSS",  "rom1.bin"),
    ("gyruss",    "gyruss_rom_sub.h",   "GYRUSS",  "rom2.bin"),
    ("gyruss",    "gyruss_rom_audio.h",  "GYRUSS", "rom3.bin"),
]

def extract_hex_bytes(filepath):
    """Extract all 0xNN hex values from a C header file."""
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    hex_values = re.findall(r'0x([0-9A-Fa-f]{2})', content)
    return bytes(int(h, 16) for h in hex_values)

def find_rom_file(subfolder, h_file):
    """Search for a ROM header file across all search paths."""
    for base in ROM_SEARCH_PATHS:
        path = os.path.join(base, subfolder, h_file)
        if os.path.exists(path):
            return path
    return None

def main():
    project_dir = r"C:\Users\Paolo\Desktop\PROGETTO_ORAQUADRA_NANO\SURVIVAL_HACKING\oraQuadra_Nano_Ver_1.5"
    zip_path = os.path.join(project_dir, "ARCADE_ROMS.zip")

    print("=== oraQuadra Nano - ROM Extractor ===")
    print()
    print("Search paths:")
    for p in ROM_SEARCH_PATHS:
        exists = os.path.isdir(p)
        print(f"  {'OK' if exists else 'N/A'}: {p}")
    print()
    print(f"Output: {zip_path}")
    print()

    total_files = 0
    total_bytes = 0
    missing = []

    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
        for (subfolder, h_file, arcade_folder, bin_name) in ROM_MAP:
            h_path = find_rom_file(subfolder, h_file)

            if not h_path:
                print(f"  SKIP: {subfolder}/{h_file} not found in any path")
                missing.append(f"{arcade_folder}/{bin_name}")
                continue

            data = extract_hex_bytes(h_path)

            if len(data) == 0:
                print(f"  SKIP: {h_file} - no data (stub file?)")
                missing.append(f"{arcade_folder}/{bin_name}")
                continue

            # Min size check: ROM stubs are 1 byte, real ROMs are 4KB+
            if len(data) < 100:
                print(f"  SKIP: {h_file} - only {len(data)} bytes (stub)")
                missing.append(f"{arcade_folder}/{bin_name}")
                continue

            arc_path = f"{arcade_folder}/{bin_name}"
            zf.writestr(arc_path, data)
            total_files += 1
            total_bytes += len(data)
            print(f"  OK: {h_file} -> {arc_path} ({len(data)} bytes)")

    print()
    print(f"Done! {total_files} ROM files, {total_bytes} bytes total")
    if missing:
        print(f"\nMISSING ({len(missing)}):")
        for m in missing:
            print(f"  - {m}")
    print(f"\nZIP saved: {zip_path}")
    print()
    print("Per usare le ROM:")
    print("1. Estrai ARCADE_ROMS.zip nella root della SD card")
    print("   Struttura: SD:/ARCADE/PACMAN/rom1.bin, SD:/ARCADE/GALAGA/rom1.bin, etc.")
    print("2. OPPURE carica i file .bin dalla pagina web /arcade (ROM Manager)")

if __name__ == "__main__":
    main()
