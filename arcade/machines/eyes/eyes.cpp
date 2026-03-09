#include "eyes.h"
#include "../../arcade_undef.h"

#define ROM_ARR      eyes_rom
#define TILEMAP_ARR  eyes_tilemap
#define SPRITES_ARR  eyes_sprites
#define COLORMAP_ARR eyes_colormap
#define WAVE_ARR     eyes_wavetable
#define LOGO_ARR     eyes_logo

unsigned char eyes::opZ80(unsigned short Addr) {
  return (Addr < 16384) ? pRom1[Addr] : 0xFF;
}

unsigned char eyes::rdZ80(unsigned short Addr) {
  Addr &= 0x7fff;
  if(Addr < 16384) {
    return pRom1[Addr];
  }
  if((Addr & 0xf000) == 0x4000) return memory[Addr - 0x4000];
  if((Addr & 0xf000) == 0x5000) {
    unsigned char keymask = input->buttons_get();
    if(Addr == 0x5080) return EYES_DIP;
    if(Addr == 0x5000) {
      unsigned char retval = 0xff;
      if(keymask & BUTTON_UP)    retval &= ~0x01;
      if(keymask & BUTTON_LEFT)  retval &= ~0x02;
      if(keymask & BUTTON_RIGHT) retval &= ~0x04;
      if(keymask & BUTTON_DOWN)  retval &= ~0x08;
      if(keymask & BUTTON_COIN)  retval &= ~0x20;
      return retval;
    }
    if(Addr == 0x5040) {
      unsigned char retval = 0xff;
      if(keymask & BUTTON_START) retval &= ~0x20;
      if(keymask & BUTTON_FIRE)  retval &= ~0x10;
      return retval;
    }
  }
  return 0xff;
}

void eyes::wrZ80(unsigned short Addr, unsigned char Value) {
  Addr &= 0x7fff;
  if((Addr & 0xf000) == 0x4000) {
    if(Addr == 0x4000 + 970 && Value == 80) game_started = 1;
    memory[Addr - 0x4000] = Value;
    return;
  }
  if((Addr & 0xff00) == 0x5000) {
    if((Addr & 0xfff0) == 0x5060) memory[Addr - 0x4000] = Value;
    if(Addr == 0x5000) irq_enable[0] = Value & 1;
    if((Addr & 0xffe0) == 0x5040) {
      if (Addr == 0x505f && Value == 7) Value = 1;
      if(soundregs[Addr - 0x5040] != Value & 0x0f)
        soundregs[Addr - 0x5040] = Value & 0x0f;
    }
    return;
  }
}

void eyes::outZ80(unsigned short Port, unsigned char Value) { irq_ptr = Value; }

void eyes::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
  if(irq_enable[0]) IntZ80(cpu, irq_ptr);
}

const unsigned short *eyes::tileRom(unsigned short addr) { return TILEMAP_ARR[memory[addr]]; }
const unsigned short *eyes::colorRom(unsigned short addr) { return COLORMAP_ARR[addr]; }
const unsigned long *eyes::spriteRom(unsigned char flags, unsigned char code) { return SPRITES_ARR[flags][code]; }
const signed char * eyes::waveRom(unsigned char value) { return WAVE_ARR[value]; }
const unsigned short *eyes::logo(void) { return LOGO_ARR; }
