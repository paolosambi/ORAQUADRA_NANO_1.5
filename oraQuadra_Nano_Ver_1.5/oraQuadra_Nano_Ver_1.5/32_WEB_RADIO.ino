// ================== WEB RADIO DISPLAY INTERFACE ==================
// Interfaccia grafica a display per controllare la Web Radio
// Simile a MP3 Player ma per streaming radio

// Define locale per garantire compilazione (il main viene compilato dopo)
#ifndef EFFECT_WEB_RADIO
#define EFFECT_WEB_RADIO
#endif

#ifdef EFFECT_WEB_RADIO

// ================== TEMA MODERNO - PALETTE COLORI ==================
// Sfondo e base - NERO PURO come da immagine riferimento
#define WR_BG_COLOR       0x0000  // Nero puro
#define WR_BG_DARK        0x0000  // Nero puro
#define WR_BG_CARD        0x0841  // Blu molto scuro per cards

// Testo
#define WR_TEXT_COLOR     0xFFFF  // Bianco
#define WR_TEXT_DIM       0xB5B6  // Grigio chiaro
#define WR_TEXT_MUTED     0x7BCF  // Grigio medio

// Accenti - Ciano/Turchese brillante
#define WR_ACCENT_COLOR   0x07FF  // Ciano brillante
#define WR_ACCENT_DARK    0x0575  // Ciano scuro
#define WR_ACCENT_GLOW    0x5FFF  // Ciano chiaro (glow)

// Bottoni - Sfondo scuro con bordi ciano
#define WR_BUTTON_COLOR   0x1082  // Grigio-blu molto scuro
#define WR_BUTTON_HOVER   0x2104  // Grigio scuro
#define WR_BUTTON_ACTIVE  0x0575  // Ciano scuro (attivo)
#define WR_BUTTON_BORDER  0x07FF  // Ciano brillante per bordi
#define WR_BUTTON_SHADOW  0x0000  // Nero per ombra

// VU meter - Gradiente classico verde->giallo->arancione->rosso
#define WR_VU_LOW         0x07E0  // Verde brillante
#define WR_VU_LOW2        0x47E0  // Verde chiaro
#define WR_VU_MID         0xFFE0  // Giallo
#define WR_VU_MID2        0xFD20  // Arancione
#define WR_VU_HIGH        0xF800  // Rosso
#define WR_VU_PEAK        0xF800  // Rosso (peak indicator)
#define WR_VU_BG          0x0000  // Nero per VU background

// Status colors
#define WR_STATUS_ON      0x07E0  // Verde streaming
#define WR_STATUS_OFF     0xF800  // Rosso stopped
#define WR_STATUS_GLOW    0x2FE4  // Glow verde

// Layout FULL SCREEN 480x480 - TEMA MODERNO
// VU meters ai lati: 0-50 sinistra, 430-480 destra
// Area centrale: 55-425 (370px)
#define WR_HEADER_Y       8
#define WR_STATION_Y      60
#define WR_STATUS_Y       120
#define WR_VOLUME_Y       165
#define WR_CONTROLS_Y     215
#define WR_LIST_Y         285
#define WR_LIST_HEIGHT    125
#define WR_EXIT_Y         420

// VU meters - ai lati con design sottile
#define WR_VU_LEFT_X      0
#define WR_VU_RIGHT_X     432
#define WR_VU_WIDTH       48
#define WR_VU_HEIGHT      400
#define WR_VU_Y_START     55
#define WR_VU_SEGMENTS    24

// Area centrale
#define WR_CENTER_X       55
#define WR_CENTER_W       370

// ================== VARIABILI GLOBALI ==================
bool webRadioInitialized = false;
bool webRadioNeedsRedraw = true;
int webRadioScrollOffset = 0;
int webRadioSelectedStation = 0;

// ================== DOUBLE BUFFERING ==================
#define WR_SCREEN_WIDTH  480
#define WR_SCREEN_HEIGHT 480

uint16_t *wrFrameBuffer = nullptr;
OffscreenGFX *wrOffscreenGfx = nullptr;

// Macro per selezionare il GFX object (offscreen se disponibile, altrimenti diretto)
#define WR_GFX (wrOffscreenGfx != nullptr ? (Arduino_GFX*)wrOffscreenGfx : gfx)

// Funzione per trasferire il buffer al display
inline void wrFlushBuffer() {
  if (wrFrameBuffer != nullptr) {
    gfx->draw16bitRGBBitmap(0, 0, wrFrameBuffer, WR_SCREEN_WIDTH, WR_SCREEN_HEIGHT);
  }
}

// VU meter variables
uint8_t wrVuLeft = 0;
uint8_t wrVuRight = 0;
uint8_t wrVuPeakLeft = 0;
uint8_t wrVuPeakRight = 0;
uint32_t wrLastVuUpdate = 0;
int8_t wrPrevActiveLeft = -1;
int8_t wrPrevActiveRight = -1;
int8_t wrPrevPeakLeft = -1;
int8_t wrPrevPeakRight = -1;
bool wrVuBackgroundDrawn = false;

// Dancing bars (equalizer animation near STREAMING)
uint8_t wrDancingBars[4] = {12, 8, 16, 10};  // Altezze delle 4 barre
uint8_t wrDancingTargets[4] = {12, 8, 16, 10};  // Target altezze
uint32_t wrLastDancingUpdate = 0;

// ================== PROTOTIPI FUNZIONI ==================
void initWebRadioUI();
void updateWebRadioUI();
void drawWebRadioUI();
void drawWebRadioHeader();
void drawWebRadioStation();
void drawWebRadioStatus();
void drawWebRadioVolumeBar();
void drawWRModernButton(int bx, int by, int bw, int bh, bool active, bool highlight);
void drawWebRadioControls();
void drawWebRadioStationList();
void drawWebRadioExitButton();
void drawWebRadioVUBackground();
void drawWebRadioVUMeter(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide);
void updateWebRadioVUMeters();
void updateDancingBars();
void drawDancingBars();
bool handleWebRadioTouch(int16_t x, int16_t y);

// ================== INIZIALIZZAZIONE ==================
void initWebRadioUI() {
  if (webRadioInitialized) return;

  Serial.println("[WEBRADIO-UI] Inizializzazione interfaccia...");

  // ===== INIZIALIZZAZIONE DOUBLE BUFFERING =====
  if (wrFrameBuffer == nullptr) {
    wrFrameBuffer = (uint16_t*)heap_caps_malloc(WR_SCREEN_WIDTH * WR_SCREEN_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (wrFrameBuffer != nullptr) {
      Serial.println("[WEBRADIO-UI] FrameBuffer allocato in PSRAM");
    } else {
      Serial.println("[WEBRADIO-UI] ERRORE: Impossibile allocare FrameBuffer!");
    }
  }

  if (wrOffscreenGfx == nullptr && wrFrameBuffer != nullptr) {
    wrOffscreenGfx = new OffscreenGFX(wrFrameBuffer, WR_SCREEN_WIDTH, WR_SCREEN_HEIGHT);
    Serial.println("[WEBRADIO-UI] OffscreenGFX creato");
  }

  webRadioSelectedStation = webRadioCurrentIndex;
  webRadioScrollOffset = 0;
  webRadioNeedsRedraw = true;
  webRadioInitialized = true;

  // Reset VU meters
  wrVuLeft = 0;
  wrVuRight = 0;
  wrVuPeakLeft = 0;
  wrVuPeakRight = 0;
  wrLastVuUpdate = 0;
  wrPrevActiveLeft = -1;
  wrPrevActiveRight = -1;
  wrPrevPeakLeft = -1;
  wrPrevPeakRight = -1;
  wrVuBackgroundDrawn = false;

  Serial.printf("[WEBRADIO-UI] Stazioni disponibili: %d\n", webRadioStationCount);
}

// ================== UPDATE LOOP ==================
void updateWebRadioUI() {
  // Traccia ultimo modo per forzare ridisegno al rientro
  static int lastActiveMode = -1;

  // Forza ridisegno quando si entra nella modalità da un'altra
  if (lastActiveMode != MODE_WEB_RADIO) {
    webRadioNeedsRedraw = true;
    webRadioInitialized = false;  // Re-inizializza
  }
  lastActiveMode = currentMode;

  if (!webRadioInitialized) {
    initWebRadioUI();
    return;
  }

  // Gestione touch
  ts.read();
  if (ts.isTouched) {
    static uint32_t lastTouch = 0;
    uint32_t now = millis();
    if (now - lastTouch > 400) {  // Aumentato da 300 a 400ms
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 479);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 479);

      if (handleWebRadioTouch(x, y)) {
        // Exit richiesto - passa alla modalità successiva nel ciclo
        webRadioInitialized = false;
        handleModeChange();
        return;
      }
      lastTouch = now;
    }
  }

  // Ridisegna se necessario
  if (webRadioNeedsRedraw) {
    drawWebRadioUI();
    webRadioNeedsRedraw = false;
    return;  // Il flush è già stato fatto in drawWebRadioUI()
  }

  // Aggiorna VU meters
  updateWebRadioVUMeters();
  drawWebRadioVUMeter(WR_VU_LEFT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT,
                      wrVuLeft, wrVuPeakLeft, true);
  drawWebRadioVUMeter(WR_VU_RIGHT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT,
                      wrVuRight, wrVuPeakRight, false);

  // Aggiorna dancing bars (equalizer animation)
  updateDancingBars();
  drawDancingBars();

  // ===== FLUSH BUFFER AL DISPLAY =====
  wrFlushBuffer();
}

// ================== DISEGNO UI COMPLETA ==================
void drawWebRadioUI() {
  Arduino_GFX* g = WR_GFX;

  // Sfondo nero puro
  g->fillScreen(WR_BG_COLOR);

  // Linee decorative sottili ciano in alto e in basso
  g->drawFastHLine(0, 2, 480, WR_ACCENT_COLOR);
  g->drawFastHLine(0, 477, 480, WR_ACCENT_COLOR);

  drawWebRadioVUBackground();
  drawWebRadioHeader();
  drawWebRadioStation();
  drawWebRadioStatus();
  drawWebRadioVolumeBar();
  drawWebRadioControls();
  drawWebRadioStationList();
  drawWebRadioExitButton();

  // ===== FLUSH BUFFER AL DISPLAY =====
  wrFlushBuffer();
}

// ================== HEADER MODERNO ==================
void drawWebRadioHeader() {
  Arduino_GFX* g = WR_GFX;
  int centerX = 240;

  // Parentesi onde sinistra "))("
  g->setFont(u8g2_font_helvB24_tr);
  g->setTextColor(WR_ACCENT_COLOR);
  g->setCursor(70, WR_HEADER_Y + 35);
  g->print("))");

  // Titolo principale "WEB RADIO"
  g->setTextColor(WR_ACCENT_COLOR);
  g->setCursor(centerX - 75, WR_HEADER_Y + 35);
  g->print("WEB RADIO");

  // Parentesi onde destra "(("
  g->setCursor(375, WR_HEADER_Y + 35);
  g->print("((");

  // Linea separatrice ciano
  g->drawFastHLine(WR_CENTER_X, WR_HEADER_Y + 48, WR_CENTER_W, WR_ACCENT_COLOR);
}

// ================== NOME STAZIONE - CARD MODERNA ==================
void drawWebRadioStation() {
  Arduino_GFX* g = WR_GFX;
  int boxX = WR_CENTER_X;
  int boxW = WR_CENTER_W;
  int boxH = 52;

  // Card principale con sfondo scuro
  g->fillRoundRect(boxX, WR_STATION_Y, boxW, boxH, 8, WR_BG_CARD);

  // Bordo ciano brillante
  g->drawRoundRect(boxX, WR_STATION_Y, boxW, boxH, 8, WR_ACCENT_COLOR);

  // Icona nota musicale stilizzata
  int iconX = boxX + 18;
  int iconY = WR_STATION_Y + 16;
  g->fillCircle(iconX, iconY + 12, 5, WR_ACCENT_COLOR);
  g->fillCircle(iconX + 10, iconY + 8, 5, WR_ACCENT_COLOR);
  g->fillRect(iconX + 3, iconY - 6, 2, 18, WR_ACCENT_COLOR);
  g->fillRect(iconX + 13, iconY - 10, 2, 18, WR_ACCENT_COLOR);
  g->fillRect(iconX + 3, iconY - 8, 12, 2, WR_ACCENT_COLOR);

  // Nome stazione
  g->setFont(u8g2_font_helvB14_tr);
  g->setTextColor(WR_TEXT_COLOR);

  String stationName = webRadioName;
  if (stationName.length() > 24) {
    stationName = stationName.substring(0, 21) + "...";
  }

  int16_t tw = stationName.length() * 9;
  int textX = boxX + 45 + (boxW - 60 - tw) / 2;
  g->setCursor(textX, WR_STATION_Y + 34);
  g->print(stationName);
}

// ================== STATO - DESIGN MODERNO ==================
void drawWebRadioStatus() {
  Arduino_GFX* g = WR_GFX;
  int centerX = 240;
  int y = WR_STATUS_Y;

  if (webRadioEnabled) {
    // 1. INDICATORE LED PULSANTE (Sinistra)
    // Lo mettiamo a -125 per dare spazio alla parola più lunga
    g->fillCircle(centerX - 125, y + 18, 8, WR_STATUS_ON);
    g->drawCircle(centerX - 125, y + 18, 11, WR_STATUS_GLOW);
    g->drawCircle(centerX - 125, y + 18, 14, WR_ACCENT_DARK);

    // 2. TESTO STREAMING (Centro perfetto)
    g->setFont(u8g2_font_helvB18_tr);
    g->setTextColor(WR_STATUS_ON);

    // "STREAMING" è lunga circa 130 pixel.
    // Per centrarla su 480px: 240 - (130 / 2) = 175.
    // Rispetto a centerX (240) il cursore deve andare a -65 o -70.
    // Proviamo -85 per compensare l'ingombro reale del font.
    g->setCursor(centerX - 85, y + 26);
    g->print("STREAMING");

    // 3. BARRE AUDIO ANIMATE (Destra) - disegnate separatamente in drawDancingBars()
    } else {
    // --- SINISTRA: LED ROSSO ---
    // Lo allontaniamo a -100 dal centro per dare aria alla scritta
    g->fillCircle(centerX - 100, y + 18, 8, WR_STATUS_OFF);
    g->drawCircle(centerX - 100, y + 18, 11, 0x7800);

    // --- CENTRO: SCRITTA STOPPED ---
    g->setFont(u8g2_font_helvB18_tr);
    g->setTextColor(WR_STATUS_OFF);

    // Se prima con -48 era spostata a destra di una lettera,
    // la portiamo a -65 per centrarla perfettamente.
    g->setCursor(centerX - 65, y + 26);
    g->print("STOPPED");

    // --- DESTRA: ICONA QUADRATO ---
    // Lo allontaniamo a +100 dal centro (simmetrico al LED)
    // Così non sembrerà più attaccato alla 'D'
    g->fillRoundRect(centerX + 85, y + 10, 16, 16, 2, WR_STATUS_OFF);
  }
}

// ================== BARRA VOLUME - DESIGN MODERNO ==================
void drawWebRadioVolumeBar() {
  Arduino_GFX* g = WR_GFX;
  int barX = 95;
  int barW = 260;
  int barH = 28;
  int y = WR_VOLUME_Y;

  // Sfondo barra nero con bordo ciano
  g->fillRoundRect(barX, y + 4, barW, barH, 6, WR_BG_DARK);
  g->drawRoundRect(barX, y + 4, barW, barH, 6, WR_ACCENT_COLOR);

  // Barra di riempimento ciano solido
  int fillW = map(webRadioVolume, 0, 100, 0, barW - 6);
  if (fillW > 0) {
    g->fillRoundRect(barX + 3, y + 7, fillW, barH - 6, 4, WR_ACCENT_COLOR);
  }

  // Valore percentuale a destra
  int valX = barX + barW + 12;
  g->setFont(u8g2_font_helvB14_tr);
  g->setTextColor(WR_ACCENT_COLOR);
  String volStr = String(webRadioVolume) + "%";
  g->setCursor(valX, y + 24);
  g->print(volStr);
}

// ================== VU METER BACKGROUND - MODERNO ==================
void drawWebRadioVUBackground() {
  Arduino_GFX* g = WR_GFX;

  // VU sinistro - sfondo nero con bordo ciano
  g->fillRoundRect(WR_VU_LEFT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 4, WR_BG_DARK);
  g->drawRoundRect(WR_VU_LEFT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 4, WR_ACCENT_COLOR);

  // Label "L" - sotto il VU meter
  g->setFont(u8g2_font_helvB14_tr);
  g->setTextColor(WR_ACCENT_COLOR);
  g->setCursor(WR_VU_LEFT_X + 16, WR_VU_Y_START + WR_VU_HEIGHT + 20);
  g->print("L");

  // VU destro - sfondo nero con bordo ciano
  g->fillRoundRect(WR_VU_RIGHT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 4, WR_BG_DARK);
  g->drawRoundRect(WR_VU_RIGHT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 4, WR_ACCENT_COLOR);

  // Label "R" - sotto il VU meter
  g->setCursor(WR_VU_RIGHT_X + 16, WR_VU_Y_START + WR_VU_HEIGHT + 20);
  g->print("R");

  // Reset stato precedente
  wrPrevActiveLeft = -1;
  wrPrevActiveRight = -1;
  wrPrevPeakLeft = -1;
  wrPrevPeakRight = -1;
  wrVuBackgroundDrawn = true;
}

// ================== VU METER DRAWING - MODERNO ==================
void drawWebRadioVUMeter(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide) {
  Arduino_GFX* g = WR_GFX;

  // Calcola dimensioni segmenti con gap
  int segmentH = (h - 30) / WR_VU_SEGMENTS;
  int segmentW = w - 16;
  int segmentX = x + 8;
  int startY = y + h - 15;  // Parte dal basso

  // Calcola quanti segmenti accendere
  int activeSegments = (level * WR_VU_SEGMENTS) / 100;
  int peakSegment = (peak * WR_VU_SEGMENTS) / 100;

  // Ottieni stato precedente
  int8_t prevActive = leftSide ? wrPrevActiveLeft : wrPrevActiveRight;
  int8_t prevPeak = leftSide ? wrPrevPeakLeft : wrPrevPeakRight;

  // Se nulla e' cambiato, esci
  if (activeSegments == prevActive && peakSegment == prevPeak) {
    return;
  }

  // Aggiorna solo i segmenti che cambiano
  int minSeg = (activeSegments < prevActive) ? activeSegments : prevActive;
  int maxSeg = (activeSegments > prevActive) ? activeSegments : prevActive;
  if (minSeg < 0) minSeg = 0;

  for (int i = minSeg; i <= maxSeg && i < WR_VU_SEGMENTS; i++) {
    int segY = startY - (i + 1) * segmentH;
    uint16_t color;
    uint16_t dimColor;

    // Gradiente colore moderno: verde -> ciano -> giallo -> arancione -> rosso
    float ratio = (float)i / WR_VU_SEGMENTS;
    if (ratio < 0.4) {
      color = WR_VU_LOW;       // Verde (0-40%)
      dimColor = 0x0320;
    } else if (ratio < 0.55) {
      color = WR_VU_LOW2;      // Verde-ciano (40-55%)
      dimColor = 0x0220;
    } else if (ratio < 0.70) {
      color = WR_VU_MID;       // Giallo (55-70%)
      dimColor = 0x4200;
    } else if (ratio < 0.85) {
      color = WR_VU_MID2;      // Arancione (70-85%)
      dimColor = 0x4100;
    } else {
      color = WR_VU_HIGH;      // Rosso (85-100%)
      dimColor = 0x4000;
    }

    if (i < activeSegments) {
      // Segmento acceso con highlight
      g->fillRoundRect(segmentX, segY, segmentW, segmentH - 3, 3, color);
      // Highlight superiore per effetto 3D
      g->drawFastHLine(segmentX + 2, segY + 1, segmentW - 4, WR_TEXT_COLOR);
    } else {
      // Segmento spento (visibile ma dim)
      g->fillRoundRect(segmentX, segY, segmentW, segmentH - 3, 3, dimColor);
    }
  }

  // Gestione picco con stile moderno (linea luminosa)
  if (peakSegment != prevPeak && peakSegment > 0 && peakSegment < WR_VU_SEGMENTS) {
    // Cancella vecchio picco
    if (prevPeak > 0 && prevPeak < WR_VU_SEGMENTS) {
      int oldPeakY = startY - (prevPeak + 1) * segmentH;
      float ratio = (float)prevPeak / WR_VU_SEGMENTS;
      uint16_t oldColor;
      if (prevPeak < activeSegments) {
        if (ratio < 0.4) oldColor = WR_VU_LOW;
        else if (ratio < 0.55) oldColor = WR_VU_LOW2;
        else if (ratio < 0.70) oldColor = WR_VU_MID;
        else if (ratio < 0.85) oldColor = WR_VU_MID2;
        else oldColor = WR_VU_HIGH;
      } else {
        if (ratio < 0.4) oldColor = 0x0320;
        else if (ratio < 0.55) oldColor = 0x0220;
        else if (ratio < 0.70) oldColor = 0x4200;
        else if (ratio < 0.85) oldColor = 0x4100;
        else oldColor = 0x4000;
      }
      g->fillRoundRect(segmentX, oldPeakY, segmentW, segmentH - 3, 3, oldColor);
    }

    // Disegna nuovo picco (magenta brillante)
    int newPeakY = startY - (peakSegment + 1) * segmentH;
    g->fillRoundRect(segmentX, newPeakY, segmentW, segmentH - 3, 3, WR_VU_PEAK);
    g->drawRoundRect(segmentX - 1, newPeakY - 1, segmentW + 2, segmentH - 1, 3, WR_TEXT_COLOR);
  }

  // Salva stato corrente
  if (leftSide) {
    wrPrevActiveLeft = activeSegments;
    wrPrevPeakLeft = peakSegment;
  } else {
    wrPrevActiveRight = activeSegments;
    wrPrevPeakRight = peakSegment;
  }
}

// ================== VU METER UPDATE ==================
void updateWebRadioVUMeters() {
  uint32_t now = millis();

  // Aggiorna VU ogni 30ms
  if (now - wrLastVuUpdate < 30) return;
  wrLastVuUpdate = now;

  if (webRadioEnabled) {
    // Simula livelli VU con variazione pseudo-casuale
    static uint8_t targetLeft = 0;
    static uint8_t targetRight = 0;

    // Genera nuovi target periodicamente
    if (random(100) < 30) {
      targetLeft = random(40, 95);
      targetRight = random(40, 95);
    }

    // Smooth attack/decay
    if (wrVuLeft < targetLeft) {
      int newVal = wrVuLeft + 8;
      wrVuLeft = (newVal > targetLeft) ? targetLeft : (uint8_t)newVal;
    } else {
      int newVal = wrVuLeft - 3;
      wrVuLeft = (newVal < targetLeft) ? targetLeft : (uint8_t)newVal;
    }

    if (wrVuRight < targetRight) {
      int newVal = wrVuRight + 8;
      wrVuRight = (newVal > targetRight) ? targetRight : (uint8_t)newVal;
    } else {
      int newVal = wrVuRight - 3;
      wrVuRight = (newVal < targetRight) ? targetRight : (uint8_t)newVal;
    }

    // Aggiorna picchi
    if (wrVuLeft > wrVuPeakLeft) {
      wrVuPeakLeft = wrVuLeft;
    } else if (wrVuPeakLeft > 0) {
      wrVuPeakLeft--;
    }
    if (wrVuRight > wrVuPeakRight) {
      wrVuPeakRight = wrVuRight;
    } else if (wrVuPeakRight > 0) {
      wrVuPeakRight--;
    }
  } else {
    // Decay quando non in streaming
    if (wrVuLeft > 0) wrVuLeft -= 2;
    if (wrVuRight > 0) wrVuRight -= 2;
    if (wrVuPeakLeft > 0) wrVuPeakLeft--;
    if (wrVuPeakRight > 0) wrVuPeakRight--;
  }
}

// ================== DANCING BARS UPDATE ==================
void updateDancingBars() {
  uint32_t now = millis();

  // Aggiorna ogni 100ms per movimento ben visibile
  if (now - wrLastDancingUpdate < 100) return;
  wrLastDancingUpdate = now;

  if (webRadioEnabled) {
    // Ogni barra cambia altezza casualmente ad ogni ciclo
    for (int i = 0; i < 4; i++) {
      // Genera direttamente nuova altezza casuale (molto più dinamico)
      wrDancingBars[i] = random(4, 24);
    }
  } else {
    // Quando non streaming, barre a zero
    for (int i = 0; i < 4; i++) {
      wrDancingBars[i] = 0;
    }
  }
}

// ================== DANCING BARS DRAWING ==================
void drawDancingBars() {
  Arduino_GFX* g = WR_GFX;
  int centerX = 240;
  int y = WR_STATUS_Y;

  // Cancella sempre area delle barre (sfondo nero)
  g->fillRect(centerX + 105, y + 4, 48, 30, WR_BG_COLOR);

  // Disegna barre solo se streaming attivo
  if (!webRadioEnabled) return;

  // Disegna 4 barre con altezze variabili
  for (int i = 0; i < 4; i++) {
    int barH = wrDancingBars[i];
    if (barH < 4) barH = 4;  // Minimo visibile
    int barX = centerX + 108 + (i * 10);
    int barY = y + 20 - barH / 2;

    // Barra verde brillante
    g->fillRoundRect(barX, barY, 6, barH, 2, WR_STATUS_ON);
  }
}

// ================== HELPER BOTTONE MODERNO ==================
void drawWRModernButton(int bx, int by, int bw, int bh, bool active, bool highlight) {
  Arduino_GFX* g = WR_GFX;
  // Sfondo scuro
  uint16_t bgColor = active ? WR_BUTTON_ACTIVE : WR_BUTTON_COLOR;
  g->fillRoundRect(bx, by, bw, bh, 8, bgColor);
  // Bordo ciano
  g->drawRoundRect(bx, by, bw, bh, 8, WR_ACCENT_COLOR);
}

// ================== CONTROLLI - DESIGN MODERNO ==================
void drawWebRadioControls() {
  Arduino_GFX* g = WR_GFX;
  int btnW = 58;
  int btnH = 48;
  int spacing = 12;
  int totalW = 5 * btnW + 4 * spacing;
  int startX = (480 - totalW) / 2;
  int y = WR_CONTROLS_Y;

  // Pulsante PREV (|<) - triangolo punta a SINISTRA
  drawWRModernButton(startX, y, btnW, btnH, false, false);
  int cx = startX + btnW / 2;
  int cy = y + btnH / 2;
  // Linea verticale a sinistra + triangolo che punta a sinistra
  g->fillRect(cx - 12, cy - 10, 3, 20, WR_TEXT_COLOR);
  g->fillTriangle(cx - 6, cy, cx + 10, cy - 10, cx + 10, cy + 10, WR_TEXT_COLOR);

  // Pulsante VOL- (-)
  int x1 = startX + btnW + spacing;
  drawWRModernButton(x1, y, btnW, btnH, false, false);
  cx = x1 + btnW / 2;
  cy = y + btnH / 2;
  // Linea orizzontale (meno)
  g->fillRect(cx - 10, cy - 2, 20, 4, WR_TEXT_COLOR);

  // Pulsante PLAY/STOP (centrale - quadrato)
  int x2 = x1 + btnW + spacing;
  drawWRModernButton(x2, y, btnW, btnH, webRadioEnabled, true);
  cx = x2 + btnW / 2;
  cy = y + btnH / 2;
  // Icona STOP (quadrato) sempre visibile
  g->fillRect(cx - 8, cy - 8, 16, 16, WR_TEXT_COLOR);

  // Pulsante VOL+ (+)
  int x3 = x2 + btnW + spacing;
  drawWRModernButton(x3, y, btnW, btnH, false, false);
  cx = x3 + btnW / 2;
  cy = y + btnH / 2;
  // Croce (più)
  g->fillRect(cx - 10, cy - 2, 20, 4, WR_TEXT_COLOR);
  g->fillRect(cx - 2, cy - 10, 4, 20, WR_TEXT_COLOR);

  // Pulsante NEXT (>|) - triangolo punta a DESTRA
  int x4 = x3 + btnW + spacing;
  drawWRModernButton(x4, y, btnW, btnH, false, false);
  cx = x4 + btnW / 2;
  cy = y + btnH / 2;
  // Triangolo che punta a destra + linea verticale a destra
  g->fillTriangle(cx + 6, cy, cx - 10, cy - 10, cx - 10, cy + 10, WR_TEXT_COLOR);
  g->fillRect(cx + 9, cy - 10, 3, 20, WR_TEXT_COLOR);
}

// ================== LISTA STAZIONI - DESIGN MODERNO ==================
void drawWebRadioStationList() {
  Arduino_GFX* g = WR_GFX;
  int listX = WR_CENTER_X;
  int listW = WR_CENTER_W;

  // Card lista con sfondo scuro e bordo ciano
  g->fillRoundRect(listX, WR_LIST_Y, listW, WR_LIST_HEIGHT, 6, WR_BG_CARD);
  g->drawRoundRect(listX, WR_LIST_Y, listW, WR_LIST_HEIGHT, 6, WR_ACCENT_COLOR);

  // Mostra stazioni visibili
  int itemH = 25;
  int maxVisible = WR_LIST_HEIGHT / itemH;

  for (int i = 0; i < maxVisible && (webRadioScrollOffset + i) < webRadioStationCount; i++) {
    int idx = webRadioScrollOffset + i;
    int itemY = WR_LIST_Y + 4 + i * itemH;

    // Stazione selezionata: sfondo ciano
    if (idx == webRadioCurrentIndex) {
      g->fillRoundRect(listX + 4, itemY, listW - 8, itemH - 2, 4, WR_ACCENT_COLOR);
      g->setFont(u8g2_font_helvR12_tr);
      g->setTextColor(WR_BG_DARK);  // Testo scuro su sfondo ciano
    } else {
      g->setFont(u8g2_font_helvR12_tr);
      g->setTextColor(WR_TEXT_COLOR);  // Testo bianco
    }

    // Nome stazione
    g->setCursor(listX + 12, itemY + 17);
    String stName = webRadioStations[idx].name;
    if (stName.length() > 30) {
      stName = stName.substring(0, 27) + "...";
    }
    g->print(stName);
  }
}

// ================== PULSANTE EXIT - DESIGN MODERNO ==================
void drawWebRadioExitButton() {
  Arduino_GFX* g = WR_GFX;
  int btnW = 100;
  int btnH = 38;
  int x = (480 - btnW) / 2;
  int y = WR_EXIT_Y;

  // Sfondo arancione/rosso
  g->fillRoundRect(x, y, btnW, btnH, 8, 0xFB20);  // Arancione

  // Testo EXIT centrato
  g->setFont(u8g2_font_helvB14_tr);
  g->setTextColor(WR_TEXT_COLOR);
  g->setCursor(x + 30, y + 26);
  g->print("EXIT");
}

// ================== GESTIONE TOUCH ==================
bool handleWebRadioTouch(int16_t x, int16_t y) {
  playTouchSound();

  int btnW = 58;
  int btnH = 48;
  int spacing = 12;
  int totalW = 5 * btnW + 4 * spacing;
  int startX = (480 - totalW) / 2;
  int ctrlY = WR_CONTROLS_Y;

  // ===== CONTROLLI =====
  if (y >= ctrlY && y <= ctrlY + btnH) {
    // PREV
    if (x >= startX && x <= startX + btnW) {
      Serial.println("[WEBRADIO-UI] PREV station");
      if (webRadioCurrentIndex > 0) {
        selectWebRadioStation(webRadioCurrentIndex - 1);
      } else {
        selectWebRadioStation(webRadioStationCount - 1);
      }
      webRadioNeedsRedraw = true;
      return false;
    }

    // VOL-
    int x1 = startX + btnW + spacing;
    if (x >= x1 && x <= x1 + btnW) {
      Serial.println("[WEBRADIO-UI] VOL-");
      if (webRadioVolume > 0) {
        if (webRadioVolume >= 5) webRadioVolume -= 5;  // Decremento di 5% per volta
        else webRadioVolume = 0;
        setWebRadioVolume(webRadioVolume);
      }
      webRadioNeedsRedraw = true;
      return false;
    }

    // PLAY/STOP - debounce lungo per evitare tocchi multipli
    int x2 = x1 + btnW + spacing;
    if (x >= x2 && x <= x2 + btnW) {
      static uint32_t lastPlayStopTouch = 0;
      uint32_t now = millis();
      if (now - lastPlayStopTouch < 700) {
        return false;  // Ignora tocco troppo ravvicinato
      }
      lastPlayStopTouch = now;

      Serial.println("[WEBRADIO-UI] PLAY/STOP");
      if (webRadioEnabled) {
        stopWebRadio();
      } else {
        startWebRadio();
      }
      webRadioNeedsRedraw = true;
      return false;
    }

    // VOL+
    int x3 = x2 + btnW + spacing;
    if (x >= x3 && x <= x3 + btnW) {
      Serial.println("[WEBRADIO-UI] VOL+");
      if (webRadioVolume < 100) {
        webRadioVolume += 5;  // Incremento di 5% per volta
        if (webRadioVolume > 100) webRadioVolume = 100;
        setWebRadioVolume(webRadioVolume);
      }
      webRadioNeedsRedraw = true;
      return false;
    }

    // NEXT
    int x4 = x3 + btnW + spacing;
    if (x >= x4 && x <= x4 + btnW) {
      Serial.println("[WEBRADIO-UI] NEXT station");
      if (webRadioCurrentIndex < webRadioStationCount - 1) {
        selectWebRadioStation(webRadioCurrentIndex + 1);
      } else {
        selectWebRadioStation(0);
      }
      webRadioNeedsRedraw = true;
      return false;
    }
  }

  // ===== BARRA VOLUME (touch diretto) =====
  int volBarX = 95;
  int volBarW = 260;
  int volBarH = 28;
  if (y >= WR_VOLUME_Y && y <= WR_VOLUME_Y + volBarH + 8) {
    if (x >= volBarX && x <= volBarX + volBarW) {
      // Calcola nuovo volume dalla posizione X (0-100)
      int newVol = map(x - volBarX, 0, volBarW, 0, 100);
      newVol = constrain(newVol, 0, 100);
      if (newVol != webRadioVolume) {
        webRadioVolume = newVol;
        setWebRadioVolume(webRadioVolume);
        Serial.printf("[WEBRADIO-UI] Volume impostato a %d%%\n", webRadioVolume);
        webRadioNeedsRedraw = true;
      }
      return false;
    }
  }

  // ===== LISTA STAZIONI =====
  if (y >= WR_LIST_Y && y <= WR_LIST_Y + WR_LIST_HEIGHT) {
    int itemH = 25;
    int maxVisible = WR_LIST_HEIGHT / itemH;
    int clickedItem = (y - WR_LIST_Y) / itemH;
    int stationIdx = webRadioScrollOffset + clickedItem;

    if (stationIdx < webRadioStationCount) {
      Serial.printf("[WEBRADIO-UI] Selezionata stazione %d\n", stationIdx);
      selectWebRadioStation(stationIdx);
      webRadioNeedsRedraw = true;
    }
    return false;
  }

  // ===== SCROLL LISTA =====
  // Scroll UP (tocco sopra la lista)
  if (y >= WR_LIST_Y - 25 && y < WR_LIST_Y && webRadioScrollOffset > 0) {
    webRadioScrollOffset--;
    webRadioNeedsRedraw = true;
    return false;
  }

  // Scroll DOWN (tocco sotto la lista)
  int itemH = 25;
  int maxVisible = WR_LIST_HEIGHT / itemH;
  if (y > WR_LIST_Y + WR_LIST_HEIGHT && y <= WR_LIST_Y + WR_LIST_HEIGHT + 35) {
    if (webRadioScrollOffset + maxVisible < webRadioStationCount) {
      webRadioScrollOffset++;
      webRadioNeedsRedraw = true;
    }
    return false;
  }

  // ===== EXIT =====
  int exitBtnW = 100;
  int exitBtnH = 38;
  int exitX = (480 - exitBtnW) / 2;
  if (y >= WR_EXIT_Y && y <= WR_EXIT_Y + exitBtnH) {
    if (x >= exitX && x <= exitX + exitBtnW) {
      Serial.println("[WEBRADIO-UI] EXIT");
      return true;  // Segnala uscita
    }
  }

  return false;
}

// ================== WEBSERVER ENDPOINTS ==================
void setup_webradio_webserver(AsyncWebServer* server) {
  Serial.println("[WEBRADIO-WEB] Configurazione endpoints web...");

  // API attiva modalità Web Radio sul display
  server->on("/webradio/activate", HTTP_GET, [](AsyncWebServerRequest *request){
    currentMode = MODE_WEB_RADIO;
    webRadioNeedsRedraw = true;
    forceDisplayUpdate();
    request->send(200, "application/json", "{\"success\":true}");
  });

  Serial.println("[WEBRADIO-WEB] Endpoints configurati su /webradio");
}

#endif // EFFECT_WEB_RADIO
