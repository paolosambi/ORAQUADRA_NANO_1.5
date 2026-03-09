#ifndef BOMBJACK_H
#define BOMBJACK_H

#include "bombjack_rom1.h"
#include "bombjack_rom2.h"
#include "bombjack_dipswitches.h"
#include "bombjack_logo.h"
#include "bombjack_tilemap.h"
#include "bombjack_spritemap.h"
#include "bombjack_bgtiles.h"
#include "bombjack_bgmap.h"
#include "../machineBase.h"

// Bomb Jack: Dual Z80 - main @ 4MHz + sound @ 3MHz
// Main CPU: NMI on VBLANK, watchdog at 0xB003, sound latch at 0xB800
// Sound CPU: 3x AY-3-8910 PSG, IRQ on VBLANK, reads commands from sound latch

class bombjack : public machineBase
{
public:
	bombjack() {
		pRom1 = bombjack_rom_cpu1;
		pRom2 = bombjack_rom_cpu2;
	}
	~bombjack() {
		freeTileCache();
	}

	void applyExternalRoms() override {
		if (externalRom[0]) pRom1 = externalRom[0];
		if (externalRom[1]) pRom2 = externalRom[1];
	}

	void reset() override;
	signed char machineType() override { return MCH_BOMBJACK; }

	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;
	void outZ80(unsigned short Port, unsigned char Value) override;
	unsigned char inZ80(unsigned short Port) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;

	const unsigned short *logo(void) override;
	bool hasNamcoAudio() override { return false; }

	// Bomb Jack: native 256x224, ROT90 → output 224x256 portrait
	virtual int gameWidth() { return 256; }
	virtual int gameHeight() { return 224; }
	virtual int gameRotation() { return 90; }

protected:
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

private:
	void blit_bg_row(short row);
	void updatePaletteCache(int index);
	void updateFullPaletteCache();
	void buildTileCache();
	void freeTileCache();
	const unsigned char *pRom1;
	const unsigned char *pRom2;

	// PSRAM-cached tile data: eliminates flash SPI reads during rendering.
	// On ESP32-S3, flash and PSRAM share the SPI bus. Heavy flash reads
	// (BG tiles during gameplay) starve LCD_CAM DMA reads from PSRAM,
	// causing display controller underrun and system reset.
	unsigned long *pBgTiles = nullptr;   // 256 tiles x 32 longs = 32KB
	unsigned char *pBgMap = nullptr;     // 4096 bytes
	unsigned long *pFgTiles = nullptr;   // 512 tiles x 8 longs = 16KB
	unsigned long *pSprites = nullptr;   // 256 tiles x 32 longs = 32KB

	// NMI enable (written at 0xB000)
	unsigned char nmi_enable = 0;

	// Background select (0-4, 5+ = no bg)
	unsigned char bg_image = 0;

	// Screen flip
	unsigned char flip_screen = 0;

	// Sound latch (written at 0xB800, read by audio CPU at 0x6000)
	// Hardware: reading 0x6000 clears latch to 0 AND deactivates NMI pin
	unsigned char sound_latch = 0;
	bool snd_nmi_ready = true;  // NMI pin HIGH (ready for next falling edge)

	// Sound CPU state (3x AY-3-8910 PSG)
	unsigned char snd_ay_port[3] = {0, 0, 0};  // Selected register per AY chip
	unsigned long snd_icnt = 0;                  // Sound CPU instruction counter

	// Watchdog
	unsigned char watchdog_counter = 0;

	// Frame counter (used for VBLANK timing)
	unsigned long bj_frame_count = 0;

	// VBLANK tracking: DSW2 bit 7 reflects VBLANK status (active-LOW)
	// Game polls this during boot for timing synchronization
	bool bj_in_vblank = false;

	// Palette cache: 128 entries in RGB565 big-endian (pre-swapped for display pipeline)
	unsigned short palette_cache[128];

	// Visible area offset: BJ screen shows lines 16-239 (tile rows 2-29)
	static const int VISIBLE_Y_OFFSET = 16;  // 2 tile rows
	static const int VISIBLE_X_OFFSET = 16;  // 2 tile columns
	static const int BJ_GAME_W = 256;
	static const int BJ_GAME_H = 224;  // 28 tile rows
	static const int BJ_TILE_ROWS = 28;
	static const int BJ_TILE_COLS = 32;
};

#endif
