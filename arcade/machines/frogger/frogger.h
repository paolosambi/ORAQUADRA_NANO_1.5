#ifndef FROGGER_H
#define FROGGER_H

#include "frogger_rom1.h"
#include "frogger_rom2.h"
#include "frogger_dipswitches.h"
#include "frogger_logo.h"
#include "frogger_tilemap.h"
#include "frogger_spritemap.h"
#include "frogger_cmap.h"
#include "../tileaddr.h"
#include "../machineBase.h"

class frogger : public machineBase
{
public:
	frogger() {
		pRom1 = frogger_rom_cpu1;
		pRom2 = frogger_rom_cpu2;
	}
	~frogger() { }

	void applyExternalRoms() override {
		if (externalRom[0]) pRom1 = externalRom[0];
		if (externalRom[1]) pRom2 = externalRom[1];
	}

	signed char machineType() override { return MCH_FROGGER; }
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	void outZ80(unsigned short Port, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;
	unsigned char inZ80(unsigned short Port) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;
	const unsigned short *logo(void) override;

protected:
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;
	void blit_tile_scroll(short row, signed char col, short scroll);
	virtual const unsigned short *tileRom(unsigned short addr);
	virtual const unsigned short *colorRom(unsigned short addr);
	virtual const unsigned long *spriteRom(unsigned char flags, unsigned char code);

	const unsigned char *pRom1;
	const unsigned char *pRom2;

	unsigned char snd_irq_state = 0;
	unsigned char snd_command;
	unsigned char snd_filter;
	unsigned long snd_icnt;
	unsigned char snd_ay_port;
};

#endif
