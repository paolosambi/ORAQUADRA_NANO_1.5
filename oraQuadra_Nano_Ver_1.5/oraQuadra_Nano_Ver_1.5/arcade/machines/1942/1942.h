#ifndef _1942_H
#define _1942_H

#include "1942_rom1.h"
#include "1942_rom2.h"
#include "1942_rom1_b0.h"
#include "1942_rom1_b1.h"
#include "1942_rom1_b2.h"
#include "1942_dipswitches.h"
#include "1942_logo.h"
#include "1942_character_cmap.h"
#include "1942_spritemap.h"
#include "1942_tilemap.h"
#include "1942_charmap.h"
#include "1942_sprite_cmap.h"
#include "1942_tile_cmap.h"
#include "../tileaddr.h"
#include "../machineBase.h"

class _1942 : public machineBase
{
public:
	_1942() {
		pRom1 = _1942_rom_cpu1;
		pRom2 = _1942_rom_cpu2;
		pRomB0 = _1942_rom_cpu1_b0;
		pRomB1 = _1942_rom_cpu1_b1;
		pRomB2 = _1942_rom_cpu1_b2;
	}
	~_1942() { }

	void applyExternalRoms() override {
		if (externalRom[0]) pRom1 = externalRom[0];
		if (externalRom[1]) pRom2 = externalRom[1];
		if (externalRom[2]) pRomB0 = externalRom[2];
		if (externalRom[3]) pRomB1 = externalRom[3];
		if (externalRom[4]) pRomB2 = externalRom[4];
	}

	signed char machineType() override { return MCH_1942; }
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;
	const unsigned short *logo(void) override;
	bool hasNamcoAudio() override { return false; }

protected:
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

private:
	const unsigned char *pRom1;
	const unsigned char *pRom2;
	const unsigned char *pRomB0;
	const unsigned char *pRomB1;
	const unsigned char *pRomB2;
	void blit_bgtile_row(short row);
	void lsl64(unsigned long *mask, int pix);
	void lsr64(unsigned long *mask, int pix);
	unsigned char _1942_bank = 0;
	unsigned char _1942_palette = 0;
	unsigned short _1942_scroll = 0;
	unsigned char _1942_sound_latch = 0;
	unsigned char _1942_ay_addr[2];
	char sub_cpu_reset = 1;
};

#endif
