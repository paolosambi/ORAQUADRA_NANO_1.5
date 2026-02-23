// ============================================================================
// 51_ARCADE.ino - Arcade Game Emulator Mode for oraQuadra Nano v1.5
// Ported from Galag Z80 emulator project
// Emulates classic arcade games: Pac-Man, Galaga, Donkey Kong, etc.
// ============================================================================
// Display: 224x288 original -> 336x432 (1.5x scale) centered on 480x480
// Threading: Z80 emulation on Core 0, rendering on Core 1 (main loop)
// Input: Touch screen mapped to arcade joystick + buttons
// Audio: 3 synthesis engines (Namco WSG, AY-3-8910, DK PCM) via existing I2S
// ============================================================================

#ifdef EFFECT_ARCADE

extern volatile bool arcadeBubbleActive;

#include "arcade/arcade_config.h"
#include "arcade/arcade_bridge.h"
#include "arcade/machines/machineBase.h"

#ifdef ENABLE_PACMAN
#include "arcade/machines/pacman/pacman.h"
#endif
#ifdef ENABLE_GALAGA
#include "arcade/machines/galaga/galaga.h"
#endif
#ifdef ENABLE_DKONG
#include "arcade/machines/dkong/dkong.h"
#endif
#ifdef ENABLE_FROGGER
#include "arcade/machines/frogger/frogger.h"
#endif
#ifdef ENABLE_DIGDUG
#include "arcade/machines/digdug/digdug.h"
#endif
#ifdef ENABLE_1942
#include "arcade/machines/1942/1942.h"
#endif
#ifdef ENABLE_EYES
#include "arcade/machines/eyes/eyes.h"
#endif
#ifdef ENABLE_MRTNT
#include "arcade/machines/mrtnt/mrtnt.h"
#endif
#ifdef ENABLE_LIZWIZ
#include "arcade/machines/lizwiz/lizwiz.h"
#endif
#ifdef ENABLE_THEGLOB
#include "arcade/machines/theglob/theglob.h"
#endif
#ifdef ENABLE_CRUSH
#include "arcade/machines/crush/crush.h"
#endif
#ifdef ENABLE_ANTEATER
#include "arcade/machines/anteater/anteater.h"
#endif

// ============================================================================
// Arduino IDE 1.8.x does NOT compile .c/.cpp files in subdirectories.
// Include source files directly so they are compiled as part of the sketch.
// ============================================================================
extern "C" {
  #include "arcade/cpus/z80/Z80.c"
  #include "arcade/cpus/i8048/i8048.c"
}
#include "arcade/arcade_bridge.cpp"
#ifdef ENABLE_PACMAN
#include "arcade/machines/pacman/pacman.cpp"
#endif
#ifdef ENABLE_GALAGA
#include "arcade/machines/galaga/galaga.cpp"
#endif
#ifdef ENABLE_DKONG
#include "arcade/machines/dkong/dkong.cpp"
#endif
#ifdef ENABLE_FROGGER
#include "arcade/machines/frogger/frogger.cpp"
#endif
#ifdef ENABLE_DIGDUG
#include "arcade/machines/digdug/digdug.cpp"
#endif
#ifdef ENABLE_1942
#include "arcade/machines/1942/1942.cpp"
#endif
#ifdef ENABLE_EYES
#include "arcade/machines/eyes/eyes.cpp"
#endif
#ifdef ENABLE_MRTNT
#include "arcade/machines/mrtnt/mrtnt.cpp"
#endif
#ifdef ENABLE_LIZWIZ
#include "arcade/machines/lizwiz/lizwiz.cpp"
#endif
#ifdef ENABLE_THEGLOB
#include "arcade/machines/theglob/theglob.cpp"
#endif
#ifdef ENABLE_CRUSH
#include "arcade/machines/crush/crush.cpp"
#endif
#ifdef ENABLE_ANTEATER
#include "arcade/machines/anteater/anteater.cpp"
#endif

// Arcade audio synthesis (Namco WSG, AY-3-8910, DK PCM)
#include "arcade/arcade_audio.h"
#include "arcade/arcade_audio.cpp"

// ===== Display Constants =====
// Row buffer: one tile row = 224 pixels * 8 lines * 2 bytes = 3584 bytes
#define ARC_ROW_PIXELS  (ARC_GAME_W * 8)
// Batched rendering: 12 tile rows per draw call = 3 draw calls instead of 36
#define ARC_BATCH_ROWS  12
#define ARC_BATCH_OUT_H (12 * ARC_BATCH_ROWS)  // 144 output lines per batch
// Scale buffer: 336 pixels * 144 lines * 2 bytes = 96768 bytes (PSRAM)
#define ARC_SCALE_PIXELS (ARC_OUT_W * ARC_BATCH_OUT_H)

// ===== Touch Zone Definitions =====
#define TOUCH_EXIT_X    80   // Exit zone: top-left corner
#define TOUCH_EXIT_Y    48
#define TOUCH_COIN_X    400  // Coin zone: top-right corner
#define TOUCH_COIN_Y    48
#define TOUCH_DPAD_LEFT  140  // D-pad left boundary
#define TOUCH_DPAD_RIGHT 340  // D-pad right boundary
#define TOUCH_DPAD_MID_Y 240  // Split between UP and DOWN zones
#define TOUCH_BOTTOM_Y   432  // Bottom control bar start
// Bottom bar: 0-159=BACK, 160-319=START, 320-479=FIRE

// ===== Menu Grid Layout (4x3, all 12 games visible) =====
#define AM_HEADER_H   48
#define AM_GRID_TOP   52
#define AM_CELL_W    113
#define AM_CELL_H    128
#define AM_GAP         6
#define AM_MARGIN      5
#define AM_COLS        4
#define AM_ROWS        3
#define AM_COL_STEP  119   // 113+6
#define AM_ROW_STEP  134   // 128+6
#define AM_CARD_BG   0x10A2  // grigio scuro (come mode selector)

// ===== Game List =====
#define ARCADE_MAX_GAMES 12

struct ArcadeGameInfo {
  const char* name;
  uint8_t machineType;
  bool enabled;
};

// ===== Global Variables =====
bool arcadeInitialized = false;
int8_t arcadeSelectedGame = -1;  // -1 = menu
bool arcadeInMenu = true;
machineBase* arcadeCurrentMachine = nullptr;
ArcadeAudio* arcadeAudio = nullptr;

// Machine instances (created on demand)
int8_t arcadeMachineCount = 0;
ArcadeGameInfo arcadeGames[ARCADE_MAX_GAMES];

// Buffers (PSRAM allocated)
unsigned short* arcadeFrameBuffer = nullptr;   // 224*8 pixels for one tile row
sprite_S* arcadeSpriteBuffer = nullptr;        // Up to 128 sprites
unsigned char* arcadeMemory = nullptr;         // Z80 RAM
unsigned short* arcadeScaleBuffer = nullptr;   // 336*12 pixels for scaled output

// Input
ArcadeInput arcadeInput;
volatile uint8_t arcadeButtonState = 0;

// Virtual button press state machines (COIN e START indipendenti)
uint8_t arcadeCoinState = 0;       // 0=idle, 1=pressed, 2=released
unsigned long arcadeCoinTimer = 0;
uint8_t arcadeStartState = 0;     // 0=idle, 1=pressed, 2=released
unsigned long arcadeStartTimer = 0;

// Emulation task
TaskHandle_t arcadeEmulationTaskHandle = nullptr;

// Frame timing
unsigned long arcadeLastFrameTime = 0;
unsigned long arcadeFrameCount = 0;
unsigned long arcadeBootStartMs = 0;   // Tracks boot duration (reset per game)

// ===== Forward Declarations =====
void arcadeStartGame(int8_t index);
void arcadeStopGame();
void arcadeEmulationTask(void* param);
IRAM_ATTR void arcadeUpscaleRow(unsigned short* src, unsigned short* dst, int dstOffsetY);
void arcadeDrawMenu();
void arcadeDrawControlOverlay();
void arcadeBuildGameList();
uint16_t arcadeGetGameColor(uint8_t machineType);
void arcadeDrawGameIcon(int cx, int cy, uint8_t machineType);
void arcadeDrawGameCard(int col, int row, int gameIndex);
void arcadeAllocateBuffers();
void arcadeFreeBuffers();
void arcadeUpdateButtonStateMachines();
void arcadeTriggerCoin();
void arcadeTriggerStart();
void handleArcadeGameTouch(int x, int y);
void arcadeUpdateTouchInput(int x, int y);
void arcadeClearTouchInput();

// ===== Initialization =====
void initArcade() {
  if (arcadeInitialized) return;

  Serial.println("[ARCADE] Initializing Arcade Emulator");

  // ★ Show loading screen IMMEDIATELY (before any blocking ops)
  // audio.stopSong() can block 10-30s if web radio TCP connection is timing out
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(0xFFE0);  // Yellow
  gfx->setCursor(150, 210);
  gfx->print("ARCADE");
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextSize(1);
  gfx->setTextColor(0x7BEF);  // Gray
  gfx->setCursor(168, 250);
  gfx->print("Initializing...");

  // Stop music playback to free I2S for arcade audio synthesis
  // This can block if an HTTP stream (web radio) is active and the
  // TCP connection takes time to close (lwIP timeout up to 20-30s).
  unsigned long t0 = millis();
  audio.stopSong();
  unsigned long dt = millis() - t0;
  Serial.printf("[ARCADE] audio.stopSong() took %lu ms\n", dt);
  delay(50);  // Let audioTask finish current loop iteration

  // Arcade audio: sets I2S to 24kHz, uses Audio library's existing driver
  arcadeAudio = new ArcadeAudio();
  arcadeAudio->init();

  // Reset gfx state (previous modes may leave custom fonts/sizes)
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextSize(1);
  gfx->setTextWrap(false);

  // Build available games list
  arcadeBuildGameList();

  // Allocate PSRAM buffers
  arcadeAllocateBuffers();

  if (!arcadeFrameBuffer || !arcadeSpriteBuffer || !arcadeMemory || !arcadeScaleBuffer) {
    Serial.println("[ARCADE] ERROR: Failed to allocate PSRAM buffers!");
    arcadeFreeBuffers();
    return;
  }

  // Start in menu
  arcadeInMenu = true;
  arcadeSelectedGame = -1;

  // Clear screen and draw menu
  gfx->fillScreen(BLACK);
  arcadeDrawMenu();

  // Attiva bubble arcade: congela tutti i servizi del loop principale
  arcadeBubbleActive = true;
  Serial.println("[ARCADE] Bubble attivata - servizi congelati");

  // BLE gamepad disabilitato - input solo via touch
  // bleGamepadInit();

  arcadeInitialized = true;
  Serial.println("[ARCADE] Initialized OK, games available: " + String(arcadeMachineCount));
}

// ===== Cleanup =====
void cleanupArcade() {
  if (!arcadeInitialized) return;

  // BLE gamepad disabilitato
  // bleGamepadCleanup();

  // Disattiva bubble arcade: ripristina tutti i servizi
  arcadeBubbleActive = false;
  Serial.println("[ARCADE] Bubble disattivata - servizi ripristinati");

  Serial.println("[ARCADE] Cleanup");

  // Stop emulation if running
  arcadeStopGame();

  // Cleanup arcade audio (stubs when ARCADE_DISABLE_AUDIO)
  if (arcadeAudio) {
    arcadeAudio->cleanup();
    delete arcadeAudio;
    arcadeAudio = nullptr;
  }

  // Free buffers
  arcadeFreeBuffers();

  arcadeInitialized = false;
  arcadeInMenu = true;
  arcadeSelectedGame = -1;
}

// ===== Build Game List =====
void arcadeBuildGameList() {
  arcadeMachineCount = 0;

#ifdef ENABLE_PACMAN
  arcadeGames[arcadeMachineCount++] = {"PAC-MAN", MCH_PACMAN, true};
#endif
#ifdef ENABLE_GALAGA
  arcadeGames[arcadeMachineCount++] = {"GALAGA", MCH_GALAGA, true};
#endif
#ifdef ENABLE_DKONG
  arcadeGames[arcadeMachineCount++] = {"DONKEY KONG", MCH_DKONG, true};
#endif
#ifdef ENABLE_FROGGER
  arcadeGames[arcadeMachineCount++] = {"FROGGER", MCH_FROGGER, true};
#endif
#ifdef ENABLE_DIGDUG
  arcadeGames[arcadeMachineCount++] = {"DIG DUG", MCH_DIGDUG, true};
#endif
#ifdef ENABLE_1942
  arcadeGames[arcadeMachineCount++] = {"1942", MCH_1942, true};
#endif
#ifdef ENABLE_EYES
  arcadeGames[arcadeMachineCount++] = {"EYES", MCH_EYES, true};
#endif
#ifdef ENABLE_MRTNT
  arcadeGames[arcadeMachineCount++] = {"MR. TNT", MCH_MRTNT, true};
#endif
#ifdef ENABLE_LIZWIZ
  arcadeGames[arcadeMachineCount++] = {"LIZ WIZ", MCH_LIZWIZ, true};
#endif
#ifdef ENABLE_THEGLOB
  arcadeGames[arcadeMachineCount++] = {"THE GLOB", MCH_THEGLOB, true};
#endif
#ifdef ENABLE_CRUSH
  arcadeGames[arcadeMachineCount++] = {"CRUSH ROLLER", MCH_CRUSH, true};
#endif
#ifdef ENABLE_ANTEATER
  arcadeGames[arcadeMachineCount++] = {"ANTEATER", MCH_ANTEATER, true};
#endif
}

// ===== Buffer Management =====
void arcadeAllocateBuffers() {
  // Prefer internal SRAM for hot-path buffers (3-8x faster than PSRAM on ESP32-S3)
  // Fall back to PSRAM if internal RAM is insufficient
  size_t fbSize = ARC_ROW_PIXELS * sizeof(unsigned short);           // ~3.5KB
  size_t sprSize = 128 * sizeof(sprite_S);                           // ~1.5KB
  size_t scaleSize = ARC_SCALE_PIXELS * sizeof(unsigned short);      // ~32KB

  arcadeFrameBuffer = (unsigned short*)heap_caps_malloc(fbSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!arcadeFrameBuffer) arcadeFrameBuffer = (unsigned short*)ps_malloc(fbSize);

  arcadeSpriteBuffer = (sprite_S*)heap_caps_malloc(sprSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!arcadeSpriteBuffer) arcadeSpriteBuffer = (sprite_S*)ps_malloc(sprSize);

  arcadeMemory = (unsigned char*)heap_caps_malloc(RAMSIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!arcadeMemory) arcadeMemory = (unsigned char*)ps_malloc(RAMSIZE);

  arcadeScaleBuffer = (unsigned short*)heap_caps_malloc(scaleSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!arcadeScaleBuffer) arcadeScaleBuffer = (unsigned short*)ps_malloc(scaleSize);

  Serial.printf("[ARCADE] Buffers: FB=%d(%s), SPR=%d(%s), MEM=%d(%s), SCALE=%d(%s)\n",
    fbSize, ((uint32_t)arcadeFrameBuffer < 0x3C000000) ? "SRAM" : "PSRAM",
    sprSize, ((uint32_t)arcadeSpriteBuffer < 0x3C000000) ? "SRAM" : "PSRAM",
    RAMSIZE, ((uint32_t)arcadeMemory < 0x3C000000) ? "SRAM" : "PSRAM",
    scaleSize, ((uint32_t)arcadeScaleBuffer < 0x3C000000) ? "SRAM" : "PSRAM");
}

void arcadeFreeBuffers() {
  if (arcadeFrameBuffer) { free(arcadeFrameBuffer); arcadeFrameBuffer = nullptr; }
  if (arcadeSpriteBuffer) { free(arcadeSpriteBuffer); arcadeSpriteBuffer = nullptr; }
  if (arcadeMemory) { free(arcadeMemory); arcadeMemory = nullptr; }
  if (arcadeScaleBuffer) { free(arcadeScaleBuffer); arcadeScaleBuffer = nullptr; }
}

// ===== Create Machine Instance =====
machineBase* arcadeCreateMachine(uint8_t type) {
  switch(type) {
#ifdef ENABLE_PACMAN
    case MCH_PACMAN: return new pacman();
#endif
#ifdef ENABLE_GALAGA
    case MCH_GALAGA: return new galaga();
#endif
#ifdef ENABLE_DKONG
    case MCH_DKONG: return new dkong();
#endif
#ifdef ENABLE_FROGGER
    case MCH_FROGGER: return new frogger();
#endif
#ifdef ENABLE_DIGDUG
    case MCH_DIGDUG: return new digdug();
#endif
#ifdef ENABLE_1942
    case MCH_1942: return new _1942();
#endif
#ifdef ENABLE_EYES
    case MCH_EYES: return new eyes();
#endif
#ifdef ENABLE_MRTNT
    case MCH_MRTNT: return new mrtnt();
#endif
#ifdef ENABLE_LIZWIZ
    case MCH_LIZWIZ: return new lizwiz();
#endif
#ifdef ENABLE_THEGLOB
    case MCH_THEGLOB: return new theglob();
#endif
#ifdef ENABLE_CRUSH
    case MCH_CRUSH: return new crush();
#endif
#ifdef ENABLE_ANTEATER
    case MCH_ANTEATER: return new anteater();
#endif
    default: return nullptr;
  }
}

// ===== Start Game =====
void arcadeStartGame(int8_t index) {
  if (index < 0 || index >= arcadeMachineCount) return;

  unsigned long tStart = millis();
  Serial.printf("[ARCADE] Starting game: %s\n", arcadeGames[index].name);

  // Show loading screen BEFORE any potentially slow operations
  gfx->fillScreen(BLACK);
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextColor(0xFFE0);  // Yellow
  gfx->setTextSize(3);
  gfx->setCursor(110, 200);
  gfx->print("LOADING");
  gfx->setTextSize(2);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(100, 260);
  gfx->printf("%s...", arcadeGames[index].name);

  // Create machine instance
  unsigned long t0 = millis();
  arcadeCurrentMachine = arcadeCreateMachine(arcadeGames[index].machineType);
  if (!arcadeCurrentMachine) {
    Serial.println("[ARCADE] Failed to create machine!");
    return;
  }
  Serial.printf("[ARCADE] createMachine took %lu ms\n", millis() - t0);

  // Initialize machine with buffers (PROGMEM ROMs - should be instant)
  t0 = millis();
  arcadeCurrentMachine->init(&arcadeInput, arcadeFrameBuffer, arcadeSpriteBuffer, arcadeMemory);
  arcadeCurrentMachine->reset();
  Serial.printf("[ARCADE] init+reset took %lu ms\n", millis() - t0);

  // Start arcade audio synthesis for this game
  if (arcadeAudio) arcadeAudio->start(arcadeCurrentMachine);

  // Draw borders and control overlay
  gfx->fillRect(0, 0, ARC_OFFSET_X, 480, 0x0000);               // Left border
  gfx->fillRect(ARC_OFFSET_X + ARC_OUT_W, 0, 480 - ARC_OFFSET_X - ARC_OUT_W, 480, 0x0000); // Right border
  arcadeDrawControlOverlay();

  arcadeInMenu = false;
  arcadeSelectedGame = index;
  arcadeButtonState = 0;
  arcadeInput.setButtons(0);
  arcadeCoinState = 0;
  arcadeStartState = 0;
  arcadeFrameCount = 0;
  arcadeBootStartMs = millis();   // Start boot timer
  arcadeLastFrameTime = millis();

  // Create emulation task on Core 0
  BaseType_t taskResult = xTaskCreatePinnedToCore(
    arcadeEmulationTask,
    "arcadeZ80",
    8192,                 // 8KB stack (Z80 callbacks use several nested virtual calls)
    nullptr,
    2,                    // Priority 2 (higher than idle)
    &arcadeEmulationTaskHandle,
    ARCADE_EMULATION_CORE
  );

  if (taskResult != pdPASS) {
    Serial.printf("[ARCADE] ERROR: Failed to create emulation task! (err=%d, free heap=%d)\n",
                  taskResult, ESP.getFreeHeap());
    // Show error on screen
    gfx->fillRect(80, 300, 320, 30, BLACK);
    gfx->setTextColor(0xF800);
    gfx->setTextSize(1);
    gfx->setCursor(80, 304);
    gfx->print("ERROR: Cannot start emulation task!");
    arcadeInMenu = true;
    return;
  }

  Serial.printf("[ARCADE] Game started in %lu ms, emulation on Core 0 (heap=%d)\n",
                millis() - tStart, ESP.getFreeHeap());
}

// ===== Stop Game =====
void arcadeStopGame() {
  // Delete emulation task
  if (arcadeEmulationTaskHandle) {
    vTaskDelete(arcadeEmulationTaskHandle);
    arcadeEmulationTaskHandle = nullptr;
    Serial.println("[ARCADE] Emulation task deleted");
  }

  // Stop arcade audio before deleting machine
  if (arcadeAudio) arcadeAudio->stop();

  // Delete machine
  if (arcadeCurrentMachine) {
    delete arcadeCurrentMachine;
    arcadeCurrentMachine = nullptr;
  }

  // Clear input
  arcadeButtonState = 0;
  arcadeInput.setButtons(0);
  arcadeCoinState = 0;
  arcadeStartState = 0;

  // Return to menu
  arcadeInMenu = true;
  arcadeSelectedGame = -1;

  // Redraw menu
  gfx->fillScreen(BLACK);
  arcadeDrawMenu();
}

// ===== Emulation Task (Core 0) =====
void arcadeEmulationTask(void* param) {
  Serial.println("[ARCADE-EMU] Task started on Core 0");
  unsigned long emuStart = millis();
  int bootFrames = 0;

  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    if (arcadeCurrentMachine) {
      if (!arcadeCurrentMachine->game_started) {
        // FAST BOOT: run 20 emulation frames per tick (~20x faster boot)
        // Pac-Man boot does RAM test + init, takes ~180 frames normally
        for (int i = 0; i < 20; i++) {
          arcadeCurrentMachine->run_frame();
          bootFrames++;
          if (arcadeCurrentMachine->game_started) {
            Serial.printf("[ARCADE-EMU] Boot done in %d frames, %lu ms\n",
                          bootFrames, millis() - emuStart);
            break;
          }
        }
        vTaskDelay(1);  // Feed watchdog
      } else {
        // Self-timed at 60fps, decoupled from render
        arcadeCurrentMachine->run_frame();
        // Audio: transmit on same core as emulation (no soundregs race).
        // Fills DMA buffer with as many samples as available space allows.
        if (arcadeAudio) arcadeAudio->transmit();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(16));
      }
    } else {
      vTaskDelay(10);
    }
  }
}

// ===== Upscale Row: 224x8 -> 336x12 (1.5x nearest-neighbor) =====
// 3:2 scaling: every 2 source pixels become 3 destination pixels
// dstOffsetY = vertical offset in destination buffer (for row batching)
IRAM_ATTR void arcadeUpscaleRow(unsigned short* src, unsigned short* dst, int dstOffsetY) {
  // Source line mapping: 0,0,1, 2,2,3, 4,4,5, 6,6,7 (3:2 pattern)
  // Lines 0,3,6,9 are duplicates of previous line -> use memcpy
  static const uint8_t srcLineMap[12] = {0,0,1, 2,2,3, 4,4,5, 6,6,7};

  for (int outY = 0; outY < 12; outY++) {
    unsigned short* dstLine = dst + (dstOffsetY + outY) * ARC_OUT_W;

    // Duplicate lines: just memcpy from the line above (4 of 12 lines)
    if (outY > 0 && srcLineMap[outY] == srcLineMap[outY - 1]) {
      memcpy(dstLine, dstLine - ARC_OUT_W, ARC_OUT_W * sizeof(unsigned short));
    } else {
      // Unique line: horizontal 3:2 scaling (224 -> 336 pixels)
      // Colormap data is big-endian (pre-swapped for SPI displays).
      // RGB parallel panel expects little-endian: byte-swap each pixel.
      unsigned short* srcLine = src + srcLineMap[outY] * ARC_GAME_W;
      int dstX = 0;
      for (int srcX = 0; srcX < ARC_GAME_W; srcX += 2) {
        unsigned short p0 = __builtin_bswap16(srcLine[srcX]);
        unsigned short p1 = __builtin_bswap16(srcLine[srcX + 1]);
        dstLine[dstX]     = p0;
        dstLine[dstX + 1] = p0;
        dstLine[dstX + 2] = p1;
        dstX += 3;
      }
    }
  }
}

// ===== Main Update Loop (called from loop() on Core 1) =====
void updateArcade() {
  if (!arcadeInitialized) {
    initArcade();
    return;
  }

  // If in menu, nothing to render continuously
  if (arcadeInMenu) {
    arcadeUpdateButtonStateMachines();
    delay(16);  // Save CPU
    return;
  }

  // Game is running - render frame
  if (!arcadeCurrentMachine) return;

  // ALWAYS update input (even during Z80 boot)
  arcadeInput.setButtons(arcadeButtonState);
  arcadeUpdateButtonStateMachines();

  // FAST BOOT: skip rendering during Z80 boot, show progress on screen
  if (!arcadeCurrentMachine->game_started) {
    unsigned long elapsed = millis() - arcadeBootStartMs;

    // Show animated boot progress so user knows system is NOT frozen
    gfx->fillRect(100, 280, 280, 20, BLACK);
    gfx->setFont((const GFXfont *)NULL);
    gfx->setTextSize(1);
    gfx->setTextColor(0x7BEF);
    gfx->setCursor(110, 284);
    gfx->printf("Booting Z80... %lu.%lus", elapsed / 1000, (elapsed / 100) % 10);

    Serial.printf("[ARCADE] Boot wait: %lu ms, game_started=%d\n", elapsed, arcadeCurrentMachine->game_started);

    delay(100);  // Don't busy-wait, let emulation run on Core 0
    return;
  }

  // Boot completed - log once
  if (arcadeFrameCount == 0) {
    unsigned long bootTime = millis() - arcadeBootStartMs;
    Serial.printf("[ARCADE] Z80 boot completed in %lu ms, rendering first frame\n", bootTime);
  }

  // Prepare frame (extract sprites from Z80 RAM)
  arcadeCurrentMachine->prepare_frame();

  // Render in batches of 12 tile rows per draw call (3 calls instead of 36)
  gfx->startWrite();
  for (int batch = 0; batch < 3; batch++) {
    int rowStart = batch * ARC_BATCH_ROWS;

    // Render and upscale 12 tile rows into the batch buffer
    for (int r = 0; r < ARC_BATCH_ROWS; r++) {
      int row = rowStart + r;
      memset(arcadeFrameBuffer, 0, ARC_ROW_PIXELS * sizeof(unsigned short));
      arcadeCurrentMachine->render_row(row);
      arcadeUpscaleRow(arcadeFrameBuffer, arcadeScaleBuffer, r * 12);
    }

    // Single blit for the entire batch (336 x 144 pixels)
    gfx->draw16bitRGBBitmap(ARC_OFFSET_X, ARC_OFFSET_Y + batch * ARC_BATCH_OUT_H,
                             arcadeScaleBuffer, ARC_OUT_W, ARC_BATCH_OUT_H);
  }
  gfx->endWrite();

  arcadeFrameCount++;
}

// ===== Game Theme Colors =====
uint16_t arcadeGetGameColor(uint8_t machineType) {
  switch (machineType) {
    case MCH_PACMAN:   return 0xFFE0;  // giallo
    case MCH_GALAGA:   return 0x07FF;  // ciano
    case MCH_DKONG:    return 0xF800;  // rosso
    case MCH_FROGGER:  return 0x07E0;  // verde
    case MCH_DIGDUG:   return 0x4C7F;  // azzurro cielo
    case MCH_1942:     return 0x05BF;  // blu chiaro
    case MCH_EYES:     return 0xF81F;  // magenta
    case MCH_MRTNT:    return 0xFB20;  // arancione
    case MCH_LIZWIZ:   return 0x780F;  // viola
    case MCH_THEGLOB:  return 0x47E0;  // verde lime
    case MCH_CRUSH:    return 0x07FF;  // ciano
    case MCH_ANTEATER: return 0xC380;  // marrone
    default:           return 0xFFFF;
  }
}

// ===== Pixel Art Game Icons (~48x48) =====
void arcadeDrawGameIcon(int cx, int cy, uint8_t machineType) {
  switch (machineType) {

    case MCH_PACMAN: {
      // Pac-Man: cerchio giallo con bocca + puntini + ciliegia
      gfx->fillCircle(cx - 4, cy - 4, 16, 0xFFE0);
      // Bocca (triangolo nero)
      gfx->fillTriangle(cx - 4, cy - 4, cx + 16, cy - 14, cx + 16, cy + 6, BLACK);
      // Occhio
      gfx->fillCircle(cx - 2, cy - 14, 3, BLACK);
      // 3 puntini
      gfx->fillCircle(cx + 18, cy - 4, 2, 0xFFFF);
      // Ciliegia in basso
      gfx->fillCircle(cx - 6, cy + 16, 5, 0xF800);
      gfx->fillCircle(cx + 2, cy + 14, 4, 0xF800);
      gfx->drawLine(cx - 4, cy + 11, cx + 2, cy + 6, 0x07E0); // gambo
      break;
    }

    case MCH_GALAGA: {
      // Nave spaziale triangolare sotto
      gfx->fillTriangle(cx, cy + 4, cx - 14, cy + 20, cx + 14, cy + 20, 0x07FF);
      gfx->fillRect(cx - 2, cy + 6, 4, 10, 0xFFFF); // corpo centrale
      gfx->fillRect(cx - 18, cy + 16, 8, 4, 0x07FF); // ala sx
      gfx->fillRect(cx + 10, cy + 16, 8, 4, 0x07FF); // ala dx
      // Bug alien sopra
      gfx->fillRect(cx - 8, cy - 16, 16, 10, 0xF800);
      gfx->fillRect(cx - 12, cy - 10, 4, 6, 0xF800); // antennina sx
      gfx->fillRect(cx + 8, cy - 10, 4, 6, 0xF800);  // antennina dx
      gfx->fillCircle(cx - 4, cy - 12, 2, 0xFFFF); // occhio sx
      gfx->fillCircle(cx + 4, cy - 12, 2, 0xFFFF); // occhio dx
      break;
    }

    case MCH_DKONG: {
      // Barile marrone
      uint16_t barrel = 0xCA40; // marrone chiaro
      gfx->fillRoundRect(cx - 12, cy - 10, 24, 20, 4, barrel);
      gfx->drawRoundRect(cx - 12, cy - 10, 24, 20, 4, 0x8200); // bordo scuro
      gfx->drawLine(cx - 10, cy, cx + 10, cy, 0x8200); // fascia centro
      // "DK" sul barile
      gfx->setFont((const GFXfont *)NULL);
      gfx->setTextSize(1);
      gfx->setTextColor(0xFFFF);
      gfx->setCursor(cx - 6, cy - 6);
      gfx->print("DK");
      // Piattaforma rossa sotto
      gfx->fillRect(cx - 18, cy + 14, 36, 4, 0xF800);
      gfx->fillRect(cx - 16, cy + 18, 32, 2, 0xA000); // ombra
      break;
    }

    case MCH_FROGGER: {
      // Rana verde vista dall'alto
      gfx->fillCircle(cx, cy - 2, 10, 0x07E0);       // corpo
      gfx->fillCircle(cx - 8, cy - 10, 5, 0x07E0);   // occhio sx
      gfx->fillCircle(cx + 8, cy - 10, 5, 0x07E0);   // occhio dx
      gfx->fillCircle(cx - 8, cy - 10, 2, BLACK);     // pupilla sx
      gfx->fillCircle(cx + 8, cy - 10, 2, BLACK);     // pupilla dx
      // Zampe
      gfx->fillRect(cx - 16, cy + 2, 6, 3, 0x07E0);  // zampa sx
      gfx->fillRect(cx + 10, cy + 2, 6, 3, 0x07E0);  // zampa dx
      // Strisce blu (acqua) sotto
      gfx->fillRect(cx - 20, cy + 14, 40, 3, 0x001F);
      gfx->fillRect(cx - 16, cy + 19, 32, 2, 0x035F);
      break;
    }

    case MCH_DIGDUG: {
      // Terreno marrone con tunnel
      gfx->fillRect(cx - 20, cy - 8, 40, 28, 0x8200); // terra
      // Tunnel (scavato)
      gfx->fillRect(cx - 16, cy - 2, 20, 8, BLACK);   // tunnel orizz
      gfx->fillRect(cx - 4, cy - 8, 8, 18, BLACK);    // tunnel vert
      // Personaggio (bianco + blu)
      gfx->fillRect(cx - 2, cy - 6, 4, 6, 0xFFFF);   // corpo
      gfx->fillRect(cx - 3, cy - 8, 6, 3, 0x001F);   // casco blu
      // Pompa (linea verso destra)
      gfx->drawLine(cx + 2, cy - 3, cx + 12, cy - 3, 0xFFFF);
      gfx->drawLine(cx + 2, cy - 2, cx + 12, cy - 2, 0xFFFF);
      // Pooka (nemico rosso a destra)
      gfx->fillCircle(cx + 16, cy - 3, 4, 0xF800);
      gfx->fillCircle(cx + 14, cy - 4, 1, 0xFFFF);   // occhio
      break;
    }

    case MCH_1942: {
      // Aereo WWII visto dall'alto
      gfx->fillRect(cx - 2, cy - 16, 4, 24, 0x4208);  // fusoliera
      gfx->fillRect(cx - 16, cy - 4, 32, 6, 0x4208);  // ali
      gfx->fillRect(cx - 6, cy + 6, 12, 4, 0x4208);   // coda
      // Dettagli
      gfx->fillCircle(cx, cy - 14, 3, 0x8410);        // muso
      gfx->fillRect(cx - 1, cy - 20, 2, 6, 0xFFE0);   // elica gialla
      // Stelle sulle ali (semplificato)
      gfx->fillCircle(cx - 10, cy - 1, 2, 0xF800);
      gfx->fillCircle(cx + 10, cy - 1, 2, 0xF800);
      // Nuvole
      gfx->fillCircle(cx - 14, cy + 16, 4, 0x6B4D);
      gfx->fillCircle(cx + 16, cy + 12, 3, 0x6B4D);
      break;
    }

    case MCH_EYES: {
      // Due grandi occhi con pupille
      // Occhio sinistro
      gfx->fillCircle(cx - 12, cy - 2, 12, 0xFFFF);           // bianco
      gfx->drawCircle(cx - 12, cy - 2, 12, 0xC618);           // contorno
      gfx->fillCircle(cx - 10, cy - 2, 6, 0x001F);            // iride blu
      gfx->fillCircle(cx - 9, cy - 2, 3, BLACK);              // pupilla
      gfx->fillCircle(cx - 7, cy - 5, 2, 0xFFFF);             // riflesso
      // Occhio destro
      gfx->fillCircle(cx + 12, cy - 2, 12, 0xFFFF);
      gfx->drawCircle(cx + 12, cy - 2, 12, 0xC618);
      gfx->fillCircle(cx + 14, cy - 2, 6, 0x001F);
      gfx->fillCircle(cx + 15, cy - 2, 3, BLACK);
      gfx->fillCircle(cx + 17, cy - 5, 2, 0xFFFF);
      // Sopracciglia
      gfx->drawLine(cx - 22, cy - 16, cx - 4, cy - 14, 0xF81F);
      gfx->drawLine(cx + 4, cy - 14, cx + 22, cy - 16, 0xF81F);
      break;
    }

    case MCH_MRTNT: {
      // Candelotto dinamite rosso
      gfx->fillRoundRect(cx - 6, cy - 6, 12, 22, 3, 0xF800); // corpo rosso
      gfx->fillRect(cx - 4, cy - 2, 8, 2, 0xFFE0);           // fascia gialla
      gfx->fillRect(cx - 4, cy + 6, 8, 2, 0xFFE0);           // fascia gialla
      // "TNT" sul corpo
      gfx->setFont((const GFXfont *)NULL);
      gfx->setTextSize(1);
      gfx->setTextColor(0xFFFF);
      gfx->setCursor(cx - 8, cy + 2);
      gfx->print("TNT");
      // Miccia
      gfx->drawLine(cx, cy - 6, cx + 4, cy - 14, 0x8410);    // filo
      gfx->drawLine(cx + 1, cy - 6, cx + 5, cy - 14, 0x8410);
      // Scintilla
      gfx->fillCircle(cx + 5, cy - 16, 3, 0xFFE0);           // gialla
      gfx->fillCircle(cx + 5, cy - 16, 1, 0xFFFF);           // centro bianco
      // Raggi scintilla
      gfx->drawLine(cx + 5, cy - 20, cx + 5, cy - 22, 0xFB20);
      gfx->drawLine(cx + 1, cy - 18, cx - 1, cy - 20, 0xFB20);
      gfx->drawLine(cx + 9, cy - 18, cx + 11, cy - 20, 0xFB20);
      break;
    }

    case MCH_LIZWIZ: {
      // Cappello wizard viola
      gfx->fillTriangle(cx, cy - 20, cx - 14, cy + 2, cx + 14, cy + 2, 0x780F);
      gfx->fillRect(cx - 18, cy + 2, 36, 5, 0x780F);         // tesa
      gfx->fillRect(cx - 16, cy + 2, 32, 2, 0xA01F);         // bordo chiaro
      // Stella sul cappello
      gfx->fillCircle(cx + 2, cy - 8, 3, 0xFFE0);
      // Bacchetta a destra
      gfx->drawLine(cx + 16, cy - 6, cx + 10, cy + 16, 0xCA40);
      gfx->drawLine(cx + 17, cy - 6, cx + 11, cy + 16, 0xCA40);
      // Stella in punta
      gfx->fillCircle(cx + 17, cy - 8, 3, 0xFFE0);
      gfx->fillCircle(cx + 17, cy - 8, 1, 0xFFFF);
      break;
    }

    case MCH_THEGLOB: {
      // Blob verde amorfo
      gfx->fillCircle(cx, cy, 14, 0x47E0);            // corpo principale
      gfx->fillCircle(cx - 8, cy + 6, 8, 0x47E0);    // pseudopodo sx
      gfx->fillCircle(cx + 10, cy + 4, 7, 0x47E0);   // pseudopodo dx
      gfx->fillCircle(cx + 4, cy - 8, 6, 0x47E0);    // protuberanza
      // Occhi
      gfx->fillCircle(cx - 6, cy - 4, 4, 0xFFFF);    // occhio sx
      gfx->fillCircle(cx + 6, cy - 4, 4, 0xFFFF);    // occhio dx
      gfx->fillCircle(cx - 5, cy - 4, 2, BLACK);      // pupilla sx
      gfx->fillCircle(cx + 7, cy - 4, 2, BLACK);      // pupilla dx
      // Bocca
      gfx->drawLine(cx - 4, cy + 4, cx + 4, cy + 6, 0x2400);
      break;
    }

    case MCH_CRUSH: {
      // Rullo schiacciasassi
      gfx->fillCircle(cx - 2, cy - 4, 10, 0x8410);   // rullo (grigio)
      gfx->drawCircle(cx - 2, cy - 4, 10, 0xC618);   // contorno
      gfx->drawCircle(cx - 2, cy - 4, 8, 0x4208);    // cerchio interno
      // Manico
      gfx->fillRect(cx - 4, cy + 6, 4, 12, 0xCA40);  // manico
      // Traccia di vernice colorata
      gfx->fillRect(cx - 18, cy + 12, 36, 4, 0x07FF); // striscia ciano
      gfx->fillRect(cx - 14, cy + 16, 28, 2, 0x03BF); // piu' scura
      // Pennello (dettaglio)
      gfx->fillRect(cx + 8, cy + 8, 6, 6, 0xF81F);   // pennellata magenta
      break;
    }

    case MCH_ANTEATER: {
      // Formicaio (montagnola marrone)
      gfx->fillTriangle(cx + 8, cy - 10, cx - 8, cy + 14, cx + 24, cy + 14, 0x8200);
      gfx->fillCircle(cx + 8, cy + 10, 6, 0x8200);   // base
      // Buco nel formicaio
      gfx->fillCircle(cx + 8, cy + 4, 3, BLACK);
      // Formichiere (a sinistra)
      gfx->fillCircle(cx - 12, cy + 4, 8, 0xC380);    // corpo
      // Muso lungo
      gfx->fillRect(cx - 20, cy + 2, 12, 4, 0xC380);
      gfx->fillRect(cx - 24, cy + 3, 6, 2, 0xA280);   // punta muso
      // Occhio
      gfx->fillCircle(cx - 10, cy + 1, 2, BLACK);
      // Orecchio
      gfx->fillCircle(cx - 8, cy - 4, 3, 0xA280);
      // Formichine
      gfx->fillRect(cx + 2, cy + 10, 2, 2, BLACK);
      gfx->fillRect(cx + 6, cy + 8, 2, 2, BLACK);
      gfx->fillRect(cx + 14, cy + 6, 2, 2, BLACK);
      break;
    }

    default: {
      // Fallback: punto interrogativo
      gfx->drawCircle(cx, cy, 16, 0xFFFF);
      gfx->setFont(u8g2_font_helvB14_tr);
      gfx->setTextColor(0xFFFF);
      gfx->setCursor(cx - 5, cy + 6);
      gfx->print("?");
      break;
    }
  }
}

// ===== Draw Single Game Card =====
void arcadeDrawGameCard(int col, int row, int gameIndex) {
  int x = AM_MARGIN + col * AM_COL_STEP;
  int y = AM_GRID_TOP + row * AM_ROW_STEP;
  uint16_t color = arcadeGetGameColor(arcadeGames[gameIndex].machineType);

  // Background card
  gfx->fillRoundRect(x, y, AM_CELL_W, AM_CELL_H, 8, AM_CARD_BG);

  // Accent bar top
  gfx->fillRect(x + 2, y, AM_CELL_W - 4, 3, color);

  // Icon centered
  arcadeDrawGameIcon(x + AM_CELL_W / 2, y + 50, arcadeGames[gameIndex].machineType);

  // Name centered at bottom
  gfx->setFont(u8g2_font_helvB10_tr);
  int nameW = strlen(arcadeGames[gameIndex].name) * 7;
  int nameX = x + (AM_CELL_W - nameW) / 2;
  if (nameX < x + 2) nameX = x + 2;
  gfx->setTextColor(0xC618);
  gfx->setCursor(nameX, y + AM_CELL_H - 10);
  gfx->print(arcadeGames[gameIndex].name);
}

// ===== Draw Game Selection Menu (4x3 Grid) =====
void arcadeDrawMenu() {
  // Reset font to built-in default (previous modes may set U8g2 fonts)
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextWrap(false);
  gfx->fillScreen(BLACK);

  // Exit button (top-left)
  gfx->fillRoundRect(5, 5, 70, 30, 4, 0xF800);
  gfx->setTextColor(0xFFFF);
  gfx->setTextSize(1);
  gfx->setCursor(12, 14);
  gfx->print("< EXIT");

  // Title "ARCADE"
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(0xFFE0);  // Yellow
  gfx->setCursor(200, 30);
  gfx->print("ARCADE");

  // Draw game cards in 4x3 grid
  for (int i = 0; i < arcadeMachineCount; i++) {
    arcadeDrawGameCard(i % AM_COLS, i / AM_COLS, i);
  }

  // Instructions at bottom
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextColor(0x7BEF);  // Gray
  gfx->setTextSize(1);
  gfx->setCursor(80, 462);
  gfx->print("TAP to select  |  TOP-LEFT to exit mode");
}

// ===== Draw Control Overlay (touch zone rectangles on game screen) =====
void arcadeDrawControlOverlay() {
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextSize(1);

  // ===== TOP BAR =====
  // EXIT zone (0 - TOUCH_EXIT_X, 0 - TOUCH_EXIT_Y)
  gfx->fillRect(0, 0, TOUCH_EXIT_X, TOUCH_EXIT_Y, 0xC000);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(14, 18);
  gfx->print("< EXIT");

  // COIN zone (TOUCH_COIN_X - 479, 0 - TOUCH_COIN_Y)
  gfx->fillRect(TOUCH_COIN_X, 0, 480 - TOUCH_COIN_X, TOUCH_COIN_Y, 0x7B00);
  gfx->setTextColor(0xFFE0);
  gfx->setCursor(TOUCH_COIN_X + 6, 18);
  gfx->print("+ COIN");

  // ===== LEFT BORDER (0 - TOUCH_DPAD_LEFT, TOUCH_EXIT_Y - TOUCH_BOTTOM_Y) =====
  gfx->drawRect(0, TOUCH_EXIT_Y, TOUCH_DPAD_LEFT, TOUCH_BOTTOM_Y - TOUCH_EXIT_Y, 0x2965);
  gfx->setTextColor(0x2965);
  gfx->setTextSize(2);
  gfx->setCursor(50, 230);
  gfx->print("<");
  gfx->setTextSize(1);

  // ===== RIGHT BORDER (TOUCH_DPAD_RIGHT - 479, TOUCH_EXIT_Y - TOUCH_BOTTOM_Y) =====
  gfx->drawRect(TOUCH_DPAD_RIGHT, TOUCH_EXIT_Y, 480 - TOUCH_DPAD_RIGHT, TOUCH_BOTTOM_Y - TOUCH_EXIT_Y, 0x2965);
  gfx->setTextColor(0x2965);
  gfx->setTextSize(2);
  gfx->setCursor(420, 230);
  gfx->print(">");
  gfx->setTextSize(1);

  // ===== UP zone (TOUCH_DPAD_LEFT - TOUCH_DPAD_RIGHT, TOUCH_EXIT_Y - TOUCH_DPAD_MID_Y) =====
  gfx->drawRect(TOUCH_DPAD_LEFT, TOUCH_EXIT_Y, TOUCH_DPAD_RIGHT - TOUCH_DPAD_LEFT, TOUCH_DPAD_MID_Y - TOUCH_EXIT_Y, 0x001F);
  gfx->setTextColor(0x001F);
  gfx->setTextSize(2);
  gfx->setCursor(220, 135);
  gfx->print("UP");
  gfx->setTextSize(1);

  // ===== DOWN zone (TOUCH_DPAD_LEFT - TOUCH_DPAD_RIGHT, TOUCH_DPAD_MID_Y - TOUCH_BOTTOM_Y) =====
  gfx->drawRect(TOUCH_DPAD_LEFT, TOUCH_DPAD_MID_Y, TOUCH_DPAD_RIGHT - TOUCH_DPAD_LEFT, TOUCH_BOTTOM_Y - TOUCH_DPAD_MID_Y, 0x001F);
  gfx->setTextColor(0x001F);
  gfx->setTextSize(2);
  gfx->setCursor(200, 330);
  gfx->print("DOWN");
  gfx->setTextSize(1);

  // ===== BOTTOM BAR (y >= TOUCH_BOTTOM_Y) =====
  // BACK: 0-159
  gfx->fillRect(0, TOUCH_BOTTOM_Y, 159, 480 - TOUCH_BOTTOM_Y, 0x4208);
  gfx->setTextColor(0xFFFF);
  gfx->setCursor(40, TOUCH_BOTTOM_Y + 18);
  gfx->print("<< BACK");

  // START: 160-319
  gfx->fillRect(160, TOUCH_BOTTOM_Y, 159, 480 - TOUCH_BOTTOM_Y, 0x0320);
  gfx->setTextColor(0x07E0);
  gfx->setCursor(200, TOUCH_BOTTOM_Y + 18);
  gfx->print("START");

  // FIRE: 320-479
  gfx->fillRect(320, TOUCH_BOTTOM_Y, 160, 480 - TOUCH_BOTTOM_Y, 0x8000);
  gfx->setTextColor(0xF800);
  gfx->setCursor(370, TOUCH_BOTTOM_Y + 18);
  gfx->print("FIRE >>");

  // Separator lines
  gfx->drawFastVLine(159, TOUCH_BOTTOM_Y, 480 - TOUCH_BOTTOM_Y, 0x7BEF);
  gfx->drawFastVLine(319, TOUCH_BOTTOM_Y, 480 - TOUCH_BOTTOM_Y, 0x7BEF);
  gfx->drawFastHLine(0, TOUCH_EXIT_Y, 480, 0x2965);
}

// ===== Virtual Button State Machines (COIN e START indipendenti) =====
void arcadeUpdateButtonStateMachines() {
  unsigned long now = millis();

  // COIN: press 120ms -> release
  if (arcadeCoinState == 1) {
    arcadeButtonState |= BUTTON_COIN;
    if (now - arcadeCoinTimer > 120) { arcadeCoinState = 2; arcadeCoinTimer = now; }
  } else if (arcadeCoinState == 2) {
    arcadeButtonState &= ~BUTTON_COIN;
    arcadeCoinState = 0;
  }

  // START: press 120ms -> release
  if (arcadeStartState == 1) {
    arcadeButtonState |= BUTTON_START;
    if (now - arcadeStartTimer > 120) { arcadeStartState = 2; arcadeStartTimer = now; }
  } else if (arcadeStartState == 2) {
    arcadeButtonState &= ~BUTTON_START;
    arcadeStartState = 0;
  }
}

// Inserisce moneta (solo COIN, non avvia il gioco)
void arcadeTriggerCoin() {
  if (arcadeCoinState == 0) { arcadeCoinState = 1; arcadeCoinTimer = millis(); }
}

// Preme START (avvia il gioco coi crediti inseriti)
void arcadeTriggerStart() {
  if (arcadeStartState == 0) { arcadeStartState = 1; arcadeStartTimer = millis(); }
}

// ===== In-Game Touch: TAP actions (EXIT, COIN, BACK, START, FIRE) =====
void handleArcadeGameTouch(int x, int y) {
  if (!arcadeInitialized || arcadeInMenu) return;

  // EXIT: top-left
  if (x < TOUCH_EXIT_X && y < TOUCH_EXIT_Y) {
    arcadeStopGame();
    handleModeChange();
    return;
  }

  // COIN: top-right (inserisce credito)
  if (x >= TOUCH_COIN_X && y < TOUCH_COIN_Y) {
    arcadeTriggerCoin();
    return;
  }

  // Bottom bar
  if (y >= TOUCH_BOTTOM_Y) {
    if (x < 160) {
      arcadeStopGame();           // BACK
    } else if (x < 320) {
      arcadeTriggerStart();       // START (avvia con crediti)
    } else {
      arcadeButtonState |= BUTTON_FIRE;  // FIRE
    }
    return;
  }
}

// ===== In-Game Touch: Continuous D-pad tracking =====
void arcadeUpdateTouchInput(int x, int y) {
  if (!arcadeInitialized || arcadeInMenu) return;

  uint8_t buttons = 0;

  // Preserve coin/start state machines
  if (arcadeCoinState > 0 || arcadeStartState > 0) {
    buttons = arcadeButtonState & (BUTTON_COIN | BUTTON_START);
  }

  // LEFT
  if (x < TOUCH_DPAD_LEFT && y >= TOUCH_EXIT_Y && y < TOUCH_BOTTOM_Y)
    buttons |= BUTTON_LEFT;

  // RIGHT
  if (x > TOUCH_DPAD_RIGHT && y >= TOUCH_EXIT_Y && y < TOUCH_BOTTOM_Y)
    buttons |= BUTTON_RIGHT;

  // UP
  if (x >= TOUCH_DPAD_LEFT && x <= TOUCH_DPAD_RIGHT &&
      y >= TOUCH_EXIT_Y && y < TOUCH_DPAD_MID_Y)
    buttons |= BUTTON_UP;

  // DOWN
  if (x >= TOUCH_DPAD_LEFT && x <= TOUCH_DPAD_RIGHT &&
      y >= TOUCH_DPAD_MID_Y && y < TOUCH_BOTTOM_Y)
    buttons |= BUTTON_DOWN;

  // FIRE held
  if (y >= TOUCH_BOTTOM_Y && x >= 320)
    buttons |= BUTTON_FIRE;

  arcadeButtonState = buttons;
}

// ===== Touch released =====
void arcadeClearTouchInput() {
  if (!arcadeInitialized || arcadeInMenu) return;
  uint8_t preserve = 0;
  if (arcadeCoinState > 0 || arcadeStartState > 0)
    preserve = arcadeButtonState & (BUTTON_COIN | BUTTON_START);
  arcadeButtonState = preserve;
}

// ===== Touch Handler (menu only) =====
void handleArcadeMenuTouch(int x, int y) {
  if (!arcadeInitialized || !arcadeInMenu) return;

  // EXIT: top-left corner (pulsante rosso < EXIT)
  if (x < 80 && y < 40) {
    handleModeChange();
    return;
  }

  // Grid hit test: seleziona gioco
  if (y >= AM_GRID_TOP && y < AM_GRID_TOP + AM_ROWS * AM_ROW_STEP) {
    int col = (x - AM_MARGIN) / AM_COL_STEP;
    int row = (y - AM_GRID_TOP) / AM_ROW_STEP;
    if (col >= 0 && col < AM_COLS && row >= 0 && row < AM_ROWS) {
      int idx = row * AM_COLS + col;
      if (idx < arcadeMachineCount) {
        // Visual feedback: highlight card
        int cx = AM_MARGIN + col * AM_COL_STEP;
        int cy = AM_GRID_TOP + row * AM_ROW_STEP;
        uint16_t color = arcadeGetGameColor(arcadeGames[idx].machineType);
        gfx->drawRoundRect(cx, cy, AM_CELL_W, AM_CELL_H, 8, color);
        gfx->drawRoundRect(cx + 1, cy + 1, AM_CELL_W - 2, AM_CELL_H - 2, 7, color);
        delay(200);
        arcadeStartGame(idx);
      }
    }
  }
}

#endif // EFFECT_ARCADE
