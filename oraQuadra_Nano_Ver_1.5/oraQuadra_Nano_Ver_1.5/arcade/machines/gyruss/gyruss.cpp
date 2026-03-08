#include "gyruss.h"
#include "../../arcade_undef.h"

// ============================================================
// Gyruss (Konami, 1983) - Machine Engine
//
// Imported from galagino_arduino_BT, adapted for oraQuadra Nano.
// Main Z80 @ 3.072 MHz + M6809 sub-CPU @ 1.536 MHz + Audio Z80 @ 3.579 MHz
// 5x AY-3-8910 sound chips (3 rendered)
//
// Memory layout in machineBase::memory[] buffer:
//   [0x0000-0x03FF] = Color RAM (1KB)
//   [0x0400-0x07FF] = Video RAM (1KB)
//   [0x0800-0x17FF] = Main Z80 Work RAM (4KB)
//   [0x1800-0x1FFF] = Audio Z80 RAM (1KB)
// ============================================================

// ============================================================
// M6809 callback bridge (C functions calling C++ methods)
// ============================================================

gyruss *g_gyruss_instance = nullptr;

extern "C" {
    uint8_t m6809_read(m6809_state *s, uint16_t addr) {
        return g_gyruss_instance->sub_read(addr);
    }
    void m6809_write(m6809_state *s, uint16_t addr, uint8_t val) {
        g_gyruss_instance->sub_write(addr, val);
    }
    uint8_t m6809_read_opcode(m6809_state *s, uint16_t addr) {
        return g_gyruss_instance->sub_read_opcode(addr);
    }
}

// ============================================================
// M6809 sub-CPU memory map
// ============================================================

uint8_t gyruss::sub_read(uint16_t addr) {
    if (addr == 0x0000) {
        return scanline_counter;
    }
    if (addr >= 0x4000 && addr <= 0x47FF) {
        return sub_ram[addr - 0x4000];
    }
    if (addr >= 0x6000 && addr <= 0x67FF) {
        return shared_ram[addr - 0x6000];
    }
    if (addr >= 0xE000) {
        return pgm_read_byte(&gyruss_rom_sub_raw[addr - 0xE000]);
    }
    return 0xFF;
}

void gyruss::sub_write(uint16_t addr, uint8_t val) {
    if (addr == 0x2000) {
        sub_irq_mask = val & 1;
        return;
    }
    if (addr >= 0x4000 && addr <= 0x47FF) {
        sub_ram[addr - 0x4000] = val;
        return;
    }
    if (addr >= 0x6000 && addr <= 0x67FF) {
        shared_ram[addr - 0x6000] = val;
        return;
    }
}

uint8_t gyruss::sub_read_opcode(uint16_t addr) {
    if (addr >= 0xE000) {
        return pgm_read_byte(&gyruss_rom_sub_decrypt[addr - 0xE000]);
    }
    return sub_read(addr);
}

// ============================================================
// Init and Reset
// ============================================================

void gyruss::init(ArcadeInput *input, unsigned short *framebuffer, sprite_S *spritebuffer, unsigned char *memorybuffer) {
    machineBase::init(input, framebuffer, spritebuffer, memorybuffer);
    g_gyruss_instance = this;
}

void gyruss::reset() {
    machineBase::reset();
    g_gyruss_instance = this;

    memset(sub_ram, 0, sizeof(sub_ram));
    memset(shared_ram, 0, sizeof(shared_ram));
    sub_irq_mask = 0;
    sound_latch = 0;
    sound_latch_pending = 0;
    sound_irq_pending = 0;
    flip_screen = 0;
    scanline_counter = 0;
    frame_odd = 0;
    memset(ay_address, 0, sizeof(ay_address));
    audio_cycle_approx = 0;

    // Reset M6809 sub-CPU
    m6809_reset(&sub_cpu);

    // Reset audio Z80 (cpu[1])
    current_cpu = 1;
    ResetZ80(&cpu[1]);
    current_cpu = 0;
}

// ============================================================
// Main Z80 memory map
// ============================================================

unsigned char gyruss::opZ80(unsigned short Addr) {
    if (current_cpu == 0) {
        if (Addr < 0x6000)
            return pRomMain[Addr];
        return rdZ80(Addr);
    } else {
        if (Addr < 0x4000)
            return pRomAudio[Addr];
        return rdZ80(Addr);
    }
}

unsigned char gyruss::rdZ80(unsigned short Addr) {
    if (current_cpu == 0) {
        if (Addr < 0x6000)
            return pRomMain[Addr];

        if (Addr >= 0x8000 && Addr <= 0x83FF)
            return memory[GYR_CRAM_OFF + (Addr & 0x3FF)];

        if (Addr >= 0x8400 && Addr <= 0x87FF)
            return memory[GYR_VRAM_OFF + (Addr & 0x3FF)];

        if (Addr >= 0x9000 && Addr <= 0x9FFF)
            return memory[GYR_WRAM_OFF + (Addr - 0x9000)];

        if (Addr >= 0xA000 && Addr <= 0xA7FF)
            return shared_ram[Addr - 0xA000];

        if (Addr == 0xC000) return GYRUSS_DSW1;

        if (Addr == 0xC080) {
            unsigned char keymask = input->buttons_get();
            unsigned char retval = 0xFF;
            if (keymask & BUTTON_COIN)  retval &= ~0x01;
            if (keymask & BUTTON_START) retval &= ~0x08;
            return retval;
        }

        if (Addr == 0xC0A0) {
            unsigned char keymask = input->buttons_get();
            unsigned char retval = 0xFF;
            if (keymask & BUTTON_LEFT)  retval &= ~0x01;
            if (keymask & BUTTON_RIGHT) retval &= ~0x02;
            if (keymask & BUTTON_UP)    retval &= ~0x04;
            if (keymask & BUTTON_DOWN)  retval &= ~0x08;
            if ((keymask & BUTTON_FIRE) && game_started) retval &= ~0x10;
            return retval;
        }

        if (Addr == 0xC0C0) return 0xFF;
        if (Addr == 0xC0E0) return GYRUSS_DSW0;
        if (Addr == 0xC100) return GYRUSS_DSW2;

        return 0xFF;
    }
    else {
        if (Addr < 0x4000)
            return pRomAudio[Addr];

        if (Addr >= 0x6000 && Addr <= 0x63FF)
            return memory[GYR_ARAM_OFF + (Addr - 0x6000)];

        if (Addr == 0x8000) {
            return sound_latch;
        }

        return 0xFF;
    }
}

void gyruss::wrZ80(unsigned short Addr, unsigned char Value) {
    if (current_cpu == 0) {
        if (Addr >= 0x8000 && Addr <= 0x83FF) {
            memory[GYR_CRAM_OFF + (Addr & 0x3FF)] = Value;
            return;
        }

        if (Addr >= 0x8400 && Addr <= 0x87FF) {
            memory[GYR_VRAM_OFF + (Addr & 0x3FF)] = Value;
            if (!game_started && Value != 0) game_started = 1;
            return;
        }

        if (Addr >= 0x9000 && Addr <= 0x9FFF) {
            memory[GYR_WRAM_OFF + (Addr - 0x9000)] = Value;
            return;
        }

        if (Addr >= 0xA000 && Addr <= 0xA7FF) {
            shared_ram[Addr - 0xA000] = Value;
            return;
        }

        if (Addr == 0xC000) return;

        if (Addr == 0xC080) {
            sound_irq_pending = 1;
            return;
        }

        if (Addr == 0xC100) {
            sound_latch = Value;
            return;
        }

        if (Addr == 0xC180) {
            irq_enable[0] = Value & 1;
            return;
        }

        if (Addr == 0xC185) {
            flip_screen = Value & 1;
            return;
        }

        return;
    }
    else {
        if (Addr >= 0x6000 && Addr <= 0x63FF) {
            memory[GYR_ARAM_OFF + (Addr - 0x6000)] = Value;
            return;
        }
    }
}

void gyruss::outZ80(unsigned short Port, unsigned char Value) {
    if (current_cpu != 1) return;

    uint8_t port_lo = Port & 0xFF;

    if (port_lo > 0x12) return;

    int ay_chip = port_lo / 4;
    int func = port_lo % 4;

    if (func == 0) {
        ay_address[ay_chip] = Value & 0x0F;
    } else if (func == 2) {
        if (ay_chip < 3) {
            soundregs[(ay_chip * 16) + ay_address[ay_chip]] = Value;
        }
    }
}

unsigned char gyruss::inZ80(unsigned short Port) {
    if (current_cpu != 1) return 0xFF;

    uint8_t port_lo = Port & 0xFF;

    if (port_lo > 0x12) return 0xFF;
    int ay_chip = port_lo / 4;
    int func = port_lo % 4;

    if (func == 1 && ay_chip < 5) {
        int reg = ay_address[ay_chip];
        if (ay_chip == 2 && reg == 14) {
            static const uint8_t timer_table[10] = {
                0x00, 0x01, 0x02, 0x03, 0x04, 0x09, 0x0a, 0x0b, 0x0a, 0x0d
            };
            return timer_table[(audio_cycle_approx / 1024) % 10];
        }
        if (ay_chip < 3) {
            return soundregs[(ay_chip * 16) + reg];
        }
    }

    return 0xFF;
}

// ============================================================
// Frame execution
// ============================================================

void gyruss::run_frame(void) {
    for (int i = 0; i < INST_PER_FRAME; i++) {
        current_cpu = 0;
        StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);

        // M6809 sub CPU @ 1.536 MHz, main Z80 @ 3.072 MHz -> ratio 0.5
        m6809_step(&sub_cpu); m6809_step(&sub_cpu);

        current_cpu = 1;
        // Audio Z80 runs at 3.579 MHz vs main Z80 at 3.072 MHz (ratio ~1.17)
        for (int s = 0; s < 5; s++) {
            StepZ80(&cpu[1]);
            if (sound_irq_pending && (cpu[1].IFF & IFF_1)) {
                IntZ80(&cpu[1], INT_RST38);
                sound_irq_pending = 0;
            }
        }
        audio_cycle_approx += 40;

        scanline_counter = (i * 256) / INST_PER_FRAME;
    }

    if (irq_enable[0]) {
        current_cpu = 0;
        IntZ80(&cpu[0], INT_NMI);
    }

    if (sub_irq_mask) {
        m6809_irq(&sub_cpu);
    }

    current_cpu = 0;
    frame_odd ^= 1;
}

// ============================================================
// Sprite preparation
// ============================================================

void gyruss::prepare_frame(void) {
    active_sprites = 0;

    unsigned char *sr = &sub_ram[0x40];

    for (int offs = 0xBC; offs >= 0 && active_sprites < 64; offs -= 4) {
        int bx = sr[offs];
        int by = 241 - sr[offs + 3];
        int gfx_bank = sr[offs + 1] & 1;
        int code = ((sr[offs + 2] & 0x20) << 2) | (sr[offs + 1] >> 1);
        int color = sr[offs + 2] & 0x0f;
        int hw_flip_x = (~sr[offs + 2] >> 6) & 1;
        int hw_flip_y = (sr[offs + 2] >> 7) & 1;

        sprite[active_sprites].x = by - 16;
        sprite[active_sprites].y = bx + 16;
        sprite[active_sprites].code = code;
        sprite[active_sprites].color = color;
        // flags: bit 0 = flip_y, bit 1 = flip_x, bit 2 = gfx_bank
        sprite[active_sprites].flags = (hw_flip_y & 1) | ((hw_flip_x & 1) << 1) | ((gfx_bank & 1) << 2);

        active_sprites++;
    }
}

// ============================================================
// Tile rendering
// ============================================================

void gyruss::blit_tile(short row, char col) {
    int vram_row = row - 2;
    int vram_col = 29 - col;

    if (vram_row < 0 || vram_row >= 32 || vram_col < 0 || vram_col >= 32) return;

    unsigned short vram_addr = (31 - vram_col) * 32 + vram_row;
    unsigned char tile_code_raw = memory[GYR_VRAM_OFF + vram_addr];
    unsigned char color_attr = memory[GYR_CRAM_OFF + vram_addr];

    unsigned short tile_code = ((color_attr & 0x20) ? 256 : 0) + tile_code_raw;
    if (tile_code >= 512) tile_code = 0;

    unsigned char color_group = color_attr & 0x0F;
    unsigned char flip_x = (color_attr >> 6) & 1;
    unsigned char flip_y = (color_attr >> 7) & 1;

    const unsigned short *tile_data = gyruss_tilemap[tile_code];
    const unsigned short *colors = gyruss_char_colormap[color_group];

    unsigned short *ptr = frame_buffer + 8 * col;

    for (int r = 0; r < 8; r++) {
        int src_r = flip_y ? (7 - r) : r;
        unsigned short pix = pgm_read_word(&tile_data[src_r]);

        for (int c = 0; c < 8; c++) {
            int src_c = flip_x ? (7 - c) : c;
            unsigned char px = (pix >> (src_c * 2)) & 3;
            if (px) {
                ptr[c] = pgm_read_word(&colors[px]);
            }
        }
        ptr += 224;
    }
}

// ============================================================
// Sprite rendering
// ============================================================

void gyruss::blit_sprite(short row, unsigned char s_idx) {
    struct sprite_S *s = &sprite[s_idx];

    int spr_start_y = s->y;
    int row_pixel_start = row * 8;

    if (spr_start_y >= row_pixel_start + 8 || spr_start_y + 8 <= row_pixel_start)
        return;

    int gfx_bank = (s->flags >> 2) & 1;
    int code = s->code + (gfx_bank ? 256 : 0);
    if (code >= 512) code = 0;

    int variant = 0;
    if (s->flags & 1) variant |= 1;   // flip_y
    if (s->flags & 2) variant |= 2;   // flip_x

    unsigned char color_group = s->color;
    const unsigned short *colors = gyruss_sprite_colormap[color_group];

    int c_start = (row_pixel_start > spr_start_y) ? (row_pixel_start - spr_start_y) : 0;
    int c_end = ((row_pixel_start + 8) < (spr_start_y + 8)) ? (row_pixel_start + 8 - spr_start_y) : 8;

    for (int r = 0; r < 16; r++) {
        int screen_x = s->x + r;
        if (screen_x < 0 || screen_x >= 224) continue;

        unsigned long row_data = pgm_read_dword(&gyruss_sprites[variant][code][r]);

        for (int c = c_start; c < c_end; c++) {
            unsigned char px = (row_data >> (c * 4)) & 0x0F;
            if (px) {
                unsigned short color = pgm_read_word(&colors[px]);
                frame_buffer[(spr_start_y + c - row_pixel_start) * 224 + screen_x] = color;
            }
        }
    }
}

// ============================================================
// Row rendering
// ============================================================

void gyruss::render_row(short row) {
    if (row < 2 || row >= 34) return;

    for (char col = 0; col < 28; col++) {
        blit_tile(row, col);
    }

    for (int s = 0; s < active_sprites; s++) {
        blit_sprite(row, s);
    }
}

// ============================================================
// Logo
// ============================================================

const unsigned short *gyruss::logo(void) {
    return gyruss_logo;
}
