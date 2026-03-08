#ifndef DKONG_H
#define DKONG_H

#define DKONG_AUDIO_QUEUE_LEN   16
#define DKONG_AUDIO_QUEUE_MASK (DKONG_AUDIO_QUEUE_LEN-1)

#include "dkong_rom1.h"
#include "dkong_rom2.h"
#include "dkong_dipswitches.h"
#include "dkong_logo.h"
#include "dkong_tilemap.h"
#include "dkong_spritemap.h"
#include "dkong_cmap.h"
#include "dkong_sample_walk0.h"
#include "dkong_sample_walk1.h"
#include "dkong_sample_walk2.h"
#include "dkong_sample_jump.h"
#include "dkong_sample_stomp.h"
#include "../tileaddr.h"
#include "../machineBase.h"

class dkong : public machineBase
{
public:
	dkong() {
		pRom1 = dkong_rom_cpu1;
		pRom2 = dkong_rom_cpu2;
	}
	~dkong() { }

	void applyExternalRoms() override {
		if (externalRom[0]) pRom1 = externalRom[0];
		if (externalRom[1]) pRom2 = externalRom[1];
	}

	void reset() override;
	signed char machineType() override { return MCH_DKONG; }
	unsigned char rdZ80(unsigned short Addr) override;
	void wrZ80(unsigned short Addr, unsigned char Value) override;
	unsigned char opZ80(unsigned short Addr) override;

	void wrI8048_port(struct i8048_state_S *state, unsigned char port, unsigned char pos) override;
	unsigned char rdI8048_port(struct i8048_state_S *state, unsigned char port) override;
	unsigned char rdI8048_xdm(struct i8048_state_S *state, unsigned char addr) override;
	unsigned char rdI8048_rom(struct i8048_state_S *state, unsigned short addr) override;

	void run_frame(void) override;
	void prepare_frame(void) override;
	void render_row(short row) override;
	const unsigned short *logo(void) override;

	unsigned char dkong_obuf_toggle = 0;
	unsigned char dkong_audio_transfer_buffer[DKONG_AUDIO_QUEUE_LEN][64];
	unsigned char dkong_audio_rptr = 0, dkong_audio_wptr = 0;
	unsigned short dkong_sample_cnt[3] = { 0,0,0 };
	const signed char *dkong_sample_ptr[3];

protected:
	void blit_tile(short row, char col) override;
	void blit_sprite(short row, unsigned char s) override;

private:
	const unsigned char *pRom1;
	const unsigned char *pRom2;
	void trigger_sound(char snd);
	i8048_state_S cpu_8048;
	unsigned char colortable_select = 0;
	unsigned char dkong_sfx_index = 0x00;
	unsigned char dkong_audio_assembly_buffer[64];
};

#endif
