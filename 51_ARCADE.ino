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

#include "esp_task_wdt.h"
#include <esp_system.h>

// RTC memory survives soft resets - tracks if arcade was running when crash occurred
RTC_DATA_ATTR uint8_t arcadeCrashFlag = 0;
RTC_DATA_ATTR uint8_t arcadeCrashGame = 0xFF;

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
#ifdef ENABLE_LADYBUG
#include "arcade/machines/ladybug/ladybug.h"
#endif
#ifdef ENABLE_XEVIOUS
#include "arcade/machines/xevious/xevious.h"
#endif
#ifdef ENABLE_BOMBJACK
#include "arcade/machines/bombjack/bombjack.h"
#endif
#ifdef ENABLE_GYRUSS
#include "arcade/machines/gyruss/gyruss.h"
#endif

// ============================================================================
// Arduino IDE 1.8.x does NOT compile .c/.cpp files in subdirectories.
// Include source files directly so they are compiled as part of the sketch.
// ============================================================================
extern "C" {
  #include "arcade/cpus/z80/Z80.c"
  #include "arcade/cpus/i8048/i8048.c"
  #ifdef ENABLE_GYRUSS
  #include "arcade/cpus/m6809/m6809.c"
  #endif
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
#ifdef ENABLE_LADYBUG
#include "arcade/machines/ladybug/ladybug.cpp"
#endif
#ifdef ENABLE_XEVIOUS
#include "arcade/machines/xevious/xevious.cpp"
#endif
#ifdef ENABLE_BOMBJACK
#include "arcade/machines/bombjack/bombjack.cpp"
#endif
#ifdef ENABLE_GYRUSS
#include "arcade/machines/gyruss/gyruss.cpp"
#endif

// Arcade audio synthesis (Namco WSG, AY-3-8910, DK PCM)
#include "arcade/arcade_audio.h"
#include "arcade/arcade_audio.cpp"

// ===== Display Constants =====
// Row buffer: one tile row = max(224,256) pixels * 8 lines * 2 bytes
#define ARC_ROW_PIXELS      (ARC_GAME_W * 8)       // 224*8 for standard games
#define ARC_ROW_PIXELS_MAX  (ARC_GAME_W_MAX * 8)   // 256*8 for wide games (Bomb Jack)
// Batched rendering: 12 tile rows per draw call = 3 draw calls instead of 36
#define ARC_BATCH_ROWS  12
#define ARC_BATCH_OUT_H (12 * ARC_BATCH_ROWS)  // 144 output lines per batch
// Scale buffer: max(336,384) * 144 for wide games (Bomb Jack: 256*1.5=384)
#define ARC_OUT_W_WIDE  384   // 256 * 1.5
#define ARC_SCALE_PIXELS_MAX (ARC_OUT_W_WIDE * ARC_BATCH_OUT_H)

// ===== Touch Zone Definitions =====
// Side panels (action buttons): Left X 0-71, Right X 408-479
#define ARC_LB  72    // Left border width (= ARC_OFFSET_X)
#define ARC_RB  408   // Right border start (= ARC_OFFSET_X + ARC_OUT_W)
#define ARC_BM  3     // Button draw margin
// Left panel action buttons
#define ARC_EXIT_Y2     75
#define ARC_BACK_Y1    404
// Right panel action buttons
#define ARC_COIN_Y2     75
#define ARC_FIRE_Y1    164
#define ARC_FIRE_Y2    315
#define ARC_START_Y1   322
// D-pad zones (full screen, same as original)
#define ARC_DPAD_LEFT   140
#define ARC_DPAD_RIGHT  340
#define ARC_DPAD_MID_Y  240
#define ARC_DPAD_TOP     48
#define ARC_DPAD_BOTTOM 432

// ===== Menu Grid Layout (4x3 per page, pagination for >12 games) =====
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
#define AM_PER_PAGE  (AM_COLS * AM_ROWS)  // 12 games per page

// ===== Game List =====
#define ARCADE_MAX_GAMES 16

struct ArcadeGameInfo {
  const char* name;
  uint8_t machineType;
  bool enabled;
};

// ===== Global Variables =====
bool arcadeInitialized = false;
int8_t arcadeSelectedGame = -1;  // -1 = menu
bool arcadeInMenu = true;
int8_t arcadeMenuPage = 0;       // Current menu page (0-based)
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
unsigned short* arcadeFullFrame = nullptr;     // Full frame for rotated games (PSRAM)

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
volatile int arcadeBootFrames = 0;  // Shared boot frame counter (Core0 writes, Core1 reads)

// Frame timing
unsigned long arcadeLastFrameTime = 0;
unsigned long arcadeFrameCount = 0;
unsigned long arcadeBootStartMs = 0;   // Tracks boot duration (reset per game)

// ===== Arcade Enabled Mask (EEPROM 949-951) =====
#define ARCADE_EEPROM_MASK_LO  949
#define ARCADE_EEPROM_MASK_HI  950
#define ARCADE_EEPROM_MASK_MARKER 951
#define ARCADE_MASK_MARKER_VAL 0xAC

uint16_t arcadeEnabledMask = 0xFFFF;  // default: all enabled

void loadArcadeEnabledMask() {
  if (EEPROM.read(ARCADE_EEPROM_MASK_MARKER) == ARCADE_MASK_MARKER_VAL) {
    arcadeEnabledMask = EEPROM.read(ARCADE_EEPROM_MASK_LO) | (EEPROM.read(ARCADE_EEPROM_MASK_HI) << 8);
  } else {
    arcadeEnabledMask = 0xFFFF;
  }
}

void saveArcadeEnabledMask() {
  EEPROM.write(ARCADE_EEPROM_MASK_LO, arcadeEnabledMask & 0xFF);
  EEPROM.write(ARCADE_EEPROM_MASK_HI, (arcadeEnabledMask >> 8) & 0xFF);
  EEPROM.write(ARCADE_EEPROM_MASK_MARKER, ARCADE_MASK_MARKER_VAL);
  EEPROM.commit();
}

bool isArcadeGameEnabled(int idx) {
  if (idx < 0 || idx >= arcadeMachineCount) return false;
  return (arcadeEnabledMask >> idx) & 1;
}

void setArcadeGameEnabled(int idx, bool en) {
  if (idx < 0 || idx >= ARCADE_MAX_GAMES) return;
  if (en) arcadeEnabledMask |= (1 << idx);
  else    arcadeEnabledMask &= ~(1 << idx);
  saveArcadeEnabledMask();
}

// ===== Forward Declarations =====
void arcadeStartGame(int8_t index);
void arcadeStopGame();
void arcadeEmulationTask(void* param);
IRAM_ATTR void arcadeUpscaleRow(unsigned short* src, unsigned short* dst, int dstOffsetY);
IRAM_ATTR void arcadeUpscaleRowWide(unsigned short* src, unsigned short* dst, int dstOffsetY);
void arcadeDrawMenu();
void arcadeDrawControlOverlay();
void arcadeBuildGameList();
uint16_t arcadeGetGameColor(uint8_t machineType);
void arcadeDrawGameIcon(int cx, int cy, uint8_t machineType);
void arcadeDrawGameCard(int col, int row, int gameIndex);
void arcadeAllocateBuffers();
void arcadeAllocateFullFrame(int natW, int natH);
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

  // Guard: SD card with ARCADE folder required
  if (!arcadeRomsAvailable()) {
    gfx->fillScreen(BLACK);
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(0xF800);  // Red
    gfx->setCursor(80, 200);
    gfx->print("SD CARD NON TROVATA");
    gfx->setFont((const GFXfont *)NULL);
    gfx->setTextSize(1);
    gfx->setTextColor(0x7BEF);  // Gray
    gfx->setCursor(100, 240);
    gfx->print("Inserisci una SD con la cartella");
    gfx->setCursor(130, 260);
    gfx->print("/ARCADE per giocare");
    delay(2000);
    handleModeChange();
    return;
  }

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
  // Uses audioStopWithTimeout() from 2_CHANGE_MODE.ino (max 3s instead of 30-60s)
  #ifdef AUDIO
  audioStopWithTimeout();
  #endif

  // Arcade audio: sets I2S to 24kHz, uses Audio library's existing driver
  arcadeAudio = new ArcadeAudio();
  arcadeAudio->init();

  // Reset gfx state (previous modes may leave custom fonts/sizes)
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextSize(1);
  gfx->setTextWrap(false);

  // Build available games list
  arcadeBuildGameList();
  loadArcadeEnabledMask();
  // Apply enabled mask to game list
  for (int i = 0; i < arcadeMachineCount; i++) {
    arcadeGames[i].enabled = isArcadeGameEnabled(i);
  }

  // Allocate PSRAM buffers
  arcadeAllocateBuffers();

  if (!arcadeFrameBuffer || !arcadeSpriteBuffer || !arcadeMemory || !arcadeScaleBuffer) {
    arcadeFreeBuffers();
    return;
  }

  // Start in menu
  arcadeInMenu = true;
  arcadeMenuPage = 0;
  arcadeSelectedGame = -1;

  // Clear screen and draw menu
  gfx->fillScreen(BLACK);
  arcadeDrawMenu();

  // Attiva bubble arcade: congela tutti i servizi del loop principale
  arcadeBubbleActive = true;

  // Spegni WiFi per liberare memoria heap per l'emulatore arcade
  Serial.println("[ARCADE] Disattivazione WiFi per liberare heap...");
  WiFi.disconnect(true);  // true = cancella credenziali in RAM (restano in EEPROM)
  WiFi.mode(WIFI_OFF);
  delay(500);  // Attendi rilascio completo risorse WiFi (BLE ha bisogno di heap libero)
  Serial.printf("[ARCADE] Heap libero dopo WiFi OFF: %d bytes\n", ESP.getFreeHeap());

  // Attiva Bluetooth gamepad (NimBLE) - supporta Xbox BLE, 8BitDo, gamepad HID generici
  // NimBLE init viene eseguito in background task - non blocca il menu
  bleGamepadInit();

  arcadeInitialized = true;
}

// ===== Cleanup =====
void cleanupArcade() {
  if (!arcadeInitialized) return;

  // PRIMA ferma il gioco: il task di emulazione chiama bleGamepadGetButtons()
  // quindi deve essere terminato PRIMA di deinizializzare NimBLE
  arcadeStopGame();

  // Disattiva bubble arcade: ripristina tutti i servizi
  arcadeBubbleActive = false;

  // ORA cleanup BLE gamepad (il task Z80 non accede piu' a NimBLE)
  bleGamepadCleanup();
  delay(100);  // Attendi rilascio risorse BLE prima di riavviare WiFi

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

  // Ripristina WiFi e tutti i servizi di rete
  Serial.println("[ARCADE] Ripristino WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // Necessario per Espalexa
  WiFi.begin();  // Riconnette con credenziali salvate in flash
  Serial.printf("[ARCADE] WiFi riconnessione avviata, heap: %d bytes\n", ESP.getFreeHeap());
}

// ===== Check if Arcade ROMs are available on SD =====
bool arcadeRomsAvailable() {
  extern bool sdCardPresent;
  if (!sdCardPresent) return false;
  if (SD.cardType() == CARD_NONE) return false;
  return SD.exists("/ARCADE");
}

// ===== Build Game List (only games with ROM files on SD) =====
void arcadeBuildGameList() {
  arcadeMachineCount = 0;

  // Check ALL required ROMs per game (multi-CPU games need rom2/rom3 too!)
  // Without all ROMs, sub-CPUs read from 1-byte PROGMEM stub → crash
#ifdef ENABLE_PACMAN
  if (SD.exists("/ARCADE/PACMAN/rom1.bin"))
    arcadeGames[arcadeMachineCount++] = {"PAC-MAN", MCH_PACMAN, true};
#endif
#ifdef ENABLE_GALAGA
  if (SD.exists("/ARCADE/GALAGA/rom1.bin") && SD.exists("/ARCADE/GALAGA/rom2.bin") &&
      SD.exists("/ARCADE/GALAGA/rom3.bin"))
    arcadeGames[arcadeMachineCount++] = {"GALAGA", MCH_GALAGA, true};
#endif
#ifdef ENABLE_DKONG
  if (SD.exists("/ARCADE/DKONG/rom1.bin") && SD.exists("/ARCADE/DKONG/rom2.bin"))
    arcadeGames[arcadeMachineCount++] = {"DONKEY KONG", MCH_DKONG, true};
#endif
#ifdef ENABLE_FROGGER
  if (SD.exists("/ARCADE/FROGGER/rom1.bin") && SD.exists("/ARCADE/FROGGER/rom2.bin"))
    arcadeGames[arcadeMachineCount++] = {"FROGGER", MCH_FROGGER, true};
#endif
#ifdef ENABLE_DIGDUG
  if (SD.exists("/ARCADE/DIGDUG/rom1.bin") && SD.exists("/ARCADE/DIGDUG/rom2.bin") &&
      SD.exists("/ARCADE/DIGDUG/rom3.bin"))
    arcadeGames[arcadeMachineCount++] = {"DIG DUG", MCH_DIGDUG, true};
#endif
#ifdef ENABLE_1942
  if (SD.exists("/ARCADE/1942/rom1.bin") && SD.exists("/ARCADE/1942/rom2.bin") &&
      SD.exists("/ARCADE/1942/rom1_b0.bin") && SD.exists("/ARCADE/1942/rom1_b1.bin") &&
      SD.exists("/ARCADE/1942/rom1_b2.bin"))
    arcadeGames[arcadeMachineCount++] = {"1942", MCH_1942, true};
#endif
#ifdef ENABLE_EYES
  if (SD.exists("/ARCADE/EYES/rom1.bin"))
    arcadeGames[arcadeMachineCount++] = {"EYES", MCH_EYES, true};
#endif
#ifdef ENABLE_MRTNT
  if (SD.exists("/ARCADE/MRTNT/rom1.bin"))
    arcadeGames[arcadeMachineCount++] = {"MR. TNT", MCH_MRTNT, true};
#endif
#ifdef ENABLE_LIZWIZ
  if (SD.exists("/ARCADE/LIZWIZ/rom1.bin"))
    arcadeGames[arcadeMachineCount++] = {"LIZ WIZ", MCH_LIZWIZ, true};
#endif
#ifdef ENABLE_THEGLOB
  if (SD.exists("/ARCADE/THEGLOB/rom1.bin"))
    arcadeGames[arcadeMachineCount++] = {"THE GLOB", MCH_THEGLOB, true};
#endif
#ifdef ENABLE_CRUSH
  if (SD.exists("/ARCADE/CRUSH/rom1.bin"))
    arcadeGames[arcadeMachineCount++] = {"CRUSH ROLLER", MCH_CRUSH, true};
#endif
#ifdef ENABLE_ANTEATER
  if (SD.exists("/ARCADE/ANTEATER/rom1.bin") && SD.exists("/ARCADE/ANTEATER/rom2.bin"))
    arcadeGames[arcadeMachineCount++] = {"ANTEATER", MCH_ANTEATER, true};
#endif
#ifdef ENABLE_LADYBUG
  // ROM embedded in PROGMEM - SD optional (override)
  arcadeGames[arcadeMachineCount++] = {"LADY BUG", MCH_LADYBUG, true};
#endif
#ifdef ENABLE_XEVIOUS
  if (SD.exists("/ARCADE/XEVIOUS/rom1.bin") && SD.exists("/ARCADE/XEVIOUS/rom2.bin") && SD.exists("/ARCADE/XEVIOUS/rom3.bin"))
    arcadeGames[arcadeMachineCount++] = {"XEVIOUS", MCH_XEVIOUS, true};
#endif
#ifdef ENABLE_BOMBJACK
  if (SD.exists("/ARCADE/BOMBJACK/rom1.bin"))
    arcadeGames[arcadeMachineCount++] = {"BOMB JACK", MCH_BOMBJACK, true};
#endif
#ifdef ENABLE_GYRUSS
  // ROM embedded in PROGMEM - SD optional (override)
  arcadeGames[arcadeMachineCount++] = {"GYRUSS", MCH_GYRUSS, true};
#endif
}

// ===== Buffer Management =====
void arcadeAllocateBuffers() {
  // frameBuffer e spriteBuffer: piccoli (~5.5KB totale), in SRAM per velocita'
  // scaleBuffer e arcadeMemory: grandi (~126KB), in PSRAM per lasciare heap interno
  //   libero per BLE controller + task FreeRTOS
  size_t fbSize = ARC_ROW_PIXELS_MAX * sizeof(unsigned short);       // ~4KB
  size_t sprSize = 128 * sizeof(sprite_S);                           // ~1.5KB
  size_t scaleSize = ARC_SCALE_PIXELS_MAX * sizeof(unsigned short);  // ~110KB

  // Piccoli buffer hot-path in SRAM (fallback PSRAM)
  arcadeFrameBuffer = (unsigned short*)heap_caps_malloc(fbSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!arcadeFrameBuffer) arcadeFrameBuffer = (unsigned short*)ps_malloc(fbSize);

  arcadeSpriteBuffer = (sprite_S*)heap_caps_malloc(sprSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!arcadeSpriteBuffer) arcadeSpriteBuffer = (sprite_S*)ps_malloc(sprSize);

  // Buffer grandi direttamente in PSRAM (risparmia ~126KB heap interno per BLE)
  arcadeMemory = (unsigned char*)ps_malloc(RAMSIZE);

  arcadeScaleBuffer = (unsigned short*)ps_malloc(scaleSize);
}

void arcadeAllocateFullFrame(int natW, int natH) {
  if (arcadeFullFrame) return;  // already allocated
  size_t sz = natW * natH * sizeof(unsigned short);
  arcadeFullFrame = (unsigned short*)ps_malloc(sz);
}

void arcadeFreeBuffers() {
  if (arcadeFrameBuffer) { free(arcadeFrameBuffer); arcadeFrameBuffer = nullptr; }
  if (arcadeSpriteBuffer) { free(arcadeSpriteBuffer); arcadeSpriteBuffer = nullptr; }
  if (arcadeMemory) { free(arcadeMemory); arcadeMemory = nullptr; }
  if (arcadeScaleBuffer) { free(arcadeScaleBuffer); arcadeScaleBuffer = nullptr; }
  if (arcadeFullFrame) { free(arcadeFullFrame); arcadeFullFrame = nullptr; }
}

// ===== Load ROM from SD to PSRAM =====
// Supports both raw binary AND galag format (8-byte header: "GALG" + uint32 size)
// Returns allocated PSRAM buffer (caller owns), or nullptr on failure
unsigned char* arcadeLoadRomFromSD(const char* path, uint32_t &outSize) {
  outSize = 0;
  File f = SD.open(path, FILE_READ);
  if (!f || f.isDirectory()) {
    if (f) f.close();
    return nullptr;
  }
  uint32_t fileSize = f.size();
  if (fileSize == 0 || fileSize > 512 * 1024) { // max 512KB per ROM
    f.close();
    return nullptr;
  }

  // Detect galag header format: first 4 bytes = "GALG" (0x47 0x41 0x4C 0x47)
  uint32_t dataOffset = 0;
  uint32_t dataSize = fileSize;
  if (fileSize > 8) {
    unsigned char header[8];
    size_t hrd = f.read(header, 8);
    if (hrd == 8 && header[0] == 0x47 && header[1] == 0x41 &&
        header[2] == 0x4C && header[3] == 0x47) {
      // GALG header detected - read data size from header (little-endian uint32)
      dataSize = header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24);
      dataOffset = 8;
      if (dataSize == 0 || dataSize > fileSize - 8) {
        dataOffset = 0;
        dataSize = fileSize;
        f.seek(0);  // Reset to start
      }
    } else {
      // No GALG header - raw binary, seek back to start
      f.seek(0);
    }
  }

  unsigned char* buf = (unsigned char*)ps_malloc(dataSize);
  if (!buf) {
    f.close();
    return nullptr;
  }

  // Read in 4KB chunks
  uint32_t offset = 0;
  while (offset < dataSize) {
    uint32_t chunk = (dataSize - offset > 4096) ? 4096 : (dataSize - offset);
    size_t rd = f.read(buf + offset, chunk);
    if (rd == 0) break;
    offset += rd;
    yield();
  }
  f.close();
  if (offset != dataSize) {
    free(buf);
    return nullptr;
  }
  outSize = dataSize;
  return buf;
}

// Map machineType to SD folder name
const char* arcadeGetRomFolder(uint8_t machineType) {
  switch (machineType) {
    case MCH_PACMAN:   return "PACMAN";
    case MCH_GALAGA:   return "GALAGA";
    case MCH_DKONG:    return "DKONG";
    case MCH_FROGGER:  return "FROGGER";
    case MCH_DIGDUG:   return "DIGDUG";
    case MCH_1942:     return "1942";
    case MCH_EYES:     return "EYES";
    case MCH_MRTNT:    return "MRTNT";
    case MCH_LIZWIZ:   return "LIZWIZ";
    case MCH_THEGLOB:  return "THEGLOB";
    case MCH_CRUSH:    return "CRUSH";
    case MCH_ANTEATER: return "ANTEATER";
    case MCH_LADYBUG:  return "LADYBUG";
    case MCH_XEVIOUS:  return "XEVIOUS";
    case MCH_BOMBJACK: return "BOMBJACK";
    case MCH_GYRUSS:   return "GYRUSS";
    default:           return nullptr;
  }
}

// Map machine type to galag-style lowercase folder name (fallback path)
const char* arcadeGetRomFolderGalag(uint8_t machineType) {
  switch (machineType) {
    case MCH_PACMAN:   return "pacman";
    case MCH_GALAGA:   return "galaga";
    case MCH_DKONG:    return "dkong";
    case MCH_FROGGER:  return "frogger";
    case MCH_DIGDUG:   return "digdug";
    case MCH_1942:     return "1942";
    case MCH_EYES:     return "eyes";
    case MCH_MRTNT:    return "mrtnt";
    case MCH_LIZWIZ:   return "lizwiz";
    case MCH_THEGLOB:  return "theglob";
    case MCH_CRUSH:    return "crush";
    case MCH_ANTEATER: return "anteater";
    case MCH_LADYBUG:  return "ladybug";
    case MCH_XEVIOUS:  return "xevious";
    case MCH_BOMBJACK: return "bombjack";
    case MCH_GYRUSS:   return "gyruss";
    default:           return nullptr;
  }
}

// Try to find a ROM file, checking multiple paths:
// 1. /ARCADE/GALAGA/rom1.bin  (oraQuadra format)
// 2. /roms/galaga/rom1.bin    (galag original format)
// Returns true and fills outPath if found
bool arcadeFindRomFile(char* outPath, size_t pathLen, const char* folder, const char* galagFolder, const char* fileName) {
  // Try oraQuadra path first
  snprintf(outPath, pathLen, "/ARCADE/%s/%s", folder, fileName);
  if (SD.exists(outPath)) return true;
  // Try galag path
  if (galagFolder) {
    snprintf(outPath, pathLen, "/roms/%s/%s", galagFolder, fileName);
    if (SD.exists(outPath)) return true;
  }
  return false;
}

// Load external ROMs from SD for a machine
// Searches both /ARCADE/GAME/ and /roms/game/ (galag compatibility)
// Supports raw binary and GALG header format
// Generic: rom1.bin, rom2.bin, rom3.bin (indices 0,1,2)
// 1942 extra: rom1_b0.bin, rom1_b1.bin, rom1_b2.bin (indices 2,3,4)
void arcadeLoadExternalRoms(machineBase* machine, uint8_t machineType) {
  const char* folder = arcadeGetRomFolder(machineType);
  if (!folder) return;
  const char* galagFolder = arcadeGetRomFolderGalag(machineType);

  // Generic ROM loading (rom1..rom3 → indices 0..2)
  int genericMax = (machineType == MCH_1942) ? 2 : EXTERNAL_ROM_MAX;
  int loadedCount = 0;
  for (int i = 0; i < genericMax; i++) {
    char romName[20];
    snprintf(romName, sizeof(romName), "rom%d.bin", i + 1);
    char path[64];
    bool found = arcadeFindRomFile(path, sizeof(path), folder, galagFolder, romName);
    if (found) {
      uint32_t sz = 0;
      unsigned char* data = arcadeLoadRomFromSD(path, sz);
      if (data) {
        machine->externalRom[i] = data;
        machine->externalRomSize[i] = sz;
        loadedCount++;
      }
    }
  }

  // 1942 bank ROMs: rom1_b0.bin→idx2, rom1_b1.bin→idx3, rom1_b2.bin→idx4
  if (machineType == MCH_1942) {
    const char* bankNames[] = {"rom1_b0.bin", "rom1_b1.bin", "rom1_b2.bin"};
    for (int b = 0; b < 3; b++) {
      char path[64];
      bool found = arcadeFindRomFile(path, sizeof(path), folder, galagFolder, bankNames[b]);
      if (found) {
        uint32_t sz = 0;
        unsigned char* data = arcadeLoadRomFromSD(path, sz);
        if (data) {
          machine->externalRom[2 + b] = data;
          machine->externalRomSize[2 + b] = sz;
        }
      }
    }
  }
}

// ===== Required ROM count per machine type =====
// Returns how many rom1..romN.bin files are needed (CPU ROMs only, not 1942 banks)
int arcadeRequiredRomCount(uint8_t machineType) {
  switch (machineType) {
    case MCH_GALAGA:   return 3;  // CPU1 + CPU2 + CPU3
    case MCH_DIGDUG:   return 3;  // CPU1 + CPU2 + CPU3
    case MCH_XEVIOUS:  return 3;  // CPU1 + CPU2 + CPU3
    case MCH_DKONG:    return 2;  // CPU1 + CPU2 (audio)
    case MCH_FROGGER:  return 2;  // CPU1 + CPU2
    case MCH_ANTEATER: return 2;  // CPU1 + CPU2
    case MCH_1942:     return 2;  // CPU1 + CPU2 (+ banks loaded separately)
    case MCH_GYRUSS:   return 0;  // All ROMs embedded in PROGMEM (SD optional)
    case MCH_LADYBUG:  return 0;  // All ROMs embedded in PROGMEM (SD optional)
    default:           return 1;  // Single CPU: pacman, eyes, mrtnt, lizwiz, theglob, crush
  }
}

// Minimum expected ROM sizes per machine type (0 = no check)
// Returns minimum expected data size in bytes for romIndex (0-based)
// MAME or galag files may be equal or larger; smaller = definitely wrong
uint32_t arcadeExpectedRomSize(uint8_t machineType, int romIndex) {
  switch (machineType) {
    case MCH_PACMAN:   if (romIndex == 0) return 16384; break;
    case MCH_GALAGA:   if (romIndex == 0) return 16384; if (romIndex == 1) return 4096; if (romIndex == 2) return 4096; break;
    case MCH_DIGDUG:   if (romIndex == 0) return 16384; if (romIndex == 1) return 8192; if (romIndex == 2) return 4096; break;
    case MCH_DKONG:    if (romIndex == 0) return 16384; if (romIndex == 1) return 4096; break;
    case MCH_FROGGER:  if (romIndex == 0) return 12288; if (romIndex == 1) return 6144; break;
    case MCH_ANTEATER: if (romIndex == 0) return 16384; if (romIndex == 1) return 4096; break;
    case MCH_1942:     if (romIndex == 0) return 32768; if (romIndex == 1) return 16384; break;
    case MCH_EYES:     if (romIndex == 0) return 16384; break;
    case MCH_MRTNT:    if (romIndex == 0) return 16384; break;
    case MCH_LIZWIZ:   if (romIndex == 0) return 16384; break;
    case MCH_THEGLOB:  if (romIndex == 0) return 16384; break;
    case MCH_CRUSH:    if (romIndex == 0) return 16384; break;
    case MCH_LADYBUG:  if (romIndex == 0) return 24576; break;
    case MCH_XEVIOUS:  if (romIndex == 0) return 16384; if (romIndex == 1) return 8192; if (romIndex == 2) return 4096; break;
    case MCH_BOMBJACK: if (romIndex == 0) return 40960; break;
    case MCH_GYRUSS:   if (romIndex == 0) return 24576; if (romIndex == 1) return 8192; if (romIndex == 2) return 16384; break;
  }
  return 0;
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
#ifdef ENABLE_LADYBUG
    case MCH_LADYBUG: return new ladybug();
#endif
#ifdef ENABLE_XEVIOUS
    case MCH_XEVIOUS: return new xevious();
#endif
#ifdef ENABLE_BOMBJACK
    case MCH_BOMBJACK: return new bombjack();
#endif
#ifdef ENABLE_GYRUSS
    case MCH_GYRUSS: return new gyruss();
#endif
    default: return nullptr;
  }
}

// ===== Start Game =====
void arcadeStartGame(int8_t index) {
  if (index < 0 || index >= arcadeMachineCount) return;

  // Loading screen with game icon
  gfx->fillScreen(BLACK);
  gfx->setFont((const GFXfont *)NULL);

  // Draw game icon at center-top (2x scale via offset)
  uint16_t gameCol = arcadeGetGameColor(arcadeGames[index].machineType);
  arcadeDrawGameIcon(240, 140, arcadeGames[index].machineType);

  // Color bar under icon
  gfx->fillRect(160, 190, 160, 4, gameCol);

  // Loading text
  gfx->setTextColor(0xFFE0);
  gfx->setTextSize(3);
  gfx->setCursor(110, 220);
  gfx->print("LOADING");
  gfx->setTextSize(2);
  gfx->setTextColor(gameCol);
  gfx->setCursor(100, 260);
  gfx->printf("%s...", arcadeGames[index].name);

  // Debug info in lower area
  esp_reset_reason_t rstReason = esp_reset_reason();
  const char* rstName =
    rstReason == ESP_RST_POWERON ? "POWER-ON" :
    rstReason == ESP_RST_SW ? "SOFTWARE" :
    rstReason == ESP_RST_PANIC ? "PANIC!" :
    rstReason == ESP_RST_INT_WDT ? "INT WDT!" :
    rstReason == ESP_RST_TASK_WDT ? "TASK WDT!" :
    rstReason == ESP_RST_WDT ? "WDT!" :
    rstReason == ESP_RST_BROWNOUT ? "BROWNOUT!" :
    rstReason == ESP_RST_DEEPSLEEP ? "DEEPSLEEP" : "OTHER";
  gfx->setTextSize(1);
  gfx->setTextColor(rstReason <= ESP_RST_SW ? 0x5AEB : 0xF800);
  gfx->setCursor(30, 400);
  gfx->printf("Reset: %s (%d)", rstName, (int)rstReason);
  gfx->setTextColor(0x4A49);
  gfx->setCursor(30, 416);
  gfx->printf("Crash flag: %d  Last game: %d", arcadeCrashFlag, arcadeCrashGame);
  gfx->setCursor(30, 432);
  gfx->printf("Heap: %d  PSRAM: %d", ESP.getFreeHeap(), ESP.getFreePsram());
  delay(4000);  // 4 seconds to read

  // Set crash tracking flag (cleared on normal game exit)
  arcadeCrashFlag = 1;
  arcadeCrashGame = arcadeGames[index].machineType;

  // Create machine
  arcadeCurrentMachine = arcadeCreateMachine(arcadeGames[index].machineType);
  if (!arcadeCurrentMachine) {
    gfx->setTextColor(0xF800);
    gfx->setCursor(100, 300);
    gfx->print("ERRORE!");
    delay(2000);
    return;
  }

  // Load ROMs from SD
  arcadeLoadExternalRoms(arcadeCurrentMachine, arcadeGames[index].machineType);

  // Verify all required ROMs were loaded
  int requiredRoms = arcadeRequiredRomCount(arcadeGames[index].machineType);
  int loadedOk = 0;
  for (int ri = 0; ri < requiredRoms; ri++) {
    if (arcadeCurrentMachine->hasExtRom(ri)) loadedOk++;
  }
  if (loadedOk < requiredRoms) {
    gfx->setTextSize(1);
    gfx->setTextColor(0xF800);
    gfx->setCursor(60, 300);
    gfx->print("ROM mancanti! Serve /ARCADE con i .bin");
    delete arcadeCurrentMachine;
    arcadeCurrentMachine = nullptr;
    arcadeInMenu = true;
    delay(3000);
    gfx->fillScreen(BLACK);
    arcadeDrawMenu();
    return;
  }

  // Apply ROM pointers, init, reset
  arcadeCurrentMachine->applyExternalRoms();
  arcadeCurrentMachine->init(&arcadeInput, arcadeFrameBuffer, arcadeSpriteBuffer, arcadeMemory);
  arcadeCurrentMachine->reset();

  // Allocate full-frame buffer for rotated games (ROT90/ROT270)
  if (arcadeCurrentMachine->gameRotation() != 0) {
    arcadeAllocateFullFrame(arcadeCurrentMachine->gameWidth(), arcadeCurrentMachine->gameHeight());
  }

  // Start arcade audio synthesis for this game
  if (arcadeAudio) arcadeAudio->start(arcadeCurrentMachine);

  // Draw borders and control overlay - use rotated output dimensions if applicable
  int rot = arcadeCurrentMachine->gameRotation();
  int dispW, dispH;
  if (rot == 90 || rot == 270) {
    dispW = (arcadeCurrentMachine->gameHeight() * 3) / 2;  // 224*1.5 = 336
    dispH = (arcadeCurrentMachine->gameWidth() * 3) / 2;   // 256*1.5 = 384
  } else {
    int gw = arcadeCurrentMachine->gameWidth();
    dispW = (gw > ARC_GAME_W) ? ARC_OUT_W_WIDE : ARC_OUT_W;
    dispH = (gw > ARC_GAME_W) ? 28 * 12 : ARC_BATCH_OUT_H * 3;
  }
  int dispOffX = (480 - dispW) / 2;
  gfx->fillRect(0, 0, dispOffX, 480, 0x0000);
  gfx->fillRect(dispOffX + dispW, 0, 480 - dispOffX - dispW, 480, 0x0000);
  arcadeDrawControlOverlay();

  arcadeInMenu = false;
  arcadeSelectedGame = index;
  arcadeButtonState = 0;
  arcadeInput.setButtons(0);
  arcadeCoinState = 0;
  arcadeStartState = 0;
  arcadeFrameCount = 0;
  arcadeBootFrames = 0;
  arcadeBootStartMs = millis();
  arcadeLastFrameTime = millis();

  // Create emulation task on Core 0
  // BT controller frammenta l'heap interno, quindi proviamo stack decrescenti
  // fino a trovare un blocco contiguo disponibile
  size_t maxBlock = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  Serial.printf("[ARCADE] Heap: %d, blocco max: %d\n", ESP.getFreeHeap(), maxBlock);

  // Stack minimo 8KB, ideale 16KB
  size_t stackSize = 16384;
  if (maxBlock < stackSize + 1024) stackSize = 12288;
  if (maxBlock < stackSize + 1024) stackSize = 8192;

  BaseType_t taskResult = xTaskCreatePinnedToCore(
    arcadeEmulationTask,
    "arcadeZ80",
    stackSize,
    nullptr,
    2,
    &arcadeEmulationTaskHandle,
    ARCADE_EMULATION_CORE
  );

  if (taskResult != pdPASS) {
    gfx->setTextSize(1);
    gfx->setTextColor(0xF800);
    gfx->setCursor(80, 300);
    gfx->printf("TASK FAIL! heap=%d blk=%d", ESP.getFreeHeap(), maxBlock);
    arcadeInMenu = true;
    delay(3000);
    gfx->fillScreen(BLACK);
    arcadeDrawMenu();
    return;
  }
  Serial.printf("[ARCADE] Task Z80 creato, stack=%d\n", stackSize);

}

// ===== Stop Game =====
void arcadeStopGame() {
  // Clear crash tracking flag (normal exit, not a crash)
  arcadeCrashFlag = 0;

  // Delete emulation task
  if (arcadeEmulationTaskHandle) {
    vTaskDelete(arcadeEmulationTaskHandle);
    arcadeEmulationTaskHandle = nullptr;
    // Let FreeRTOS idle task reclaim the 32KB task stack before we proceed
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  // Stop arcade audio before deleting machine
  if (arcadeAudio) arcadeAudio->stop();

  // Delete machine
  if (arcadeCurrentMachine) {
    delete arcadeCurrentMachine;
    arcadeCurrentMachine = nullptr;
  }

  // Free full-frame rotation buffer (112KB PSRAM, used by Bomb Jack etc.)
  if (arcadeFullFrame) {
    free(arcadeFullFrame);
    arcadeFullFrame = nullptr;
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
  // Rimuovi questo task dal Task Watchdog Timer
  // Il WDT di Core 0 puo' scattare durante boot lunghi (Digdug ~950+ frame)
  esp_task_wdt_delete(NULL);

  unsigned long emuStart = millis();
  int bootFrames = 0;

  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    if (arcadeCurrentMachine) {
      if (!arcadeCurrentMachine->game_started) {
        // BOOT: 20 frames per yield (come versione funzionante)
        // Le sub-CPU (Galaga, DigDug) hanno bisogno di esecuzione continua per bootare
        for (int i = 0; i < 20; i++) {
          arcadeCurrentMachine->run_frame();
          bootFrames++;
          arcadeBootFrames = bootFrames;
          if (arcadeCurrentMachine->game_started) break;  // esce dal FOR, non dal WHILE
        }

        // Boot completato? → reset timing per vTaskDelayUntil
        if (arcadeCurrentMachine->game_started) {
          lastWake = xTaskGetTickCount();
          continue;
        }

        // Boot timeout: 60s max
        unsigned long elapsed = millis() - emuStart;
        if (elapsed > 60000) {
          arcadeCurrentMachine->game_started = -1;
          while (true) { vTaskDelay(1000); }
        }

        vTaskDelay(1);  // Yield per idle task Core 0
      } else {
        // Self-timed at 60fps, decoupled from render
        arcadeCurrentMachine->run_frame();
        // Audio: transmit on same core as emulation (no soundregs race).
        // Fills DMA buffer with as many samples as available space allows.
        if (arcadeAudio) arcadeAudio->transmit();

        // Frame timing with MANDATORY yield to prevent IDLE task starvation.
        // Bomb Jack's run_frame (20K Z80 steps ~15-20ms) + transmit (~5ms)
        // can exceed the 16ms budget. vTaskDelayUntil returns immediately
        // when deadline is passed, starving IDLE → TWDT reset after 5s.
        // Fix: always yield at least 1 tick so IDLE task feeds its watchdog.
        TickType_t now = xTaskGetTickCount();
        if ((now - lastWake) < pdMS_TO_TICKS(16)) {
          vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(16));
        } else {
          lastWake = now;
          vTaskDelay(1);  // Mandatory yield even when running behind
        }
      }
    } else {
      vTaskDelay(10);
    }
  }
}

// ===== Adaptive blend: smooth gradients, preserve sprite edges =====
// Blends only when top RGB bits match (colors similar = same object).
// Falls back to nearest-neighbor p0 at edges (sprite vs background).
// Mask 0xE71C = top 3 bits of R(15-13), G(10-8), B(4-2).
IRAM_ATTR inline unsigned short adaptiveBlend565(unsigned short c1, unsigned short c2) {
  if (((c1 ^ c2) & 0xE71C) == 0)
    return ((c1 & 0xF7DE) >> 1) + ((c2 & 0xF7DE) >> 1);
  return c1;
}

// ===== Upscale Row: 224x8 -> 336x12 (1.5x adaptive blend) =====
// 3:2 scaling: every 2 source pixels become 3 destination pixels
// Middle pixel is blended only if colors are similar (preserves edges)
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
      // Unique line: horizontal 3:2 adaptive scaling (224 -> 336 pixels)
      // Colormap data is big-endian (pre-swapped for SPI displays).
      // RGB parallel panel expects little-endian: byte-swap BEFORE blend.
      unsigned short* srcLine = src + srcLineMap[outY] * ARC_GAME_W;
      int dstX = 0;
      for (int srcX = 0; srcX < ARC_GAME_W; srcX += 2) {
        unsigned short p0 = __builtin_bswap16(srcLine[srcX]);
        unsigned short p1 = __builtin_bswap16(srcLine[srcX + 1]);
        dstLine[dstX]     = p0;
        dstLine[dstX + 1] = adaptiveBlend565(p0, p1);
        dstLine[dstX + 2] = p1;
        dstX += 3;
      }
    }
  }
}

// ===== Upscale Row for 256-wide games: 256x8 -> 384x12 (1.5x adaptive blend) =====
IRAM_ATTR void arcadeUpscaleRowWide(unsigned short* src, unsigned short* dst, int dstOffsetY) {
  static const uint8_t srcLineMap[12] = {0,0,1, 2,2,3, 4,4,5, 6,6,7};

  for (int outY = 0; outY < 12; outY++) {
    unsigned short* dstLine = dst + (dstOffsetY + outY) * ARC_OUT_W_WIDE;

    if (outY > 0 && srcLineMap[outY] == srcLineMap[outY - 1]) {
      memcpy(dstLine, dstLine - ARC_OUT_W_WIDE, ARC_OUT_W_WIDE * sizeof(unsigned short));
    } else {
      unsigned short* srcLine = src + srcLineMap[outY] * ARC_GAME_W_MAX;
      int dstX = 0;
      for (int srcX = 0; srcX < ARC_GAME_W_MAX; srcX += 2) {
        unsigned short p0 = __builtin_bswap16(srcLine[srcX]);
        unsigned short p1 = __builtin_bswap16(srcLine[srcX + 1]);
        dstLine[dstX]     = p0;
        dstLine[dstX + 1] = adaptiveBlend565(p0, p1);
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

  // Aggiorna BLE gamepad (NimBLE)
  bleGamepadUpdate();

  // Merge ESP-NOW joystick + BLE gamepad in variabile locale
  // (NON modificare arcadeButtonState perche' arcadeClearTouchInput lo resetta
  // solo al rilascio del dito, non ogni frame, e un |= accumulerebbe bit che non vengono mai puliti)
  extern volatile uint8_t espNowJoystickButtons;
  uint8_t merged = arcadeButtonState | (espNowJoystickButtons & 0x9F) | bleGamepadGetButtons();

  // ALWAYS update input (even during Z80 boot)
  arcadeInput.setButtons(merged);
  arcadeUpdateButtonStateMachines();

  // FAST BOOT: skip rendering during Z80 boot, show progress on screen
  if (arcadeCurrentMachine->game_started != 1) {
    // Boot timeout detected (game_started == -1)
    if (arcadeCurrentMachine->game_started == -1) {
      gfx->fillScreen(BLACK);
      gfx->setFont(u8g2_font_helvB14_tr);
      gfx->setTextColor(0xF800);  // Red
      gfx->setCursor(80, 180);
      gfx->print("BOOT TIMEOUT");
      gfx->setFont((const GFXfont *)NULL);
      gfx->setTextSize(1);
      gfx->setTextColor(0x7BEF);
      gfx->setCursor(80, 220);
      gfx->print("Il gioco non ha completato il boot.");
      gfx->setCursor(80, 240);
      gfx->print("Verifica che tutti i file ROM siano");
      gfx->setCursor(80, 260);
      gfx->print("presenti e corretti sulla SD card.");

      // Show final diagnostic state
      gfx->setTextColor(0x5AEB);
      gfx->setCursor(80, 290);
      gfx->printf("Frames: %d", arcadeBootFrames);
      gfx->setCursor(80, 306);
      gfx->printf("PC0=0x%04X PC1=0x%04X PC2=0x%04X",
                  arcadeCurrentMachine->getPC(0),
                  arcadeCurrentMachine->getPC(1),
                  arcadeCurrentMachine->getPC(2));

      // Show ROM sizes for verification
      int reqRoms = arcadeRequiredRomCount(arcadeGames[arcadeSelectedGame].machineType);
      for (int ri = 0; ri < reqRoms; ri++) {
        gfx->setCursor(80, 326 + ri * 14);
        uint32_t sz = arcadeCurrentMachine->externalRomSize[ri];
        uint32_t exp = arcadeExpectedRomSize(arcadeGames[arcadeSelectedGame].machineType, ri);
        if (sz < exp && exp > 0) {
          gfx->setTextColor(0xFBE0);
          gfx->printf("ROM%d: %uB (min %u!)", ri + 1, sz, exp);
        } else {
          gfx->setTextColor(0x5AEB);
          gfx->printf("ROM%d: %uB OK", ri + 1, sz);
        }
      }

      delay(6000);
      arcadeStopGame();
      return;
    }

    unsigned long elapsed = millis() - arcadeBootStartMs;

    // Show animated boot progress with diagnostic info
    gfx->fillRect(60, 280, 360, 50, BLACK);
    gfx->setFont((const GFXfont *)NULL);
    gfx->setTextSize(1);
    gfx->setTextColor(0x7BEF);
    gfx->setCursor(110, 284);
    gfx->printf("Booting Z80... %lu.%lus  [%d frames]",
                elapsed / 1000, (elapsed / 100) % 10, arcadeBootFrames);

    // Show CPU PC values (helps diagnose stuck CPUs)
    gfx->setTextColor(0x5AEB);  // Gray
    gfx->setCursor(80, 300);
    gfx->printf("PC0=0x%04X  PC1=0x%04X  PC2=0x%04X",
                arcadeCurrentMachine->getPC(0),
                arcadeCurrentMachine->getPC(1),
                arcadeCurrentMachine->getPC(2));

    delay(200);  // Don't busy-wait, let emulation run on Core 0
    return;
  }

  if (arcadeFrameCount == 0) {
    // Boot completed - first frame
  }

  // Prepare frame (extract sprites from Z80 RAM)
  arcadeCurrentMachine->prepare_frame();

  int gw = arcadeCurrentMachine->gameWidth();
  int gh = arcadeCurrentMachine->gameHeight();
  int rotation = arcadeCurrentMachine->gameRotation();

  if (rotation != 0 && arcadeFullFrame) {
    // ===== ROTATED GAME PIPELINE (Bomb Jack ROT90, etc.) =====
    // Phase 1: Render all native rows into full frame buffer
    // Yield periodically to prevent starving system tasks (TWDT, WiFi, GDMA).
    // BG rendering (activated during gameplay) is flash+PSRAM heavy.
    int natRows = gh / 8;                  // native visible rows (e.g. 224/8 = 28)
    int rowPixels = gw * 8;               // pixels per native row band
    for (int nr = 0; nr < natRows; nr++) {
      memset(arcadeFrameBuffer, 0, rowPixels * sizeof(unsigned short));
      arcadeCurrentMachine->render_row(nr);
      memcpy(&arcadeFullFrame[nr * rowPixels], arcadeFrameBuffer, rowPixels * sizeof(unsigned short));
      if ((nr & 7) == 7) vTaskDelay(1);   // Yield every 8 rows (~3ms chunks)
    }
    vTaskDelay(1);  // Let RTOS scheduler + GDMA settle between render phases

    // Phase 2: Extract rotated rows and display
    // After ROT90: output = gh_wide x gw_tall (e.g. 224 x 256)
    int outNatW = gh;   // rotated output width = native height (224)
    int outNatH = gw;   // rotated output height = native width (256)
    int outRows = outNatH / 8;            // output tile rows (256/8 = 32)
    int outW = (outNatW * 3) / 2;         // upscaled width (224*1.5 = 336)
    int outH = (outNatH * 3) / 2;         // upscaled height (256*1.5 = 384)
    int offX = (480 - outW) / 2;
    int offY = (480 - outH) / 2;

    gfx->startWrite();
    for (int batch = 0; batch < 3; batch++) {
      int rowStart = batch * ARC_BATCH_ROWS;
      int batchRows = outRows - rowStart;
      if (batchRows <= 0) break;
      if (batchRows > ARC_BATCH_ROWS) batchRows = ARC_BATCH_ROWS;

      for (int r = 0; r < batchRows; r++) {
        int outRow = rowStart + r;
        // Extract ROT90 rotated strip into arcadeFrameBuffer (outNatW wide x 8 tall)
        for (int l = 0; l < 8; l++) {
          int natX = outRow * 8 + l;
          for (int ox = 0; ox < outNatW; ox++) {
            int natVisY = (gh - 1) - ox;  // ROT90: output X maps to inverted native Y
            arcadeFrameBuffer[l * outNatW + ox] = arcadeFullFrame[natVisY * gw + natX];
          }
        }
        arcadeUpscaleRow(arcadeFrameBuffer, arcadeScaleBuffer, r * 12);
      }

      gfx->draw16bitRGBBitmap(offX, offY + batch * ARC_BATCH_OUT_H,
                               arcadeScaleBuffer, outW, batchRows * 12);
    }
    gfx->endWrite();
  } else {
    // ===== STANDARD (NON-ROTATED) PIPELINE =====
    bool isWide = (gw > ARC_GAME_W);
    int gameRows = isWide ? (gh / 8) : (gh / 8);  // rows from visible height
    int outW = isWide ? ARC_OUT_W_WIDE : ARC_OUT_W;
    int gameOutH = gameRows * 12;
    int offX = (480 - outW) / 2;
    int offY = (480 - gameOutH) / 2;
    int rowPixels = gw * 8;
    int8_t mt = arcadeCurrentMachine->machineType();
    bool flipV = (mt == MCH_LADYBUG);
    bool mirrorX = (mt == MCH_GYRUSS);
    gfx->startWrite();
    for (int batch = 0; batch < 3; batch++) {
      int rowStart = batch * ARC_BATCH_ROWS;
      int batchRows = gameRows - rowStart;
      if (batchRows <= 0) break;
      if (batchRows > ARC_BATCH_ROWS) batchRows = ARC_BATCH_ROWS;

      for (int r = 0; r < batchRows; r++) {
        int row = rowStart + r;
        // Ladybug: vertical flip; Gyruss: horizontal mirror
        int renderRow = flipV ? (gameRows - 1 - row) : row;
        memset(arcadeFrameBuffer, 0, rowPixels * sizeof(unsigned short));
        arcadeCurrentMachine->render_row(renderRow);
        if (flipV) {
          // Flip Y: reverse line order within 8-line strip
          for (int line = 0; line < 4; line++) {
            unsigned short *a = arcadeFrameBuffer + line * gw;
            unsigned short *b = arcadeFrameBuffer + (7 - line) * gw;
            for (int px = 0; px < gw; px++) {
              unsigned short tmp = a[px]; a[px] = b[px]; b[px] = tmp;
            }
          }
        }
        if (mirrorX) {
          // Mirror X: reverse each scanline
          for (int line = 0; line < 8; line++) {
            unsigned short *lp = arcadeFrameBuffer + line * gw;
            for (int i = 0, j = gw - 1; i < j; i++, j--) {
              unsigned short tmp = lp[i]; lp[i] = lp[j]; lp[j] = tmp;
            }
          }
        }
        if (isWide)
          arcadeUpscaleRowWide(arcadeFrameBuffer, arcadeScaleBuffer, r * 12);
        else
          arcadeUpscaleRow(arcadeFrameBuffer, arcadeScaleBuffer, r * 12);
      }

      gfx->draw16bitRGBBitmap(offX, offY + batch * ARC_BATCH_OUT_H,
                               arcadeScaleBuffer, outW, batchRows * 12);
    }
    gfx->endWrite();
  }

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
    case MCH_LADYBUG:  return 0xF986;  // rosso-arancio (ladybug wings)
    case MCH_XEVIOUS:  return 0x867F;  // argento-azzurro (cielo)
    case MCH_BOMBJACK: return 0xFD20;  // arancione acceso (esplosione)
    case MCH_GYRUSS:   return 0x07FF;  // ciano (spazio)
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

    case MCH_LADYBUG: {
      // Lady Bug: coccinella rossa con punti neri + antennine + labirinto verde
      // Corpo (ellisse rossa)
      gfx->fillCircle(cx, cy, 12, 0xF800);           // corpo rosso
      gfx->drawCircle(cx, cy, 12, 0x8000);            // contorno scuro
      // Linea centrale (elitre)
      gfx->drawLine(cx, cy - 12, cx, cy + 12, BLACK);
      // Punti neri
      gfx->fillCircle(cx - 5, cy - 4, 3, BLACK);
      gfx->fillCircle(cx + 5, cy - 4, 3, BLACK);
      gfx->fillCircle(cx - 4, cy + 5, 2, BLACK);
      gfx->fillCircle(cx + 4, cy + 5, 2, BLACK);
      // Testa
      gfx->fillCircle(cx, cy - 14, 5, BLACK);
      // Antenne
      gfx->drawLine(cx - 3, cy - 18, cx - 8, cy - 22, BLACK);
      gfx->drawLine(cx + 3, cy - 18, cx + 8, cy - 22, BLACK);
      gfx->fillCircle(cx - 8, cy - 22, 1, BLACK);
      gfx->fillCircle(cx + 8, cy - 22, 1, BLACK);
      // Linee labirinto verdi sotto
      gfx->fillRect(cx - 18, cy + 16, 36, 2, 0x07E0);
      gfx->fillRect(cx - 14, cy + 20, 4, 4, 0x07E0);
      gfx->fillRect(cx + 10, cy + 20, 4, 4, 0x07E0);
      break;
    }

    case MCH_XEVIOUS: {
      // Solvalou dettagliato (vista dall'alto, punta in su)
      uint16_t bodyDk  = 0x6B6D;  // grigio-azzurro scuro
      uint16_t bodyMd  = 0x8C71;  // grigio-azzurro medio
      uint16_t bodyLt  = 0xAD75;  // grigio-azzurro chiaro
      uint16_t wingDk  = 0x3C1F;  // azzurro ala scuro
      uint16_t wingLt  = 0x5C7F;  // azzurro ala chiaro
      uint16_t cockpit = 0x07FF;  // ciano vetro
      uint16_t cockHi  = 0xAFFF;  // riflesso cockpit
      uint16_t fireY   = 0xFFE0;  // giallo fuoco
      uint16_t fireO   = 0xFBE0;  // arancio fuoco
      uint16_t fireR   = 0xF800;  // rosso fuoco
      uint16_t terrain = 0x2C60;  // verde terreno
      uint16_t terrLt  = 0x3CA0;  // verde terreno chiaro

      // Sfondo cielo sfumato (dietro nave)
      gfx->fillRect(cx - 24, cy - 22, 48, 48, 0x194A);  // blu scuro
      gfx->fillRect(cx - 22, cy + 16, 44, 10, 0x1129);  // fascia bassa piu' scura

      // Terreno scrollante (pattern strisce verdi)
      gfx->fillRect(cx - 22, cy + 18, 44, 2, terrain);
      gfx->fillRect(cx - 18, cy + 22, 12, 1, terrLt);
      gfx->fillRect(cx + 4,  cy + 21, 10, 1, terrain);
      gfx->fillRect(cx - 10, cy + 24, 8, 1, terrLt);
      gfx->fillRect(cx + 8,  cy + 24, 6, 1, terrain);

      // Fusoliera centrale - 3 toni sfumati
      gfx->fillRect(cx - 4, cy - 12, 8, 22, bodyMd);    // corpo medio
      gfx->fillRect(cx - 3, cy - 10, 2, 18, bodyLt);    // highlight sinistro
      gfx->fillRect(cx + 2, cy - 10, 2, 18, bodyDk);    // ombra destro
      gfx->fillRect(cx - 1, cy - 12, 2, 22, bodyLt);    // spina dorsale chiara

      // Punta (naso affusolato)
      gfx->fillTriangle(cx, cy - 20, cx - 5, cy - 10, cx + 5, cy - 10, bodyMd);
      gfx->fillTriangle(cx, cy - 20, cx - 2, cy - 12, cx + 1, cy - 12, bodyLt);

      // Cockpit vetro con riflesso
      gfx->fillRoundRect(cx - 3, cy - 14, 6, 6, 2, cockpit);
      gfx->fillRect(cx - 2, cy - 14, 2, 3, cockHi);     // riflesso vetro

      // Ala sinistra - profilo aerodinamico
      gfx->fillTriangle(cx - 4, cy - 2, cx - 22, cy + 8, cx - 4, cy + 10, wingDk);
      gfx->fillTriangle(cx - 4, cy - 1, cx - 18, cy + 6, cx - 4, cy + 6, wingLt);
      gfx->drawLine(cx - 4, cy + 1, cx - 20, cy + 8, bodyDk);  // bordo attacco

      // Ala destra - profilo aerodinamico
      gfx->fillTriangle(cx + 4, cy - 2, cx + 22, cy + 8, cx + 4, cy + 10, wingDk);
      gfx->fillTriangle(cx + 4, cy - 1, cx + 18, cy + 6, cx + 4, cy + 6, wingLt);
      gfx->drawLine(cx + 4, cy + 1, cx + 20, cy + 8, bodyDk);  // bordo attacco

      // Dettagli ali - linee interne
      gfx->drawLine(cx - 6, cy + 2, cx - 14, cy + 7, bodyMd);
      gfx->drawLine(cx + 6, cy + 2, cx + 14, cy + 7, bodyMd);

      // Doppio scarico motore sinistro
      gfx->fillRect(cx - 4, cy + 10, 3, 3, fireY);
      gfx->fillRect(cx - 4, cy + 13, 3, 2, fireO);
      gfx->fillRect(cx - 3, cy + 15, 2, 2, fireR);

      // Doppio scarico motore destro
      gfx->fillRect(cx + 1, cy + 10, 3, 3, fireY);
      gfx->fillRect(cx + 1, cy + 13, 3, 2, fireO);
      gfx->fillRect(cx + 2, cy + 15, 2, 2, fireR);
      break;
    }

    case MCH_BOMBJACK: {
      // Bomb Jack: bomba nera con miccia + personaggio che salta
      uint16_t bomb = 0x2104;   // grigio scuro
      uint16_t fuse = 0xFD20;   // arancione
      uint16_t spark = 0xFFE0;  // giallo
      uint16_t hero = 0x07FF;   // ciano (Jack)
      // Bomba
      gfx->fillCircle(cx - 4, cy + 4, 12, bomb);
      // Highlight
      gfx->fillCircle(cx - 8, cy, 3, 0x4208);
      // Miccia
      gfx->drawLine(cx + 6, cy - 6, cx + 10, cy - 12, fuse);
      gfx->drawLine(cx + 10, cy - 12, cx + 8, cy - 14, fuse);
      // Scintilla
      gfx->fillCircle(cx + 8, cy - 15, 2, spark);
      gfx->drawPixel(cx + 6, cy - 17, spark);
      gfx->drawPixel(cx + 11, cy - 16, spark);
      // Jack (piccolo, che salta)
      gfx->fillCircle(cx + 14, cy - 6, 3, hero);     // testa
      gfx->fillRect(cx + 13, cy - 2, 3, 6, hero);    // corpo
      gfx->drawLine(cx + 11, cy, cx + 13, cy + 3, hero);  // braccio
      gfx->drawLine(cx + 16, cy, cx + 18, cy - 2, hero);  // braccio
      break;
    }

    case MCH_GYRUSS: {
      // Gyruss: navicella al centro di cerchi concentrici (tunnel spaziale)
      uint16_t ring1 = 0x0410;  // blu scuro
      uint16_t ring2 = 0x04BF;  // blu medio
      uint16_t ring3 = 0x07FF;  // ciano
      uint16_t ship  = 0xFFFF;  // bianco
      // Cerchi concentrici (tunnel)
      gfx->drawCircle(cx, cy, 18, ring1);
      gfx->drawCircle(cx, cy, 13, ring2);
      gfx->drawCircle(cx, cy, 8, ring3);
      // Navicella al centro (vista dall'alto)
      gfx->fillTriangle(cx, cy - 5, cx - 4, cy + 3, cx + 4, cy + 3, ship);
      gfx->fillRect(cx - 1, cy + 3, 3, 3, ring3);  // motore
      // Stelline decorative
      gfx->drawPixel(cx - 12, cy - 10, 0xFFFF);
      gfx->drawPixel(cx + 14, cy + 6, 0xFFFF);
      gfx->drawPixel(cx + 8, cy - 14, 0xFFFF);
      gfx->drawPixel(cx - 16, cy + 4, 0xFFFF);
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
  bool enabled = arcadeGames[gameIndex].enabled;
  uint16_t color = enabled ? arcadeGetGameColor(arcadeGames[gameIndex].machineType) : 0x4208; // grigio se OFF

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
  gfx->setTextColor(enabled ? 0xC618 : 0x4208);
  gfx->setCursor(nameX, y + AM_CELL_H - 10);
  gfx->print(arcadeGames[gameIndex].name);

  // Overlay scacchiera + "OFF" se disabilitato
  if (!enabled) {
    // Scacchiera semitrasparente (pattern 4x4)
    for (int py = y + 4; py < y + AM_CELL_H - 4; py += 8) {
      for (int px = x + 4; px < x + AM_CELL_W - 4; px += 8) {
        if (((px + py) / 8) % 2 == 0)
          gfx->fillRect(px, py, 4, 4, 0x0841); // scuro
      }
    }
    // Badge "OFF"
    gfx->fillRoundRect(x + AM_CELL_W / 2 - 20, y + 40, 40, 18, 4, 0x7800); // rosso scuro
    gfx->setFont((const GFXfont *)NULL);
    gfx->setTextColor(0xFBE0); // giallo
    gfx->setTextSize(1);
    gfx->setCursor(x + AM_CELL_W / 2 - 9, y + 44);
    gfx->print("OFF");
  }
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

  // Draw game cards in 4x3 grid (paginated)
  int totalPages = (arcadeMachineCount + AM_PER_PAGE - 1) / AM_PER_PAGE;
  if (arcadeMenuPage >= totalPages) arcadeMenuPage = 0;
  int startIdx = arcadeMenuPage * AM_PER_PAGE;
  int endIdx = startIdx + AM_PER_PAGE;
  if (endIdx > arcadeMachineCount) endIdx = arcadeMachineCount;

  for (int i = startIdx; i < endIdx; i++) {
    int slot = i - startIdx;
    arcadeDrawGameCard(slot % AM_COLS, slot / AM_COLS, i);
  }

  // Page indicator + arrows (only if multiple pages)
  if (totalPages > 1) {
    gfx->setFont((const GFXfont *)NULL);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(1);
    // Right arrow ">" at top-right
    gfx->fillRoundRect(430, 5, 45, 30, 4, 0x4208);
    gfx->setCursor(445, 14);
    gfx->printf("%d/%d", arcadeMenuPage + 1, totalPages);
  }

  // Instructions at bottom
  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextColor(0x7BEF);  // Gray
  gfx->setTextSize(1);
  gfx->setCursor(80, 462);
  gfx->print("TAP to select  |  TOP-LEFT to exit mode");
}

// ===== Draw Control Overlay (action buttons on side panels) =====
void arcadeDrawControlOverlay() {
  int bw = ARC_LB - ARC_BM * 2;   // button width = 66
  int rX = ARC_RB + ARC_BM;       // right panel button X = 411

  gfx->setFont((const GFXfont *)NULL);
  gfx->setTextWrap(false);

  // ==================== LEFT PANEL (x 0-71) ====================

  // --- EXIT (rosso) ---
  int eh = ARC_EXIT_Y2 - ARC_BM;
  gfx->fillRoundRect(ARC_BM, ARC_BM, bw, eh, 6, 0xC000);
  gfx->drawRoundRect(ARC_BM, ARC_BM, bw, eh, 6, 0xF800);
  gfx->setTextColor(0xFFFF);
  gfx->setTextSize(1);
  gfx->setCursor(8, ARC_BM + eh / 2 - 10);
  gfx->print("< EXIT");

  // --- BACK (grigio) ---
  int bh = 479 - ARC_BACK_Y1;
  gfx->fillRoundRect(ARC_BM, ARC_BACK_Y1, bw, bh, 6, 0x2104);
  gfx->drawRoundRect(ARC_BM, ARC_BACK_Y1, bw, bh, 6, 0x4208);
  gfx->setTextColor(0x9CF3);
  gfx->setCursor(8, ARC_BACK_Y1 + bh / 2 - 10);
  gfx->print("< BACK");

  // ==================== RIGHT PANEL (x 408-479) ====================

  // --- COIN (arancio/oro) ---
  int ch = ARC_COIN_Y2 - ARC_BM;
  gfx->fillRoundRect(rX, ARC_BM, bw, ch, 6, 0x7B00);
  gfx->drawRoundRect(rX, ARC_BM, bw, ch, 6, 0xFBE0);
  // Coin circle icon
  int cCx = ARC_RB + ARC_LB / 2, cCy = ARC_BM + ch / 2 - 8;
  gfx->fillCircle(cCx, cCy, 14, 0xFBE0);
  gfx->drawCircle(cCx, cCy, 14, 0xFFE0);
  gfx->drawCircle(cCx, cCy, 10, 0xC560);
  gfx->setTextColor(0x7B00);
  gfx->setTextSize(2);
  gfx->setCursor(cCx - 5, cCy - 7);
  gfx->print("C");
  gfx->setTextSize(1);
  gfx->setTextColor(0xFFE0);
  gfx->setCursor(rX + 10, ARC_COIN_Y2 - 16);
  gfx->print("COIN");

  // --- FIRE (rosso) ---
  int fh = ARC_FIRE_Y2 - ARC_FIRE_Y1;
  gfx->fillRoundRect(rX, ARC_FIRE_Y1, bw, fh, 6, 0x6000);
  gfx->drawRoundRect(rX, ARC_FIRE_Y1, bw, fh, 6, 0xF800);
  int fCx = ARC_RB + ARC_LB / 2, fCy = ARC_FIRE_Y1 + fh / 2 - 6;
  gfx->fillCircle(fCx, fCy, 20, 0xA000);
  gfx->drawCircle(fCx, fCy, 20, 0xF800);
  gfx->fillCircle(fCx, fCy, 14, 0xF800);
  gfx->setTextColor(0xFFFF);
  gfx->setTextSize(1);
  gfx->setCursor(rX + 12, ARC_FIRE_Y2 - 16);
  gfx->print("FIRE");

  // --- START (verde) ---
  int sh = 479 - ARC_START_Y1;
  gfx->fillRoundRect(rX, ARC_START_Y1, bw, sh, 6, 0x0320);
  gfx->drawRoundRect(rX, ARC_START_Y1, bw, sh, 6, 0x07E0);
  int sCx = ARC_RB + ARC_LB / 2, sCy = ARC_START_Y1 + sh / 2 - 8;
  gfx->fillTriangle(sCx - 10, sCy - 14, sCx - 10, sCy + 14, sCx + 14, sCy, 0x07E0);
  gfx->setTextColor(0x07E0);
  gfx->setCursor(rX + 4, ARC_START_Y1 + sh - 16);
  gfx->print("START");
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

// ===== In-Game Touch: TAP actions (EXIT, COIN, BACK, START) =====
void handleArcadeGameTouch(int x, int y) {
  if (!arcadeInitialized || arcadeInMenu) return;

  // LEFT PANEL taps
  if (x < ARC_LB) {
    if (y <= ARC_EXIT_Y2) { arcadeStopGame(); handleModeChange(); return; }  // EXIT
    if (y >= ARC_BACK_Y1)  { arcadeStopGame(); return; }                     // BACK
  }

  // RIGHT PANEL taps
  if (x >= ARC_RB) {
    if (y <= ARC_COIN_Y2)  { arcadeTriggerCoin(); return; }                  // COIN
    if (y >= ARC_START_Y1) { arcadeTriggerStart(); return; }                 // START
  }
}

// ===== In-Game Touch: Continuous D-pad + FIRE tracking (full screen) =====
void arcadeUpdateTouchInput(int x, int y) {
  if (!arcadeInitialized || arcadeInMenu) return;

  uint8_t buttons = 0;

  // Preserve coin/start state machines
  if (arcadeCoinState > 0 || arcadeStartState > 0)
    buttons = arcadeButtonState & (BUTTON_COIN | BUTTON_START);

  // D-pad: zone a tutto schermo (come originale)
  // LEFT: fascia sinistra (x < 140)
  if (x < ARC_DPAD_LEFT && y >= ARC_DPAD_TOP && y < ARC_DPAD_BOTTOM)
    buttons |= BUTTON_LEFT;

  // RIGHT: fascia destra (x > 340)
  if (x > ARC_DPAD_RIGHT && y >= ARC_DPAD_TOP && y < ARC_DPAD_BOTTOM)
    buttons |= BUTTON_RIGHT;

  // UP: zona centrale alta (140-340, y < 240)
  if (x >= ARC_DPAD_LEFT && x <= ARC_DPAD_RIGHT &&
      y >= ARC_DPAD_TOP && y < ARC_DPAD_MID_Y)
    buttons |= BUTTON_UP;

  // DOWN: zona centrale bassa (140-340, y >= 240)
  if (x >= ARC_DPAD_LEFT && x <= ARC_DPAD_RIGHT &&
      y >= ARC_DPAD_MID_Y && y < ARC_DPAD_BOTTOM)
    buttons |= BUTTON_DOWN;

  // FIRE: pannello destro zona FIRE
  if (x >= ARC_RB && y >= ARC_FIRE_Y1 && y <= ARC_FIRE_Y2)
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

  // Page indicator tap: top-right corner -> next page
  int totalPages = (arcadeMachineCount + AM_PER_PAGE - 1) / AM_PER_PAGE;
  if (totalPages > 1 && x > 420 && y < 40) {
    arcadeMenuPage = (arcadeMenuPage + 1) % totalPages;
    arcadeDrawMenu();
    return;
  }

  // Grid hit test: seleziona gioco
  if (y >= AM_GRID_TOP && y < AM_GRID_TOP + AM_ROWS * AM_ROW_STEP) {
    int col = (x - AM_MARGIN) / AM_COL_STEP;
    int row = (y - AM_GRID_TOP) / AM_ROW_STEP;
    if (col >= 0 && col < AM_COLS && row >= 0 && row < AM_ROWS) {
      int idx = arcadeMenuPage * AM_PER_PAGE + row * AM_COLS + col;
      if (idx < arcadeMachineCount) {
        int cx = AM_MARGIN + col * AM_COL_STEP;
        int cy = AM_GRID_TOP + row * AM_ROW_STEP;
        if (!arcadeGames[idx].enabled) {
          // Flash rosso per gioco disabilitato
          gfx->drawRoundRect(cx, cy, AM_CELL_W, AM_CELL_H, 8, 0xF800);
          gfx->drawRoundRect(cx + 1, cy + 1, AM_CELL_W - 2, AM_CELL_H - 2, 7, 0xF800);
          delay(300);
          arcadeDrawGameCard(col, row, idx); // ridisegna card
          return;
        }
        // Visual feedback: highlight card
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
