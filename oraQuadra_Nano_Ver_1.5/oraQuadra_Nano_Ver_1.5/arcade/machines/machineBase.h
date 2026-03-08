#ifndef MACHINEBASE_H
#define MACHINEBASE_H

// ============================================================================
// MACHINE BASE CLASS - Adapted from Galag for oraQuadra Nano
// Removed: LED_PIN, FastLED, Bluepad32 Input class
// Changed: Uses ArcadeInput instead of Input
// Added: External ROM support (load from SD card to PSRAM)
// ============================================================================

#include "Arduino.h"
#include "../cpus/z80/Z80.h"
#include "../cpus/i8048/i8048.h"
#include "../arcade_config.h"

// Forward declaration
class ArcadeInput;

struct sprite_S {
  unsigned short code;
  unsigned char color, flags;
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
  MCH_ANTEATER,
  MCH_LADYBUG,
  MCH_XEVIOUS,
  MCH_BOMBJACK,
  MCH_GYRUSS
};

#define EXTERNAL_ROM_MAX 5  // Max 5 ROMs per machine (1942 has 5: rom1,rom2,b0,b1,b2)

class machineBase
{
public:
    machineBase() {
      memset(externalRom, 0, sizeof(externalRom));
      memset(externalRomSize, 0, sizeof(externalRomSize));
    }

    virtual ~machineBase() {
      freeExternalRoms();
    }

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
      irq_ptr = 0;
      active_sprites = 0;
      memset(irq_enable, 0, sizeof(irq_enable));

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

    // Game dimensions (default 224x288 portrait; Bomb Jack overrides to 256x224 landscape)
    virtual int gameWidth() { return ARC_GAME_W; }
    virtual int gameHeight() { return ARC_GAME_H; }

    // Rotation: 0=none (or handled internally), 90=ROT90 CW, 270=ROT270 CW
    // Non-zero triggers full-frame buffer rotation in the render pipeline
    virtual int gameRotation() { return 0; }

    // Called after externalRom[] are loaded - lets subclass copy pointers for direct access
    virtual void applyExternalRoms() { }

    char game_started;
    unsigned char soundregs[48];  // 3 AY chips x 16 regs (Bomb Jack uses all 3)

    // Diagnostic: read CPU program counter (for boot debugging)
    unsigned short getPC(int cpuIdx) const { return cpu[cpuIdx].PC.W; }

    // ===== External ROM support (loaded from SD to PSRAM) =====
    unsigned char* externalRom[EXTERNAL_ROM_MAX];
    uint32_t externalRomSize[EXTERNAL_ROM_MAX];

    void freeExternalRoms() {
      for (int i = 0; i < EXTERNAL_ROM_MAX; i++) {
        if (externalRom[i]) {
          free(externalRom[i]);
          externalRom[i] = nullptr;
          externalRomSize[i] = 0;
        }
      }
    }

    // Helper: read from external ROM if available, else return 0xFF (miss)
    inline bool hasExtRom(int cpuIdx) const {
      return externalRom[cpuIdx] != nullptr;
    }

    inline unsigned char readExtRom(int cpuIdx, unsigned short Addr) const {
      if (externalRom[cpuIdx] && Addr < externalRomSize[cpuIdx])
        return externalRom[cpuIdx][Addr];
      return 0xFF;
    }

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
