// ============================================================================
// BOMB JACK (1984 Tehkan) - Arcade Machine Emulation Engine
// Dual Z80: main CPU @ 4MHz + sound CPU @ 3MHz
// Sound CPU: 3x AY-3-8910 PSG, IRQ on VBLANK, reads commands from sound latch
// 3 rendering layers: BG (16x16 tiles) + FG (8x8 tiles) + Sprites (16x16/32x32)
// RAM-based palette: 128 entries x RGB444
// ============================================================================

#include "../../arcade_undef.h"

#define ROM_CPU1_ARR      bombjack_rom_cpu1
#define FGTILES_ARR       bombjack_fgtiles
#define BGTILES_ARR       bombjack_bgtiles
#define SPRITES_ARR       bombjack_sprites
#define BGMAP_ARR         bombjack_bgmap
#define LOGO_ARR          bombjack_logo

// ===== Memory Map =====
// Main CPU (cpu[0]):
// Z80 address      -> memory[] offset   Description
// 0x0000-0x7FFF    -> pRom1[0-0x7FFF]   32KB main ROM
// 0x8000-0x8FFF    -> memory[0x0000]     4KB RAM
// 0x9000-0x93FF    -> memory[0x1000]     1KB Video RAM (tile codes)
// 0x9400-0x97FF    -> memory[0x1400]     1KB Color RAM (tile attributes)
// 0x9800-0x987F    -> memory[0x1800]     Sprite RAM (24 sprites x 4 bytes)
// 0x9A00-0x9A01    -> memory[0x1A00]     Sprite control
// 0x9C00-0x9CFF    -> memory[0x1C00]     Palette RAM (128 x 2 bytes)
// 0x9E00           -> memory[0x1E00]     BG image select
// 0xB000-0xB007    -> I/O (inputs, NMI, watchdog, DIP, flip)
// 0xB800           -> sound latch
// 0xC000-0xDFFF    -> pRom1[0x8000]      8KB additional ROM
//
// Sound CPU (cpu[1]):
// 0x0000-0x1FFF    -> pRom2[0-0x1FFF]   8KB sound ROM
// 0x4000-0x43FF    -> memory[0x2000]     1KB sound RAM
// 0x6000           -> sound_latch        Command from main CPU (read-only)

#define MEM_OFFSET  0x8000  // Z80 address 0x8000 -> memory[0]
#define VRAM_OFF    0x1000  // Video RAM at memory[0x1000]
#define CRAM_OFF    0x1400  // Color RAM at memory[0x1400]
#define SPRAM_OFF   0x1800  // Sprite RAM at memory[0x1800]
#define PALRAM_OFF  0x1C00  // Palette RAM at memory[0x1C00]
#define BGSEL_OFF   0x1E00  // BG select at memory[0x1E00]

// ===== Reset =====
void bombjack::reset() {
	machineBase::reset();

	nmi_enable = 0;
	bg_image = 0;
	flip_screen = 0;
	sound_latch = 0;
	watchdog_counter = 0;
	bj_frame_count = 0;
	bj_in_vblank = false;
	snd_ay_port[0] = snd_ay_port[1] = snd_ay_port[2] = 0;
	snd_icnt = 0;
	snd_nmi_ready = true;

	memset(palette_cache, 0, sizeof(palette_cache));

	// Copy tile data from flash PROGMEM to PSRAM.
	// On ESP32-S3, flash and PSRAM share the same SPI bus.
	// Heavy flash reads during rendering (both cores accessing flash)
	// starve LCD_CAM GDMA reads from PSRAM → display underrun → panic.
	buildTileCache();
}

// ===== PSRAM Tile Cache =====
void bombjack::buildTileCache() {
	freeTileCache();

	// BG tiles: 256 tiles × 32 unsigned longs = 32768 bytes
	pBgTiles = (unsigned long*)ps_malloc(256 * 32 * sizeof(unsigned long));
	if (pBgTiles) memcpy_P(pBgTiles, BGTILES_ARR, 256 * 32 * sizeof(unsigned long));

	// BG map: 4096 bytes
	pBgMap = (unsigned char*)ps_malloc(4096);
	if (pBgMap) memcpy_P(pBgMap, BGMAP_ARR, 4096);

	// FG tiles: 512 tiles × 8 unsigned longs = 16384 bytes
	pFgTiles = (unsigned long*)ps_malloc(512 * 8 * sizeof(unsigned long));
	if (pFgTiles) memcpy_P(pFgTiles, FGTILES_ARR, 512 * 8 * sizeof(unsigned long));

	// Sprites: 256 tiles × 32 unsigned longs = 32768 bytes
	pSprites = (unsigned long*)ps_malloc(256 * 32 * sizeof(unsigned long));
	if (pSprites) memcpy_P(pSprites, SPRITES_ARR, 256 * 32 * sizeof(unsigned long));
}

void bombjack::freeTileCache() {
	if (pBgTiles) { free(pBgTiles); pBgTiles = nullptr; }
	if (pBgMap) { free(pBgMap); pBgMap = nullptr; }
	if (pFgTiles) { free(pFgTiles); pFgTiles = nullptr; }
	if (pSprites) { free(pSprites); pSprites = nullptr; }
}

// ===== Palette Conversion: RGB444 -> RGB565-BE =====
void bombjack::updatePaletteCache(int index) {
	// Palette RAM layout: 2 bytes per entry (MAME xBGR_444 format)
	// Byte 0 (even): GGGGRRRR (green 4 bits high nibble, red 4 bits low nibble)
	// Byte 1 (odd):  xxxxBBBB (blue 4 bits in low nibble)
	int off = PALRAM_OFF + index * 2;
	unsigned char byte0 = memory[off];
	unsigned char byte1 = memory[off + 1];

	unsigned char r4 = byte0 & 0x0F;
	unsigned char g4 = (byte0 >> 4) & 0x0F;
	unsigned char b4 = byte1 & 0x0F;

	// Expand 4-bit to 5/6-bit channels
	unsigned char r5 = (r4 << 1) | (r4 >> 3);
	unsigned char g6 = (g4 << 2) | (g4 >> 2);
	unsigned char b5 = (b4 << 1) | (b4 >> 3);

	unsigned short rgb565 = (r5 << 11) | (g6 << 5) | b5;

	// Store as big-endian (display pipeline expects this, upscaler does bswap16)
	palette_cache[index] = ((rgb565 & 0xFF) << 8) | ((rgb565 >> 8) & 0xFF);
}

void bombjack::updateFullPaletteCache() {
	for (int i = 0; i < 128; i++)
		updatePaletteCache(i);
}

// ===== Z80 Memory Read =====
unsigned char bombjack::rdZ80(unsigned short Addr) {
	// ---- Main CPU (cpu[0]) ----
	if (current_cpu == 0) {
		// ROM: 0x0000-0x7FFF (32KB)
		if (Addr < 0x8000)
			return pRom1[Addr];

		// RAM + VRAM + CRAM + sprites + palette: 0x8000-0x9FFF
		if (Addr < 0xA000)
			return memory[Addr - MEM_OFFSET];

		// I/O: 0xB000-0xB007 (mirrored every 0x800)
		if ((Addr & 0xF800) == 0xB000) {
			switch (Addr & 0x07) {
				case 0: {
					unsigned char val = 0;
					unsigned char btn = input->buttons_get();
					if (btn & BUTTON_RIGHT) val |= 0x01;
					if (btn & BUTTON_LEFT)  val |= 0x02;
					if (btn & BUTTON_UP)    val |= 0x04;
					if (btn & BUTTON_DOWN)  val |= 0x08;
					if (btn & BUTTON_FIRE)  val |= 0x10;
					return val;
				}
				case 1:
					return 0x00;
				case 2: {
					unsigned char val = 0xF0;
					unsigned char btn = input->buttons_get();
					if (btn & BUTTON_COIN)  val |= 0x01;
					if (btn & BUTTON_START) val |= 0x04;
					return val;
				}
				case 3:
					watchdog_counter = 0;
					return 0x00;
				case 4:
					return BJ_DIPA;
				case 5: {
					unsigned char val = BJ_DIPB;
					if (!bj_in_vblank) val |= 0x80;
					return val;
				}
				default:
					return 0xFF;
			}
		}

		// Additional ROM: 0xC000-0xDFFF (8KB)
		if (Addr >= 0xC000 && Addr <= 0xDFFF)
			return pRom1[0x8000 + (Addr - 0xC000)];

		return 0xFF;
	}

	// ---- Sound CPU (cpu[1]) ----
	// 0x0000-0x1FFF: 8KB sound ROM
	if (Addr < 0x2000)
		return pRom2[Addr];

	// 0x4000-0x43FF: 1KB sound RAM (mapped to memory[0x2000])
	if ((Addr & 0xFC00) == 0x4000)
		return memory[0x2000 + (Addr - 0x4000)];

	// 0x6000: sound latch (command from main CPU)
	// Hardware: read clears latch to 0 AND deactivates NMI (re-arms for next VBLANK)
	if ((Addr & 0xE000) == 0x6000) {
		unsigned char val = sound_latch;
		sound_latch = 0;
		snd_nmi_ready = true;  // NMI pin goes HIGH → ready for next falling edge
		return val;
	}

	return 0xFF;
}

// ===== Z80 Memory Write =====
void bombjack::wrZ80(unsigned short Addr, unsigned char Value) {
	// ---- Main CPU (cpu[0]) ----
	if (current_cpu == 0) {
		// RAM: 0x8000-0x8FFF
		if (Addr >= 0x8000 && Addr <= 0x8FFF) {
			memory[Addr - MEM_OFFSET] = Value;
			return;
		}

		// Video RAM: 0x9000-0x93FF
		if (Addr >= 0x9000 && Addr <= 0x93FF) {
			memory[Addr - MEM_OFFSET] = Value;
			return;
		}

		// Color RAM: 0x9400-0x97FF
		if (Addr >= 0x9400 && Addr <= 0x97FF) {
			memory[Addr - MEM_OFFSET] = Value;
			return;
		}

		// Sprite RAM: 0x9800-0x987F (mirrored at +0x0180)
		if (Addr >= 0x9800 && Addr <= 0x987F) {
			memory[Addr - MEM_OFFSET] = Value;
			return;
		}
		if (Addr >= 0x9980 && Addr <= 0x99FF) {
			memory[Addr - 0x180 - MEM_OFFSET] = Value;
			return;
		}

		// Sprite control: 0x9A00-0x9A01
		if (Addr >= 0x9A00 && Addr <= 0x9A01) {
			memory[Addr - MEM_OFFSET] = Value;
			return;
		}

		// Palette RAM: 0x9C00-0x9CFF (mirrored at +0x0100)
		if (Addr >= 0x9C00 && Addr <= 0x9CFF) {
			memory[Addr - MEM_OFFSET] = Value;
			int palIndex = (Addr - 0x9C00) >> 1;
			if (palIndex < 128)
				updatePaletteCache(palIndex);
			return;
		}
		if (Addr >= 0x9D00 && Addr <= 0x9DFF) {
			memory[(Addr - 0x100) - MEM_OFFSET] = Value;
			int palIndex = (Addr - 0x9D00) >> 1;
			if (palIndex < 128)
				updatePaletteCache(palIndex);
			return;
		}

		// BG image select: 0x9E00 (mirrored at +0x01FF)
		if ((Addr & 0xFE00) == 0x9E00) {
			bg_image = Value;
			memory[BGSEL_OFF] = Value;
			return;
		}

		// NMI enable: 0xB000
		if ((Addr & 0xF800) == 0xB000) {
			switch (Addr & 0x07) {
				case 0:
					nmi_enable = Value & 1;
					return;
				case 3:
					watchdog_counter = 0;
					return;
				case 4:
					flip_screen = Value & 1;
					return;
			}
			return;
		}

		// Sound latch: 0xB800 — main CPU writes command for sound CPU
		if ((Addr & 0xF800) == 0xB800) {
			sound_latch = Value;
			return;
		}
		return;
	}

	// ---- Sound CPU (cpu[1]) ----
	// 0x4000-0x43FF: 1KB sound RAM
	if ((Addr & 0xFC00) == 0x4000) {
		memory[0x2000 + (Addr - 0x4000)] = Value;
		return;
	}
}

// ===== Z80 Opcode Fetch =====
unsigned char bombjack::opZ80(unsigned short Addr) {
	if (current_cpu == 0) {
		if (Addr < 0x8000)
			return pRom1[Addr];
		if (Addr >= 0xC000 && Addr <= 0xDFFF)
			return pRom1[0x8000 + (Addr - 0xC000)];
		if (Addr >= 0x8000 && Addr < 0xA000)
			return memory[Addr - MEM_OFFSET];
		return 0x00;
	}
	// Sound CPU: opcodes from ROM only
	return (Addr < 0x2000) ? pRom2[Addr] : 0x00;
}

// ===== Sound CPU I/O: AY-3-8910 Port Writes =====
// Bomb Jack has 3 AY chips at ports: 0x00/0x01, 0x10/0x11, 0x80/0x81
// MAME: all ports mirrored with 0x6E → effective mask = 0x91
// address_data_w: offset 0 = latch address register, offset 1 = write data
void bombjack::outZ80(unsigned short Port, unsigned char Value) {
	if (current_cpu != 1) return;
	unsigned char p = Port & 0x91;  // mask off mirror bits (0x6E)

	switch (p) {
		case 0x00: snd_ay_port[0] = Value & 0x0F; return;
		case 0x01: soundregs[snd_ay_port[0]] = Value; return;
		case 0x10: snd_ay_port[1] = Value & 0x0F; return;
		case 0x11: soundregs[16 + snd_ay_port[1]] = Value; return;
		case 0x80: snd_ay_port[2] = Value & 0x0F; return;
		case 0x81: soundregs[32 + snd_ay_port[2]] = Value; return;
	}
}

// ===== Sound CPU I/O: AY-3-8910 Port Reads =====
// Accept both even and odd ports with 0x6E mirror (ROM may use either)
unsigned char bombjack::inZ80(unsigned short Port) {
	if (current_cpu != 1) return 0xFF;
	unsigned char p = Port & 0x90;  // mask: check only bits 7,4 (AY chip select)

	switch (p) {
		case 0x00: return soundregs[snd_ay_port[0]];
		case 0x10: return soundregs[16 + snd_ay_port[1]];
		case 0x80: return soundregs[32 + snd_ay_port[2]];
	}
	return 0xFF;
}

// ===== Run One Frame =====
void bombjack::run_frame() {
	// Dual Z80: main CPU @ 4MHz + sound CPU @ 3MHz, 60fps
	// VBLANK timing: 40 out of 264 scanlines (~15% of frame)
	// Main CPU: NMI at start of VBLANK (if nmi_enable)
	// Sound CPU: NMI at VBLANK (non-maskable, always fires)

	bj_frame_count++;

	// === VBLANK period starts ===
	bj_in_vblank = true;

	// Fire NMI on main CPU at start of VBLANK
	if (nmi_enable) {
		current_cpu = 0;
		IntZ80(&cpu[0], INT_NMI);
	}

	// Fire NMI on sound CPU at VBLANK — only if previous NMI was acknowledged
	// Hardware: NMI pin stays LOW until sound CPU reads 0x6000 (which sets snd_nmi_ready)
	// This prevents NMI re-entrance and stack overflow
	if (snd_nmi_ready) {
		snd_nmi_ready = false;  // NMI pin goes LOW (active)
		current_cpu = 1;
		IntZ80(&cpu[1], INT_NMI);
	}

	// Interleaved execution: 4 main steps + 3 sound steps per iteration
	// ~6668 main + ~5001 sound steps = hardware-equivalent cycle count
	// 4MHz / 60fps = 66667 T-states → ~6667 StepZ80 (avg 10 T-states each)
	int steps = INST_PER_FRAME * 4 / 3;  // ~1667 iterations
	int vblank_end = steps * 15 / 100;

	for (int i = 0; i < steps; i++) {
		if (i == vblank_end) bj_in_vblank = false;

		// Main CPU: 4 steps (4MHz)
		current_cpu = 0;
		StepZ80(&cpu[0]);
		StepZ80(&cpu[0]);
		StepZ80(&cpu[0]);
		StepZ80(&cpu[0]);

		// Sound CPU: 3 steps (3MHz)
		current_cpu = 1;
		StepZ80(&cpu[1]);
		StepZ80(&cpu[1]);
		StepZ80(&cpu[1]);
		snd_icnt += 3;
	}

	// Watchdog tracking
	if (watchdog_counter < 255) watchdog_counter++;

	// Boot detection: check if game has written to VRAM
	if (game_started == 0) {
		for (int i = 0; i < 0x400; i++) {
			if (memory[VRAM_OFF + i] != 0) {
				game_started = 1;
				updateFullPaletteCache();
				break;
			}
		}
	}
}

// ===== Prepare Frame (extract sprite list) =====
// MAME (refactored bombjack.cpp):
// Byte 0: full 8-bit tile code (bit 7 is NOT disabled!)
// Byte 1: bit 7=flipY, bit 6=flipX, bits 3-0=color
// Byte 2: raw Y position
// Byte 3: X position
// 32x32 determination: via sprite control registers at 0x9A00-0x9A01
// Y position: normal = 241 - rawY, large = 225 - rawY
void bombjack::prepare_frame() {
	active_sprites = 0;

	// Sprite control registers determine which sprites are 32x32
	// MAME: spritectrl_w masks to 4 bits (data &= 0x0f)
	unsigned char ctrl0 = memory[0x1A00] & 0x0F;
	unsigned char ctrl1 = memory[0x1A01] & 0x0F;

	// MAME iterates sprites 31 down to 8 (24 sprites), offs = sprite * 4
	for (int i = 23; i >= 0; i--) {
		int spriteNum = 8 + i;
		int base = SPRAM_OFF + spriteNum * 4;
		unsigned char code = memory[base];          // full 8-bit tile code
		unsigned char attr = memory[base + 1];
		unsigned char rawY = memory[base + 2];
		unsigned char sx = memory[base + 3];

		unsigned char color = attr & 0x0F;
		unsigned char flipX = (attr >> 6) & 1;  // MAME: BIT(attr, 6) = flipX
		unsigned char flipY = (attr >> 7) & 1;  // MAME: BIT(attr, 7) = flipY

		// Check if this sprite is large (32x32) using sprite control registers
		// MAME: large_sprite(sprite >> 1, attr) checks range between ctrl0 and ctrl1
		int halfIdx = spriteNum >> 1;  // range 4..15
		bool large;
		if (ctrl0 > ctrl1)
			large = (halfIdx > ctrl1) && (halfIdx <= ctrl0);
		else
			large = (halfIdx > ctrl0) && (halfIdx <= ctrl1);

		if (large) {
			// Skip odd sprites of a pair (MAME: if (BIT(sprite, 0)) continue)
			if (spriteNum & 1) continue;

			// 32x32 sprite: decompose into 4 adjacent 16x16 tiles
			// MAME: code |= 0x40, gfx[3] has 64 elements → effective index = code & 0x3F
			// Corresponding 16x16 base = (code & 0x3F) * 4
			int baseCode = (code & 0x3F) * 4;

			// MAME: large Y = (VBSTART - 16 + 1) - rawY = 225 - rawY
			int screenY = 225 - (int)rawY - VISIBLE_Y_OFFSET;

			for (int q = 0; q < 4; q++) {
				int baseDx = (q & 1) ? 16 : 0;   // q=1,3 → right column
				int baseDy = (q & 2) ? 16 : 0;   // q=2,3 → bottom row
				int qx = flipX ? (16 - baseDx) : baseDx;
				int qy = flipY ? (16 - baseDy) : baseDy;

				int finalX = (int)sx + qx;
				int finalY = screenY + qy;

				if (finalX > -16 && finalX < BJ_GAME_W && finalY > -16 && finalY < BJ_GAME_H) {
					sprite[active_sprites].code = baseCode + q;
					sprite[active_sprites].color = color;
					sprite[active_sprites].flags = (flipX << 1) | flipY;
					sprite[active_sprites].x = finalX;
					sprite[active_sprites].y = finalY;
					active_sprites++;
					if (active_sprites >= 96) return;
				}
			}
		} else {
			// 16x16 sprite
			// MAME: normal Y = (VBSTART + 1) - rawY = 241 - rawY
			int screenY = 241 - (int)rawY - VISIBLE_Y_OFFSET;

			if ((int)sx > -16 && (int)sx < BJ_GAME_W && screenY > -16 && screenY < BJ_GAME_H) {
				sprite[active_sprites].code = code;  // full 8-bit code
				sprite[active_sprites].color = color;
				sprite[active_sprites].flags = (flipX << 1) | flipY;
				sprite[active_sprites].x = sx;
				sprite[active_sprites].y = screenY;
				active_sprites++;
				if (active_sprites >= 96) return;
			}
		}
	}
}

// ===== Render One Row (8 pixels high) =====
// Row 0 = display lines 0-7 (corresponds to BJ screen Y=16-23, tile row 2)
// Row 27 = display lines 216-223 (BJ screen Y=232-239, tile row 29)
void bombjack::render_row(short row) {
	if (row >= BJ_TILE_ROWS) return;

	int screenY = row * 8;  // display Y position (0-based)

	// Layer 1: Background (16x16 tiles, static per bg_image)
	// When BG is disabled (bit 4 of bg_image = 0), clear row to black.
	// Without this, stale frame_buffer data causes visual artifacts.
	if (!(bg_image & 0x10)) {
		memset(frame_buffer, 0, BJ_GAME_W * 8 * sizeof(unsigned short));
	}
	blit_bg_row(row);

	// Layer 2: Foreground tiles (8x8, transparent where pixel=0)
	for (int col = 0; col < BJ_TILE_COLS; col++) {
		blit_tile(row, col);
	}

	// Layer 3: Sprites (on top, transparent where pixel=0)
	for (int s = 0; s < active_sprites; s++) {
		int sy = sprite[s].y;
		if (sy >= screenY - 15 && sy < screenY + 8) {
			blit_sprite(row, s);
		}
	}
}

// ===== Background Row Rendering =====
// BG is 16x16 tiles in a 16x16 grid = 256x256 pixels.
// Visible area starts at Y=16 (aligned to 16), so each 8-pixel display band
// always falls entirely within one BG tile row (no boundary crossing).
//
// MAME BG map layout: TILEMAP_SCAN_ROWS (row-major)
//   tile_index = row * 16 + col
//   bg_image bit 4 = enable, bits 0-2 = image number
// Attr: bit 7 = flipY (MAME confirmed), bits 3-0 = color
void bombjack::blit_bg_row(short row) {
	// MAME: BG drawn only when bit 4 of bg_image register is set
	if (!(bg_image & 0x10)) return;  // BG disabled

	int imageNum = bg_image & 0x07;  // image 0-4 valid
	if (imageNum > 4) return;        // safety: only 5 backgrounds exist
	int mapBase = imageNum * 0x200;

	for (int line = 0; line < 8; line++) {
		int py = row * 8 + VISIBLE_Y_OFFSET + line;
		if (py >= 256) continue;  // out of BG range

		int bgTileRow = py >> 4;       // which 16x16 tile row
		int bgRowInTile = py & 0x0F;   // line within the tile (0-15)

		for (int bgTileCol = 0; bgTileCol < 16; bgTileCol++) {
			// MAME TILEMAP_SCAN_ROWS: row-major indexing
			int tileIndex = bgTileRow * 16 + bgTileCol;
			unsigned char tileCode, attr;
			if (pBgMap) {
				tileCode = pBgMap[mapBase + tileIndex];
				attr = pBgMap[mapBase + 0x100 + tileIndex];
			} else {
				tileCode = pgm_read_byte(&BGMAP_ARR[mapBase + tileIndex]);
				attr = pgm_read_byte(&BGMAP_ARR[mapBase + 0x100 + tileIndex]);
			}
			unsigned char color_group = attr & 0x0F;
			unsigned char flipY = (attr >> 7) & 1;  // MAME: BIT(attr, 7) = flipY

			// Tile code is 8-bit from map byte (no 9-bit extension for standard Bomb Jack)
			int fullCode = tileCode;

			// Apply flipY: read rows in reverse order
			int actualRow = flipY ? (15 - bgRowInTile) : bgRowInTile;

			unsigned long lp, rp;
			if (pBgTiles) {
				lp = pBgTiles[fullCode * 32 + actualRow * 2];
				rp = pBgTiles[fullCode * 32 + actualRow * 2 + 1];
			} else {
				lp = pgm_read_dword(&BGTILES_ARR[fullCode][actualRow * 2]);
				rp = pgm_read_dword(&BGTILES_ARR[fullCode][actualRow * 2 + 1]);
			}

			int screenX = bgTileCol * 16;
			unsigned short *ptr = &frame_buffer[line * BJ_GAME_W + screenX];

			// OPAQUE rendering: draw ALL pixels including pixel 0 (MAME uses gfx->opaque)
			// No flipX for BG tiles (MAME: gfx->opaque(code, color, 0, flipy, sx, sy))
			for (int x = 0; x < 8; x++) {
				unsigned char px = (lp >> (28 - x * 4)) & 0x07;
				ptr[x] = palette_cache[(color_group << 3) | px];
			}
			for (int x = 0; x < 8; x++) {
				unsigned char px = (rp >> (28 - x * 4)) & 0x07;
				ptr[8 + x] = palette_cache[(color_group << 3) | px];
			}
		}
	}
}

// ===== Foreground Tile Rendering (8x8) =====
void bombjack::blit_tile(short row, char col) {
	// Map display row/col to BJ tile grid position
	// Display row 0 = BJ tile row 2 (VISIBLE_Y_OFFSET / 8)
	int tileRow = row + (VISIBLE_Y_OFFSET >> 3);  // +2
	int tileCol = col;  // no X crop in 256-wide mode

	int vaddr = tileRow * 32 + tileCol;

	unsigned char tcode = memory[VRAM_OFF + vaddr];
	unsigned char attr = memory[CRAM_OFF + vaddr];

	// 9-bit tile code: 8 from VRAM + bit 4 of color as bit 8
	int code = tcode | ((attr & 0x10) << 4);
	int color_group = attr & 0x0F;

	// Skip completely blank tiles (common optimization)
	// Tile 0 with color 0 is usually space
	if (code == 0 && color_group == 0) return;

	int screenX = col * 8;

	for (int y = 0; y < 8; y++) {
		unsigned long packed = pFgTiles ? pFgTiles[code * 8 + y]
		                                : pgm_read_dword(&FGTILES_ARR[code][y]);
		if (packed == 0) continue;  // all transparent

		unsigned short *ptr = &frame_buffer[y * BJ_GAME_W + screenX];

		for (int x = 0; x < 8; x++) {
			unsigned char px = (packed >> (28 - x * 4)) & 0x07;
			if (px) {  // 0 = transparent
				int palIdx = (color_group << 3) | px;
				*ptr = palette_cache[palIdx];
			}
			ptr++;
		}
	}
}

// ===== Sprite Rendering (16x16) =====
void bombjack::blit_sprite(short row, unsigned char s) {
	int sx = sprite[s].x;
	int sy = sprite[s].y;
	int screenY = row * 8;

	// Calculate which lines of the sprite are visible in this row band
	int startLine = screenY - sy;
	if (startLine < 0) startLine = 0;
	int endLine = screenY + 8 - sy;
	if (endLine > 16) endLine = 16;
	if (startLine >= endLine) return;

	unsigned char tileCode = sprite[s].code;
	unsigned char color_group = sprite[s].color & 0x0F;
	unsigned char flipX = (sprite[s].flags >> 1) & 1;
	unsigned char flipY = sprite[s].flags & 1;

	for (int line = startLine; line < endLine; line++) {
		int tileY = flipY ? (15 - line) : line;
		int displayY = line + sy - screenY;  // 0-7 within the row band

		// Get the two packed halves for this sprite row
		unsigned long left_packed, right_packed;
		if (pSprites) {
			left_packed = pSprites[tileCode * 32 + tileY * 2];
			right_packed = pSprites[tileCode * 32 + tileY * 2 + 1];
		} else {
			left_packed = pgm_read_dword(&SPRITES_ARR[tileCode][tileY * 2]);
			right_packed = pgm_read_dword(&SPRITES_ARR[tileCode][tileY * 2 + 1]);
		}

		unsigned short *ptr = &frame_buffer[displayY * BJ_GAME_W + sx];

		if (flipX) {
			// Reversed: right half first (bits reversed), then left half
			for (int x = 0; x < 16; x++) {
				int drawX = sx + x;
				if (drawX < 0 || drawX >= BJ_GAME_W) continue;

				int srcX = 15 - x;
				unsigned long packed;
				int shift;
				if (srcX < 8) {
					packed = left_packed;
					shift = 28 - srcX * 4;
				} else {
					packed = right_packed;
					shift = 28 - (srcX - 8) * 4;
				}
				unsigned char px = (packed >> shift) & 0x07;
				if (px) {
					int palIdx = (color_group << 3) | px;
					frame_buffer[displayY * BJ_GAME_W + drawX] = palette_cache[palIdx];
				}
			}
		} else {
			// Normal
			for (int x = 0; x < 16; x++) {
				int drawX = sx + x;
				if (drawX < 0 || drawX >= BJ_GAME_W) continue;

				unsigned long packed;
				int shift;
				if (x < 8) {
					packed = left_packed;
					shift = 28 - x * 4;
				} else {
					packed = right_packed;
					shift = 28 - (x - 8) * 4;
				}
				unsigned char px = (packed >> shift) & 0x07;
				if (px) {
					int palIdx = (color_group << 3) | px;
					frame_buffer[displayY * BJ_GAME_W + drawX] = palette_cache[palIdx];
				}
			}
		}
	}
}

// ===== Logo =====
const unsigned short *bombjack::logo(void) {
	return LOGO_ARR;
}
