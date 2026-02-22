#include "lizwiz.h"
#include "../../arcade_undef.h"

#define ROM_ARR      lizwiz_rom
#define TILEMAP_ARR  lizwiz_tilemap
#define SPRITES_ARR  lizwiz_sprites
#define COLORMAP_ARR lizwiz_colormap
#define WAVE_ARR     lizwiz_wavetable
#define LOGO_ARR     lizwiz_logo

unsigned char lizwiz::opZ80(unsigned short Addr) {
  if(Addr < 16384) return ROM_ARR[Addr];
  else return ROM_ARR[Addr - 0x4000];
}

unsigned char lizwiz::rdZ80(unsigned short Addr) {
  if(Addr < 16384) return ROM_ARR[Addr];
  else if (Addr >= 0x8000 && Addr <= 0xbfff) return ROM_ARR[Addr - 0x4000];
  if((Addr & 0xf000) == 0x4000) return memory[Addr - 0x4000];
  if((Addr & 0xf000) == 0x5000) {
    unsigned char keymask = input->buttons_get();
    if(Addr == 0x5080) return LIZWIZ_DIP;
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

void lizwiz::wrZ80(unsigned short Addr, unsigned char Value) {
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

void lizwiz::outZ80(unsigned short Port, unsigned char Value) { irq_ptr = Value; }

void lizwiz::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
  if(irq_enable[0]) IntZ80(cpu, irq_ptr);
}

const unsigned short *lizwiz::tileRom(unsigned short addr) { return TILEMAP_ARR[memory[addr]]; }
const unsigned short *lizwiz::colorRom(unsigned short addr) { return COLORMAP_ARR[addr]; }
const unsigned long *lizwiz::spriteRom(unsigned char flags, unsigned char code) { return SPRITES_ARR[flags][code]; }
const signed char *lizwiz::waveRom(unsigned char value) { return WAVE_ARR[value]; }
const unsigned short *lizwiz::logo(void) { return LOGO_ARR; }
