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
// Batched rendering: 4 tile rows per draw call = 9 draw calls instead of 36
#define ARC_BATCH_ROWS  4
#define ARC_BATCH_OUT_H (12 * ARC_BATCH_ROWS)  // 48 output lines per batch
// Scale buffer: 336 pixels * 48 lines * 2 bytes = 32256 bytes (PSRAM)
#define ARC_SCALE_PIXELS (ARC_OUT_W * ARC_BATCH_OUT_H)

// ===== Touch Zone Definitions =====
// Game area: 72-407 X, 24-455 Y (336x432 centered)
// Control zones on full 480x480 touch area
#define TOUCH_EXIT_X    80   // Exit zone: top-left corner
#define TOUCH_EXIT_Y    48
#define TOUCH_COIN_X    400  // Coin zone: top-right corner
#define TOUCH_COIN_Y    48
#define TOUCH_DPAD_LEFT  140  // D-pad left boundary
#define TOUCH_DPAD_RIGHT 340  // D-pad right boundary
#define TOUCH_DPAD_MID_Y 240  // Split between UP and DOWN zones
#define TOUCH_BOTTOM_Y   432  // Bottom control bar start
// Bottom bar: 0-159=BACK, 160-319=START, 320-479=FIRE

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

// Emulation task
TaskHandle_t arcadeEmulationTaskHandle = nullptr;

// Frame timing
unsigned long arcadeLastFrameTime = 0;
unsigned long arcadeFrameCount = 0;

// Virtual coin state machine (for touch coin insertion)
uint8_t arcadeCoinState = 0;       // 0=idle, 1=coin_press, 2=coin_release, 3=start_press, 4=start_release
unsigned long arcadeCoinTimer = 0;

// Menu scroll
int8_t arcadeMenuScroll = 0;
int8_t arcadeMenuCursor = 0;

// ===== Forward Declarations =====
void arcadeStartGame(int8_t index);
void arcadeStopGame();
void arcadeEmulationTask(void* param);
void arcadeUpscaleRow(unsigned short* src, unsigned short* dst, int dstOffsetY);
void arcadeDrawMenu();
void arcadeDrawControlOverlay();
void arcadeBuildGameList();
void arcadeAllocateBuffers();
void arcadeFreeBuffers();
void arcadeUpdateCoinStateMachine();
void arcadeTriggerCoinStart();

// ===== Initialization =====
void initArcade() {
  if (arcadeInitialized) return;

  Serial.println("[ARCADE] Initializing Arcade Emulator");

  // Stop music playback to free I2S for arcade audio synthesis
  audio.stopSong();
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
  arcadeMenuCursor = 0;
  arcadeMenuScroll = 0;

  // Clear screen and draw menu
  gfx->fillScreen(BLACK);
  arcadeDrawMenu();

  arcadeInitialized = true;
  Serial.println("[ARCADE] Initialized OK, games available: " + String(arcadeMachineCount));
}

// ===== Cleanup =====
void cleanupArcade() {
  if (!arcadeInitialized) return;

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

  Serial.printf("[ARCADE] Starting game: %s\n", arcadeGames[index].name);

  // Create machine instance
  arcadeCurrentMachine = arcadeCreateMachine(arcadeGames[index].machineType);
  if (!arcadeCurrentMachine) {
    Serial.println("[ARCADE] Failed to create machine!");
    return;
  }

  // Initialize machine with buffers
  arcadeCurrentMachine->init(&arcadeInput, arcadeFrameBuffer, arcadeSpriteBuffer, arcadeMemory);
  arcadeCurrentMachine->reset();

  // Start arcade audio synthesis for this game
  if (arcadeAudio) arcadeAudio->start(arcadeCurrentMachine);

  // Clear screen with black, draw border
  gfx->fillScreen(BLACK);

  // Draw side borders (decorative)
  gfx->fillRect(0, 0, ARC_OFFSET_X, 480, 0x0000);               // Left border
  gfx->fillRect(ARC_OFFSET_X + ARC_OUT_W, 0, 480 - ARC_OFFSET_X - ARC_OUT_W, 480, 0x0000); // Right border

  arcadeInMenu = false;
  arcadeSelectedGame = index;
  arcadeButtonState = 0;
  arcadeInput.setButtons(0);
  arcadeCoinState = 0;
  arcadeFrameCount = 0;
  arcadeLastFrameTime = millis();

  // Create emulation task on Core 0
  xTaskCreatePinnedToCore(
    arcadeEmulationTask,
    "arcadeZ80",
    4096,
    nullptr,
    2,                    // Priority 2 (higher than idle)
    &arcadeEmulationTaskHandle,
    ARCADE_EMULATION_CORE
  );

  Serial.println("[ARCADE] Game started, emulation task created on Core 0");
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

  // Return to menu
  arcadeInMenu = true;
  arcadeSelectedGame = -1;

  // Redraw menu
  gfx->fillScreen(BLACK);
  arcadeDrawMenu();
}

// ===== Emulation Task (Core 0) =====
void arcadeEmulationTask(void* param) {
  while (true) {
    if (arcadeCurrentMachine) {
      if (!arcadeCurrentMachine->game_started) {
        // FAST BOOT: run 20 emulation frames per tick (~20x faster boot)
        // Pac-Man boot does RAM test + init, takes ~180 frames normally
        // With this optimization: ~200ms instead of ~3 seconds
        for (int i = 0; i < 20; i++) {
          arcadeCurrentMachine->run_frame();
          if (arcadeCurrentMachine->game_started) break;
        }
        vTaskDelay(1);  // Feed watchdog
      } else {
        // Normal gameplay: one frame, then wait for vsync from renderer
        arcadeCurrentMachine->run_frame();
        ulTaskNotifyTake(1, portMAX_DELAY);
      }
    } else {
      vTaskDelay(10);
    }
  }
}

// ===== Upscale Row: 224x8 -> 336x12 (1.5x nearest-neighbor) =====
// 3:2 scaling: every 2 source pixels become 3 destination pixels
// dstOffsetY = vertical offset in destination buffer (for row batching)
void arcadeUpscaleRow(unsigned short* src, unsigned short* dst, int dstOffsetY) {
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
        unsigned short p0 = (srcLine[srcX] >> 8) | (srcLine[srcX] << 8);
        unsigned short p1 = (srcLine[srcX + 1] >> 8) | (srcLine[srcX + 1] << 8);
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
    // Handle virtual coin state machine
    arcadeUpdateCoinStateMachine();
    delay(16);  // Save CPU
    return;
  }

  // Game is running - render frame
  if (!arcadeCurrentMachine) return;

  // FAST BOOT: skip rendering during Z80 boot, show loading text
  if (!arcadeCurrentMachine->game_started) {
    static bool loadingShown = false;
    if (!loadingShown) {
      gfx->fillScreen(BLACK);
      gfx->setFont((const GFXfont *)NULL);
      gfx->setTextColor(0xFFE0);  // Yellow
      gfx->setTextSize(3);
      gfx->setCursor(110, 210);
      gfx->print("LOADING...");
      gfx->setTextSize(1);
      gfx->setTextColor(0x7BEF);
      gfx->setCursor(140, 260);
      gfx->print("Z80 boot in corso");
      loadingShown = true;
    }
    delay(50);  // Don't busy-wait, let emulation run on Core 0
    return;
  }

  unsigned long frameStart = millis();

  // Update input from touch state
  arcadeInput.setButtons(arcadeButtonState);

  // Handle virtual coin state machine
  arcadeUpdateCoinStateMachine();

  // Prepare frame (extract sprites from Z80 RAM)
  arcadeCurrentMachine->prepare_frame();

  // Render in batches of 4 tile rows per draw call (9 calls instead of 36)
  // startWrite/endWrite keeps SPI transaction open for entire frame
  gfx->startWrite();
  for (int batch = 0; batch < 9; batch++) {
    int rowStart = batch * ARC_BATCH_ROWS;

    // Render and upscale 4 tile rows into the batch buffer
    for (int r = 0; r < ARC_BATCH_ROWS; r++) {
      int row = rowStart + r;
      memset(arcadeFrameBuffer, 0, ARC_ROW_PIXELS * sizeof(unsigned short));
      arcadeCurrentMachine->render_row(row);
      arcadeUpscaleRow(arcadeFrameBuffer, arcadeScaleBuffer, r * 12);
    }

    // Single blit for the entire batch (336 x 48 pixels)
    gfx->draw16bitRGBBitmap(ARC_OFFSET_X, ARC_OFFSET_Y + batch * ARC_BATCH_OUT_H,
                             arcadeScaleBuffer, ARC_OUT_W, ARC_BATCH_OUT_H);

    // Transmit arcade audio every 3 batches (3 calls/frame, evenly spaced ~5.6ms)
    // Not every batch (9x = too much overhead) or once (1x = DMA starvation)
    if (arcadeAudio && (batch % 3 == 2)) arcadeAudio->transmit();
  }
  gfx->endWrite();

  // Frame timing
  unsigned long frameTime = millis() - frameStart;
  if (frameTime < ARC_FRAME_MS) {
    delay(ARC_FRAME_MS - frameTime);
  }

  // Notify emulation task that frame is rendered (vsync)
  if (arcadeEmulationTaskHandle) {
    xTaskNotifyGive(arcadeEmulationTaskHandle);
  }

  arcadeFrameCount++;
}

// ===== Virtual Coin State Machine =====
// Simulates coin insertion: COIN press -> release -> START press -> release
void arcadeUpdateCoinStateMachine() {
  if (arcadeCoinState == 0) return;

  unsigned long now = millis();
  switch (arcadeCoinState) {
    case 1: // Coin pressed
      arcadeButtonState |= BUTTON_COIN;
      if (now - arcadeCoinTimer > 100) {
        arcadeCoinState = 2;
        arcadeCoinTimer = now;
      }
      break;

    case 2: // Coin released, pause
      arcadeButtonState &= ~BUTTON_COIN;
      if (now - arcadeCoinTimer > 200) {
        arcadeCoinState = 3;
        arcadeCoinTimer = now;
      }
      break;

    case 3: // Start pressed
      arcadeButtonState |= BUTTON_START;
      if (now - arcadeCoinTimer > 100) {
        arcadeCoinState = 4;
        arcadeCoinTimer = now;
      }
      break;

    case 4: // Start released, done
      arcadeButtonState &= ~BUTTON_START;
      arcadeCoinState = 0;
      break;
  }
}

// ===== Trigger Coin Insert =====
void arcadeTriggerCoinStart() {
  if (arcadeCoinState == 0) {
    arcadeCoinState = 1;
    arcadeCoinTimer = millis();
  }
}

// ===== Draw Game Selection Menu =====
void arcadeDrawMenu() {
  // Reset font to built-in default (previous modes may set U8g2 fonts)
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextWrap(false);
  gfx->fillScreen(BLACK);

  // Title
  gfx->setTextColor(0xFFE0);  // Yellow
  gfx->setTextSize(3);
  gfx->setCursor(100, 20);
  gfx->print("ARCADE");

  gfx->setTextSize(2);
  gfx->setTextColor(0xF800);  // Red
  gfx->setCursor(100, 55);
  gfx->print("SELECT GAME");

  // Draw game list
  int maxVisible = 8;
  int startIdx = arcadeMenuScroll;
  int endIdx = min((int)arcadeMachineCount, startIdx + maxVisible);

  for (int i = startIdx; i < endIdx; i++) {
    int y = 100 + (i - startIdx) * 42;
    bool selected = (i == arcadeMenuCursor);

    if (selected) {
      // Highlight selected game
      gfx->fillRoundRect(30, y - 4, 420, 38, 6, 0x001F);  // Blue background
      gfx->setTextColor(0xFFFF);  // White text
    } else {
      gfx->setTextColor(0xC618);  // Light gray
    }

    gfx->setTextSize(2);
    gfx->setCursor(50, y + 6);

    // Game number
    gfx->print(String(i + 1) + ". ");
    gfx->print(arcadeGames[i].name);
  }

  // Scroll indicators
  if (startIdx > 0) {
    gfx->setTextColor(0x07E0);  // Green
    gfx->setTextSize(2);
    gfx->setCursor(220, 85);
    gfx->print("^");
  }
  if (endIdx < arcadeMachineCount) {
    gfx->setTextColor(0x07E0);
    gfx->setTextSize(2);
    gfx->setCursor(220, 100 + maxVisible * 42);
    gfx->print("v");
  }

  // Instructions
  gfx->setTextColor(0x7BEF);  // Gray
  gfx->setTextSize(1);
  gfx->setCursor(80, 450);
  gfx->print("TAP to select  |  TOP-LEFT to exit mode");

  // Exit button indicator
  gfx->fillRoundRect(5, 5, 70, 30, 4, 0xF800);
  gfx->setTextColor(0xFFFF);
  gfx->setTextSize(1);
  gfx->setCursor(12, 14);
  gfx->print("< EXIT");
}

// ===== Draw Control Overlay (semi-transparent touch zones) =====
void arcadeDrawControlOverlay() {
  // Draw faint control zone indicators on the sides and bottom
  // Left arrow
  gfx->drawRect(0, 48, TOUCH_DPAD_LEFT, 384, 0x2104);
  gfx->setTextColor(0x4208);
  gfx->setTextSize(2);
  gfx->setCursor(50, 230);
  gfx->print("<");

  // Right arrow
  gfx->drawRect(TOUCH_DPAD_RIGHT, 48, 480 - TOUCH_DPAD_RIGHT, 384, 0x2104);
  gfx->setCursor(400, 230);
  gfx->print(">");

  // Bottom bar labels
  gfx->setTextSize(1);
  gfx->setCursor(55, 455);
  gfx->print("BACK");
  gfx->setCursor(215, 455);
  gfx->print("START");
  gfx->setCursor(380, 455);
  gfx->print("FIRE");
}

// ===== Touch Handlers =====

// Called on TAP (from 1_TOUCH.ino - waitingForRelease pattern)
void handleArcadeTouch(int x, int y) {
  if (!arcadeInitialized) return;

  // EXIT: top-left corner (always active)
  if (x < TOUCH_EXIT_X && y < TOUCH_EXIT_Y) {
    if (arcadeInMenu) {
      // Exit arcade mode entirely
      handleModeChange();
    } else {
      // Exit current game, return to menu
      arcadeStopGame();
    }
    return;
  }

  if (arcadeInMenu) {
    // Menu touch handling
    int maxVisible = 8;
    int startIdx = arcadeMenuScroll;

    // Check if touch is on a game entry
    for (int i = startIdx; i < min((int)arcadeMachineCount, startIdx + maxVisible); i++) {
      int y_top = 96 + (i - startIdx) * 42;
      int y_bot = y_top + 38;
      if (x > 30 && x < 450 && y >= y_top && y <= y_bot) {
        arcadeMenuCursor = i;
        arcadeDrawMenu();
        delay(200);  // Brief visual feedback
        arcadeStartGame(i);
        return;
      }
    }

    // Scroll up/down
    if (y > 400 && arcadeMenuScroll + maxVisible < arcadeMachineCount) {
      arcadeMenuScroll++;
      arcadeDrawMenu();
    } else if (y < 90 && y > TOUCH_EXIT_Y && arcadeMenuScroll > 0) {
      arcadeMenuScroll--;
      arcadeDrawMenu();
    }
    return;
  }

  // In-game tap handling

  // COIN: top-right corner
  if (x > TOUCH_COIN_X && y < TOUCH_COIN_Y) {
    arcadeTriggerCoinStart();
    return;
  }

  // Bottom bar: BACK / START / FIRE (single tap actions)
  if (y >= TOUCH_BOTTOM_Y) {
    if (x < 160) {
      // BACK = same as EXIT for games
      arcadeStopGame();
    } else if (x < 320) {
      // START button
      arcadeTriggerCoinStart();
    } else {
      // FIRE button (momentary via tap)
      arcadeButtonState |= BUTTON_FIRE;
      // Will be cleared by arcadeClearTouchInput on release
    }
    return;
  }
}

// Called continuously while touch is held (from 1_TOUCH.ino tracking)
void arcadeUpdateTouchInput(int x, int y) {
  if (!arcadeInitialized || arcadeInMenu) return;

  uint8_t buttons = 0;

  // Preserve coin/start state machine buttons
  if (arcadeCoinState > 0) {
    buttons = arcadeButtonState & (BUTTON_COIN | BUTTON_START);
  }

  // D-pad: LEFT zone (left side of screen)
  if (x < TOUCH_DPAD_LEFT && y >= TOUCH_EXIT_Y && y < TOUCH_BOTTOM_Y) {
    buttons |= BUTTON_LEFT;
  }

  // D-pad: RIGHT zone (right side of screen)
  if (x > TOUCH_DPAD_RIGHT && y >= TOUCH_COIN_Y && y < TOUCH_BOTTOM_Y) {
    buttons |= BUTTON_RIGHT;
  }

  // D-pad: UP zone (center-top area)
  if (x >= TOUCH_DPAD_LEFT && x <= TOUCH_DPAD_RIGHT &&
      y >= TOUCH_EXIT_Y && y < TOUCH_DPAD_MID_Y) {
    buttons |= BUTTON_UP;
  }

  // D-pad: DOWN zone (center-bottom area)
  if (x >= TOUCH_DPAD_LEFT && x <= TOUCH_DPAD_RIGHT &&
      y >= TOUCH_DPAD_MID_Y && y < TOUCH_BOTTOM_Y) {
    buttons |= BUTTON_DOWN;
  }

  // FIRE: bottom-right held
  if (y >= TOUCH_BOTTOM_Y && x >= 320) {
    buttons |= BUTTON_FIRE;
  }

  arcadeButtonState = buttons;
}

// Called when touch is released
void arcadeClearTouchInput() {
  if (!arcadeInitialized || arcadeInMenu) return;

  // Clear directional + fire buttons, preserve coin state machine
  uint8_t preserve = 0;
  if (arcadeCoinState > 0) {
    preserve = arcadeButtonState & (BUTTON_COIN | BUTTON_START);
  }
  arcadeButtonState = preserve;
}

#endif // EFFECT_ARCADE
