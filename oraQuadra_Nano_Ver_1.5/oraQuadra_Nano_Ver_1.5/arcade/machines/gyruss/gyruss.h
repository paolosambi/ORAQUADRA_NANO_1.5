#ifndef GYRUSS_H
#define GYRUSS_H

#include "../machineBase.h"
#include "../../cpus/m6809/m6809.h"
#include "gyruss_rom_main.h"
#include "gyruss_rom_sub.h"
#include "gyruss_rom_audio.h"
#include "gyruss_tilemap.h"
#include "gyruss_spritemap.h"
#include "gyruss_palette.h"
#include "gyruss_dipswitches.h"
#include "gyruss_logo.h"
#include "gyruss_logo_tft.h"

// Gyruss memory layout offsets in shared memory[] buffer
// memory[0x0000..0x03FF] = Color RAM (1KB)
// memory[0x0400..0x07FF] = Video RAM (1KB)
// memory[0x0800..0x17FF] = Main Z80 Work RAM (4KB)
// memory[0x1800..0x1FFF] = Audio Z80 RAM (1KB)
#define GYR_CRAM_OFF   0x0000
#define GYR_VRAM_OFF   0x0400
#define GYR_WRAM_OFF   0x0800
#define GYR_ARAM_OFF   0x1800

class gyruss : public machineBase
{
public:
    gyruss() {
        pRomMain = gyruss_rom_main;
        pRomAudio = gyruss_rom_audio;
    }
    ~gyruss() { }

    void applyExternalRoms() override {
        if (externalRom[0]) pRomMain = externalRom[0];
        // rom2 = sub CPU (Konami-1 encrypted, uses embedded PROGMEM)
        if (externalRom[2]) pRomAudio = externalRom[2];
    }

    void init(ArcadeInput *input, unsigned short *framebuffer, sprite_S *spritebuffer, unsigned char *memorybuffer) override;
    void reset() override;

    signed char machineType() override { return MCH_GYRUSS; }

    unsigned char rdZ80(unsigned short Addr) override;
    void wrZ80(unsigned short Addr, unsigned char Value) override;
    void outZ80(unsigned short Port, unsigned char Value) override;
    unsigned char opZ80(unsigned short Addr) override;
    unsigned char inZ80(unsigned short Port) override;

    void run_frame(void) override;
    void prepare_frame(void) override;
    void render_row(short row) override;
    const unsigned short *logo(void) override;

    // M6809 sub-CPU memory access (called from C callbacks)
    uint8_t sub_read(uint16_t addr);
    void sub_write(uint16_t addr, uint8_t val);
    uint8_t sub_read_opcode(uint16_t addr);

protected:
    void blit_tile(short row, char col);
    void blit_sprite(short row, unsigned char s_idx);

    const unsigned char *pRomMain;
    const unsigned char *pRomAudio;

private:
    // M6809 sub-CPU
    m6809_state sub_cpu;
    unsigned char sub_ram[0x800];
    unsigned char shared_ram[0x800];
    unsigned char sub_irq_mask;

    // Audio
    volatile unsigned char sound_latch;
    unsigned char sound_latch_pending;
    volatile unsigned char sound_irq_pending;
    unsigned char ay_address[5];
    volatile unsigned long audio_cycle_approx;

    // Video
    unsigned char flip_screen;
    unsigned char scanline_counter;
    unsigned char frame_odd;
};

// Global pointer for M6809 callbacks (only one gyruss instance)
extern gyruss *g_gyruss_instance;

#endif
