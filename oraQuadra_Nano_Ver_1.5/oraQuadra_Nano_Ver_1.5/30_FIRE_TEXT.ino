// ================== FIRE TEXT - LETTERE FIAMMEGGIANTI ==================
/*
 * Effetto orario con lettere che bruciano come legno
 * - Tutte le lettere dell'orario visibili con effetto fuoco
 * - Piccole fiamme sopra le lettere
 * - Double buffering per eliminare flickering
 * - Colori legno ardente (rosso/arancione scuro)
 */

#ifdef EFFECT_FIRE_TEXT

// ================== CONFIGURAZIONE ==================
#define FTEXT_CELL_SIZE    4      // Dimensione cella simulazione
#define FTEXT_FIRE_HEIGHT  36     // Altezza fiamme - brace piu alta
#define FTEXT_COOLING      70     // Raffreddamento - brace piu intensa
#define FTEXT_SPARKING     160    // Scintille - piu brace

// Array heat per ogni colonna dello schermo
#define FTEXT_SIM_COLS     (480 / FTEXT_CELL_SIZE)  // 120 colonne
#define FTEXT_SIM_ROWS     (FTEXT_FIRE_HEIGHT / FTEXT_CELL_SIZE)  // 6 righe di fiamma

uint8_t ftextHeat[FTEXT_SIM_COLS][FTEXT_SIM_ROWS];
bool fireTextInitialized = false;
uint32_t ftextLastUpdate = 0;
int ftextLastMinute = -1;
int ftextLastHour = -1;

// Double buffering
uint16_t* ftextFrameBuffer = nullptr;
OffscreenGFX* ftextOffscreenGfx = nullptr;

// ================== PALETTE FUOCO LEGNO ==================
uint16_t ftextHeatColor(uint8_t temp) {
  uint8_t t192 = (uint16_t)temp * 191 / 255;
  uint8_t ramp = (t192 & 0x3F) << 2;
  uint8_t r, g, b;

  if (t192 >= 128) {
    r = 255; g = 160; b = ramp / 4;  // Arancione (no bianco/giallo)
  } else if (t192 >= 64) {
    r = 255; g = ramp / 2; b = 0;    // Arancione scuro
  } else {
    r = ramp; g = 0; b = 0;          // Nero -> rosso
  }
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Colore per le lettere (effetto brace intensa)
uint16_t ftextEmberColor(uint8_t intensity) {
  // Brace: varia da rosso scuro a arancione brillante
  uint8_t r, g, b;

  if (intensity > 200) {
    // Brace molto calda - arancione chiaro
    r = 255;
    g = 120 + (intensity - 200) * 2;  // 120-230
    b = 0;
  } else if (intensity > 100) {
    // Brace media - arancione
    r = 200 + (intensity - 100) * 55 / 100;  // 200-255
    g = 40 + (intensity - 100) * 80 / 100;   // 40-120
    b = 0;
  } else {
    // Brace fredda - rosso scuro
    r = 120 + intensity * 80 / 100;  // 120-200
    g = intensity * 40 / 100;         // 0-40
    b = 0;
  }
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// ================== AGGIORNA ORARIO ATTIVO ==================
void ftextUpdateTime() {
  memset(targetPixels, 0, sizeof(targetPixels));

  if (currentHour == 0) {
    strncpy(&TFT_L[6], "MEZZANOTTE", 10);
    displayWordToTarget(WORD_MEZZANOTTE);
  } else {
    strncpy(&TFT_L[6], "EYOREXZERO", 10);
    displayWordToTarget(WORD_SONO_LE);
    const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
    displayWordToTarget(hourWord);
  }

  if (currentMinute > 0) {
    displayWordToTarget(WORD_E);
    displayMinutesToTarget(currentMinute);
    displayWordToTarget(WORD_MINUTI);
  }

  memcpy(activePixels, targetPixels, sizeof(activePixels));
}

// ================== INIZIALIZZAZIONE ==================
void initFireText() {
  Serial.println("[FIRE_TEXT] Inizializzazione...");

  // Double buffering
  if (ftextFrameBuffer == nullptr) {
    ftextFrameBuffer = (uint16_t*)heap_caps_malloc(480 * 480 * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (ftextFrameBuffer != nullptr) {
      Serial.println("[FIRE_TEXT] Frame buffer allocato in PSRAM");
    }
  }

  if (ftextFrameBuffer != nullptr && ftextOffscreenGfx == nullptr) {
    ftextOffscreenGfx = new OffscreenGFX(ftextFrameBuffer, 480, 480);
  }

  if (ftextFrameBuffer != nullptr) {
    memset(ftextFrameBuffer, 0, 480 * 480 * sizeof(uint16_t));
  }

  memset(ftextHeat, 0, sizeof(ftextHeat));
  ftextUpdateTime();

  gfx->fillScreen(BLACK);

  fireTextInitialized = true;
  ftextLastUpdate = millis();
  ftextLastMinute = currentMinute;
  ftextLastHour = currentHour;

  Serial.println("[FIRE_TEXT] Inizializzato!");
}

// ================== SIMULAZIONE FUOCO ==================
void ftextSimulateFire() {
  for (int col = 0; col < FTEXT_SIM_COLS; col++) {
    // Raffredda
    for (int row = 0; row < FTEXT_SIM_ROWS; row++) {
      int cool = random(0, ((FTEXT_COOLING * 10) / FTEXT_SIM_ROWS) + 2);
      if (cool > ftextHeat[col][row]) {
        ftextHeat[col][row] = 0;
      } else {
        ftextHeat[col][row] -= cool;
      }
    }

    // Convezione
    for (int row = FTEXT_SIM_ROWS - 1; row >= 2; row--) {
      ftextHeat[col][row] = (ftextHeat[col][row-1] + ftextHeat[col][row-2] + ftextHeat[col][row-2]) / 3;
    }

    // Scintille random su tutto lo schermo
    if (random(255) < FTEXT_SPARKING) {
      int y = random(3);
      int newHeat = ftextHeat[col][y] + random(140, 200);
      if (newHeat > 255) newHeat = 255;
      ftextHeat[col][y] = newHeat;
    }
  }
}

// ================== UPDATE PRINCIPALE ==================
void updateFireText() {
  if (!fireTextInitialized) {
    initFireText();
    return;
  }

  uint32_t now = millis();
  if (now - ftextLastUpdate < 25) return;
  ftextLastUpdate = now;

  // Aggiorna orario se cambiato
  if (currentMinute != ftextLastMinute || currentHour != ftextLastHour) {
    ftextLastMinute = currentMinute;
    ftextLastHour = currentHour;
    ftextUpdateTime();
  }

  // Simula fuoco
  ftextSimulateFire();

  // Target per disegno
  Arduino_GFX* targetGfx = (ftextOffscreenGfx != nullptr) ? (Arduino_GFX*)ftextOffscreenGfx : gfx;

  // Pulisci buffer
  if (ftextFrameBuffer != nullptr) {
    memset(ftextFrameBuffer, 0, 480 * 480 * sizeof(uint16_t));
  } else {
    targetGfx->fillScreen(0x0000);
  }

  // === DISEGNA PICCOLE FIAMME SOPRA OGNI LETTERA ATTIVA ===
  for (int i = 0; i < NUM_LEDS; i++) {
    if (activePixels[i]) {
      int letterX = pgm_read_word(&TFT_X[i]);
      int letterY = pgm_read_word(&TFT_Y[i]);

      // Fiammelle sopra la lettera (la lettera e' alta ~25px, baseline a letterY)
      int flameBaseY = letterY - 28;  // Sopra la lettera

      // 3-5 piccole fiamme per lettera
      for (int f = 0; f < 4; f++) {
        int flameX = letterX + random(20);  // Larghezza lettera ~20px
        int flameHeight = random(8, 18);    // Altezza fiamma leggera

        // Disegna fiamma verticale (dal basso verso alto)
        for (int h = 0; h < flameHeight; h++) {
          int fy = flameBaseY - h;
          if (fy > 0 && fy < 480) {
            // Colore: piu caldo in basso, piu freddo in alto
            uint8_t heat = 255 - (h * 255 / flameHeight);
            heat = heat * random(70, 100) / 100;  // Variazione
            if (heat > 30) {
              uint16_t flameColor = ftextHeatColor(heat);
              targetGfx->drawPixel(flameX, fy, flameColor);
              // Fiamma un po' piu larga alla base
              if (h < flameHeight / 3) {
                targetGfx->drawPixel(flameX + 1, fy, flameColor);
              }
            }
          }
        }
      }
    }
  }

  // === DISEGNA TUTTE LE LETTERE CON EFFETTO BRACE ===
  targetGfx->setFont(u8g2_font_inb21_mr);

  for (int i = 0; i < NUM_LEDS; i++) {
    int screenX = pgm_read_word(&TFT_X[i]);
    int screenY = pgm_read_word(&TFT_Y[i]);
    char letter = TFT_L[i];  // TFT_L è in RAM, non PROGMEM

    if (activePixels[i]) {
      // Lettera attiva - effetto brace che pulsa
      uint8_t flicker = random(80, 255);  // Ampia variazione per effetto brace
      uint16_t emberColor = ftextEmberColor(flicker);
      targetGfx->setTextColor(emberColor);
    } else {
      // Lettera non attiva - rosso scurissimo (quasi spenta)
      targetGfx->setTextColor(0x1800);
    }

    targetGfx->setCursor(screenX, screenY);
    targetGfx->write(letter);
  }

  // === COPIA BUFFER SU SCHERMO ===
  if (ftextFrameBuffer != nullptr) {
    gfx->draw16bitRGBBitmap((gfx->width()-480)/2, (gfx->height()-480)/2, ftextFrameBuffer, 480, 480);
  }
}

#endif // EFFECT_FIRE_TEXT
