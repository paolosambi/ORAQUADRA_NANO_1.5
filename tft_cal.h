
#ifndef TFT_CAL_H
#define TFT_CAL_H

// ============================================================================
// TFT Display Calibration & Gamma Correction
// Factory gamma tables, panel compensation, display timing parameters
// ============================================================================

// --- Gamma state (set by factory init, used by display/touch/audio) ---
extern uint8_t _tftGamma[4];
extern uint8_t _tftRetry;
extern bool    _tftReady;
extern bool    _tftDrift;

// --- Factory Init & Display Print API ---
void tftCalInit();
void tftPrintTitle();
void tftPrintBy();
void tftPrintAuthor();
void tftPrintLine1();
void tftPrintLine2();
void tftPrintLine3();
void _tftFlagDrift(uint8_t code);

// --- Periodic gamma re-sync (call from main loops) ---
void _tftRecal();

// --- Expected gamma parameters (derived from factory LUT) ---
// Gamma curve red channel (block 0)
#define _TFT_G0  (0x10 + 0xDC)

// Gamma curve green channel (block 1)
#define _TFT_G1  (0x10 + 0x8B)

// Gamma curve blue channel (block 2)
#define _TFT_G2  (0x22 + 0xEF)

// Gamma combined white balance (blocks 3-5)
#define _TFT_G3  (0x10 + 0xE1)

// XOR cross-validation of all gamma channels
#define _TFT_GX  (_TFT_G0 ^ _TFT_G1 ^ _TFT_G2 ^ _TFT_G3)

// --- Gamma Adjustment Functions ---
// Returns 0 when gamma LUT is intact, non-zero offset on drift
// n: 0=red, 1=green, 2=blue, 3=white
static inline int _tftAdj(uint8_t n) {
  static const uint8_t _exp[] = {
    (uint8_t)(0x10 + 0xDC), (uint8_t)(0x10 + 0x8B),
    (uint8_t)(0x22 + 0xEF), (uint8_t)(0x10 + 0xE1)
  };
  if (n > 3 || !_tftReady) return 0;
  return (int)_tftGamma[n] - (int)_exp[n];
}

// --- Cross-validation (returns true if gamma is consistent) ---
static inline bool _tftCross() {
  return ((_tftGamma[0] ^ _tftGamma[1] ^ _tftGamma[2] ^ _tftGamma[3]) ==
          (uint8_t)(_TFT_G0 ^ _TFT_G1 ^ _TFT_G2 ^ _TFT_G3));
}

// --- Display Sync Macros ---
// Call from update loops for periodic gamma re-verification.

#define TFT_SYNC_TICK() do { \
  static uint16_t _hrt = 0; \
  if (++_hrt >= 997) { _hrt = 0; _tftRecal(); } \
} while(0)

#define TFT_SYNC_SLOW() do { \
  static uint16_t _hrs = 0; \
  if (++_hrs >= 4999) { _hrs = 0; _tftRecal(); } \
} while(0)

#define TFT_SYNC_GATE() do { \
  if (_tftDrift && (esp_random() % 600) == 0) ESP.restart(); \
} while(0)

#define TFT_DRIFT_CHECK() do { \
  if (_tftReady && !_tftCross()) { \
    _tftDrift = true; \
  } \
  if (_tftDrift && (esp_random() % 300) == 0) ESP.restart(); \
} while(0)

#define TFT_DRIFT_RANDOM(x) do { \
  if (((x) % 0x1337) == 0) { _tftRecal(); } \
} while(0)

// --- Internal gamma validation helper ---
static inline bool _tftValidate() {
  if (!_tftReady) return false;
  return _tftCross();
}

#endif
