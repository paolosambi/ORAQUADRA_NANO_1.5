// ================== FIRE - EFFETTO FUOCO CAMINO ==================
/*
 * Effetto fuoco a schermo intero basato su Fire2012 di Mark Kriegsman
 * - Simulazione fisica del fuoco con calore che sale
 * - Palette colori realistica (nero → rosso → arancione → giallo → bianco)
 * - Double buffering per animazione fluida senza flickering
 * - Orario visualizzato in overlay
 */

#ifdef EFFECT_FIRE

// ================== CONFIGURAZIONE ==================
#define FIRE_COLS         480     // Larghezza schermo
#define FIRE_ROWS         160     // Altezza massima fiamme (1/3 schermo)
#define FIRE_CELL_WIDTH   4       // Larghezza cella (480/4 = 120 colonne simulate)
#define FIRE_CELL_HEIGHT  4       // Altezza cella (160/4 = 40 righe simulate)
#define FIRE_SIM_WIDTH    (FIRE_COLS / FIRE_CELL_WIDTH)   // 120 colonne
#define FIRE_SIM_HEIGHT   (FIRE_ROWS / FIRE_CELL_HEIGHT)  // 40 righe

#define FIRE_COOLING_MIN   55     // Raffreddamento minimo (fiamme più alte ma contenute)
#define FIRE_COOLING_MAX   95     // Raffreddamento massimo (fiamme più basse)
#define FIRE_SPARKING_BASE 180    // Probabilità scintille (più alto = più fuoco)

// Variabili globali
uint8_t fireSimHeat[FIRE_SIM_WIDTH][FIRE_SIM_HEIGHT];
uint8_t fireCooling[FIRE_SIM_WIDTH];  // Raffreddamento per colonna (determina altezza fiamma)
bool fireInitialized = false;
uint32_t fireLastUpdate = 0;
uint32_t fireCoolingUpdate = 0;  // Timer per variare il cooling

// Double buffering
uint16_t* fireFrameBuffer = nullptr;
OffscreenGFX* fireOffscreenGfx = nullptr;

// ================== PALETTE FUOCO ==================
// Converte temperatura (0-255) in colore RGB565
uint16_t fireHeatToColor(uint8_t temperature) {
  // Scala temperatura a 0-191 per mappatura colore
  uint8_t t192 = (uint16_t)temperature * 191 / 255;

  uint8_t heatramp = t192 & 0x3F;  // 0-63
  heatramp <<= 2;  // Scala a 0-252

  uint8_t r, g, b;

  if (t192 >= 128) {
    // Zona più calda: giallo → bianco
    r = 255;
    g = 255;
    b = heatramp;
  } else if (t192 >= 64) {
    // Zona media: arancione → giallo
    r = 255;
    g = heatramp;
    b = 0;
  } else {
    // Zona fredda: nero → rosso
    r = heatramp;
    g = 0;
    b = 0;
  }

  // Converti RGB888 a RGB565
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// ================== INIZIALIZZAZIONE ==================
void initFire() {
  Serial.println("[FIRE] Inizializzazione effetto fuoco...");

  // ===== INIZIALIZZAZIONE DOUBLE BUFFERING =====
  if (fireFrameBuffer == nullptr) {
    Serial.println("[FIRE] Allocazione frame buffer (480x480x2 = 460KB)...");
    fireFrameBuffer = (uint16_t*)heap_caps_malloc(480 * 480 * sizeof(uint16_t), MALLOC_CAP_SPIRAM);

    if (fireFrameBuffer == nullptr) {
      Serial.println("[FIRE] ERRORE: Impossibile allocare frame buffer!");
      // Continua comunque senza double buffering
    } else {
      Serial.println("[FIRE] Frame buffer allocato in PSRAM");
    }
  }

  if (fireFrameBuffer != nullptr && fireOffscreenGfx == nullptr) {
    Serial.println("[FIRE] Creazione OffscreenGFX...");
    fireOffscreenGfx = new OffscreenGFX(fireFrameBuffer, 480, 480);
    Serial.println("[FIRE] OffscreenGFX creato - Double buffering attivo!");
  }

  // Azzera frame buffer (nero)
  if (fireFrameBuffer != nullptr) {
    for (int i = 0; i < 480 * 480; i++) {
      fireFrameBuffer[i] = 0x0000;
    }
  }

  // Azzera array heat
  for (int x = 0; x < FIRE_SIM_WIDTH; x++) {
    for (int y = 0; y < FIRE_SIM_HEIGHT; y++) {
      fireSimHeat[x][y] = 0;
    }
    // Inizializza cooling random per ogni colonna
    fireCooling[x] = random(FIRE_COOLING_MIN, FIRE_COOLING_MAX);
  }

  // Sfondo nero iniziale
  gfx->fillScreen(0x0000);

  fireInitialized = true;
  fireLastUpdate = millis();
  fireCoolingUpdate = millis();

  Serial.println("[FIRE] Inizializzato!");
}

// ================== SIMULAZIONE FIRE2012 PER COLONNA ==================
void fireSimulateCol(int col) {
  // Usa il cooling specifico per questa colonna (determina altezza fiamma)
  int cooling = fireCooling[col];
  int sparking = FIRE_SPARKING_BASE;

  // Step 1: Raffredda ogni cella (il calore si dissipa)
  for (int y = 0; y < FIRE_SIM_HEIGHT; y++) {
    int cooldown = random(0, ((cooling * 10) / FIRE_SIM_HEIGHT) + 2);
    if (cooldown > fireSimHeat[col][y]) {
      fireSimHeat[col][y] = 0;
    } else {
      fireSimHeat[col][y] -= cooldown;
    }
  }

  // Step 2: Il calore sale e si diffonde (convezione)
  for (int y = FIRE_SIM_HEIGHT - 1; y >= 2; y--) {
    fireSimHeat[col][y] = (fireSimHeat[col][y-1] + fireSimHeat[col][y-2] + fireSimHeat[col][y-2]) / 3;
  }

  // Step 3: Genera scintille casuali alla base
  if (random(255) < sparking) {
    int y = random(7);  // Prime 7 righe
    int newHeat = fireSimHeat[col][y] + random(160, 255);
    if (newHeat > 255) newHeat = 255;
    fireSimHeat[col][y] = newHeat;
  }
}

// ================== AGGIORNAMENTO PRINCIPALE ==================
void updateFire() {
  if (!fireInitialized) {
    initFire();
    return;
  }

  uint32_t now = millis();

  // Aggiorna a ~60 FPS per animazione più fluida e veloce
  if (now - fireLastUpdate < 16) return;
  fireLastUpdate = now;

  // Ogni 150ms varia gradualmente il cooling di alcune colonne (altezza fiamme)
  if (now - fireCoolingUpdate > 150) {
    fireCoolingUpdate = now;
    // Modifica cooling di ~20% delle colonne casualmente
    for (int i = 0; i < FIRE_SIM_WIDTH / 5; i++) {
      int col = random(FIRE_SIM_WIDTH);
      // Varia gradualmente verso un nuovo target
      int target = random(FIRE_COOLING_MIN, FIRE_COOLING_MAX);
      // Movimento graduale (non brusco)
      if (fireCooling[col] < target) {
        fireCooling[col] += random(1, 4);
        if (fireCooling[col] > FIRE_COOLING_MAX) fireCooling[col] = FIRE_COOLING_MAX;
      } else {
        fireCooling[col] -= random(1, 4);
        if (fireCooling[col] < FIRE_COOLING_MIN) fireCooling[col] = FIRE_COOLING_MIN;
      }
    }
    // Liscia con colonne vicine per transizioni morbide
    for (int col = 1; col < FIRE_SIM_WIDTH - 1; col++) {
      if (random(100) < 30) {
        fireCooling[col] = (fireCooling[col-1] + fireCooling[col] + fireCooling[col+1]) / 3;
      }
    }
  }

  // Simula tutte le colonne
  for (int col = 0; col < FIRE_SIM_WIDTH; col++) {
    fireSimulateCol(col);
  }

  // Scegli target per disegno (double buffer o diretto)
  Arduino_GFX* targetGfx = (fireOffscreenGfx != nullptr) ? (Arduino_GFX*)fireOffscreenGfx : gfx;

  // Pulisci buffer (nero)
  if (fireFrameBuffer != nullptr) {
    memset(fireFrameBuffer, 0, 480 * 480 * sizeof(uint16_t));
  } else {
    targetGfx->fillScreen(0x0000);
  }

  // Disegna il fuoco nella parte inferiore (dal basso verso l'alto)
  for (int col = 0; col < FIRE_SIM_WIDTH; col++) {
    int screenX = col * FIRE_CELL_WIDTH;

    for (int row = 0; row < FIRE_SIM_HEIGHT; row++) {
      // Il fuoco parte dal fondo dello schermo (Y=480) e sale
      int screenY = 480 - 1 - (row * FIRE_CELL_HEIGHT);

      uint16_t color = fireHeatToColor(fireSimHeat[col][row]);

      // Disegna cella (4x4 pixel)
      if (color != 0x0000) {  // Ottimizza: non disegnare nero
        targetGfx->fillRect(screenX, screenY - FIRE_CELL_HEIGHT + 1, FIRE_CELL_WIDTH, FIRE_CELL_HEIGHT, color);
      }
    }
  }

  // === DISEGNA ORARIO NELLA PARTE SUPERIORE ===
  // Box per orario nella zona nera superiore
  targetGfx->fillRoundRect(115, 70, 250, 100, 15, 0x0000);
  targetGfx->drawRoundRect(115, 70, 250, 100, 15, 0xFBE0);  // Bordo arancione
  targetGfx->drawRoundRect(116, 71, 248, 98, 14, 0xF800);  // Bordo rosso interno

  // Orario grande
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", currentHour, currentMinute);

  targetGfx->setFont(u8g2_font_logisoso50_tn);
  targetGfx->setTextColor(0xFBE0);  // Arancione fuoco

  int16_t tx, ty;
  uint16_t tw, th;
  targetGfx->getTextBounds(timeStr, 0, 0, &tx, &ty, &tw, &th);
  targetGfx->setCursor(240 - tw/2, 145);
  targetGfx->print(timeStr);

  // === COPIA BUFFER SU SCHERMO (se double buffering attivo) ===
  if (fireFrameBuffer != nullptr) {
    gfx->draw16bitRGBBitmap(0, 0, fireFrameBuffer, 480, 480);
  }
}

#endif // EFFECT_FIRE
