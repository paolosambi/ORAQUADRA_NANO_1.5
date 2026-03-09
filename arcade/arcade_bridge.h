#ifndef _ARCADE_BRIDGE_H_
#define _ARCADE_BRIDGE_H_

// ============================================================================
// ARCADE BRIDGE - Connects Z80/i8048 CPU emulators to oraQuadra machine classes
// This file replaces Galag's emulation.h for the oraQuadra port
// NOTE: This header is included from both C (Z80.c) and C++ files.
//       All C++ constructs MUST be wrapped in #ifdef __cplusplus
// ============================================================================

#include "cpus/z80/Z80.h"
#include "cpus/i8048/i8048.h"

// ============================================================================
// ArcadeInput - Replaces Galag's Input class (C++ only)
// Provides button state from oraQuadra touch screen
// ============================================================================

#ifdef __cplusplus

class ArcadeInput {
public:
  ArcadeInput() : buttonState(0) {}

  void setButtons(unsigned char state) { buttonState = state; }
  unsigned char buttons_get() { return buttonState; }

  void enable() {}
  void disable() {}

private:
  volatile unsigned char buttonState;
};

#endif // __cplusplus

// ============================================================================
// C callback declarations for Z80 emulator
// These are called by Z80.c and forward to the current machine object
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

void OutZ80(unsigned short Port, unsigned char Value);
unsigned char InZ80(unsigned short Port);
void WrZ80(unsigned short Addr, unsigned char Value);
unsigned char RdZ80(unsigned short Addr);
unsigned char OpZ80_INL(unsigned short Addr);
void PatchZ80(Z80 *R);
word LoopZ80(Z80 *R);

#ifdef __cplusplus
}
#endif

// ============================================================================
// C callback declarations for i8048 emulator (Donkey Kong audio CPU)
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

void i8048_port_write(i8048_state_S *state, unsigned char port, unsigned char pos);
unsigned char i8048_port_read(i8048_state_S *state, unsigned char port);
unsigned char i8048_xdm_read(i8048_state_S *state, unsigned char addr);
void i8048_xdm_write(i8048_state_S *state, unsigned char addr, unsigned char data);
unsigned char i8048_rom_read(i8048_state_S *state, unsigned short addr);

#ifdef __cplusplus
}
#endif

#endif // _ARCADE_BRIDGE_H_
