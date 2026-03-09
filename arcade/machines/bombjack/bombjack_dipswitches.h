// Bomb Jack DIP switch configuration
// SW1 at 0xB004, SW2 at 0xB005

#ifndef BOMBJACK_DIPSWITCHES_H
#define BOMBJACK_DIPSWITCHES_H

// DIP Switch 1 (0xB004) - Factory defaults (MAME verified)
// Bits 0-1: Coin A = 1C/1Cr (0x00)
// Bits 2-3: Coin B = 1C/1Cr (0x00)
// Bits 4-5: Lives = 3 (0x00)
// Bit 6:    Cabinet = Upright (0x40)
// Bit 7:    Demo Sounds = On (0x80)
#define BJ_DIPA  0xC0

// DIP Switch 2 (0xB005) - MAME defaults
// Bits 0-2: Bonus Life = Every 100k (0x02)
// Bit 3:    Bird Speed = Easy (0x00)
// Bits 4-5: Enemies = Medium (0x10)
// Bit 6:    Special Coin = Easy (0x00)
// Bit 7:    *** VBLANK signal *** (NOT a DIP switch!)
//           Active-LOW: 0 during VBLANK, 1 during visible area
//           Handled dynamically in rdZ80, not included here
#define BJ_DIPB  0x12

#endif
