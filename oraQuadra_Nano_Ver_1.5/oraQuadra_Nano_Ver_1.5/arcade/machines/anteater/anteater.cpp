#include "anteater.h"
#include "../../arcade_undef.h"

#define ROM_CPU1_ARR    anteater_rom_cpu1
#define ROM_CPU2_ARR    anteater_rom_cpu2
#define TILEMAP_ARR     anteater_tilemap
#define SPRITES_ARR     anteater_sprites
#define COLORMAP_ARR    anteater_colormap
#define LOGO_ARR        anteater_logo

void anteater::reset() {
  machineBase::reset();
  skipFirstInterrupt = 1;
  ignoreFireButton = 1;
  game_started = 1;
}

unsigned char anteater::opZ80(unsigned short Addr) {
  if (current_cpu == 0) return ROM_CPU1_ARR[Addr];
  else return ROM_CPU2_ARR[Addr];
}

unsigned char anteater::rdZ80(unsigned short Addr) {
  if(current_cpu == 0) {
    if(Addr <= 0x3fff) return ROM_CPU1_ARR[Addr];
    if((Addr & 0xf800) == 0x8000) return memory[Addr - 0x8000];
    if((Addr >= 0x8800) & (Addr <= 0x8bff)) return memory[Addr - 0x8800 + 0x800];
    if((Addr >= 0x9800) && (Addr <= 0x9803)) {
      unsigned char keymask = input->buttons_get();
      unsigned char retval = 0xff;
      if(Addr == 0x9800) {
        if(keymask & BUTTON_COIN)  retval &= ~0x40;
        if(keymask & BUTTON_LEFT)  retval &= ~0x20;
        if(keymask & BUTTON_RIGHT) retval &= ~0x10;
        if(keymask & BUTTON_DOWN)  retval &= ~0x08;
        if(keymask & BUTTON_UP)    retval &= ~0x04;
        if (ignoreFireButton & !(keymask & BUTTON_FIRE)) ignoreFireButton = 0;
        if(!ignoreFireButton && (keymask & BUTTON_FIRE)) retval &= ~0x01;
      }
      if(Addr == 0x9801) return ANTEATER_DIP1;
      if(Addr == 0x9802) {
        retval = ANTEATER_DIP2;
        if(keymask & BUTTON_START) retval += 0x00;
        else retval += 0x01;
        return retval;
      }
      if(Addr == 0x9803) return 0xff;
      return retval;
    }
    if((Addr >= 0xa000) & (Addr <= 0xa003)) return 0x00;
    if(Addr == 0xb000) return 0x00;
  } else {
    if(Addr < 4096) return ROM_CPU2_ARR[Addr];
    if((Addr & 0xf000) == 0x8000) return memory[Addr - 0x8000 + 0x0d00];
  }
  return 0xff;
}

void anteater::wrZ80(unsigned short Addr, unsigned char Value) {
  if(current_cpu == 0) {
    if((Addr & 0xf800) == 0x8000) { memory[Addr - 0x8000] = Value; return; }
    if((Addr >= 0x8800) & (Addr <= 0x8bff)) { memory[Addr - 0x8800 + 0x0800] = Value; return; }
    if((Addr >= 0x9000) & (Addr <= 0x90ff)) { memory[Addr - 0x9000 + 0x0c00] = Value; return; }
    if((Addr >= 0x9800) & (Addr <= 0x9803)) return;
    if((Addr >= 0xa000) & (Addr <= 0xa003)) {
      if(Addr == 0xa000) {
        if (Value == 0x3) snd_command = 0x6;
        else if (Value == 0x5) snd_command = 0x7;
        else if (Value == 0xa) snd_command = 0xb;
        else snd_command = Value;
      }
      if(Addr == 0xa001) snd_irq_state = Value & 8;
      return;
    }
    if(Addr == 0xa801) {
      irq_enable[0] = Value & 1;
      if (irq_enable[0] && skipFirstInterrupt) {
        irq_enable[0] = 0;
        skipFirstInterrupt = 0;
      }
      return;
    }
    if(Addr == 0xa803) { showCustomBackground = Value & 1; return; }
  } else {
    if((Addr & 0xf000) == 0x8000) { memory[Addr - 0x8000 + 0x0d00] = Value; return; }
    if((Addr & 0xf000) == 0x9000) {
      snd_filter = Value;
      konami_sound_filter(Addr, Value);
    }
  }
}

void anteater::konami_sound_filter(unsigned short offset, uint8_t data) {
  return;
}

void anteater::outZ80(unsigned short Port, unsigned char Value) {
  if((Port & 0xff) == 0x10) { snd_ay_port = Value & 15; return; }
  if((Port & 0xff) == 0x20) { soundregs[16 + snd_ay_port] = Value; return; }
  if((Port & 0xff) == 0x40) { snd_ay_port = Value & 15; return; }
  if((Port & 0xff) == 0x80) { soundregs[snd_ay_port] = Value; return; }
}

unsigned char anteater::inZ80(unsigned short Port) {
  if((Port & 0xff) == 0x20) {
    if(snd_ay_port < 14) return soundregs[16 + snd_ay_port];
  }
  if((Port & 0xff) == 0x80) {
    if(snd_ay_port < 14) return soundregs[snd_ay_port];
    if(snd_ay_port == 14) return snd_command;
  }
  return 0x0;
}

void anteater::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    current_cpu=0; StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    current_cpu=1; StepZ80(&cpu[1]); StepZ80(&cpu[1]);
    if(snd_irq_state) {
      snd_irq_state = 0;
      IntZ80(&cpu[1], INT_RST38);
    }
  }
  if(irq_enable[0]) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_NMI);
  }
}

void anteater::prepare_frame(void) {
  active_sprites = 0;
  for(int idx=7;idx>=0 && active_sprites < 92;idx--) {
    unsigned char *sprite_base_ptr = memory + 0xc40 + 4*idx;
    struct sprite_S spr;
    if(sprite_base_ptr[3]) {
      spr.x = sprite_base_ptr[0] - 16;
      spr.y = sprite_base_ptr[3] + 17;
      spr.color = sprite_base_ptr[2] & 7;
      spr.code = sprite_base_ptr[1] & 0x3f;
      spr.flags = ((sprite_base_ptr[1] & 0x80)?2:0);
      if((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224))
        sprite[active_sprites++] = spr;
    }
  }
}

void anteater::blit_tile(short row, char col) {
  if((row < 2) || (row >= 34)) return;
  unsigned short addr = tileaddr[row][col];
  const unsigned short *tile = tileRom(memory[0x0800 + addr]);
  int c = memory[0xc00 + 2 * (addr & 31) + 1] & 0x07;
  const unsigned short *colors = colorRom(c);
  unsigned short *ptr = frame_buffer + 8*col;
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++;
    for(char c=0;c<8;c++,pix>>=2) {
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);
      if(pix & 3) *ptr = colors[index];
      ptr++;
    }
  }
}

void anteater::blit_tile_scroll(short row, signed char col, short scroll) {
  if((row < 2) || (row >= 34)) return;
  unsigned short addr;
  unsigned short mask = 0xffff;
  int sub = scroll & 0x07;
  if(col >= 0) {
    addr = tileaddr[row][col];
    addr = (addr + ((scroll & ~7) << 2)) & 1023;
    if((sub != 0) && (col == 27)) mask = 0xffff >> (2*sub);
  } else {
    addr = tileaddr[row][0];
    addr = (addr + 32 + ((scroll & ~7) << 2)) & 1023;
    mask = 0xffff << (2*(8-sub));
  }
  const unsigned char chr = memory[0x0800 + addr];
  const unsigned short *tile = tileRom(chr);
  int c = memory[0xc00 + 2 * (addr & 31) + 1] & 7;
  const unsigned short *colors = colorRom(c);
  unsigned short *ptr = frame_buffer + 8*col + sub;
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++ & mask;
    for(char c=0;c<8;c++,pix>>=2) {
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);
      if(pix & 3) *ptr = colors[index];
      ptr++;
    }
  }
}

void anteater::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = spriteRom(sprite[s].flags, sprite[s].code);
  const unsigned short *colors = colorRom(sprite[s].color);
  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)      mask <<= -2*sprite[s].x;
  if(sprite[s].x > 224-16) mask >>= 2*(sprite[s].x-224-16);
  short y_offset = sprite[s].y - 8*row;
  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16+y_offset;
  unsigned short startline = 0;
  if(y_offset > 0) { startline = y_offset; lines2draw = 8 - y_offset; }
  if(y_offset < 0) spr -= y_offset;
  unsigned short *ptr = frame_buffer + sprite[s].x + 224*startline;
  for(char r=0;r<lines2draw;r++,ptr+=(224-16)) {
    unsigned long pix = *spr++ & mask;
    for(char c=0;c<16;c++,pix>>=2) {
      long index = ((pix & 2) >> 1) | ((pix & 1) << 1);
      unsigned short col = colors[index];
      if(pix & 3) *ptr = col;
      ptr++;
    }
  }
}

void anteater::render_row(short row) {
  if(row <= 1 || row >= 34) return;
  if (showCustomBackground && row >=2 && row <= 33)
    memset(frame_buffer, 8, 2 * 224 * 8);
  unsigned char scroll = memory[0xc00 + 2 * (row - 2)];
  if(scroll == 0)
    for(char col=0;col<28;col++) blit_tile(row, col);
  else {
    if(scroll & 7) blit_tile_scroll(row, -1, scroll);
    for(char col=0;col<28;col++) blit_tile_scroll(row, col, scroll);
  }
  for(unsigned char s=0;s<active_sprites;s++) {
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }
}

const unsigned short *anteater::tileRom(unsigned short addr) { return TILEMAP_ARR[addr]; }
const unsigned short *anteater::colorRom(unsigned short addr) { return COLORMAP_ARR[addr]; }
const unsigned long *anteater::spriteRom(unsigned char flags, unsigned char code) { return SPRITES_ARR[flags][code]; }
const unsigned short *anteater::logo(void) { return LOGO_ARR; }
