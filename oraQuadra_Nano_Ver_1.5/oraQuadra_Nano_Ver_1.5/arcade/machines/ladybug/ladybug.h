#ifndef LADYBUG_H
#define LADYBUG_H

#include "ladybug_rom.h"
#include "ladybug_dipswitches.h"
#include "ladybug_logo.h"
#include "ladybug_tilemap.h"
#include "ladybug_spritemap.h"
#include "ladybug_cmap.h"
// NOTE: Lady Bug does NOT use shared tileaddr.h - has custom tilemap scan
#include "../machineBase.h"

class ladybug : public machineBase
{
public:
	ladybug() {
		pRom1 = ladybug_rom_cpu1;
	}
	~ladybug() { }

	void applyExternalRoms() override {
		if (externalRom[0]) pRom1 = externalRom[0];
	}

	signed char machineType() override { return MCH_LADYBUG; }
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

	const unsigned char *pRom1;    // 24KB CPU ROM

	// Video state
	unsigned char flipScreen = 0;

	// SN76489 sound state (2 chips, mapped to soundregs[] as 2 AY chips)
	unsigned char sn_latch[2] = {0, 0};       // Last latched register per chip
	unsigned short sn_freq[2][3] = {{0}};      // 10-bit tone periods
	unsigned char sn_atten[2][4] = {{0}};      // 4-bit attenuations (0=loud, 15=off)
	unsigned char sn_noise_ctrl[2] = {0, 0};   // Noise control register

	// Vblank state (toggled in run_frame for polling via IN1 bits 6-7)
	unsigned char vblankActive = 0;

	// Coin NMI tracking
	unsigned char coinPrev = 0;

	// Frame counter for game_started detection
	unsigned short frameCount = 0;
};

#endif
