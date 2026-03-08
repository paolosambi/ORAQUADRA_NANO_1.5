#include "xevious.h"
#include "../../arcade_undef.h"

#define TILEMAP_ARR       xevious_tilemap
#define BGTILES_ARR       xevious_bgtiles
#define SPRITES_ARR       xevious_sprites
#define CMAP_FG_ARR       xevious_cmap_fg
#define CMAP_SPRITES_ARR  xevious_cmap_sprites
#define CMAP_BG_ARR       xevious_cmap_bg
#define SR3_ARR           xevious_sr3
#define BGMAP_ARR         xevious_bgmap
#define WAVE_ARR          xevious_wavetable
#define LOGO_ARR          xevious_logo

// ============================================================================
// Xevious (Namco, 1982) - Machine Engine
//
// 3x Z80, Namco 06xx/51xx I/O, Namco WSG audio.
// Memory map based on MAME xevious_map (NOT galaga/digdug layout!).
//
// Memory map (RAMSIZE = 16384 = 16KB):
//   memory[0x0000-0x07FF] = sr1           (CPU 0x8000-0x87FF) work + sprite RAM
//   memory[0x0800-0x0FFF] = sr2           (CPU 0x9000-0x97FF) work + sprite RAM
//   memory[0x1000-0x17FF] = sr3           (CPU 0xA000-0xA7FF) work + sprite RAM
//   memory[0x1800-0x1FFF] = Shared work   (CPU 0x7800-0x7FFF)
//   memory[0x2000-0x27FF] = FG colorram   (CPU 0xB000-0xB7FF)
//   memory[0x2800-0x2FFF] = BG colorram   (CPU 0xB800-0xBFFF)
//   memory[0x3000-0x37FF] = FG videoram   (CPU 0xC000-0xC7FF) tile codes
//   memory[0x3800-0x3FFF] = BG videoram   (CPU 0xC800-0xCFFF) tile codes
//
// Sprite RAM offsets (within memory[]):
//   sr1+0x780 = memory[0x0780] -> spriteram_2 (positions)
//   sr2+0x780 = memory[0x0F80] -> spriteram_3 (flip/size)
//   sr3+0x780 = memory[0x1780] -> spriteram   (code/color/visibility)
//
// Scroll registers at 0xD000-0xD07F (MAME xevious_vh_latch_w):
//   D000-D00F: BG scroll X (data=low8, offset&1=bit8)
//   D010-D01F: FG scroll X
//   D020-D02F: BG scroll Y
//   D030-D03F: FG scroll Y
//   D070-D07F: Flip screen
//
// Protection: 0xF000-0xFFFF read = bb_r GFX4 cascade lookup, write = bs_w terrain regs
//
// FG/BG tilemap: 64 cols x 32 rows, scan_rows (addr = row*64 + col)
// ============================================================================

void xevious::reset() {
  machineBase::reset();

  // CRITICAL: Our Z80 emulator initializes SP=0xF000, but MAME uses SP=0x0000.
  // Xevious boot code at 0x0000 is a subroutine (not JP xxxx) that does RET.
  // With SP=0xF000 (protection latch area, returns 0) -> RET goes back to 0x0000,
  // CPU loops forever, SP drifts, eventually crashes into 0xFF -> RST 38h -> hang.
  // With SP=0x0000 (ROM area) -> RET pops ROM data -> PC=0x103E -> boot continues.
  for(int i = 0; i < 3; i++)
    cpu[i].SP.W = 0x0000;

  sub_cpu_reset = 1;
  scrollBgX = 0;
  scrollBgY = 0;
  scrollFgX = 0;
  scrollFgY = 0;
  flipScreen = 0;
  memset(bs_reg, 0, sizeof(bs_reg));
  namco_command = 0;
  namco_mode = 0;
  namco_nmi_counter = 0;
  namco_credit = 0;
  memset(namco_customio, 0, sizeof(namco_customio));
  watchdog_counter = 0;
  demoActive = false;
  demoFrame = 0;
  coinInserted = false;
}

// ============================================================================
// Opcode fetch - direct pointer, NO bounds check (matches galaga pattern)
// ============================================================================
unsigned char xevious::opZ80(unsigned short Addr) {
  // Patch: bypass ROM/RAM self-test for xevios bootleg
  // At 0x01DD: JR Z -> JR (unconditional) so game boots despite checksum mismatch
  if(current_cpu == 0 && Addr == 0x01DD) return 0x18;

  // ROM space (0x0000-0x3FFF): direct pointer access
  if(Addr < 0x4000) {
    if (current_cpu == 0) return pRom1[Addr];
    else if (current_cpu == 1) return pRom2[Addr];
    else return pRom3[Addr];
  }
  // Non-ROM space: fall back to memory read (avoids out-of-bounds ROM access)
  return rdZ80(Addr);
}

// ============================================================================
// Z80 memory read - MAME xevious_map layout
// ============================================================================
unsigned char xevious::rdZ80(unsigned short Addr) {
  // ROM (CPU-specific) - direct pointer access
  if(Addr < 0x4000) {
    if (current_cpu == 0) return pRom1[Addr];
    else if (current_cpu == 1) return pRom2[Addr];
    else return pRom3[Addr];
  }

  // I/O region (0x6800-0x6FFF)
  if(Addr >= 0x6800 && Addr < 0x7000) {
    // DIP switches (0x6800-0x6807) - bosco_dsw_r
    if((Addr & 0xFFF8) == 0x6800) {
      int bit = Addr & 7;
      return ((XEVIOUS_DIP_A >> bit) & 1) | (((XEVIOUS_DIP_B >> bit) & 1) << 1);
    }
    // Sound registers (0x6808-0x681F READ)
    if((Addr & 0xFFE0) == 0x6800)
      return soundregs[Addr - 0x6800];
    return 0xFF;
  }

  // Namco 06xx custom I/O (0x7000-0x71FF)
  if((Addr & 0xFE00) == 0x7000)
    return namco_read(Addr & 0x1FF);

  // Shared work RAM (0x7800-0x7FFF -> memory[0x1800-0x1FFF])
  if((Addr & 0xF800) == 0x7800)
    return memory[0x1800 + (Addr & 0x7FF)];

  // sr1, sr2, sr3, colorram, videoram (0x8000-0xCFFF)
  if(Addr >= 0x8000 && Addr < 0xD000) {
    switch(Addr & 0xF800) {
      case 0x8000: return memory[Addr & 0x7FF];             // sr1
      case 0x9000: return memory[0x0800 + (Addr & 0x7FF)];  // sr2
      case 0xA000: return memory[0x1000 + (Addr & 0x7FF)];  // sr3
      case 0xB000: return memory[0x2000 + (Addr & 0x7FF)];  // FG colorram
      case 0xB800: return memory[0x2800 + (Addr & 0x7FF)];  // BG colorram
      case 0xC000: return memory[0x3000 + (Addr & 0x7FF)];  // FG videoram
      case 0xC800: return memory[0x3800 + (Addr & 0x7FF)];  // BG videoram
    }
    return 0xFF; // unmapped gaps (0x8800, 0x9800, 0xA800)
  }

  // Background terrain ROM lookup (0xF000-0xFFFF) - MAME xevious_bb_r
  // Uses bs_reg[0..1] written via bs_w to compute a 3-ROM cascade address
  // into the GFX4 region (xevious_bgmap: rom2a=+0x0000, rom2b=+0x1000, rom2c=+0x3000)
  if(Addr >= 0xF000) {
    const unsigned char *rom2a = BGMAP_ARR;
    const unsigned char *rom2b = BGMAP_ARR + 0x1000;
    const unsigned char *rom2c = BGMAP_ARR + 0x3000;

    int adr_2b = ((bs_reg[1] & 0x7e) << 6) | ((bs_reg[0] & 0xfe) >> 1);
    int dat1;
    if(adr_2b & 1)
      dat1 = ((pgm_read_byte(&rom2a[adr_2b >> 1]) & 0xf0) << 4) | pgm_read_byte(&rom2b[adr_2b]);
    else
      dat1 = ((pgm_read_byte(&rom2a[adr_2b >> 1]) & 0x0f) << 8) | pgm_read_byte(&rom2b[adr_2b]);

    int adr_2c = ((dat1 & 0x1ff) << 2) | ((bs_reg[1] & 1) << 1) | (bs_reg[0] & 1);
    if(dat1 & 0x400) adr_2c ^= 1;
    if(dat1 & 0x200) adr_2c ^= 2;

    int dat2;
    if(Addr & 1) {
      dat2 = pgm_read_byte(&rom2c[adr_2c | 0x800]);
    } else {
      dat2 = pgm_read_byte(&rom2c[adr_2c]);
      dat2 = (dat2 & 0x3f) | ((dat2 & 0x80) >> 1) | ((dat2 & 0x40) << 1);
      if(dat1 & 0x400) dat2 ^= 0x40;
      if(dat1 & 0x200) dat2 ^= 0x80;
    }
    return dat2;
  }

  return 0xFF;
}

// ============================================================================
// Z80 memory write - MAME xevious_map layout
// ============================================================================
void xevious::wrZ80(unsigned short Addr, unsigned char Value) {
  // Sound registers (0x6800-0x681F) - same as digdug
  if((Addr & 0xFFE0) == 0x6800) {
    soundregs[Addr - 0x6800] = Value & 0x0F;
    return;
  }

  // IRQ enable + sub CPU reset (0x6820-0x6827) - LS259 latch
  // MAME Q outputs:
  //   Q0 (0x6820) = irq_enable[0] (CPU0 vblank IRQ mask)
  //   Q1 (0x6821) = irq_enable[1] (CPU1 vblank IRQ mask)
  //   Q2 (0x6822) = irq_enable[2] (CPU2 vblank NMI mask)
  //   Q3 (0x6823) = sub_cpu_reset (inverted: write 1 = run, write 0 = reset)
  if((Addr & 0xFFF8) == 0x6820) {
    switch(Addr & 7) {
      case 0: irq_enable[0] = Value & 1; break;
      case 1: irq_enable[1] = Value & 1; break;
      case 2: irq_enable[2] = Value & 1; break;
      case 3:
        sub_cpu_reset = !(Value & 1);
        if(sub_cpu_reset) {
          namco_command = 0x00;
          namco_mode = 0;
          namco_nmi_counter = 0;
          current_cpu = 1; ResetZ80(&cpu[1]);
          current_cpu = 2; ResetZ80(&cpu[2]);
        }
        break;
    }
    return;
  }

  // Watchdog timer feed (0x6830)
  if(Addr == 0x6830) {
    watchdog_counter = 0;
    watchdog_feed_count++;
    if(!game_started && watchdog_feed_count > 120) {
      game_started = 1;
      // Safety net: the xevios bootleg ROM never writes to 0x6820/0x6821.
      // After 2s of stable running, all CPUs are initialized - safe to force-enable.
      // Previous crashes with this were caused by wrong scroll/bb_r/bs_w, now fixed.
      if(!irq_enable[0]) irq_enable[0] = 1;
      if(!irq_enable[1]) irq_enable[1] = 1;
    }
    return;
  }

  // Namco 06xx (0x7000-0x71FF)
  if((Addr & 0xFE00) == 0x7000) {
    namco_write(Addr & 0x1FF, Value);
    return;
  }

  // Shared work RAM (0x7800-0x7FFF)
  if((Addr & 0xF800) == 0x7800) {
    memory[0x1800 + (Addr & 0x7FF)] = Value;
    return;
  }

  // sr1, sr2, sr3, colorram, videoram (0x8000-0xCFFF)
  if(Addr >= 0x8000 && Addr < 0xD000) {
    switch(Addr & 0xF800) {
      case 0x8000: memory[Addr & 0x7FF] = Value; return;             // sr1
      case 0x9000: memory[0x0800 + (Addr & 0x7FF)] = Value; return;  // sr2
      case 0xA000: memory[0x1000 + (Addr & 0x7FF)] = Value; return;  // sr3
      case 0xB000: memory[0x2000 + (Addr & 0x7FF)] = Value; return;  // FG colorram
      case 0xB800: memory[0x2800 + (Addr & 0x7FF)] = Value; return;  // BG colorram
      case 0xC000: memory[0x3000 + (Addr & 0x7FF)] = Value; return;  // FG videoram
      case 0xC800: memory[0x3800 + (Addr & 0x7FF)] = Value; return;  // BG videoram
    }
    return; // unmapped gaps - writes dropped
  }

  // Video latches / scroll registers (0xD000-0xD07F) - MAME xevious_vh_latch_w
  // Each write sets a 9-bit scroll value: data = low 8 bits, (offset & 1) = bit 8
  // Register selected by (offset >> 4): 0=BG scrollX, 1=FG scrollX, 2=BG scrollY, 3=FG scrollY, 7=flip
  if(Addr >= 0xD000 && Addr < 0xD080) {
    int offset = Addr & 0x7F;
    int reg = (offset >> 4) & 0x7;
    unsigned short scroll = Value + ((offset & 1) << 8);

    switch(reg) {
      case 0: scrollBgX = scroll; break;
      case 1: scrollFgX = scroll; break;
      case 2: scrollBgY = scroll; break;
      case 3: scrollFgY = scroll; break;
      case 7: flipScreen = scroll & 1; break;
    }
    return;
  }

  // Background terrain register (0xF000-0xFFFF) - MAME xevious_bs_w
  // Stores two bytes used by bb_r for the GFX4 cascade ROM lookup
  if(Addr >= 0xF000) {
    bs_reg[Addr & 1] = Value;
    return;
  }
}

// ============================================================================
// Frame execution - identical to digdug (3x Z80, NMI + IRQ timing)
// ============================================================================
void xevious::run_frame(void) {
  // Watchdog timer: MAME uses 8 frames, but our emulation runs slower.
  // Use 120 frames (~2 seconds) to give the boot sequence enough time.
  watchdog_counter++;
  if(watchdog_counter >= 120) {
    watchdog_counter = 0;
    watchdog_feed_count = 0;
    for(int i = 0; i < 3; i++) {
      current_cpu = i;
      ResetZ80(&cpu[i]);
      cpu[i].SP.W = 0x0000;
    }
    memset(irq_enable, 0, sizeof(irq_enable));
    sub_cpu_reset = 1;
    namco_command = 0;
    namco_mode = 0;
    namco_nmi_counter = 0;
    // NOTE: memory NOT cleared - boot relies on state accumulating
  }

  // Xevious CPU speed: MAME expects 50,688 cycles/frame @ 3.072 MHz.
  // INST_PER_FRAME=1250 gives ~22,500 cycles (~44%). Use 2x for ~45,000 (~89%).
  for(char c = 0; c < 4; c++) {
    for(int i = 0; i < INST_PER_FRAME / 2; i++) {
      current_cpu = 0;
      StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
      if(!sub_cpu_reset) {
        current_cpu = 1;
        StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1); StepZ80(cpu+1);
        current_cpu = 2;
        StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2); StepZ80(cpu+2);
      }

      // NMI counter for CPU0 (06xx timer simulation)
      if(namco_nmi_counter) {
        namco_nmi_counter--;
        if(!namco_nmi_counter) {
          current_cpu = 0;
          IntZ80(&cpu[0], INT_NMI);
          namco_nmi_counter = XEVIOUS_NMI_DELAY;
        }
      }
    }

    // CPU2 NMI at ~scanline 64 and 192 (MAME cpu3_interrupt_callback)
    // MAME: m_sub2_nmi_mask = !state (INVERTED gating)
    // Write 0 to 0x6822 → irq_enable[2]=0 → !0=true → NMI ENABLED
    // Write 1 to 0x6822 → irq_enable[2]=1 → !1=false → NMI DISABLED
    if(!sub_cpu_reset && !irq_enable[2] && ((c == 1) || (c == 3))) {
      current_cpu = 2;
      IntZ80(&cpu[2], INT_NMI);
    }
  }

  // xevios bootleg: ROM never writes to 0x6820/0x6821 IRQ enable latches
  // and may never execute EI instruction, leaving IFF1=0 (Z80 maskable
  // interrupts disabled). Force-enable both the emulation gate AND the Z80
  // IFF1 flag so VBLANK RST38 is accepted. Without this, only NMIs fire
  // and the attract mode timer never advances (demo never starts).
  if(game_started) {
    irq_enable[0] = 1;
    irq_enable[1] = 1;
    cpu[0].IFF |= IFF_1;
    if(!sub_cpu_reset) cpu[1].IFF |= IFF_1;
  }

  if(irq_enable[0]) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_RST38);
  }

  if(!sub_cpu_reset && irq_enable[1]) {
    current_cpu = 1;
    IntZ80(&cpu[1], INT_RST38);
  }
}

// ============================================================================
// Namco 06xx/51xx I/O - identical protocol to digdug
// ============================================================================
void xevious::namco_write(unsigned short Addr, unsigned char Value) {
  if(Addr & 0x100) {
    // Command register write (0x7100)
    namco_command = Value;
    if(Value == 0xa1) {
      namco_mode = 1;
      // Demo auto-play: only if no coin was inserted this attract cycle
      // (prevents demo input from overriding real gameplay after START)
      if(namco_credit == 0 && !coinInserted) { demoActive = true; demoFrame = 0; }
    }
    if((Value == 0xc1) || (Value == 0xe1)) {
      namco_mode = 0;
      demoActive = false;
      coinInserted = false;  // reset for next attract cycle
    }

    if(Value != 0x10) namco_nmi_counter = XEVIOUS_NMI_DELAY;
    else              namco_nmi_counter = 0;
  } else {
    // Data port write (0x7000-0x70FF) - store for custom chip use
    namco_customio[Addr & 0x0F] = Value;
  }
}

unsigned char xevious::namco_read(unsigned short Addr) {
  if(Addr & 0x100)
    return namco_command;

  unsigned char keymask = input->buttons_get();

  static unsigned char keymask_d[] = { 0x00, 0x00, 0x00 };
  keymask_d[2] = keymask_d[1];
  keymask_d[1] = keymask_d[0];
  keymask_d[0] = keymask;

  unsigned char keymask_p = (keymask ^ keymask_d[2]) & keymask;

  if((keymask_d[0] & BUTTON_COIN) && !(keymask_d[1] & BUTTON_COIN) && (namco_credit < 99)) {
    namco_credit++;
    coinInserted = true;  // mark coin inserted to prevent demo during real gameplay
  }
  if((keymask_d[0] & BUTTON_START) && !(keymask_d[1] & BUTTON_START) && (namco_credit > 0))
    namco_credit--;

  // Command 0xB1 during boot only: init/test query returns 0x00/0xFF.
  // After boot, 0xB1 is a read from chip 0 (51xx) with a different NMI rate
  // and must return real input data (same as 0x71).
  if(!game_started && namco_command == 0xb1) {
    if((Addr & 15) <= 2) return 0x00;
    return 0xff;
  }

  // Cancel demo if player inserts coin
  if(keymask & BUTTON_COIN) demoActive = false;

  // Handle ALL read commands targeting chip 0 (51xx):
  // bit 0 = chip 0 selected, bit 4 = read mode.
  // This covers 0x71, 0xB1, 0x31, 0x51 etc. (different NMI rates, same I/O)
  if((namco_command & 0x11) == 0x11) {
    if(namco_mode) {
      // 51xx game mode output format (all active-LOW except byte 0):
      //   Byte 0: coin/start events (active-HIGH, 0x00 = nothing)
      //   Byte 1: full ~retval inversion (0xFF = nothing, bit goes LOW when active)
      //     Bits 0-3: directions, Bits 4-5: fire, Bits 6-7: P2 (always HIGH=inactive)
      //   Byte 2: player 2 (0xFF = no P2)

      // Demo auto-play
      if(demoActive) {
        demoFrame++;
        if((Addr & 15) == 0) return 0x00;  // no coin/start during demo
        if((Addr & 15) == 1) {
          unsigned char retval = 0;
          int phase = (demoFrame / 25) % 12;
          switch(phase) {
            case 0: case 1:  retval = 0x01; break;           // up
            case 2:          retval = 0x01 | 0x02; break;    // up-right
            case 3:          retval = 0x20; break;            // fire
            case 4: case 5:  retval = 0x02; break;            // right
            case 6:          retval = 0x20; break;            // fire
            case 7: case 8:  retval = 0x01 | 0x08; break;    // up-left
            case 9:          retval = 0x20; break;            // fire
            case 10:         retval = 0x08; break;            // left
            case 11:         retval = 0x01 | 0x20; break;    // up + fire
          }
          return ~retval;  // full inversion: 0xFF=nothing, active-LOW
        }
        if((Addr & 15) == 2) return 0xFF;  // no P2
      }

      // Byte 0: coin/start events (active-HIGH, 0x00 = nothing)
      // NOTE: byte 0 stays active-HIGH! Changing to ~retval makes game see
      // constant COIN+START (0xFF) and demo starts immediately.
      if((Addr & 15) == 0) {
        unsigned char byte0 = 0x00;
        if(keymask_p & BUTTON_COIN)  byte0 |= 0x01;
        if(keymask_p & BUTTON_START) byte0 |= 0x04;
        return byte0;
      }
      // Byte 1: directions + fire + P2 (ALL active-LOW via ~retval)
      // 0xFF = nothing pressed. Bit goes LOW when active.
      // BUG FIX: old format had upper nibble=0x00 when idle → game saw
      // fire always pressed (active-LOW) + P2 buttons active → LEFT drift!
      if((Addr & 15) == 1) {
        unsigned char retval = 0x00;
        if(keymask & BUTTON_UP)        retval |= 0x01;
        if(keymask & BUTTON_DOWN)      retval |= 0x04;
        if(keymask & BUTTON_LEFT)      retval |= 0x08;
        if(keymask & BUTTON_RIGHT)     retval |= 0x02;
        if(keymask_d[1] & BUTTON_FIRE) retval |= 0x20;  // level (prev frame)
        if(keymask_p & BUTTON_FIRE)    retval |= 0x10;  // edge (just pressed)
        return ~retval;
      }
      // Byte 2: player 2 (0xFF = no P2 controls)
      return 0xFF;

    } else {
      // Credit mode (0xC1/0xE1) - 51xx output format:
      //   Byte 0: BCD credit count (no inversion)
      //   Byte 1 lower nibble: encoded direction (MAME joy_map), NOT inverted
      //   Byte 1 bits 4-5: fire (active-HIGH), NOT inverted
      //   Byte 2: 0xF8 (cabinet/service status)
      if((Addr & 15) == 0) {
        return 16*(namco_credit/10) + namco_credit % 10;
      }
      if((Addr & 15) == 1) {
        // MAME joy_map: maps raw 4-bit direction (UP=1,R=2,D=4,L=8) to encoded nibble
        static const unsigned char joy_map[16] = {
          0x0F, 0x06, 0x02, 0x04, 0x0A, 0x07, 0x03, 0x05,
          0x00, 0x08, 0x01, 0x09, 0x0D, 0x0B, 0x0E, 0x0C
        };
        unsigned char raw_dir = 0;
        if(keymask & BUTTON_UP)    raw_dir |= 0x01;
        if(keymask & BUTTON_RIGHT) raw_dir |= 0x02;
        if(keymask & BUTTON_DOWN)  raw_dir |= 0x04;
        if(keymask & BUTTON_LEFT)  raw_dir |= 0x08;
        unsigned char fire = 0;
        if(keymask & BUTTON_FIRE)   fire |= 0x20;
        if(keymask_p & BUTTON_FIRE) fire |= 0x10;
        return joy_map[raw_dir & 0x0F] | fire;
      }
      return 0xF8;  // byte 2: cabinet status
    }
  }

  // DIP switches via 06xx (command 0xD2)
  // DIP values are already in active-LOW convention (1=OFF, 0=ON).
  // Return them directly - do NOT invert, or Freeze/ServiceMode activate!
  if(namco_command == 0xd2) {
    if((Addr & 15) == 0) return XEVIOUS_DIP_A;
    if((Addr & 15) == 1) return XEVIOUS_DIP_B;
    return 0xFF;
  }

  // 50xx score/protection (command 0x74 = read from 50xx via 06xx slot 2)
  // Protection check must stay active even after boot: the game may verify
  // 50xx responses during attract mode transitions (demo game init).
  // Offset 0: echo last written value (handshake)
  // Offset 3: protection response (0x05 for init commands, 0x95 default)
  // Other offsets: 0x00 (no score updates)
  if(namco_command == 0x74) {
    if((Addr & 15) == 0) return namco_customio[0];
    if((Addr & 15) == 3) {
      if(namco_customio[0] == 0x80 || namco_customio[0] == 0x10)
        return 0x05;
      return 0x95;
    }
    return 0x00;
  }

  // Write-mode commands: reading returns bus float (0xFF)
  if(namco_command == 0x08 || namco_command == 0xc1)
    return 0xff;

  return 0xFF;
}

// ============================================================================
// Sprite preprocessing - MAME xevious sprite attribute layout:
//   sr3+0x780 (memory[0x1780]) = spriteram   (code, color, visibility)
//   sr1+0x780 (memory[0x0780]) = spriteram_2 (positions)
//   sr2+0x780 (memory[0x0F80]) = spriteram_3 (flip, size)
//
// MAME draw_sprites:
//   visibility: (sr3[offs+1] & 0x40) == 0
//   code:       sr3[offs]
//   color:      sr3[offs+1] & 0x7F
//   sy:         223 - sr1[offs]                    (224-pixel dimension = display x)
//   sx:         sr1[offs+1] - 40 + 256*(sr2[offs+1]&1)  (288-pixel dimension = display y)
//   flipx:      sr2[offs] & 4  (native)
//   flipy:      sr2[offs] & 8  (native)
//   sizex:      sr2[offs] & 2  (native double-width -> display double-height)
//   sizey:      sr2[offs] & 1  (native double-height -> display double-width)
// ============================================================================
void xevious::prepare_frame(void) {
  active_sprites = 0;

  for(int idx = 0; idx < 64 && active_sprites < 124; idx++) {
    int offs = 2 * idx;
    unsigned char *sr1_spr = memory + 0x0780;  // positions
    unsigned char *sr2_spr = memory + 0x0F80;  // flip/size
    unsigned char *sr3_spr = memory + 0x1780;  // code/color/visibility

    // Visibility check: MAME uses (sr3[offs+1] & 0x40) == 0
    if((sr3_spr[offs + 1] & 0x40) == 0) {
      struct sprite_S spr;

      // Code from sr3, with bank bit from sr2 (MAME xevious draw_sprites)
      unsigned char sr2_val = sr2_spr[offs];
      if(sr2_val & 0x80) {
        // Bank bit set: 6-bit code + 0x100
        spr.code = (sr3_spr[offs] & 0x3F) + 0x100;
      } else {
        spr.code = sr3_spr[offs];
      }

      // Color from sr3 (7-bit in MAME, but our colormap has 64 entries, so mask to 6 bits)
      spr.color = sr3_spr[offs + 1] & 0x3F;

      // Flip/size from sr2 - remap MAME bit positions to our format:
      //   sr2[offs] bit 2 (flipx native) -> our bit 0 (flip variant selection)
      //   sr2[offs] bit 3 (flipy native) -> our bit 1 (flip variant selection)
      //   sr2[offs] bit 1 (double height) -> our bit 2
      //   sr2[offs] bit 0 (double width)  -> our bit 3
      spr.flags = 0;
      if(sr2_val & 0x04) spr.flags |= 0x01;  // flipx -> bit 0
      if(sr2_val & 0x08) spr.flags |= 0x02;  // flipy -> bit 1
      if(sr2_val & 0x02) spr.flags |= 0x04;  // double-height -> bit 2
      if(sr2_val & 0x01) spr.flags |= 0x08;  // double-width -> bit 3
      spr.flags ^= 0x01;
      // Positions from sr1
      // MAME: sy = 223 - sr1[offs], sx = sr1[offs+1] - 40 + 256*(sr2[offs+1]&1)
      spr.x = 223 - (short)sr1_spr[offs];
      spr.y = (short)sr1_spr[offs + 1] - 40 + 256 * (short)(sr2_spr[offs + 1] & 1);

      // Align code for multi-tile sprites
      // MAME: if(sizex) code &= ~1; if(sizey) code &= ~2;
      // MAME sizex = sr2 bit 1 = our flag 0x04 (doubles in MAME sx = our spr.y)
      // MAME sizey = sr2 bit 0 = our flag 0x08 (doubles in MAME sy = our spr.x)
      if((spr.flags & 0x0c) == 0x0c) spr.code &= ~3;       // 32x32: both
      else if(spr.flags & 0x04)       spr.code &= ~1;       // MAME sizex: clear bit 0
      else if(spr.flags & 0x08)       spr.code &= ~2;       // MAME sizey: clear bit 1

      // Emit sub-sprites with MAME-correct code offsets.
      // MAME formula: code + (sizex - MAME_x) + MAME_y * 2
      // MAME x iterates sx direction = our spr.y direction (y+0, y+16)
      // MAME y iterates sy direction = our spr.x direction (x+0, x+16)
      int sizex = (spr.flags & 0x04) ? 1 : 0;

      // Sub 0: (x, y) - always. MAME (x=0,y=0): offset = sizex
      if((spr.y > -16) && (spr.y < 288) &&
         (spr.x > -16) && (spr.x < 224)) {
        sprite[active_sprites] = spr;
        sprite[active_sprites].code += sizex;
        active_sprites++;
      }

      // Sub 1: (x+16, y) - if MAME sizey (flag 0x08)
      // MAME (x=0, y=1): offset = sizex + 2
      if((spr.flags & 0x08) &&
         (spr.y > -16) && (spr.y < 288) &&
         ((spr.x+16) > -16) && ((spr.x+16) < 224)) {
        sprite[active_sprites] = spr;
        sprite[active_sprites].x += 16;
        sprite[active_sprites].code += sizex + 2;
        active_sprites++;
      }

      // Sub 2: MAME sizex (flag 0x04) - extends in +native_x = +display_y (downward)
      // MAME (x=1, y=0): offset = sizex - 1 = 0
      if((spr.flags & 0x04) &&
         ((spr.y+16) > -16) && ((spr.y+16) < 288) &&
         (spr.x > -16) && (spr.x < 224)) {
        sprite[active_sprites] = spr;
        sprite[active_sprites].code += sizex - 1;
        sprite[active_sprites].y += 16;
        active_sprites++;
      }

      // Sub 3: 32x32 (both flags) - extends in +native_x and +native_y
      // MAME (x=1, y=1): offset = sizex + 1
      if(((spr.flags & 0x0c) == 0x0c) &&
         ((spr.y+16) > -16) && ((spr.y+16) < 288) &&
         ((spr.x+16) > -16) && ((spr.x+16) < 224)) {
        sprite[active_sprites] = spr;
        sprite[active_sprites].code += sizex + 1;
        sprite[active_sprites].x += 16;
        sprite[active_sprites].y += 16;
        active_sprites++;
      }
    }
  }
}

// ============================================================================
// Background rendering (scrolling BG tiles)
//
// BG tilemap: 64 cols x 32 rows (scan_rows: addr = row*64 + col)
// BG VRAM: memory[0x3800] = tile codes, memory[0x2800] = color attrs
//
// MAME bg_get_tile_info:
//   code = videoram[tile_index] | ((colorram[tile_index] & 0x01) << 8)  (9-bit)
//   color = ((attr & 0x3c) >> 2) | ((code & 0x80) >> 3) | ((attr & 0x03) << 5)
//   flip  = TILE_FLIPYX((attr & 0xc0) >> 6)
//
// MAME scroll: set_scrolldx(-20), set_scrolldy(-16)
//   effective = raw_scroll + scrolld
// ============================================================================
void xevious::blit_bg_row(short row) {
  // MAME: effective = raw + scrolld. BG scrolldx=-20, BG scrolldy=-16
  int hw_scroll_x = (scrollBgX & 0x1FF) - 20;
  int hw_scroll_y = (scrollBgY & 0xFF) - 16;

  for(char r = 0; r < 8; r++) {
    int native_x = row * 8 + r;
    int tm_px = (native_x + hw_scroll_x) & 0x1FF;
    int tile_col = tm_px >> 3;
    int rotated_row = tm_px & 7;

    unsigned short *ptr = frame_buffer + r * 224;

    for(char col = 0; col < 28; col++) {
      int tm_py = ((27 - col) * 8 + hw_scroll_y) & 0xFF;
      int tile_row = tm_py >> 3;
      int sub_y = tm_py & 7;

      unsigned short bgAddr = tile_row * 64 + tile_col;
      unsigned char tileCode = memory[0x3800 + bgAddr];   // BG videoram
      unsigned char tileAttr = memory[0x2800 + bgAddr];   // BG colorram
      unsigned short tileIdx = tileCode | ((tileAttr & 0x01) << 8);

      // MAME color formula: ((attr & 0x3c) >> 2) | ((code & 0x80) >> 3) | ((attr & 0x03) << 5)
      unsigned char colorGroup = ((tileAttr & 0x3c) >> 2)
                               | ((tileCode & 0x80) >> 3)
                               | ((tileAttr & 0x03) << 5);

      const unsigned short *tile = BGTILES_ARR[tileIdx & 0x1FF];
      const unsigned short *colors = CMAP_BG_ARR[colorGroup & 0x7F];

      // BG flip from attr bits 6-7 (MAME: TILE_FLIPYX((attr & 0xc0) >> 6))
      //   attr bit 6 = TILE_FLIPX (native horizontal flip) → display row index (rotated)
      //   attr bit 7 = TILE_FLIPY (native vertical flip) → display pixel order (rotated)
      int disp_flipx = (tileAttr >> 7) & 1;  // TILE_FLIPY → pixel order
      int disp_flipy = (tileAttr >> 6) & 1;  // TILE_FLIPX → row index

      int actual_row = disp_flipy ? (7 - rotated_row) : rotated_row;

      // ROT90 CW: screen LEFT = high native_y = MSB. Normal: MSB-first. Flip: LSB-first.
      if(sub_y == 0) {
        // Y-aligned: all 8 pixels from one tile row
        unsigned short pix = tile[actual_row];
        if(disp_flipx) {
          for(char c = 0; c < 8; c++, pix >>= 2)
            *ptr++ = colors[pix & 3];
        } else {
          for(char c = 0; c < 8; c++) {
            *ptr++ = colors[(pix >> 14) & 3];
            pix <<= 2;
          }
        }
      } else {
        // Y-misaligned: pixels split across current tile and next tile
        int nextTileRow = (tile_row + 1) & 31;
        unsigned short nextAddr = nextTileRow * 64 + tile_col;
        unsigned char nextCode = memory[0x3800 + nextAddr];
        unsigned char nextAttr = memory[0x2800 + nextAddr];
        unsigned short nextIdx = nextCode | ((nextAttr & 0x01) << 8);
        unsigned char nextClrGrp = ((nextAttr & 0x3c) >> 2)
                                 | ((nextCode & 0x80) >> 3)
                                 | ((nextAttr & 0x03) << 5);
        const unsigned short *nextTile = BGTILES_ARR[nextIdx & 0x1FF];
        const unsigned short *nextColors = CMAP_BG_ARR[nextClrGrp & 0x7F];
        int nextDispFlipX = (nextAttr >> 7) & 1;
        int nextDispFlipY = (nextAttr >> 6) & 1;
        int nextActualRow = nextDispFlipY ? (7 - rotated_row) : rotated_row;

        for(char c = 0; c < 8; c++) {
          int s = sub_y + c;  // native sub_y position
          if(s >= 8) {
            // Pixel in next tile
            int sn = s - 8;
            int bp = nextDispFlipX ? sn : (7 - sn);
            *ptr++ = nextColors[(nextTile[nextActualRow] >> (bp * 2)) & 3];
          } else {
            // Pixel in current tile
            int bp = disp_flipx ? s : (7 - s);
            *ptr++ = colors[(tile[actual_row] >> (bp * 2)) & 3];
          }
        }
      }
    }
  }
}

// ============================================================================
// FG character rendering (overlay with transparency)
//
// FG tilemap: 64 cols x 32 rows (scan_rows: addr = row*64 + col)
// FG VRAM: memory[0x3000] = codes, memory[0x2000] = color attrs
//
// MAME fg_get_tile_info:
//   code = videoram[tile_index]     (8-bit)
//   color = ((attr & 0x03) << 4) | ((attr & 0x3c) >> 2)  (6-bit, 0-63)
//   flip  = TILE_FLIPYX((attr & 0xc0) >> 6)
//
// FG scroll: from D010-D03F registers (usually 0) + scrolld offset
// ============================================================================
void xevious::blit_tile(short row, char col) {
  // FG scroll: effective = raw + scrolld. FG scrolldx=-32, scrolldy=-18
  // NOTE: FG Y rounded to -16 (nearest 8-pixel boundary) because our FG renderer
  // doesn't handle sub-tile Y splits. MAME's -18 causes 6px misalignment that garbles
  // FG tiles. -16 is only 2px off from MAME-correct but keeps tiles aligned.
  int fg_scroll_x = (scrollFgX & 0x1FF) - 32;
  int fg_scroll_y = (scrollFgY & 0xFF) - 16;

  int tm_py = ((27 - col) * 8 + fg_scroll_y) & 0xFF;
  int tile_row = tm_py >> 3;

  unsigned short *ptr = frame_buffer + 8 * col;

  for(char r = 0; r < 8; r++, ptr += (224 - 8)) {
    int native_x = row * 8 + r;
    int tm_px = (native_x + fg_scroll_x) & 0x1FF;
    int tile_col = tm_px >> 3;
    int rotated_row = tm_px & 7;

    unsigned short addr = tile_row * 64 + tile_col;
    unsigned char code = memory[0x3000 + addr];     // FG videoram
    unsigned char colorAttr = memory[0x2000 + addr]; // FG colorram

    // MAME FG color: ((attr & 0x03) << 4) | ((attr & 0x3c) >> 2)  (6-bit, 0-63)
    unsigned char colorGroup = ((colorAttr & 0x03) << 4) | ((colorAttr & 0x3c) >> 2);

    // MAME FG flip: TILE_FLIPYX((attr & 0xc0) >> 6)
    //   attr bit 6 = TILE_FLIPX (native horizontal flip) → display row index (rotated)
    //   attr bit 7 = TILE_FLIPY (native vertical flip) → display pixel order (rotated)
    unsigned char disp_flipx = (colorAttr >> 7) & 1;  // TILE_FLIPY → pixel order
    unsigned char disp_flipy = (colorAttr >> 6) & 1;  // TILE_FLIPX → row index

    const unsigned short *tile = TILEMAP_ARR[code];
    const unsigned short *colors = CMAP_FG_ARR[colorGroup & 0x3F];

    int actual_row = disp_flipy ? (7 - rotated_row) : rotated_row;

    // ROT90 CW: screen LEFT = high native_y = MSB. Normal: MSB-first. Flip: LSB-first.
    unsigned short pix = tile[actual_row];
    if(disp_flipx) {
      for(char c = 0; c < 8; c++) {
        if(pix & 3) *ptr = colors[pix & 3];
        ptr++;
        pix >>= 2;
      }
    } else {
      for(char c = 0; c < 8; c++) {
        unsigned char px = (pix >> 14) & 3;
        if(px) *ptr = colors[px];
        ptr++;
        pix <<= 2;
      }
    }
  }
}

// ============================================================================
// Sprite rendering (3bpp - MAME spritelayout_xevious)
// Uses explicit horizontal clipping to prevent buffer underflow when
// sprite.x < 0 (the old mask-based approach wrote to memory before
// frame_buffer when colors[0] != 0, corrupting heap/data).
// ============================================================================
void xevious::blit_sprite(short row, unsigned char s) {
  const unsigned long long *spr = SPRITES_ARR[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = CMAP_SPRITES_ARR[sprite[s].color & 63];

  // Horizontal clipping: compute visible pixel range [px_start, px_end)
  int sx = sprite[s].x;
  int px_start = 0;
  int px_end = 16;
  if(sx < 0)        { px_start = -sx; }
  if(sx + 16 > 224) { px_end = 224 - sx; }
  if(px_start >= px_end) return;
  int draw_width = px_end - px_start;

  short y_offset = sprite[s].y - 8*row;

  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16+y_offset;

  unsigned short startline = 0;
  if(y_offset > 0) {
    startline = y_offset;
    lines2draw = 8 - y_offset;
  }

  if(y_offset < 0) spr -= y_offset;

  // ptr always starts at valid buffer position (clamped to 0)
  unsigned short *ptr = frame_buffer + (sx < 0 ? 0 : sx) + 224*startline;

  for(char r = 0; r < lines2draw; r++, ptr += (224 - draw_width)) {
    unsigned long long pix = *spr++;
    // LSB-first extraction, 3bpp (pixel 0 at bits 0-2).
    // Skip left-clipped pixels by shifting right.
    pix >>= (px_start * 3);
    for(int c = px_start; c < px_end; c++) {
      unsigned short col = colors[pix & 7];
      if(col) *ptr = col;
      ptr++;
      pix >>= 3;
    }
  }
}

// ============================================================================
// Row rendering: BG (scrolling) -> Sprites -> FG (overlay)
// ============================================================================
void xevious::render_row(short row) {
  blit_bg_row(row);

  for(unsigned char s = 0; s < active_sprites; s++) {
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }

  for(char col = 0; col < 28; col++)
    blit_tile(row, col);
}

const signed char * xevious::waveRom(unsigned char value) {
  return WAVE_ARR[value];
}

const unsigned short *xevious::logo(void) {
  return LOGO_ARR;
}
