#include "1942.h"
#include "../../arcade_undef.h"

#define ROM_CPU1_ARR       _1942_rom_cpu1
#define ROM_CPU2_ARR       _1942_rom_cpu2
#define ROM_CPU1_B0_ARR    _1942_rom_cpu1_b0
#define ROM_CPU1_B1_ARR    _1942_rom_cpu1_b1
#define ROM_CPU1_B2_ARR    _1942_rom_cpu1_b2
#define CHARMAP_ARR        _1942_charmap
#define TILEMAP_ARR        _1942_tilemap
#define SPRITES_ARR        _1942_sprites
#define CMAP_CHARS_ARR     _1942_colormap_chars
#define CMAP_SPRITES_ARR   _1942_colormap_sprites
#define CMAP_TILES_ARR     _1942_colormap_tiles
#define LOGO_ARR           _1942_logo

unsigned char _1942::opZ80(unsigned short Addr) {
  if (current_cpu == 0)
    return ROM_CPU1_ARR[Addr];
  else
    return ROM_CPU2_ARR[Addr];
}

unsigned char _1942::rdZ80(unsigned short Addr) {
  if(current_cpu == 0) {
    if(Addr < 32768) return ROM_CPU1_ARR[Addr];
    if((Addr & 0xc000) == 0x8000) {
      if(_1942_bank == 0)                           return ROM_CPU1_B0_ARR[Addr - 0x8000];
      else if((_1942_bank == 1) && (Addr < 0xb000)) return ROM_CPU1_B1_ARR[Addr - 0x8000];
      else if(_1942_bank == 2)                       return ROM_CPU1_B2_ARR[Addr - 0x8000];
    }
    if((Addr & 0xf000) == 0xe000) return memory[Addr - 0xe000];
    if((Addr & 0xff80) == 0xcc00) return memory[Addr - 0xcc00 + 0x2400];
    if((Addr & 0xf800) == 0xd000) return memory[Addr - 0xd000 + 0x1000];
    if((Addr & 0xfc00) == 0xd800) return memory[Addr - 0xd800 + 0x2000];
    if(Addr == 0xc000) {
      unsigned char keymask = input->buttons_get();
      unsigned char retval = 0xff;
      static unsigned char last_coin = 0;
      if(keymask & BUTTON_COIN && !last_coin) retval &= ~0x80;
      if(keymask & BUTTON_START) retval &= ~0x01;
      last_coin = keymask & BUTTON_COIN;
      return retval;
    }
    if(Addr == 0xc001) {
      unsigned char keymask = input->buttons_get();
      unsigned char retval = 0xff;
      if(keymask & BUTTON_RIGHT) retval &= ~0x01;
      if(keymask & BUTTON_LEFT)  retval &= ~0x02;
      if(keymask & BUTTON_DOWN)  retval &= ~0x04;
      if(keymask & BUTTON_UP)    retval &= ~0x08;
      if(keymask & BUTTON_FIRE)  retval &= ~0x10;
      if(keymask & BUTTON_START) retval &= ~0x20;
      return retval;
    }
    if(Addr == 0xc002) return 0xff;
    if(Addr == 0xc003) return (unsigned char)~_1942_DIP_A;
    if(Addr == 0xc004) return (unsigned char)~_1942_DIP_B;
  } else {
    if(Addr < 16384) return ROM_CPU2_ARR[Addr];
    if((Addr & 0xf800) == 0x4000) return memory[Addr - 0x4000 + 0x1800];
    if(Addr == 0x6000) return _1942_sound_latch;
  }
  return 0xff;
}

void _1942::wrZ80(unsigned short Addr, unsigned char Value) {
  if(current_cpu == 0) {
    if((Addr & 0xf000) == 0xe000) { memory[Addr - 0xe000] = Value; return; }
    if((Addr & 0xff80) == 0xcc00) { memory[Addr - 0xcc00 + 0x2400] = Value; return; }
    if((Addr & 0xf800) == 0xd000) { memory[Addr - 0xd000 + 0x1000] = Value; return; }
    if((Addr & 0xfc00) == 0xd800) { memory[Addr - 0xd800 + 0x2000] = Value; return; }
    if((Addr & 0xfff0) == 0xc800) {
      if(Addr == 0xc800) { _1942_sound_latch = Value; return; }
      if(Addr == 0xc802) { _1942_scroll = Value | (_1942_scroll & 0xff00); return; }
      if(Addr == 0xc803) { _1942_scroll = (Value << 8)|(_1942_scroll & 0xff); return; }
      if(Addr == 0xc804) {
        sub_cpu_reset = Value & 0x10;
        if(sub_cpu_reset) { current_cpu = 1; ResetZ80(&cpu[1]); }
        return;
      }
      if(Addr == 0xc805) { _1942_palette = Value; return; }
      if(Addr == 0xc806) { _1942_bank = Value; return; }
    }
  } else {
    if((Addr & 0xf800) == 0x4000) { memory[Addr - 0x4000 + 0x1800] = Value; return; }
    if((Addr & 0x8000) == 0x8000) {
      int ay = (Addr & 0x4000) != 0;
      if(Addr & 1) soundregs[16 * ay + _1942_ay_addr[ay]] = Value;
      else         _1942_ay_addr[ay] = Value & 15;
      return;
    }
  }
}

void _1942::run_frame(void) {
  current_cpu = 0;
  game_started = 1;
  for(char f = 0; f < 4; f++) {
    for(int i = 0; i < INST_PER_FRAME / 3; i++) {
      current_cpu = 0;
      StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
      if(!sub_cpu_reset) {
        current_cpu = 1;
        StepZ80(&cpu[1]); StepZ80(&cpu[1]); StepZ80(&cpu[1]);
      }
    }
    if(f & 1) {
      current_cpu = 0;
      IntZ80(&cpu[0], 0xc7 | ((f & 2) ? (1 << 3) : (2 << 3)));
    }
    if(!sub_cpu_reset) {
      current_cpu = 1;
      IntZ80(&cpu[1], INT_RST38);
    }
  }
}

void _1942::prepare_frame(void) {
  active_sprites = 0;
  for(int idx = 0; idx < 32 && active_sprites < 124; idx++) {
    struct sprite_S spr;
    unsigned char *sprite_base_ptr = memory + 0x2400 + 4 * (31 - idx);
    if(sprite_base_ptr[3] && sprite_base_ptr[2]) {
      spr.code = (sprite_base_ptr[0] & 0x7f) + 4*(sprite_base_ptr[1] & 0x20);
      spr.color = sprite_base_ptr[1] & 0x0f;
      spr.x = sprite_base_ptr[2] - 16;
      spr.y = 256 - (sprite_base_ptr[3] - 0x10 * (sprite_base_ptr[1] & 0x10));
      spr.flags = (sprite_base_ptr[0] & 0x80) >> 7;
      if((idx >= 8) && (idx < 16)) spr.flags |= 0x80;
      if((idx >= 0) && (idx < 8))  spr.flags |= 0x40;
      if((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224)) {
        sprite[active_sprites++] = spr;
        if(sprite_base_ptr[1] & 0xc0) {
          spr.x += 16; spr.code += 1;
          if(spr.x < 224) {
            sprite[active_sprites++] = spr;
            if(sprite_base_ptr[1] & 0x80) {
              spr.x += 16; spr.code += 1;
              if(spr.x < 224) {
                sprite[active_sprites++] = spr;
                spr.x += 16; spr.code += 1;
                if(spr.x < 224) sprite[active_sprites++] = spr;
              }
            }
          }
        }
      }
    }
  }
}

void _1942::blit_bgtile_row(short row) {
  row += 32 - 2;
  int line = (-8 * row + _1942_scroll) & 511;
  char yoffset = (16 - line) & 15;
  int lines2draw = (yoffset > 8) ? (16 - yoffset) : 8;
  unsigned short addr = 0x2000 + 32 * (((line - 1) & 511) / 16) + 1;
  for(char col = 0; col < 14; col++) {
    unsigned short *ptr = frame_buffer + 16 * col;
    unsigned char attr = memory[addr + col + 16];
    const unsigned short *colors = CMAP_TILES_ARR[_1942_palette][attr & 31];
    unsigned short chr = memory[addr + col] + ((attr & 0x80) <<1);
    const unsigned long *tile = TILEMAP_ARR[(attr >> 5) &3][chr] + 2 * yoffset;
    char r;
    for(r = 0; r < lines2draw; r++, ptr += (224 -16)) {
      unsigned long pix = *tile++;
      for(char c = 0; c < 8; c++, pix >>= 4) *ptr++ = colors[pix & 7];
      pix = *tile++;
      for(char c = 0; c < 8; c++,pix >>= 4) *ptr++ = colors[pix & 7];
    }
    if(r != 8) {
      int next_line = (line - 16) & 511;
      unsigned short next_addr = 0x2000 + 32 * (((line - 17) & 511) / 16) + 1;
      attr = memory[next_addr + col + 16];
      colors = CMAP_TILES_ARR[_1942_palette][attr & 31];
      chr = memory[next_addr + col] + ((attr & 0x80) << 1);
      tile = TILEMAP_ARR[(attr >> 5) & 3][chr];
      for(; r < 8; r++, ptr += (224 - 16)) {
        unsigned long pix = *tile++;
        for(char c = 0; c < 8; c++, pix >>= 4) *ptr++ = colors[pix & 7];
        pix = *tile++;
        for(char c = 0; c < 8; c++, pix >>= 4) *ptr++ = colors[pix & 7];
      }
    }
  }
}

void _1942::blit_tile(short row, char col) {
  unsigned short addr = tileaddr[35 - row][27 - col];
  unsigned short chr = memory[0x1000 + addr];
  if(chr == 0x30) return;
  unsigned char attr = memory[0x1400 + addr];
  if(attr & 0x80) chr += 256;
  const unsigned short *tile = CHARMAP_ARR[chr];
  const unsigned short *colors = CMAP_CHARS_ARR[attr & 63];
  unsigned short *ptr = frame_buffer + 8*col;
  for(char r = 0; r < 8; r++, ptr += (224 - 8)) {
    unsigned short pix = *tile++;
    for(char c = 0; c < 8; c++, pix >>= 2) {
      if(pix & 3) *ptr = colors[pix & 3];
      ptr++;
    }
  }
}

void _1942::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = SPRITES_ARR[sprite[s].code + 256 * (sprite[s].flags & 1)];
  const unsigned short *colors = CMAP_SPRITES_ARR[4 * (sprite[s].color & 15)];
  unsigned long mask[2] = { 0xffffffff, 0xffffffff };
  if(sprite[s].x < 0)        lsl64(mask, -sprite[s].x);
  if(sprite[s].x > 224 - 16) lsr64(mask,  sprite[s].x - (224 - 16));
  if(sprite[s].flags & 0x80) {
    if(sprite[s].x >= 113) return;
    if(sprite[s].x > 113 - 16) lsr64(mask, sprite[s].x - (113 -16));
  }
  if(sprite[s].flags & 0x40) {
    if(sprite[s].x <= 113 - 16) return;
    if(sprite[s].x < 113) lsl64(mask, 113 - sprite[s].x);
  }
  mask[0] = ~mask[0]; mask[1] = ~mask[1];
  short y_offset = sprite[s].y - 8 * row;
  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16 + y_offset;
  unsigned short startline = 0;
  if(y_offset > 0) { startline = y_offset; lines2draw = 8 - y_offset; }
  if(y_offset < 0) spr -= 2 * y_offset;
  unsigned short *ptr = frame_buffer + sprite[s].x + 224 * startline;
  for(char r = 0; r < lines2draw; r++, ptr += (224 - 16)) {
    unsigned long pix = *spr++ | mask[0];
    for(char c = 0; c < 8; c++, pix >>= 4) {
      if((pix & 15) != 15) *ptr = colors[pix & 15];
      ptr++;
    }
    pix = *spr++ | mask[1];
    for(char c = 0; c < 8; c++, pix >>= 4) {
      if((pix & 15) != 15) *ptr = colors[pix & 15];
      ptr++;
    }
  }
}

void _1942::render_row(short row) {
  if((row < 2) || (row >= 34)) return;
  blit_bgtile_row(row);
  for(unsigned char s = 0; s < active_sprites; s++) {
    if((sprite[s].y < 8 * (row + 1)) && ((sprite[s].y + 16) > 8 * row))
      blit_sprite(row, s);
  }
  for(char col = 0; col < 28; col++)
    blit_tile(row, col);
}

const unsigned short *_1942::logo(void) { return LOGO_ARR; }

void _1942::lsl64(unsigned long *mask, int pix) {
  if(pix < 8) mask[0] <<= 4 * pix;
  else { mask[0] = 0; mask[1] <<= 4 * (pix - 8); }
}

void _1942::lsr64(unsigned long *mask, int pix) {
  if(pix < 8) mask[1] >>= 4 * pix;
  else { mask[1] = 0; mask[0] >>= 4 * (pix - 8); }
}
