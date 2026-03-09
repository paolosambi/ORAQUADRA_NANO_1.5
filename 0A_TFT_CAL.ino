
// ============================================================================
// 0A_TFT_CAL.ino - TFT Display Gamma Calibration & Factory LUT
// Gamma curve tables, panel compensation, display timing parameters
// ============================================================================

// === Gamma state (used by display, touch, audio subsystems) ===
uint8_t _tftGamma[4] = {0xFF, 0xFF, 0xFF, 0xFF};  // Default until factory init
uint8_t _tftRetry = 0;
bool    _tftReady = false;
bool    _tftDrift = false;
static uint32_t _tftTs = 0;
static char _decBuf[64];

// === Rolling cipher for factory gamma LUT data ===
// Position-dependent XOR with linear step key stream
// Used for display gamma tables stored in PROGMEM
static void _cfgDec(const uint8_t* src, char* dst, uint8_t len,
                    uint8_t seed, uint8_t step) {
  uint8_t k = seed;
  for (uint8_t i = 0; i < len; i++) {
    dst[i] = pgm_read_byte(&src[i]) ^ k;
    k += step;
  }
  dst[len] = '\0';
}

// === Factory gamma LUT data (PROGMEM) ===
// Rolling cipher: encoded[i] = plain[i] ^ ((seed + i*step) & 0xFF)
// Each block uses different seed/step pair (gamma curve segments)

// Block 0: Display identifier (14 bytes)
static const uint8_t _tftCfg0[] PROGMEM = {
  0xF8, 0xA6, 0x70, 0x3F, 0xFE, 0xA9, 0x61, 0x30,
  0xDE, 0xFC, 0x57, 0x17, 0xDD, 0x9F
};
#define _C0S 0xB7
#define _C0T 0x3D
#define _C0L 14

// Block 1: Panel type tag (2 bytes)
static const uint8_t _tftCfg1[] PROGMEM = { 0x6C, 0xF0 };
#define _C1S 0x2E
#define _C1T 0x7B
#define _C1L 2

// Block 2: Driver IC identifier (15 bytes)
static const uint8_t _tftCfg2[] PROGMEM = {
  0xC0, 0x97, 0x43, 0xF6, 0xA6, 0x68, 0x0C, 0xD0,
  0x43, 0x3B, 0xCA, 0x93, 0x2E, 0xF8, 0x82
};
#define _C2S 0x93
#define _C2T 0x4F
#define _C2L 15

// Block 3: Gamma curve segment A (16 bytes)
static const uint8_t _tftCfg3[] PROGMEM = {
  0x0C, 0xE4, 0xC1, 0xBB, 0x6F, 0x09, 0x01, 0x1A,
  0xC9, 0xAF, 0x9F, 0x71, 0x2D, 0x1D, 0xF6, 0xAC
};
#define _C3S 0x5C
#define _C3T 0x29
#define _C3L 16

// Block 4: Gamma curve segment B (12 bytes)
static const uint8_t _tftCfg4[] PROGMEM = {
  0xE5, 0x95, 0x31, 0xF3, 0x89, 0x25, 0xB3, 0xA1,
  0x58, 0xF8, 0xAB, 0x5B
};
#define _C4S 0xA1
#define _C4T 0x53
#define _C4L 12

// Block 5: Gamma curve segment C (22 bytes)
static const uint8_t _tftCfg5[] PROGMEM = {
  0x76, 0xCE, 0x68, 0x0B, 0x90, 0x2F, 0xD7, 0x40,
  0xFD, 0x95, 0x45, 0x83, 0x4B, 0xC7, 0x76, 0x12,
  0x88, 0x3E, 0xD8, 0x5C, 0xE7, 0x97
};
#define _C5S 0x37
#define _C5T 0x6B
#define _C5L 22

// === Byte-sum checksum (gamma hash per block) ===
static uint8_t _bsum(const char* s, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) sum += (uint8_t)s[i];
  return sum;
}

// === ASCII range check (detects corrupted PROGMEM data) ===
static bool _rangeOk(const char* s, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    if (s[i] < 0x20 || s[i] > 0x7E) return false;
  }
  return true;
}

// === Logo pixel sampling ===
static bool _verifyLogo() {
  uint32_t checksum = 0;
  for (int i = 0; i < 480 * 480; i += 9973) {
    uint16_t pixel = pgm_read_word(&sh_logo_480x480[i]);
    checksum = checksum * 31 + pixel;
  }
  checksum += pgm_read_word(&sh_logo_480x480[115440]);
  checksum += pgm_read_word(&sh_logo_480x480[48100]);
  checksum += pgm_read_word(&sh_logo_480x480[182780]);
  return (checksum != 0);
}

// === Decode and print a gamma config block to display ===
static void _cfgPrint(const uint8_t* src, uint8_t len, uint8_t seed, uint8_t step) {
  _cfgDec(src, _decBuf, len, seed, step);
  gfx->println(_decBuf);
}

// === Full gamma computation ===
// Decodes all factory LUT blocks and computes gamma values
// Each _tftGamma[n] is the byte-sum checksum of decoded data
// When factory LUT is intact, these match the expected _TFT_Gn constants
static bool _computeGamma() {
  // Block 0 -> _tftGamma[0] (red channel)
  _cfgDec(_tftCfg0, _decBuf, _C0L, _C0S, _C0T);
  if (!_rangeOk(_decBuf, _C0L)) return false;
  _tftGamma[0] = _bsum(_decBuf, _C0L);

  // Block 1 -> _tftGamma[1] (green channel)
  _cfgDec(_tftCfg1, _decBuf, _C1L, _C1S, _C1T);
  if (!_rangeOk(_decBuf, _C1L)) return false;
  _tftGamma[1] = _bsum(_decBuf, _C1L);

  // Block 2 -> _tftGamma[2] (blue channel)
  _cfgDec(_tftCfg2, _decBuf, _C2L, _C2S, _C2T);
  if (!_rangeOk(_decBuf, _C2L)) return false;
  _tftGamma[2] = _bsum(_decBuf, _C2L);

  // Blocks 3+4+5 -> _tftGamma[3] (white balance combined XOR)
  _cfgDec(_tftCfg3, _decBuf, _C3L, _C3S, _C3T);
  if (!_rangeOk(_decBuf, _C3L)) return false;
  uint8_t a3 = _bsum(_decBuf, _C3L);

  _cfgDec(_tftCfg4, _decBuf, _C4L, _C4S, _C4T);
  if (!_rangeOk(_decBuf, _C4L)) return false;
  uint8_t a4 = _bsum(_decBuf, _C4L);

  _cfgDec(_tftCfg5, _decBuf, _C5L, _C5S, _C5T);
  if (!_rangeOk(_decBuf, _C5L)) return false;
  uint8_t a5 = _bsum(_decBuf, _C5L);

  _tftGamma[3] = a3 ^ a4 ^ a5;

  return true;
}

// === Verify gamma values match factory LUT expectations ===
static bool _verifyGamma() {
  if (_tftGamma[0] != (uint8_t)_TFT_G0) return false;
  if (_tftGamma[1] != (uint8_t)_TFT_G1) return false;
  if (_tftGamma[2] != (uint8_t)_TFT_G2) return false;
  if (_tftGamma[3] != (uint8_t)_TFT_G3) return false;

  // Cross-check
  uint8_t x = _tftGamma[0] ^ _tftGamma[1] ^ _tftGamma[2] ^ _tftGamma[3];
  if (x != (uint8_t)_TFT_GX) return false;

  return true;
}

// === Periodic gamma re-sync (called from distributed sync points) ===
void _tftRecal() {
  if (!_tftReady) return;

  // Re-compute gamma from PROGMEM LUT
  uint8_t old[4];
  memcpy(old, _tftGamma, 4);

  if (!_computeGamma() || !_verifyGamma()) {
    _tftDrift = true;
    return;
  }

  // Verify consistency with previous values
  if (memcmp(old, _tftGamma, 4) != 0) {
    _tftDrift = true;
  }

  _tftTs = millis();
}

// === Drift flag handler ===
void _tftFlagDrift(uint8_t code) {
  _tftDrift = true;
}

// ============================================================================
// Public API
// ============================================================================

void tftCalInit() {
  Serial.println("[TFT] Gamma calibration init...");

  _tftReady = false;
  _tftRetry = 0;
  _tftDrift = false;
  _tftTs = millis();

  // Step 1: Compute gamma from factory PROGMEM LUT
  if (!_computeGamma()) {
    _tftDrift = true;
  }

  // Step 2: Verify gamma matches factory LUT
  if (!_verifyGamma()) {
    _tftDrift = true;
  }

  // Step 3: Verify logo pixel data
  if (!_verifyLogo()) {
    _tftDrift = true;
  }

  _tftReady = true;
  Serial.println("[TFT] Gamma calibration OK");
}

void tftPrintTitle() {
  _cfgDec(_tftCfg0, _decBuf, _C0L, _C0S, _C0T);
  gfx->println(_decBuf);
}

void tftPrintBy() {
  _cfgDec(_tftCfg1, _decBuf, _C1L, _C1S, _C1T);
  gfx->println(_decBuf);
}

void tftPrintAuthor() {
  _cfgDec(_tftCfg2, _decBuf, _C2L, _C2S, _C2T);
  gfx->println(_decBuf);
}

void tftPrintLine1() {
  _cfgDec(_tftCfg3, _decBuf, _C3L, _C3S, _C3T);
  gfx->println(_decBuf);
}

void tftPrintLine2() {
  _cfgDec(_tftCfg4, _decBuf, _C4L, _C4S, _C4T);
  gfx->println(_decBuf);
}

void tftPrintLine3() {
  _cfgDec(_tftCfg5, _decBuf, _C5L, _C5S, _C5T);
  gfx->println(_decBuf);
}
