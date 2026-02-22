#include "mrtnt.h"
#include "../../arcade_undef.h"

#define ROM_ARR      mrtnt_rom
#define TILEMAP_ARR  mrtnt_tilemap
#define SPRITES_ARR  mrtnt_sprites
#define COLORMAP_ARR mrtnt_colormap
#define WAVE_ARR     mrtnt_wavetable
#define LOGO_ARR     mrtnt_logo

unsigned char mrtnt::opZ80(unsigned short Addr) { return ROM_ARR[Addr]; }

unsigned char mrtnt::rdZ80(unsigned short Addr) {
  Addr &= 0x7fff;
  if(Addr < 16384) return ROM_ARR[Addr];
  if((Addr & 0xf000) == 0x4000) return memory[Addr - 0x4000];
  if((Addr & 0xf000) == 0x5000) {
    unsigned char keymask = input->buttons_get();
    if(Addr == 0x5080) return MRTNT_DIP;
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

void mrtnt::wrZ80(unsigned short Addr, unsigned char Value) {
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
      if(soundregs[Addr - 0x5040] != Value & 0x0f)
        soundregs[Addr - 0x5040] = Value & 0x0f;
    }
    return;
  }
}

void mrtnt::outZ80(unsigned short Port, unsigned char Value) { irq_ptr = Value; }

void mrtnt::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
  if(irq_enable[0]) IntZ80(cpu, irq_ptr);
}

const unsigned short *mrtnt::tileRom(unsigned short addr) { return TILEMAP_ARR[memory[addr]]; }
const unsigned short *mrtnt::colorRom(unsigned short addr) { return COLORMAP_ARR[addr]; }
const unsigned long *mrtnt::spriteRom(unsigned char flags, unsigned char code) { return SPRITES_ARR[flags][code]; }
const signed char * mrtnt::waveRom(unsigned char value) { return WAVE_ARR[value]; }
const unsigned short *mrtnt::logo(void) { return LOGO_ARR; }
