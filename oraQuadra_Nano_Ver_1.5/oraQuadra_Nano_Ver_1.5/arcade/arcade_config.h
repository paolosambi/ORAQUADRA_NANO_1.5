#ifndef _ARCADE_CONFIG_H_
#define _ARCADE_CONFIG_H_

// ============================================================================
// ARCADE EMULATOR CONFIGURATION FOR ORAQUADRA NANO
// Ported from Galag project - Z80 arcade emulator
// ============================================================================

// Core configuration for FreeRTOS tasks
// oraQuadra main loop runs on core 1, emulation on core 0
#define ARCADE_EMULATION_CORE 0

// ============================================================================
// GAME SELECTION - Comment out games you don't want to include
// Each game adds ROM data to flash (~36-300KB per game)
// ============================================================================

// Phase 1: Pac-Man only
#define ENABLE_PACMAN

// Phase 2: Galaga
#define ENABLE_GALAGA

// Phase 3: Donkey Kong + Frogger
#define ENABLE_DKONG
#define ENABLE_FROGGER

// Phase 4: All remaining games
#define ENABLE_DIGDUG
#define ENABLE_1942
#define ENABLE_EYES
#define ENABLE_MRTNT
#define ENABLE_LIZWIZ
#define ENABLE_THEGLOB
#define ENABLE_CRUSH
#define ENABLE_ANTEATER

// ============================================================================
// MEMORY CONFIGURATION
// ============================================================================

// RAM size depends on which games are enabled
// 1942 needs extra RAM (9344 bytes), others need 8192
#ifdef ENABLE_1942
  #define RAMSIZE (8192 + 1024 + 128)
#else
  #define RAMSIZE (8192)
#endif

// Z80 instructions per frame: 300000/60/4 = 1250
#define INST_PER_FRAME (300000/60/4)

// ============================================================================
// DISPLAY CONFIGURATION
// ============================================================================

// Original arcade resolution
#define ARC_GAME_W    224
#define ARC_GAME_H    288

// Scaling factor: 1.5x (3:2 nearest-neighbor)
// 224 * 1.5 = 336, 288 * 1.5 = 432
#define ARC_OUT_W     336
#define ARC_OUT_H     432

// Centering on 480x480 display
#define ARC_OFFSET_X  72
#define ARC_OFFSET_Y  24

// Frame timing
#define ARC_FRAME_MS  16  // ~60fps target (16.67ms)

// ============================================================================
// AUDIO CONFIGURATION
// ============================================================================

// Audio: use Audio library's existing I2S driver (no install/uninstall)
// Uncomment to disable arcade audio synthesis:
// #define ARCADE_DISABLE_AUDIO

// ============================================================================
// INPUT BUTTON DEFINITIONS (same as Galag)
// ============================================================================

#define BUTTON_LEFT   0x01
#define BUTTON_RIGHT  0x02
#define BUTTON_UP     0x04
#define BUTTON_DOWN   0x08
#define BUTTON_FIRE   0x10
#define BUTTON_START  0x20
#define BUTTON_COIN   0x40
#define BUTTON_EXTRA  0x80

#endif // _ARCADE_CONFIG_H_
