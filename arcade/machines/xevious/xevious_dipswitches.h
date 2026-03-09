#ifndef _xevious_dipswitches_h_
#define _xevious_dipswitches_h_

// Xevious DIP Switches - ACTIVE LOW convention (1=OFF, 0=ON)
// Matches MAME PORT_DIPSETTING definitions in galaga.cpp

// DIP Switch A
// Bits 0-2: Coin A (default 0x07 = 1 coin 1 credit)
// Bits 3-5: Coin B (default 0x38 = 1 coin 1 credit)
// Bits 6-7: Lives
//   0x00 = 1 life, 0x40 = 2 lives, 0x80 = 3 lives, 0xC0 = 5 lives
#define XEVIOUS_DIP_A  0xBF   // 1C1C, 1C1C, 3 lives (MAME default)

// DIP Switch B
// Bits 0-1: Difficulty (0x03=Easy, 0x02=Normal, 0x01=Hard, 0x00=Hardest)
// Bits 2-4: Bonus Life (0x1C = "10K 40K Every 40K")
// Bit 5:    Unknown (0x20 = OFF)
// Bit 6:    Freeze (0x40 = OFF, 0x00 = ON)
// Bit 7:    Service Mode (0x80 = OFF, 0x00 = ON = test loop!)
#define XEVIOUS_DIP_B  0xFF   // All OFF: easy, no freeze, no service mode

#endif
