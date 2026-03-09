#ifndef XEVIOUS_H
#define XEVIOUS_H

#include "xevious_rom1.h"
#include "xevious_rom2.h"
#include "xevious_rom3.h"
#include "xevious_dipswitches.h"
#include "xevious_logo.h"
#include "xevious_spritemap.h"
#include "xevious_tilemap.h"
#include "xevious_bgtiles.h"
#include "xevious_cmap.h"
#include "xevious_bgmap.h"
#include "xevious_sr3.h"
#include "xevious_wavetable.h"
#include "xevious_scrolltable.h"
// NOTE: Xevious does NOT use shared tileaddr.h - uses 64x32 scan_rows addressing
#include "../machineBase.h"

#define XEVIOUS_NMI_DELAY  30

class xevious : public machineBase
{
public:
	xevious() {
		pRom1 = xevious_rom_cpu1;
		pRom2 = xevious_rom_cpu2;
		pRom3 = xevious_rom_cpu3;
	}
	~xevious() { }

	void applyExternalRoms() override {
		if (externalRom[0]) pRom1 = externalRom[0];
		if (externalRom[1]) pRom2 = externalRom[1];
		if (externalRom[2]) pRom3 = externalRom[2];
	}

	void reset() override;
	signed char machineType() override { return MCH_XEVIOUS; }

	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;

	const signed char *waveRom(unsigned char value) override;
	const unsigned short *logo(void) override;
	bool hasNamcoAudio() override { return true; }

protected:
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

private:
	void blit_bg_row(short row);
	inline unsigned char namco_read(unsigned short Addr);
	inline void namco_write(unsigned short Addr, unsigned char Value);

	unsigned char namco_command = 0;
	unsigned char namco_mode = 0;
	unsigned char namco_nmi_counter = 0;
	unsigned char namco_credit = 0x00;
	unsigned char namco_customio[16] = {};  // 06xx data buffer for custom chip I/O

	const unsigned char *pRom1;
	const unsigned char *pRom2;
	const unsigned char *pRom3;

	// Scroll registers (0xD000-0xD07F, MAME xevious_vh_latch_w)
	unsigned short scrollBgX = 0;
	unsigned short scrollBgY = 0;
	unsigned short scrollFgX = 0;
	unsigned short scrollFgY = 0;
	unsigned char flipScreen = 0;

	// Background terrain registers (0xF000 write, MAME xevious_bs_w)
	// Two bytes used by bb_r for GFX4 cascade ROM lookup
	unsigned char bs_reg[2] = {};

	// Watchdog timer (MAME: 8 vblanks without feed → CPU reset, no RAM clear)
	unsigned char watchdog_counter = 0;
	unsigned short watchdog_feed_count = 0;  // Consecutive frames with watchdog fed

	char sub_cpu_reset = 1;

	// Demo auto-play: simulates joystick input when 51xx MCU demo data unavailable
	bool demoActive = false;
	unsigned short demoFrame = 0;
	bool coinInserted = false;  // tracks if coin was inserted this attract cycle
};

#endif
