#include "dkong.h"
#include "../../arcade_undef.h"

static inline unsigned short dkong_swap_rb(unsigned short c) {
  return (c & 0xE007) | ((c >> 5) & 0x00F8) | ((c << 5) & 0x1F00);
}

#define ROM_CPU1_ARR       dkong_rom_cpu1
#define ROM_CPU2_ARR       dkong_rom_cpu2
#define TILEMAP_ARR        dkong_tilemap
#define SPRITES_ARR        dkong_sprites
#define COLORMAP_ARR       dkong_colormap
#define CMAP_SPRITE_ARR    dkong_colormap_sprite
#define SAMPLE_WALK0_ARR   dkong_sample_walk0
#define SAMPLE_WALK0_SIZE  sizeof(dkong_sample_walk0)
#define SAMPLE_WALK1_ARR   dkong_sample_walk1
#define SAMPLE_WALK1_SIZE  sizeof(dkong_sample_walk1)
#define SAMPLE_WALK2_ARR   dkong_sample_walk2
#define SAMPLE_WALK2_SIZE  sizeof(dkong_sample_walk2)
#define SAMPLE_JUMP_ARR    dkong_sample_jump
#define SAMPLE_JUMP_SIZE   sizeof(dkong_sample_jump)
#define SAMPLE_STOMP_ARR   dkong_sample_stomp
#define SAMPLE_STOMP_SIZE  sizeof(dkong_sample_stomp)
#define LOGO_ARR           dkong_logo

void dkong::reset() {
  machineBase::reset();
  i8048_reset(&cpu_8048);
  memset(dkong_audio_transfer_buffer, 0, sizeof(dkong_audio_transfer_buffer));
}

void dkong::wrI8048_port(struct i8048_state_S *state, unsigned char port, unsigned char pos) {
  if(port == 0)
    return;
  else if(port == 1) {
    static int bptr = 0;
    dkong_audio_assembly_buffer[bptr++] = pos;
    if(bptr == 64) {
      bptr = 0;
      if(((dkong_audio_wptr + 1)&DKONG_AUDIO_QUEUE_MASK) == dkong_audio_rptr) {
      } else {
        memcpy(dkong_audio_transfer_buffer[dkong_audio_wptr], dkong_audio_assembly_buffer, 64);
        dkong_audio_wptr = (dkong_audio_wptr + 1)&DKONG_AUDIO_QUEUE_MASK;
      }
    }
  } else if(port == 2)
    state->p2_state = pos;
}

unsigned char dkong::rdI8048_port(struct i8048_state_S *state, unsigned char port) {
  if(port == 2) return state->p2_state;
  return 0;
}

unsigned char dkong::rdI8048_xdm(struct i8048_state_S *state, unsigned char addr) {
  if(state->p2_state & 0x40)
    return dkong_sfx_index ^ 0x0f;
  return pRom2[2048 + addr + 256 * (state->p2_state & 7)];
}

unsigned char dkong::rdI8048_rom(struct i8048_state_S *state, unsigned short addr) {
  return pRom2[addr];
}

unsigned char dkong::opZ80(unsigned short Addr) {
  return (Addr < 16384) ? pRom1[Addr] : 0xFF;
}

unsigned char dkong::rdZ80(unsigned short Addr) {
  if(Addr < 16384) {
    return pRom1[Addr];
  }
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000))
    return memory[Addr - 0x6000];
  if((Addr & 0xfff0) == 0x7c00) {
    unsigned char keymask = input->buttons_get();
    unsigned char retval = 0x00;
    if(keymask & BUTTON_RIGHT) retval |= 0x01;
    if(keymask & BUTTON_LEFT)  retval |= 0x02;
    if(keymask & BUTTON_UP)    retval |= 0x04;
    if(keymask & BUTTON_DOWN)  retval |= 0x08;
    if(keymask & BUTTON_FIRE)  retval |= 0x10;
    return retval;
  }
  if((Addr & 0xfff0) == 0x7c80) return 0x00;
  if((Addr & 0xfff0) == 0x7d00) {
    unsigned char keymask = input->buttons_get();
    unsigned char retval = 0x00;
    if(keymask & BUTTON_COIN)   retval |= 0x80;
    if(keymask & BUTTON_START)  retval |= 0x04;
    return retval;
  }
  if((Addr & 0xfff0) == 0x7d80)
    return DKONG_DIP;
  return 0xff;
}

void dkong::wrZ80(unsigned short Addr, unsigned char Value) {
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) {
    memory[Addr - 0x6000] = Value;
    return;
  }
  if((Addr & 0xfe00) == 0x7800) return;
  if((Addr & 0xfe00) == 0x7c00) {
    if(Addr == 0x7c00) dkong_sfx_index = Value;
    if((Addr & 0xfff0) == 0x7d00) {
      if((Addr & 0x0f) <= 2  && Value)
        trigger_sound(Addr & 0x0f);
      if((Addr & 0x0f) == 3) {
        if(Value & 1) cpu_8048.p2_state &= ~0x20;
        else          cpu_8048.p2_state |=  0x20;
      }
      if((Addr & 0x0f) == 4)
        cpu_8048.T1 = !(Value & 1);
      if((Addr & 0x0f) == 5)
        cpu_8048.T0 = !(Value & 1);
    }
    if(Addr == 0x7d80)
      cpu_8048.notINT = !(Value & 1);
    if(Addr == 0x7d84)
      irq_enable[0] = Value & 1;
    if((Addr == 0x7d85) && (Value & 1)) {
      memcpy(memory+0x1000, memory+0x900, 384);
    }
    if(Addr == 0x7d86) {
      colortable_select &= ~1;
      colortable_select |= (Value & 1);
    }
    if(Addr == 0x7d87) {
      colortable_select &= ~2;
      colortable_select |= ((Value<<1) & 2);
    }
    return;
  }
}

void dkong::run_frame(void) {
  current_cpu = 0;
  game_started = 1;
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
    if(((dkong_audio_wptr+1)&DKONG_AUDIO_QUEUE_MASK) != dkong_audio_rptr) {
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
      i8048_step(&cpu_8048); i8048_step(&cpu_8048);
    }
  }
  if(irq_enable[0])
    IntZ80(cpu, INT_NMI);
}

void dkong::trigger_sound(char snd) {
  const struct {
    const signed char *data;
    const unsigned short length;
  } samples[] = {
    { (const signed char *)SAMPLE_WALK0_ARR, (unsigned short)SAMPLE_WALK0_SIZE },
    { (const signed char *)SAMPLE_WALK1_ARR, (unsigned short)SAMPLE_WALK1_SIZE },
    { (const signed char *)SAMPLE_WALK2_ARR, (unsigned short)SAMPLE_WALK2_SIZE },
    { (const signed char *)SAMPLE_JUMP_ARR,  (unsigned short)SAMPLE_JUMP_SIZE  },
    { (const signed char *)SAMPLE_STOMP_ARR, (unsigned short)SAMPLE_STOMP_SIZE }
  };
  if(!snd) {
    char rnd = random() % 3;
    dkong_sample_cnt[0] = samples[rnd].length;
    dkong_sample_ptr[0] = samples[rnd].data;
  } else {
    dkong_sample_cnt[snd] = samples[snd+2].length;
    dkong_sample_ptr[snd] = samples[snd+2].data;
  }
}

void dkong::prepare_frame(void) {
  active_sprites = 0;
  for(int idx=0;idx<96 && active_sprites<92;idx++) {
    unsigned char *sprite_base_ptr = memory + 0x1000 + 4*idx;
    struct sprite_S spr;
    spr.x = sprite_base_ptr[0] - 23;
    spr.y = sprite_base_ptr[3] + 8;
    spr.code = sprite_base_ptr[1] & 0x7f;
    spr.color = sprite_base_ptr[2] & 0x0f;
    spr.flags =  ((sprite_base_ptr[2] & 0x80)?1:0) |
      ((sprite_base_ptr[1] & 0x80)?2:0);
    if((spr.y > -16) && (spr.y < 288) &&
       (spr.x > -16) && (spr.x < 224))
      sprite[active_sprites++] = spr;
  }
}

void dkong::blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];
  if((row < 2) || (row >= 34)) return;
  if(memory[0x1400 + addr] == 0x10) return;
  const unsigned short *tile = TILEMAP_ARR[memory[0x1400 + addr]];
  const unsigned short *src_colors = COLORMAP_ARR[colortable_select][row-2 + 32*(col/4)];
  unsigned short colors[4] = { dkong_swap_rb(src_colors[0]), dkong_swap_rb(src_colors[1]),
                                dkong_swap_rb(src_colors[2]), dkong_swap_rb(src_colors[3]) };
  unsigned short *ptr = frame_buffer + 8*col;
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++;
    for(char c=0;c<8;c++,pix>>=2) {
      if(pix & 3) *ptr = colors[pix&3];
      ptr++;
    }
  }
}

void dkong::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = SPRITES_ARR[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *src_colors = CMAP_SPRITE_ARR[colortable_select][sprite[s].color];
  unsigned short colors[4] = { dkong_swap_rb(src_colors[0]), dkong_swap_rb(src_colors[1]),
                                dkong_swap_rb(src_colors[2]), dkong_swap_rb(src_colors[3]) };
  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)      mask <<= -2*sprite[s].x;
  if(sprite[s].x > 224-16) mask >>= 2*(sprite[s].x-224-16);
  short y_offset = sprite[s].y - 8*row;
  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16+y_offset;
  unsigned short startline = 0;
  if(y_offset > 0) {
    startline = y_offset;
    lines2draw = 8 - y_offset;
  }
  if(y_offset < 0) spr -= y_offset;
  unsigned short *ptr = frame_buffer + sprite[s].x + 224*startline;
  for(char r=0;r<lines2draw;r++,ptr+=(224-16)) {
    unsigned long pix = *spr++ & mask;
    for(char c=0;c<16;c++,pix>>=2) {
      unsigned short col = colors[pix&3];
      if(pix & 3) *ptr = col;
      ptr++;
    }
  }
}

void dkong::render_row(short row) {
  for(char col=0;col<28;col++)
    blit_tile(row, col);
  for(unsigned char s=0;s<active_sprites;s++) {
    if((sprite[s].y < 8*(row+1)) && ((sprite[s].y+16) > 8*row))
      blit_sprite(row, s);
  }
}

const unsigned short *dkong::logo(void) {
  return LOGO_ARR;
}
