// ============================================================================
// ARCADE BRIDGE - C callback implementations for Z80/i8048 CPU emulators
// Forwards all CPU memory/IO access to the current machine object
// ============================================================================

#include "Arduino.h"
#include "machines/machineBase.h"

// The current machine is set by 51_ARCADE.ino when a game is started
extern machineBase *arcadeCurrentMachine;

// ============================================================================
// Z80 CPU Callbacks - Called by Z80.c during instruction execution
// ============================================================================

unsigned char OpZ80_INL(unsigned short Addr) {
  return arcadeCurrentMachine->opZ80(Addr);
}

void OutZ80(unsigned short Port, unsigned char Value) {
  arcadeCurrentMachine->outZ80(Port, Value);
}

unsigned char InZ80(unsigned short Port) {
  return arcadeCurrentMachine->inZ80(Port);
}

void WrZ80(unsigned short Addr, unsigned char Value) {
  arcadeCurrentMachine->wrZ80(Addr, Value);
}

unsigned char RdZ80(unsigned short Addr) {
  return arcadeCurrentMachine->rdZ80(Addr);
}

void PatchZ80(Z80 *R) {
  // No patching needed
}

word LoopZ80(Z80 *R) {
  // Not used - we call StepZ80() directly, not RunZ80()
  // But must exist for linker
  return 0xFFFF; // INT_NONE
}

// ============================================================================
// i8048 CPU Callbacks - Called by i8048.c (Donkey Kong audio CPU)
// ============================================================================

void i8048_port_write(i8048_state_S *state, unsigned char port, unsigned char pos) {
  arcadeCurrentMachine->wrI8048_port(state, port, pos);
}

unsigned char i8048_port_read(i8048_state_S *state, unsigned char port) {
  return arcadeCurrentMachine->rdI8048_port(state, port);
}

unsigned char i8048_rom_read(i8048_state_S *state, unsigned short addr) {
  return arcadeCurrentMachine->rdI8048_rom(state, addr);
}

unsigned char i8048_xdm_read(i8048_state_S *state, unsigned char addr) {
  return arcadeCurrentMachine->rdI8048_xdm(state, addr);
}

void i8048_xdm_write(i8048_state_S *state, unsigned char addr, unsigned char data) {
  // Default: no external data memory write handler
}
