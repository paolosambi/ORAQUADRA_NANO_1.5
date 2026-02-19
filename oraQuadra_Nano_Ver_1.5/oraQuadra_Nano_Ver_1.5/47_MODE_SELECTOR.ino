// ================== MODE SELECTOR OVERLAY ==================
// Overlay a griglia per selezione diretta della modalita' display.
// Attivato da tocco centro-sinistra (x:0-140, y:190-290).
// Pattern: stessa logica di setupPage (flag + intercept + timeout).

// ====================== VARIABILI GLOBALI ======================
bool modeSelectorActive = false;
uint32_t modeSelectorLastActivity = 0;
int modeSelectorScroll = 0;         // prima riga visibile (0-based)
int modeSelectorTotalModes = 0;     // quanti modi validi+abilitati
uint8_t modeSelectorModeList[32];   // indici dei modi visibili

// ====================== COSTANTI LAYOUT ======================
#define MS_HEADER_H     44
#define MS_GRID_TOP     50
#define MS_CELL_W      113
#define MS_CELL_H       93
#define MS_GAP           6
#define MS_MARGIN        5
#define MS_COLS          4
#define MS_VISIBLE_ROWS  4
#define MS_ROW_STEP     (MS_CELL_H + MS_GAP)   // 99
#define MS_COL_STEP     (MS_CELL_W + MS_GAP)    // 119
#define MS_SCROLL_Y    440
#define MS_TIMEOUT     15000

// ====================== COLORI ======================
#define MS_BG_COLOR      0x0000  // BLACK
#define MS_CARD_BG       0x10A2  // grigio scuro ~(0x1a,0x1a,0x2e)
#define MS_HEADER_COLOR  0xFFFF  // WHITE
#define MS_TEXT_COLOR     0xC618  // grigio chiaro
#define MS_CLOSE_COLOR   0xF800  // RED

// ====================== buildModeList ======================
void buildModeList() {
  modeSelectorTotalModes = 0;
  for (int i = 0; i < NUM_MODES; i++) {
    if (isValidMode((DisplayMode)i) && isModeEnabled((uint8_t)i)) {
      modeSelectorModeList[modeSelectorTotalModes] = (uint8_t)i;
      modeSelectorTotalModes++;
    }
  }
}

// ====================== getModeSelectorColor ======================
uint16_t getModeSelectorColor(uint8_t mode) {
  switch (mode) {
    case 0:  return 0x001F;  // MODE_FADE - BLUE
    case 1:  return 0x780F;  // MODE_SLOW - PURPLE
    case 2:  return 0x07FF;  // MODE_FAST - CYAN
    case 3:  return 0x07E0;  // MODE_MATRIX - GREEN
    case 4:  return 0x001F;  // MODE_MATRIX2 - BLUE
    case 5:  return 0xFFE0;  // MODE_SNAKE - YELLOW
    case 6:  return 0x07FF;  // MODE_WATER - CYAN
    case 7:  return 0xF800;  // MODE_MARIO - RED
    case 8:  return 0x07FF;  // MODE_TRON - CYAN
    case 9:  return 0x07FF;  // MODE_GALAGA - CYAN
    case 10: return 0xFFFF;  // MODE_ANALOG_CLOCK - WHITE
    case 11: return 0x07FF;  // MODE_FLIP_CLOCK - CYAN
    case 12: return 0x07E0;  // MODE_BTTF - GREEN
    case 13: return 0x07FF;  // MODE_LED_RING - CYAN
    case 14: return 0x07FF;  // MODE_WEATHER - CYAN
    case 15: return 0xFFE0;  // MODE_CLOCK - YELLOW
    case 16: return 0x001F;  // MODE_GEMINI - BLUE
    case 17: return 0x07FF;  // MODE_GALAGA2 - CYAN
    case 18: return 0xF81F;  // MODE_MJPEG - MAGENTA
    case 19: return 0x07FF;  // MODE_ESP32CAM - CYAN
    case 20: return 0xFFE0;  // MODE_FLUX_CAPACITOR - YELLOW
    case 21: return 0x07FF;  // MODE_MP3_PLAYER - CYAN
    case 22: return 0x07E0;  // MODE_WEB_RADIO - GREEN
    case 23: return 0x07FF;  // MODE_RADIO_ALARM - CYAN
    case 24: return 0xF81F;  // MODE_WEB_TV - MAGENTA
    case 25: return 0x07FF;  // MODE_CALENDAR - CYAN
    case 26: return 0xF800;  // MODE_YOUTUBE - RED
    case 27: return 0x05BF;  // MODE_NEWS - azzurro (0,180,255)
    case 28: return 0xFFFF;  // MODE_PONG - WHITE
    case 29: return 0x67EA;  // MODE_SCROLLTEXT - verde lime
    default: return 0xFFFF;
  }
}

// ====================== getModeSelectorName ======================
const char* getModeSelectorName(uint8_t mode) {
  switch (mode) {
    case 0:  return "Fade";
    case 1:  return "Slow";
    case 2:  return "Fast";
    case 3:  return "Matrix";
    case 4:  return "Matrix 2";
    case 5:  return "Snake";
    case 6:  return "Water";
    case 7:  return "Mario";
    case 8:  return "Tron";
    case 9:  return "Galaga";
    case 10: return "Orologio";
    case 11: return "Flip Clock";
    case 12: return "BTTF";
    case 13: return "LED Ring";
    case 14: return "Meteo";
    case 15: return "Clock";
    case 16: return "Gemini";
    case 17: return "Galaga 2";
    case 18: return "MJPEG";
    case 19: return "ESP32CAM";
    case 20: return "Flux Cap";
    case 21: return "MP3";
    case 22: return "Radio";
    case 23: return "Sveglia";
    case 24: return "Web TV";
    case 25: return "Calendario";
    case 26: return "YouTube";
    case 27: return "News";
    case 28: return "Pong";
    case 29: return "Scroll";
    default: return "???";
  }
}

// ====================== drawModeIcon ======================
// Disegna icona geometrica 32x32 al centro di (cx, cy)
void drawModeIcon(int cx, int cy, uint8_t mode) {
  uint16_t col = getModeSelectorColor(mode);
  int x0 = cx - 16;  // top-left della zona 32x32
  int y0 = cy - 16;

  switch (mode) {
    case 0: // FADE - 3 barre con gradazione
    case 1: // SLOW
    case 2: // FAST
      gfx->fillRect(x0 + 2, y0 + 4, 28, 6, col);
      gfx->fillRect(x0 + 6, y0 + 14, 20, 6, col & 0x7BEF); // piu' scuro
      gfx->fillRect(x0 + 10, y0 + 24, 12, 6, col & 0x39E7); // ancora piu' scuro
      break;

    case 3: // MATRIX - linee verticali verdi
    case 4: // MATRIX2
      for (int i = 0; i < 6; i++) {
        int lx = x0 + 3 + i * 5;
        int lh = 10 + (i * 7) % 20;
        gfx->fillRect(lx, y0 + 32 - lh, 3, lh, col);
      }
      break;

    case 5: // SNAKE - zigzag
      gfx->drawLine(x0 + 2, y0 + 24, x0 + 10, y0 + 8, col);
      gfx->drawLine(x0 + 10, y0 + 8, x0 + 18, y0 + 24, col);
      gfx->drawLine(x0 + 18, y0 + 24, x0 + 28, y0 + 8, col);
      gfx->drawLine(x0 + 3, y0 + 24, x0 + 11, y0 + 8, col);
      gfx->drawLine(x0 + 11, y0 + 8, x0 + 19, y0 + 24, col);
      gfx->drawLine(x0 + 19, y0 + 24, x0 + 29, y0 + 8, col);
      break;

    case 6: // WATER - goccia
      gfx->fillCircle(cx, cy + 4, 8, col);
      gfx->fillTriangle(cx, y0 + 2, cx - 8, cy + 4, cx + 8, cy + 4, col);
      break;

    case 7: // MARIO - funghetto
      gfx->fillCircle(cx, cy - 4, 10, 0xF800); // cappello rosso
      gfx->fillRect(cx - 5, cy + 6, 10, 8, 0xFFE0); // gambo giallo
      gfx->fillCircle(cx - 4, cy - 6, 3, 0xFFFF); // puntino
      gfx->fillCircle(cx + 4, cy - 2, 3, 0xFFFF); // puntino
      break;

    case 8: // TRON - linee a L
      gfx->fillRect(x0 + 4, y0 + 4, 3, 24, col);
      gfx->fillRect(x0 + 4, y0 + 25, 20, 3, col);
      gfx->fillRect(x0 + 14, y0 + 10, 14, 3, col);
      gfx->fillRect(x0 + 14, y0 + 10, 3, 15, col);
      break;

    case 9:  // GALAGA - space invader
    case 17: // GALAGA2
      gfx->fillRect(cx - 2, y0 + 2, 4, 6, col);   // corpo centrale
      gfx->fillRect(cx - 8, y0 + 8, 16, 8, col);   // ala
      gfx->fillRect(cx - 12, y0 + 14, 24, 4, col);  // base
      gfx->fillRect(cx - 12, y0 + 18, 4, 6, col);   // gamba sx
      gfx->fillRect(cx + 8, y0 + 18, 4, 6, col);    // gamba dx
      gfx->fillRect(cx - 6, y0 + 22, 4, 4, col);    // piede sx
      gfx->fillRect(cx + 2, y0 + 22, 4, 4, col);    // piede dx
      break;

    case 10: // ANALOG_CLOCK - cerchio + lancette
      gfx->drawCircle(cx, cy, 14, col);
      gfx->drawLine(cx, cy, cx, cy - 10, col);   // lancetta ore
      gfx->drawLine(cx, cy, cx + 8, cy + 4, col); // lancetta minuti
      gfx->fillCircle(cx, cy, 2, col);
      break;

    case 11: // FLIP_CLOCK - due rettangoli
      gfx->fillRoundRect(x0 + 1, y0 + 2, 13, 12, 2, MS_CARD_BG);
      gfx->drawRoundRect(x0 + 1, y0 + 2, 13, 12, 2, col);
      gfx->fillRoundRect(x0 + 17, y0 + 2, 13, 12, 2, MS_CARD_BG);
      gfx->drawRoundRect(x0 + 17, y0 + 2, 13, 12, 2, col);
      gfx->fillRoundRect(x0 + 1, y0 + 17, 13, 12, 2, MS_CARD_BG);
      gfx->drawRoundRect(x0 + 1, y0 + 17, 13, 12, 2, col);
      gfx->fillRoundRect(x0 + 17, y0 + 17, 13, 12, 2, MS_CARD_BG);
      gfx->drawRoundRect(x0 + 17, y0 + 17, 13, 12, 2, col);
      gfx->fillRect(x0 + 15, y0 + 8, 2, 2, col);  // due punti
      gfx->fillRect(x0 + 15, y0 + 22, 2, 2, col);
      break;

    case 12: // BTTF - "88" verde
      gfx->setFont(u8g2_font_helvB14_tr);
      gfx->setTextColor(0x07E0);
      gfx->setCursor(cx - 12, cy + 6);
      gfx->print("88");
      gfx->setFont(u8g2_font_helvB10_tr);
      break;

    case 13: // LED_RING - anello
      gfx->drawCircle(cx, cy, 13, col);
      gfx->drawCircle(cx, cy, 12, col);
      gfx->drawCircle(cx, cy, 11, col);
      gfx->fillCircle(cx, cy, 4, col);
      break;

    case 14: // WEATHER - sole + nuvola
      gfx->fillCircle(cx - 4, cy - 2, 7, 0xFFE0); // sole giallo
      // Raggi
      gfx->drawLine(cx - 4, cy - 12, cx - 4, cy - 14, 0xFFE0);
      gfx->drawLine(cx + 5, cy - 7, cx + 7, cy - 9, 0xFFE0);
      gfx->drawLine(cx - 13, cy - 7, cx - 15, cy - 9, 0xFFE0);
      // Nuvola
      gfx->fillCircle(cx + 4, cy + 6, 5, 0xC618);
      gfx->fillCircle(cx + 10, cy + 8, 4, 0xC618);
      gfx->fillRect(cx - 1, cy + 8, 14, 5, 0xC618);
      break;

    case 15: // CLOCK - cerchio orologio
      gfx->drawCircle(cx, cy, 14, col);
      gfx->drawCircle(cx, cy, 13, col);
      for (int h = 0; h < 12; h++) {
        float a = h * 3.14159 / 6.0;
        int hx = cx + 11 * sin(a);
        int hy = cy - 11 * cos(a);
        gfx->fillCircle(hx, hy, 1, col);
      }
      gfx->drawLine(cx, cy, cx + 3, cy - 8, col);
      gfx->drawLine(cx, cy, cx + 7, cy + 2, col);
      break;

    case 16: // GEMINI_AI - stella/sparkle
      gfx->fillTriangle(cx, y0 + 2, cx - 4, cy, cx + 4, cy, col);
      gfx->fillTriangle(cx, y0 + 30, cx - 4, cy, cx + 4, cy, col);
      gfx->fillTriangle(x0 + 2, cy, cx, cy - 4, cx, cy + 4, col);
      gfx->fillTriangle(x0 + 30, cy, cx, cy - 4, cx, cy + 4, col);
      break;

    case 18: // MJPEG - rettangolo play
    case 19: // ESP32CAM
      gfx->drawRect(x0 + 2, y0 + 4, 28, 24, col);
      gfx->fillTriangle(cx - 4, cy - 6, cx - 4, cy + 6, cx + 8, cy, col);
      break;

    case 20: // FLUX_CAPACITOR - Y
      gfx->drawLine(cx, cy, cx, y0 + 28, col);       // linea giu'
      gfx->drawLine(cx, cy, cx - 10, y0 + 4, col);   // braccio sx
      gfx->drawLine(cx, cy, cx + 10, y0 + 4, col);   // braccio dx
      gfx->drawLine(cx + 1, cy, cx + 1, y0 + 28, col);
      gfx->drawLine(cx + 1, cy, cx - 9, y0 + 4, col);
      gfx->drawLine(cx + 1, cy, cx + 11, y0 + 4, col);
      gfx->fillCircle(cx, cy, 3, col);
      break;

    case 21: // MP3_PLAYER - nota musicale
      gfx->fillCircle(cx - 4, cy + 8, 5, col);
      gfx->fillRect(cx, y0 + 4, 3, 22, col);
      gfx->fillRect(cx, y0 + 4, 10, 3, col);
      gfx->fillRect(cx + 10, y0 + 4, 3, 8, col);
      gfx->fillCircle(cx + 8, cy + 2, 4, col);
      break;

    case 22: // WEB_RADIO - antenna con onde
      gfx->fillRect(cx - 1, cy - 2, 3, 16, col);    // antenna
      gfx->fillTriangle(cx - 8, cy + 14, cx + 8, cy + 14, cx, cy + 8, col); // base
      // Onde
      gfx->drawCircle(cx, cy - 6, 7, col);
      gfx->drawCircle(cx, cy - 6, 11, col);
      break;

    case 23: // RADIO_ALARM - sveglia
      gfx->drawCircle(cx, cy + 2, 12, col);
      gfx->drawCircle(cx, cy + 2, 11, col);
      gfx->drawLine(cx, cy + 2, cx, cy - 5, col); // lancetta
      gfx->drawLine(cx, cy + 2, cx + 5, cy + 2, col);
      // Campanelle
      gfx->fillCircle(cx - 10, cy - 8, 4, col);
      gfx->fillCircle(cx + 10, cy - 8, 4, col);
      break;

    case 24: // WEB_TV - schermo
      gfx->drawRoundRect(x0 + 2, y0 + 2, 28, 20, 3, col);
      gfx->drawRoundRect(x0 + 3, y0 + 3, 26, 18, 2, col);
      gfx->fillRect(cx - 5, y0 + 24, 10, 3, col); // piedistallo
      gfx->fillRect(cx - 8, y0 + 27, 16, 2, col); // base
      break;

    case 25: // CALENDAR - griglia
      gfx->drawRect(x0 + 2, y0 + 6, 28, 24, col);
      gfx->fillRect(x0 + 2, y0 + 6, 28, 6, col);  // header
      // Righe
      gfx->drawLine(x0 + 2, y0 + 18, x0 + 30, y0 + 18, col);
      // Colonne
      gfx->drawLine(x0 + 11, y0 + 12, x0 + 11, y0 + 30, col);
      gfx->drawLine(x0 + 20, y0 + 12, x0 + 20, y0 + 30, col);
      break;

    case 26: // YOUTUBE - rettangolo rosso + play
      gfx->fillRoundRect(x0 + 1, y0 + 6, 30, 20, 4, 0xF800);
      gfx->fillTriangle(cx - 3, cy - 4, cx - 3, cy + 5, cx + 6, cy, 0xFFFF);
      break;

    case 27: // NEWS - righe di testo
      gfx->fillRect(x0 + 2, y0 + 4, 28, 4, col);
      gfx->fillRect(x0 + 2, y0 + 12, 22, 3, 0xC618);
      gfx->fillRect(x0 + 2, y0 + 18, 26, 3, 0xC618);
      gfx->fillRect(x0 + 2, y0 + 24, 18, 3, 0xC618);
      break;

    case 28: // PONG - paddle + pallina
      gfx->fillRect(x0 + 3, cy - 8, 4, 16, col);     // paddle sx
      gfx->fillRect(x0 + 25, cy - 6, 4, 12, col);    // paddle dx
      gfx->fillCircle(cx + 2, cy - 2, 3, col);        // pallina
      // linea centrale tratteggiata
      for (int dy = 0; dy < 32; dy += 6) {
        gfx->fillRect(cx - 1, y0 + dy, 2, 3, 0x4208);
      }
      break;

    case 29: // SCROLLTEXT - testo scorrevole
      gfx->fillRect(x0 + 2, y0 + 12, 28, 8, col);
      gfx->fillTriangle(x0 + 26, y0 + 8, x0 + 26, y0 + 24, x0 + 32, y0 + 16, col);
      break;

    default: // fallback - punto interrogativo
      gfx->drawCircle(cx, cy, 12, col);
      gfx->setFont(u8g2_font_helvB14_tr);
      gfx->setTextColor(col);
      gfx->setCursor(cx - 5, cy + 6);
      gfx->print("?");
      gfx->setFont(u8g2_font_helvB10_tr);
      break;
  }
}

// ====================== drawModeCard ======================
void drawModeCard(int gridX, int gridY, int modeIndex, bool isCurrentMode) {
  int x = MS_MARGIN + gridX * MS_COL_STEP;
  int y = MS_GRID_TOP + gridY * MS_ROW_STEP;
  uint16_t themeColor = getModeSelectorColor(modeIndex);
  const char* name = getModeSelectorName(modeIndex);

  // Background card
  gfx->fillRoundRect(x, y, MS_CELL_W, MS_CELL_H, 8, MS_CARD_BG);

  // Accent bar top
  gfx->fillRect(x + 2, y, MS_CELL_W - 4, 3, themeColor);

  // Bordo evidenziato se modo corrente
  if (isCurrentMode) {
    gfx->drawRoundRect(x, y, MS_CELL_W, MS_CELL_H, 8, themeColor);
    gfx->drawRoundRect(x + 1, y + 1, MS_CELL_W - 2, MS_CELL_H - 2, 7, themeColor);
  }

  // Icona al centro della card (y spostata in alto per lasciare spazio al nome)
  int iconCX = x + MS_CELL_W / 2;
  int iconCY = y + 38;
  drawModeIcon(iconCX, iconCY, modeIndex);

  // Nome modo sotto l'icona
  gfx->setFont(u8g2_font_helvB10_tr);
  int nameLen = strlen(name);
  int nameW = nameLen * 7;  // stima ~7px per char con helvB10
  int nameX = x + (MS_CELL_W - nameW) / 2;
  if (nameX < x + 2) nameX = x + 2;
  gfx->setTextColor(MS_TEXT_COLOR);
  gfx->setCursor(nameX, y + MS_CELL_H - 8);
  gfx->print(name);
}

// ====================== drawModeSelectorGrid ======================
void drawModeSelectorGrid() {
  // Pulisci area griglia
  gfx->fillRect(0, MS_HEADER_H, 480, 480 - MS_HEADER_H, MS_BG_COLOR);

  int startIndex = modeSelectorScroll * MS_COLS;

  for (int row = 0; row < MS_VISIBLE_ROWS; row++) {
    for (int col = 0; col < MS_COLS; col++) {
      int idx = startIndex + row * MS_COLS + col;
      if (idx >= modeSelectorTotalModes) break;

      uint8_t modeIdx = modeSelectorModeList[idx];
      bool isCurrent = (modeIdx == (uint8_t)currentMode);
      drawModeCard(col, row, modeIdx, isCurrent);
    }
  }

  // Indicatori scroll se necessario
  int totalRows = (modeSelectorTotalModes + MS_COLS - 1) / MS_COLS;
  if (totalRows > MS_VISIBLE_ROWS) {
    gfx->setFont(u8g2_font_helvB12_tr);

    // Freccia SU
    if (modeSelectorScroll > 0) {
      gfx->setTextColor(0xFFFF);
      gfx->setCursor(60, 470);
      gfx->print("<< PREV");
    } else {
      gfx->setTextColor(0x4208); // grigio scuro = disabilitato
      gfx->setCursor(60, 470);
      gfx->print("<< PREV");
    }

    // Freccia GIU'
    if (modeSelectorScroll + MS_VISIBLE_ROWS < totalRows) {
      gfx->setTextColor(0xFFFF);
      gfx->setCursor(320, 470);
      gfx->print("NEXT >>");
    } else {
      gfx->setTextColor(0x4208);
      gfx->setCursor(320, 470);
      gfx->print("NEXT >>");
    }

    // Indicatore pagina
    gfx->setTextColor(0x8410);
    gfx->setCursor(210, 470);
    char pageBuf[8];
    snprintf(pageBuf, sizeof(pageBuf), "%d/%d", modeSelectorScroll / MS_VISIBLE_ROWS + 1,
             (totalRows + MS_VISIBLE_ROWS - 1) / MS_VISIBLE_ROWS);
    gfx->print(pageBuf);
  }
}

// ====================== showModeSelector ======================
void showModeSelector() {
  // Slave multi-display: menu modo solo sul master
  #ifdef EFFECT_DUAL_DISPLAY
  extern bool isDualSlave();
  if (isDualSlave()) return;
  // Bypass proxy DualGFX per disegnare a schermo intero (480x480 locale)
  extern void dualGfxBypass(bool bypass);
  extern bool isDualDisplayActive();
  if (isDualDisplayActive()) dualGfxBypass(true);
  #endif

  modeSelectorActive = true;
  modeSelectorScroll = 0;
  modeSelectorLastActivity = millis();

  buildModeList();

  // Se il modo corrente non e' nella prima pagina, scrolla fino a renderlo visibile
  for (int i = 0; i < modeSelectorTotalModes; i++) {
    if (modeSelectorModeList[i] == (uint8_t)currentMode) {
      modeSelectorScroll = (i / MS_COLS) / MS_VISIBLE_ROWS * MS_VISIBLE_ROWS;
      break;
    }
  }

  gfx->fillScreen(MS_BG_COLOR);

  // Header
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(MS_HEADER_COLOR);
  gfx->setCursor(100, 30);
  gfx->print("SELEZIONA MODO");

  // Pulsante X (chiudi)
  gfx->fillRoundRect(440, 8, 32, 28, 6, 0x8000); // rosso scuro bg
  gfx->setTextColor(MS_CLOSE_COLOR);
  gfx->setCursor(448, 30);
  gfx->print("X");

  drawModeSelectorGrid();

  Serial.printf("[MODE_SEL] Overlay aperto - %d modi disponibili\n", modeSelectorTotalModes);
}

// ====================== hideModeSelector ======================
void hideModeSelector() {
  modeSelectorActive = false;

  // Ripristina proxy DualGFX prima di forceDisplayUpdate
  #ifdef EFFECT_DUAL_DISPLAY
  extern void dualGfxBypass(bool bypass);
  extern bool isDualDisplayActive();
  if (isDualDisplayActive()) dualGfxBypass(false);
  #endif

  gfx->fillScreen(MS_BG_COLOR);
  forceDisplayUpdate();
  Serial.println("[MODE_SEL] Overlay chiuso");
}

// ====================== handleModeSelectorTouch ======================
void handleModeSelectorTouch(int x, int y) {
  modeSelectorLastActivity = millis();

  // Header area - pulsante X chiudi
  if (y < MS_HEADER_H) {
    if (x > 430) {
      playTouchSound();
      hideModeSelector();
    }
    return;
  }

  // Area griglia
  if (y >= MS_GRID_TOP && y <= MS_SCROLL_Y) {
    int col = (x - MS_MARGIN) / MS_COL_STEP;
    int row = (y - MS_GRID_TOP) / MS_ROW_STEP;

    if (col < 0) col = 0;
    if (col >= MS_COLS) return;
    if (row < 0) row = 0;
    if (row >= MS_VISIBLE_ROWS) return;

    int idx = (modeSelectorScroll + row) * MS_COLS + col;
    if (idx >= 0 && idx < modeSelectorTotalModes) {
      playTouchSound();
      switchToMode(modeSelectorModeList[idx]);
    }
    return;
  }

  // Area scroll (sotto la griglia)
  if (y > MS_SCROLL_Y) {
    int totalRows = (modeSelectorTotalModes + MS_COLS - 1) / MS_COLS;
    if (totalRows <= MS_VISIBLE_ROWS) return; // niente scroll necessario

    playTouchSound();
    if (x < 240) {
      // Scroll UP (pagina precedente)
      if (modeSelectorScroll > 0) {
        modeSelectorScroll -= MS_VISIBLE_ROWS;
        if (modeSelectorScroll < 0) modeSelectorScroll = 0;
        drawModeSelectorGrid();
      }
    } else {
      // Scroll DOWN (pagina successiva)
      if (modeSelectorScroll + MS_VISIBLE_ROWS < totalRows) {
        modeSelectorScroll += MS_VISIBLE_ROWS;
        drawModeSelectorGrid();
      }
    }
    return;
  }
}

// ====================== PENDING MODE SWITCH ======================
volatile int8_t pendingModeSelectorSwitch = -1;  // -1 = nessun cambio, 0-31 = modo target

// ====================== switchToMode ======================
// Salva il target e chiude l'overlay. Il cambio modo effettivo
// avviene nel loop principale (stesso livello di handleModeChange).
void switchToMode(uint8_t targetMode) {
  pendingModeSelectorSwitch = (int8_t)targetMode;
  modeSelectorActive = false;

  // Ripristina proxy DualGFX prima di disegnare il modo
  #ifdef EFFECT_DUAL_DISPLAY
  extern void dualGfxBypass(bool bypass);
  extern bool isDualDisplayActive();
  if (isDualDisplayActive()) dualGfxBypass(false);
  #endif

  gfx->fillScreen(BLACK);
  Serial.printf("[MODE_SEL] Richiesto switch a modo %d\n", targetMode);
}

// ====================== executePendingModeSwitch ======================
// Chiamata dal loop principale - esegue il cambio modo con la stessa
// logica di handleModeChange() ma verso un modo specifico.
void executePendingModeSwitch() {
  uint8_t targetMode = (uint8_t)pendingModeSelectorSwitch;
  pendingModeSelectorSwitch = -1;

  // Protezione distribuita (come handleModeChange)
  if (!distributedCheck2()) {
    protectionFailCount++;
    if (protectionFailCount >= 5) protectionFailed(0xD2);
  }

  // Cleanup
  DisplayMode previousMode = currentMode;
  cleanupPreviousMode(previousMode);

  // Imposta nuova modalita'
  currentMode = (DisplayMode)targetMode;
  userMode = currentMode;

  // Carica preset e colore (come handleModeChange)
  uint8_t modePreset = loadModePreset((uint8_t)currentMode);
  if (modePreset != 255) {
    currentPreset = modePreset;
  } else {
    currentPreset = 13;
  }

  extern bool rainbowModeEnabled;
  if (modePreset == 100) {
    rainbowModeEnabled = true;
    EEPROM.write(704, 1);
  } else {
    rainbowModeEnabled = false;
    EEPROM.write(704, 0);
  }

  uint8_t modeR, modeG, modeB;
  getColorForPreset(currentPreset, (uint8_t)currentMode, modeR, modeG, modeB);
  currentColor = Color(modeR, modeG, modeB);
  userColor = currentColor;

  EEPROM.write(EEPROM_MODE_ADDR, currentMode);
  EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);
  EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
  EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
  EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);
  EEPROM.commit();

#ifdef EFFECT_MJPEG_STREAM
  if (previousMode == MODE_MJPEG_STREAM && currentMode != MODE_MJPEG_STREAM) {
    setMjpegAudioMute(true);
  } else if (previousMode != MODE_MJPEG_STREAM && currentMode == MODE_MJPEG_STREAM) {
    setMjpegAudioMute(false);
  }
#endif

  // Mostra nome modo (come handleModeChange)
  const char* modeName = getModeSelectorName(targetMode);
  uint16_t modeColor = getModeSelectorColor(targetMode);

  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(modeColor);

  const char* title = "MODE:";
  int textWidth = strlen(title) * 18;
  int xPos = (480 - textWidth) / 2;
  if (xPos < 0) xPos = 0;
  gfx->setCursor(xPos, 210);
  gfx->println(title);

  textWidth = strlen(modeName) * 18;
  xPos = (480 - textWidth) / 2;
  if (xPos < 0) xPos = 0;
  gfx->setCursor(xPos, 240);
  gfx->println(modeName);

  delay(1000);

  gfx->setFont(u8g2_font_inb21_mr);
  gfx->fillScreen(BLACK);
  forceDisplayUpdate();

#ifdef EFFECT_DUAL_DISPLAY
  extern bool dualDisplayEnabled;
  extern bool dualDisplayInitialized;
  extern uint8_t panelRole;
  if (dualDisplayEnabled && dualDisplayInitialized && panelRole == 1) {
    extern void sendSyncPacket();
    sendSyncPacket();
  }
#endif

  Serial.printf("[MODE_SEL] Switch completato a modo %d (%s)\n", targetMode, modeName);
}
