#ifndef MACHINEBASE_H
#define MACHINEBASE_H

// ============================================================================
// MACHINE BASE CLASS - Adapted from Galag for oraQuadra Nano
// Removed: LED_PIN, FastLED, Bluepad32 Input class
// Changed: Uses ArcadeInput instead of Input
// ============================================================================

#include "Arduino.h"
#include "../cpus/z80/Z80.h"
#include "../cpus/i8048/i8048.h"
#include "../arcade_config.h"

// Forward declaration
class ArcadeInput;

struct sprite_S {
  unsigned char code, color, flags;
  short x, y;
};

enum {
  MCH_MENU = 0,
  MCH_PACMAN,
  MCH_GALAGA,
  MCH_DKONG,
  MCH_FROGGER,
  MCH_DIGDUG,
  MCH_1942,
  MCH_EYES,
  MCH_MRTNT,
  MCH_LIZWIZ,
  MCH_THEGLOB,
  MCH_CRUSH,
  MCH_ANTEATER
};

class machineBase
{
public:
    machineBase() { }
    virtual ~machineBase() { }

    virtual void init(ArcadeInput *input, unsigned short *framebuffer, sprite_S *spritebuffer, unsigned char *memorybuffer) {
      this->input = input;
      this->frame_buffer = framebuffer;
      this->sprite = spritebuffer;
      this->memory = memorybuffer;
      memset(soundregs, 0, sizeof(soundregs));
     }

    virtual void reset() {
      for(current_cpu = 0; current_cpu < sizeof(cpu) / sizeof(Z80); current_cpu++)
        ResetZ80(&cpu[current_cpu]);

      current_cpu = 0;
      game_started = 0;

      memset(soundregs, 0, sizeof(soundregs));
      memset(memory, 0, RAMSIZE);
    }

    virtual signed char machineType() { return MCH_MENU; }

    virtual unsigned char rdZ80(unsigned short Addr) { return 0xff; }
    virtual void wrZ80(unsigned short Addr, unsigned char Value) { };
    virtual void outZ80(unsigned short Port, unsigned char Value) { };
    virtual unsigned char opZ80(unsigned short Addr) { return 0x00; }
    virtual unsigned char inZ80(unsigned short Port) { return 0x00; }

    virtual void wrI8048_port(struct i8048_state_S *state, unsigned char port, unsigned char pos) { }
    virtual unsigned char rdI8048_port(struct i8048_state_S *state, unsigned char port) { return 0x00; };
    virtual unsigned char rdI8048_xdm(struct i8048_state_S *state, unsigned char addr)  { return 0x00; };
    virtual unsigned char rdI8048_rom(struct i8048_state_S *state, unsigned short addr) { return 0x00; };

    virtual void run_frame(void) { };
    virtual void prepare_frame(void) { };
    virtual void render_row(short row) { };

    virtual const signed char *waveRom(unsigned char value) { return 0; }
    virtual const unsigned short *logo(void) { return 0; };
    virtual bool hasNamcoAudio() { return false; }

    char game_started;
    unsigned char soundregs[32];

protected:
    virtual void blit_tile(short row, char col) { }
    virtual void blit_sprite(short row, unsigned char s) { }

    ArcadeInput *input;
    Z80 cpu[3];
    char irq_enable[3];
    char current_cpu;
    unsigned char irq_ptr;

    int active_sprites;
    sprite_S *sprite;
    unsigned short *frame_buffer;
    unsigned char *memory;
};

#endif
