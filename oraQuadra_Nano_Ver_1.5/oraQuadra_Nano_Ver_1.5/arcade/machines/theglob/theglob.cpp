#include "theglob.h"
#include "../../arcade_undef.h"

#define ROM_ARR      theglob_rom
#define TILEMAP_ARR  theglob_tilemap
#define SPRITES_ARR  theglob_sprites
#define COLORMAP_ARR theglob_colormap
#define WAVE_ARR     theglob_wavetable
#define LOGO_ARR     theglob_logo

void theglob::init(ArcadeInput *input, unsigned short *framebuffer, sprite_S *spritebuffer, unsigned char *memorybuffer) {
  machineBase::init(input, framebuffer, spritebuffer, memorybuffer);

  int bs[4][8] = {
    { 3,7,0,6,4,1,2,5 },
    { 1,7,0,3,4,6,2,5 },
    { 3,0,4,6,7,1,2,5 },
    { 1,0,4,3,7,6,2,5 },
  };

  decrypt_rom_buffer = (unsigned char*)ps_malloc(0x4000 * 4);

  epos_decrypt_rom(decrypt_rom_buffer, 0xfc, 0x0000, bs[0]);
  epos_decrypt_rom(decrypt_rom_buffer, 0xf6, 0x4000, bs[1]);
  epos_decrypt_rom(decrypt_rom_buffer, 0x7d, 0x8000, bs[2]);
  epos_decrypt_rom(decrypt_rom_buffer, 0x77, 0xc000, bs[3]);

  m_counter = 0x0A;
  decrypt_rom_index = m_counter & 3;
}

void theglob::reset() {
  machineBase::reset();
  m_counter = 0x0A;
  decrypt_rom_index = m_counter & 3;
}

void theglob::epos_decrypt_rom(uint8_t *ROM, uint8_t invert, int offset, int *bs) {
  for (int mem = 0; mem < 0x4000; mem++ ) {
    ROM[mem + offset] = bitswap<8>(ROM_ARR[mem] ^ invert, bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6], bs[7]);
  }
}

uint8_t theglob::epos_decryption_w(short offset) {
  if (offset & 0x01) {
    m_counter = (m_counter - 1) & 0x0F;
  } else {
    m_counter = (m_counter + 1) & 0x0F;
  }

  switch (m_counter) {
  case 0x08:
  case 0x09:
  case 0x0A:
  case 0x0B:
    decrypt_rom_index = m_counter & 3;
    break;
  default:
    break;
  }
  return 0;
}

unsigned char theglob::inZ80(unsigned short Port) {
  return epos_decryption_w(Port);
}

unsigned char theglob::opZ80(unsigned short Addr) {
  Addr &= 0x7fff;
  return decrypt_rom_buffer[(0x4000 * decrypt_rom_index) + Addr];
}

unsigned char theglob::rdZ80(unsigned short Addr) {
  Addr &= 0x7fff;
  if(Addr < 16384)
    return decrypt_rom_buffer[(0x4000 * decrypt_rom_index) + Addr];

  if((Addr & 0xf000) == 0x4000) return memory[Addr - 0x4000];

  if((Addr & 0xf000) == 0x5000) {
    game_started = 1;
    unsigned char keymask = input->buttons_get();
    if(Addr == 0x5080) return THEGLOB_DIP;
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
      if(keymask & BUTTON_COIN) retval &= ~0x20;
      if(keymask & BUTTON_FIRE) retval &= ~0x40;
      return retval;
    }
  }
  return 0x00;
}

void theglob::wrZ80(unsigned short Addr, unsigned char Value) {
  Addr &= 0x7fff;
  if((Addr & 0xf000) == 0x4000) {
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

void theglob::outZ80(unsigned short Port, unsigned char Value) { irq_ptr = Value; }

void theglob::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME; i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
  if(irq_enable[0]) IntZ80(cpu, irq_ptr);
}

const unsigned short *theglob::tileRom(unsigned short addr) { return TILEMAP_ARR[memory[addr]]; }
const unsigned short *theglob::colorRom(unsigned short addr) { return COLORMAP_ARR[addr]; }
const unsigned long *theglob::spriteRom(unsigned char flags, unsigned char code) { return SPRITES_ARR[flags][code]; }
const signed char *theglob::waveRom(unsigned char value) { return WAVE_ARR[value]; }
const unsigned short *theglob::logo(void) { return LOGO_ARR; }
