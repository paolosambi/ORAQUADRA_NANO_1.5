// ================== WEB RADIO DISPLAY INTERFACE ==================
// Interfaccia grafica a display per controllare la Web Radio
// Simile a MP3 Player ma per streaming radio

// Define locale per garantire compilazione (il main viene compilato dopo)
#ifndef EFFECT_WEB_RADIO
#define EFFECT_WEB_RADIO
#endif

#ifdef EFFECT_WEB_RADIO

// ================== TEMA MODERNO - PALETTE COLORI ==================
// Sfondo e base
#define WR_BG_COLOR       0x0841  // Blu molto scuro (8,16,8)
#define WR_BG_DARK        0x0000  // Nero puro per contrasto
#define WR_BG_CARD        0x1082  // Grigio-blu scuro per cards

// Testo
#define WR_TEXT_COLOR     0xFFFF  // Bianco
#define WR_TEXT_DIM       0xB5B6  // Grigio chiaro
#define WR_TEXT_MUTED     0x7BCF  // Grigio medio

// Accenti - Ciano/Turchese moderno
#define WR_ACCENT_COLOR   0x07FF  // Ciano brillante
#define WR_ACCENT_DARK    0x0575  // Ciano scuro
#define WR_ACCENT_GLOW    0x5FFF  // Ciano chiaro (glow)

// Bottoni
#define WR_BUTTON_COLOR   0x2124  // Grigio scuro
#define WR_BUTTON_HOVER   0x3186  // Grigio medio
#define WR_BUTTON_ACTIVE  0x0575  // Ciano scuro (attivo)
#define WR_BUTTON_BORDER  0x4A69  // Grigio bordo
#define WR_BUTTON_SHADOW  0x1082  // Ombra bottone

// VU meter - Gradiente moderno
#define WR_VU_LOW         0x07E0  // Verde brillante
#define WR_VU_LOW2        0x2FE0  // Verde-ciano
#define WR_VU_MID         0xFFE0  // Giallo
#define WR_VU_MID2        0xFD20  // Arancione
#define WR_VU_HIGH        0xF800  // Rosso
#define WR_VU_PEAK        0xF81F  // Magenta (peak indicator)
#define WR_VU_BG          0x18C3  // Grigio-blu VU background

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
bool handleWebRadioTouch(int16_t x, int16_t y);

// ================== INIZIALIZZAZIONE ==================
void initWebRadioUI() {
  if (webRadioInitialized) return;

  Serial.println("[WEBRADIO-UI] Inizializzazione interfaccia...");

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
  }

  // Aggiorna VU meters
  updateWebRadioVUMeters();
  drawWebRadioVUMeter(WR_VU_LEFT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT,
                      wrVuLeft, wrVuPeakLeft, true);
  drawWebRadioVUMeter(WR_VU_RIGHT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT,
                      wrVuRight, wrVuPeakRight, false);
}

// ================== DISEGNO UI COMPLETA ==================
void drawWebRadioUI() {
  // Sfondo con gradiente simulato (bande orizzontali)
  for (int y = 0; y < 480; y += 4) {
    uint16_t shade = (y < 240) ? WR_BG_COLOR : WR_BG_DARK;
    gfx->fillRect(0, y, 480, 4, shade);
  }

  // Linee decorative sottili in alto e in basso
  gfx->drawFastHLine(0, 2, 480, WR_ACCENT_DARK);
  gfx->drawFastHLine(0, 477, 480, WR_ACCENT_DARK);

  drawWebRadioVUBackground();
  drawWebRadioHeader();
  drawWebRadioStation();
  drawWebRadioStatus();
  drawWebRadioVolumeBar();
  drawWebRadioControls();
  drawWebRadioStationList();
  drawWebRadioExitButton();
}

// ================== HEADER MODERNO ==================
void drawWebRadioHeader() {
  int centerX = 240;

  // Icona radio stilizzata (onde)
  for (int i = 0; i < 3; i++) {
    int r = 12 + i * 8;
    gfx->drawCircle(85, WR_HEADER_Y + 25, r, WR_ACCENT_DARK);
    // Solo arco destro (simulato con punti)
    for (int a = -45; a <= 45; a += 5) {
      float rad = a * 3.14159 / 180.0;
      int px = 85 + cos(rad) * r;
      int py = WR_HEADER_Y + 25 + sin(rad) * r;
      gfx->drawPixel(px, py, WR_ACCENT_COLOR);
    }
  }

  // Titolo con effetto glow
  gfx->setFont(u8g2_font_helvB24_tr);

  // Glow effect (testo sfumato dietro)
  gfx->setTextColor(WR_ACCENT_DARK);
  gfx->setCursor(centerX - 75 + 1, WR_HEADER_Y + 36);
  gfx->print("WEB RADIO");

  // Testo principale
  gfx->setTextColor(WR_ACCENT_COLOR);
  gfx->setCursor(centerX - 75, WR_HEADER_Y + 35);
  gfx->print("WEB RADIO");

  // Icona radio destra (simmetrica)
  for (int i = 0; i < 3; i++) {
    int r = 12 + i * 8;
    for (int a = 135; a <= 225; a += 5) {
      float rad = a * 3.14159 / 180.0;
      int px = 395 + cos(rad) * r;
      int py = WR_HEADER_Y + 25 + sin(rad) * r;
      gfx->drawPixel(px, py, WR_ACCENT_COLOR);
    }
  }

  // Linea separatrice elegante con gradiente
  for (int i = 0; i < 3; i++) {
    uint16_t lineColor = (i == 1) ? WR_ACCENT_COLOR : WR_ACCENT_DARK;
    gfx->drawFastHLine(WR_CENTER_X, WR_HEADER_Y + 48 + i, WR_CENTER_W, lineColor);
  }
}

// ================== NOME STAZIONE - CARD MODERNA ==================
void drawWebRadioStation() {
  int boxX = WR_CENTER_X;
  int boxW = WR_CENTER_W;
  int boxH = 52;

  // Ombra della card
  gfx->fillRoundRect(boxX + 3, WR_STATION_Y + 3, boxW, boxH, 12, WR_BUTTON_SHADOW);

  // Card principale con bordo ciano
  gfx->fillRoundRect(boxX, WR_STATION_Y, boxW, boxH, 12, WR_BG_CARD);

  // Bordo con effetto glow
  gfx->drawRoundRect(boxX, WR_STATION_Y, boxW, boxH, 12, WR_ACCENT_DARK);
  gfx->drawRoundRect(boxX + 1, WR_STATION_Y + 1, boxW - 2, boxH - 2, 11, WR_ACCENT_COLOR);

  // Icona nota musicale stilizzata
  int iconX = boxX + 15;
  int iconY = WR_STATION_Y + 18;
  gfx->fillCircle(iconX, iconY + 10, 6, WR_ACCENT_COLOR);
  gfx->fillCircle(iconX + 12, iconY + 6, 6, WR_ACCENT_COLOR);
  gfx->fillRect(iconX + 4, iconY - 8, 3, 18, WR_ACCENT_COLOR);
  gfx->fillRect(iconX + 16, iconY - 12, 3, 18, WR_ACCENT_COLOR);
  gfx->fillRect(iconX + 4, iconY - 10, 15, 3, WR_ACCENT_COLOR);

  // Nome stazione
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WR_TEXT_COLOR);

  String stationName = webRadioName;
  if (stationName.length() > 24) {
    stationName = stationName.substring(0, 21) + "...";
  }

  int16_t tw = stationName.length() * 9;
  int textX = boxX + 45 + (boxW - 60 - tw) / 2;
  gfx->setCursor(textX, WR_STATION_Y + 34);
  gfx->print(stationName);
}

// ================== STATO - DESIGN MODERNO ==================
void drawWebRadioStatus() {
  int centerX = 240;
  int y = WR_STATUS_Y;

  if (webRadioEnabled) {
    // 1. INDICATORE LED PULSANTE (Sinistra)
    // Lo mettiamo a -125 per dare spazio alla parola più lunga
    gfx->fillCircle(centerX - 125, y + 18, 8, WR_STATUS_ON);
    gfx->drawCircle(centerX - 125, y + 18, 11, WR_STATUS_GLOW);
    gfx->drawCircle(centerX - 125, y + 18, 14, WR_ACCENT_DARK);

    // 2. TESTO STREAMING (Centro perfetto)
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setTextColor(WR_STATUS_ON);
    
    // "STREAMING" è lunga circa 130 pixel.
    // Per centrarla su 480px: 240 - (130 / 2) = 175.
    // Rispetto a centerX (240) il cursore deve andare a -65 o -70.
    // Proviamo -85 per compensare l'ingombro reale del font.
    gfx->setCursor(centerX - 85, y + 26); 
    gfx->print("STREAMING");

    // 3. BARRE AUDIO ANIMATE (Destra)
    // Le spostiamo a +110 per non toccare la 'G' finale
    for (int i = 0; i < 3; i++) {
      int barH = 8 + (i % 2) * 8;
      int barX = centerX + 110 + (i * 10); // Distanziate tra loro
      gfx->fillRoundRect(barX, y + 22 - barH / 2, 5, barH, 2, WR_STATUS_ON);
    }
    // Indicatore LED spento
    } else {
    // --- SINISTRA: LED ROSSO ---
    // Lo allontaniamo a -100 dal centro per dare aria alla scritta
    gfx->fillCircle(centerX - 100, y + 18, 8, WR_STATUS_OFF);
    gfx->drawCircle(centerX - 100, y + 18, 11, 0x7800);

    // --- CENTRO: SCRITTA STOPPED ---
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setTextColor(WR_STATUS_OFF);
    
    // Se prima con -48 era spostata a destra di una lettera, 
    // la portiamo a -65 per centrarla perfettamente.
    gfx->setCursor(centerX - 65, y + 26); 
    gfx->print("STOPPED");

    // --- DESTRA: ICONA QUADRATO ---
    // Lo allontaniamo a +100 dal centro (simmetrico al LED)
    // Così non sembrerà più attaccato alla 'D'
    gfx->fillRoundRect(centerX + 85, y + 10, 16, 16, 2, WR_STATUS_OFF);
  }
}

// ================== BARRA VOLUME - DESIGN MODERNO ==================
void drawWebRadioVolumeBar() {
  int barX = 95;
  int barW = 290;
  int barH = 36;
  int y = WR_VOLUME_Y;

  // Icona speaker moderna
  int spkX = WR_CENTER_X + 8;
  int spkY = y + 18;
  // Corpo speaker
  gfx->fillRect(spkX - 6, spkY - 4, 8, 8, WR_ACCENT_COLOR);
  gfx->fillTriangle(spkX + 2, spkY - 8, spkX + 2, spkY + 8, spkX + 12, spkY, WR_ACCENT_COLOR);

  // Onde sonore (in base al volume) - semicerchi a destra
  if (webRadioVolume > 5) {
    for (int a = -40; a <= 40; a += 8) {
      float rad = a * 3.14159 / 180.0;
      int px = spkX + 16 + cos(rad) * 8;
      int py = spkY + sin(rad) * 8;
      gfx->drawPixel(px, py, WR_ACCENT_DARK);
      gfx->drawPixel(px + 1, py, WR_ACCENT_DARK);
    }
  }
  if (webRadioVolume > 12) {
    for (int a = -40; a <= 40; a += 6) {
      float rad = a * 3.14159 / 180.0;
      int px = spkX + 16 + cos(rad) * 14;
      int py = spkY + sin(rad) * 14;
      gfx->drawPixel(px, py, WR_ACCENT_COLOR);
      gfx->drawPixel(px + 1, py, WR_ACCENT_COLOR);
    }
  }

  // Sfondo barra con effetto scavato
  gfx->fillRoundRect(barX, y + 2, barW, barH - 4, 10, WR_BG_DARK);
  gfx->drawRoundRect(barX, y + 2, barW, barH - 4, 10, WR_BUTTON_BORDER);

  // Barra di riempimento con gradiente simulato
  int fillW = map(webRadioVolume, 0, 21, 0, barW - 8);
  if (fillW > 0) {
    // Gradiente: ciano scuro -> ciano brillante
    int segments = fillW / 4;
    for (int i = 0; i < segments; i++) {
      int segX = barX + 4 + i * 4;
      uint16_t segColor = (i < segments / 2) ? WR_ACCENT_DARK : WR_ACCENT_COLOR;
      gfx->fillRoundRect(segX, y + 6, 3, barH - 12, 1, segColor);
    }

    // Highlight superiore
    gfx->drawFastHLine(barX + 4, y + 6, fillW, WR_ACCENT_GLOW);
  }

  // Indicatore posizione (knob)
  int knobX = barX + 4 + fillW;
  if (knobX > barX + barW - 10) knobX = barX + barW - 10;
  gfx->fillCircle(knobX, y + barH / 2, 8, WR_ACCENT_COLOR);
  gfx->fillCircle(knobX, y + barH / 2, 4, WR_TEXT_COLOR);

  // Valore numerico in box
  int valX = barX + barW + 8;
  gfx->fillRoundRect(valX, y + 4, 36, barH - 8, 6, WR_BG_CARD);
  gfx->drawRoundRect(valX, y + 4, 36, barH - 8, 6, WR_ACCENT_DARK);

  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);
  String volStr = String(webRadioVolume);
  int textOffset = (webRadioVolume < 10) ? 12 : 6;
  gfx->setCursor(valX + textOffset, y + 26);
  gfx->print(volStr);
}

// ================== VU METER BACKGROUND - MODERNO ==================
void drawWebRadioVUBackground() {
  // VU sinistro - design moderno con bordo doppio
  gfx->fillRoundRect(WR_VU_LEFT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 8, WR_BG_DARK);
  gfx->drawRoundRect(WR_VU_LEFT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 8, WR_BUTTON_BORDER);
  gfx->drawRoundRect(WR_VU_LEFT_X + 4, WR_VU_Y_START + 2, WR_VU_WIDTH - 8, WR_VU_HEIGHT - 4, 6, WR_ACCENT_DARK);

  // Label "L"
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);
  gfx->setCursor(WR_VU_LEFT_X + 18, WR_VU_Y_START + WR_VU_HEIGHT + 18);
  gfx->print("L");

  // VU destro
  gfx->fillRoundRect(WR_VU_RIGHT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 8, WR_BG_DARK);
  gfx->drawRoundRect(WR_VU_RIGHT_X + 2, WR_VU_Y_START, WR_VU_WIDTH - 4, WR_VU_HEIGHT, 8, WR_BUTTON_BORDER);
  gfx->drawRoundRect(WR_VU_RIGHT_X + 4, WR_VU_Y_START + 2, WR_VU_WIDTH - 8, WR_VU_HEIGHT - 4, 6, WR_ACCENT_DARK);

  // Label "R"
  gfx->setCursor(WR_VU_RIGHT_X + 18, WR_VU_Y_START + WR_VU_HEIGHT + 18);
  gfx->print("R");

  // Reset stato precedente
  wrPrevActiveLeft = -1;
  wrPrevActiveRight = -1;
  wrPrevPeakLeft = -1;
  wrPrevPeakRight = -1;
  wrVuBackgroundDrawn = true;
}

// ================== VU METER DRAWING - MODERNO ==================
void drawWebRadioVUMeter(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide) {
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
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 3, 3, color);
      // Highlight superiore per effetto 3D
      gfx->drawFastHLine(segmentX + 2, segY + 1, segmentW - 4, WR_TEXT_COLOR);
    } else {
      // Segmento spento (visibile ma dim)
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 3, 3, dimColor);
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
      gfx->fillRoundRect(segmentX, oldPeakY, segmentW, segmentH - 3, 3, oldColor);
    }

    // Disegna nuovo picco (magenta brillante)
    int newPeakY = startY - (peakSegment + 1) * segmentH;
    gfx->fillRoundRect(segmentX, newPeakY, segmentW, segmentH - 3, 3, WR_VU_PEAK);
    gfx->drawRoundRect(segmentX - 1, newPeakY - 1, segmentW + 2, segmentH - 1, 3, WR_TEXT_COLOR);
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

// ================== HELPER BOTTONE MODERNO ==================
void drawWRModernButton(int bx, int by, int bw, int bh, bool active, bool highlight) {
  // Ombra
  gfx->fillRoundRect(bx + 2, by + 2, bw, bh, 12, WR_BUTTON_SHADOW);
  // Sfondo
  uint16_t bgColor = active ? WR_BUTTON_ACTIVE : (highlight ? WR_BUTTON_HOVER : WR_BUTTON_COLOR);
  gfx->fillRoundRect(bx, by, bw, bh, 12, bgColor);
  // Bordo esterno
  gfx->drawRoundRect(bx, by, bw, bh, 12, WR_BUTTON_BORDER);
  // Highlight interno superiore (effetto 3D)
  if (active) {
    gfx->drawRoundRect(bx + 2, by + 2, bw - 4, bh / 2, 10, WR_ACCENT_GLOW);
  } else {
    gfx->drawFastHLine(bx + 8, by + 4, bw - 16, WR_BUTTON_HOVER);
  }
}

// ================== CONTROLLI - DESIGN MODERNO ==================
void drawWebRadioControls() {
  int btnW = 62;
  int btnH = 52;
  int spacing = 10;
  int totalW = 5 * btnW + 4 * spacing;
  int startX = (480 - totalW) / 2;
  int y = WR_CONTROLS_Y;

  // Pulsante PREV (<<)
  drawWRModernButton(startX, y, btnW, btnH, false, false);
  // Icona freccia doppia sinistra
  int cx = startX + btnW / 2;
  int cy = y + btnH / 2;
  gfx->fillTriangle(cx - 5, cy, cx + 5, cy - 10, cx + 5, cy + 10, WR_ACCENT_COLOR);
  gfx->fillTriangle(cx - 15, cy, cx - 5, cy - 10, cx - 5, cy + 10, WR_ACCENT_COLOR);

  // Pulsante VOL- (speaker con -)
  int x1 = startX + btnW + spacing;
  drawWRModernButton(x1, y, btnW, btnH, false, false);
  cx = x1 + btnW / 2;
  cy = y + btnH / 2;
  // Icona volume meno
  gfx->fillRect(cx - 12, cy - 2, 10, 4, WR_ACCENT_COLOR);
  gfx->fillTriangle(cx + 2, cy, cx + 12, cy - 8, cx + 12, cy + 8, WR_ACCENT_COLOR);

  // Pulsante PLAY/STOP (centrale - più grande)
  int x2 = x1 + btnW + spacing;
  int playBtnW = btnW + 4;
  drawWRModernButton(x2 - 2, y - 3, playBtnW, btnH + 6, webRadioEnabled, true);
  cx = x2 + btnW / 2;
  cy = y + btnH / 2;
  if (webRadioEnabled) {
    // Icona STOP (quadrato)
    gfx->fillRoundRect(cx - 10, cy - 10, 20, 20, 3, WR_TEXT_COLOR);
  } else {
    // Icona PLAY (triangolo)
    gfx->fillTriangle(cx - 8, cy - 12, cx - 8, cy + 12, cx + 12, cy, WR_TEXT_COLOR);
  }

  // Pulsante VOL+ (speaker con +)
  int x3 = x2 + btnW + spacing;
  drawWRModernButton(x3, y, btnW, btnH, false, false);
  cx = x3 + btnW / 2;
  cy = y + btnH / 2;
  // Icona volume più
  gfx->fillRect(cx - 14, cy - 2, 10, 4, WR_ACCENT_COLOR);
  gfx->fillRect(cx - 11, cy - 5, 4, 10, WR_ACCENT_COLOR);
  gfx->fillTriangle(cx, cy, cx + 10, cy - 8, cx + 10, cy + 8, WR_ACCENT_COLOR);

  // Pulsante NEXT (>>)
  int x4 = x3 + btnW + spacing;
  drawWRModernButton(x4, y, btnW, btnH, false, false);
  cx = x4 + btnW / 2;
  cy = y + btnH / 2;
  // Icona freccia doppia destra
  gfx->fillTriangle(cx + 5, cy, cx - 5, cy - 10, cx - 5, cy + 10, WR_ACCENT_COLOR);
  gfx->fillTriangle(cx + 15, cy, cx + 5, cy - 10, cx + 5, cy + 10, WR_ACCENT_COLOR);
}

// ================== LISTA STAZIONI - DESIGN MODERNO ==================
void drawWebRadioStationList() {
  int listX = WR_CENTER_X;
  int listW = WR_CENTER_W;

  // Header della lista con icona
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);

  // Icona antenna
  gfx->fillRect(listX + 5, WR_LIST_Y - 18, 2, 12, WR_ACCENT_COLOR);
  gfx->fillCircle(listX + 6, WR_LIST_Y - 20, 3, WR_ACCENT_COLOR);
  gfx->drawLine(listX + 6, WR_LIST_Y - 15, listX + 2, WR_LIST_Y - 8, WR_ACCENT_DARK);
  gfx->drawLine(listX + 6, WR_LIST_Y - 15, listX + 10, WR_LIST_Y - 8, WR_ACCENT_DARK);

  gfx->setCursor(listX + 18, WR_LIST_Y - 6);
  gfx->print("STATIONS");

  // Contatore stazioni
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(WR_TEXT_DIM);
  String countStr = "(" + String(webRadioStationCount) + ")";
  gfx->setCursor(listX + 95, WR_LIST_Y - 6);
  gfx->print(countStr);

  // Card lista con ombra
  gfx->fillRoundRect(listX + 2, WR_LIST_Y + 2, listW, WR_LIST_HEIGHT, 10, WR_BUTTON_SHADOW);
  gfx->fillRoundRect(listX, WR_LIST_Y, listW, WR_LIST_HEIGHT, 10, WR_BG_CARD);
  gfx->drawRoundRect(listX, WR_LIST_Y, listW, WR_LIST_HEIGHT, 10, WR_BUTTON_BORDER);

  // Mostra stazioni visibili
  int itemH = 25;
  int maxVisible = WR_LIST_HEIGHT / itemH;
  int itemPadding = 4;

  for (int i = 0; i < maxVisible && (webRadioScrollOffset + i) < webRadioStationCount; i++) {
    int idx = webRadioScrollOffset + i;
    int itemY = WR_LIST_Y + 4 + i * itemH;

    // Evidenzia stazione corrente con bordo ciano
    if (idx == webRadioCurrentIndex) {
      gfx->fillRoundRect(listX + 4, itemY, listW - 8, itemH - 2, 6, WR_ACCENT_DARK);
      gfx->drawRoundRect(listX + 4, itemY, listW - 8, itemH - 2, 6, WR_ACCENT_COLOR);

      // Indicatore play attivo
      int triX = listX + 12;
      int triY = itemY + itemH / 2;
      gfx->fillTriangle(triX, triY - 5, triX, triY + 5, triX + 8, triY, WR_ACCENT_GLOW);
    } else {
      // Linea separatrice sottile
      if (i > 0) {
        gfx->drawFastHLine(listX + 20, itemY, listW - 40, WR_BUTTON_SHADOW);
      }
    }

    // Numero stazione
    gfx->setFont(u8g2_font_helvR10_tr);
    uint16_t numColor = (idx == webRadioCurrentIndex) ? WR_ACCENT_GLOW : WR_TEXT_MUTED;
    gfx->setTextColor(numColor);
    int numX = (idx == webRadioCurrentIndex) ? listX + 24 : listX + 12;
    gfx->setCursor(numX, itemY + 16);
    gfx->print(String(idx + 1) + ".");

    // Nome stazione
    gfx->setFont(u8g2_font_helvR12_tr);
    uint16_t nameColor = (idx == webRadioCurrentIndex) ? WR_TEXT_COLOR : WR_TEXT_DIM;
    gfx->setTextColor(nameColor);
    gfx->setCursor(numX + 28, itemY + 17);

    String stName = webRadioStations[idx].name;
    if (stName.length() > 28) {
      stName = stName.substring(0, 25) + "...";
    }
    gfx->print(stName);
  }

  // Indicatori scroll moderni
  int scrollX = listX + listW - 22;

  if (webRadioScrollOffset > 0) {
    // Freccia su
    gfx->fillTriangle(scrollX, WR_LIST_Y + 18, scrollX - 8, WR_LIST_Y + 28, scrollX + 8, WR_LIST_Y + 28, WR_ACCENT_COLOR);
  }

  if (webRadioScrollOffset + maxVisible < webRadioStationCount) {
    // Freccia giu
    int downY = WR_LIST_Y + WR_LIST_HEIGHT - 18;
    gfx->fillTriangle(scrollX, downY + 10, scrollX - 8, downY, scrollX + 8, downY, WR_ACCENT_COLOR);
  }

  // Barra di scorrimento laterale
  if (webRadioStationCount > maxVisible) {
    int scrollBarH = WR_LIST_HEIGHT - 20;
    int thumbH = (scrollBarH * maxVisible) / webRadioStationCount;
    if (thumbH < 20) thumbH = 20;
    int thumbY = WR_LIST_Y + 10 + (scrollBarH - thumbH) * webRadioScrollOffset / (webRadioStationCount - maxVisible);

    // Track
    gfx->fillRoundRect(scrollX + 14, WR_LIST_Y + 10, 4, scrollBarH, 2, WR_BUTTON_SHADOW);
    // Thumb
    gfx->fillRoundRect(scrollX + 14, thumbY, 4, thumbH, 2, WR_ACCENT_COLOR);
  }
}

// ================== PULSANTE EXIT - DESIGN MODERNO ==================
void drawWebRadioExitButton() {
  int btnW = 140;
  int btnH = 46;
  int x = (480 - btnW) / 2;
  int y = WR_EXIT_Y;

  // Ombra profonda
  gfx->fillRoundRect(x + 3, y + 3, btnW, btnH, 14, 0x4000);

  // Sfondo gradiente rosso (simulato)
  gfx->fillRoundRect(x, y, btnW, btnH, 14, 0xC000);
  gfx->fillRoundRect(x + 4, y + 4, btnW - 8, btnH - 16, 10, 0xF800);

  // Bordo
  gfx->drawRoundRect(x, y, btnW, btnH, 14, 0x7800);

  // Highlight superiore
  gfx->drawFastHLine(x + 15, y + 4, btnW - 30, 0xFC00);

  // Icona X
  int iconX = x + 25;
  int iconY = y + btnH / 2;
  gfx->drawLine(iconX - 6, iconY - 6, iconX + 6, iconY + 6, WR_TEXT_COLOR);
  gfx->drawLine(iconX - 6, iconY + 6, iconX + 6, iconY - 6, WR_TEXT_COLOR);
  gfx->drawLine(iconX - 5, iconY - 6, iconX + 7, iconY + 6, WR_TEXT_COLOR);
  gfx->drawLine(iconX - 5, iconY + 6, iconX + 7, iconY - 6, WR_TEXT_COLOR);

  // Testo EXIT
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(WR_TEXT_COLOR);
  gfx->setCursor(x + 48, y + 32);
  gfx->print("EXIT");
}

// ================== GESTIONE TOUCH ==================
bool handleWebRadioTouch(int16_t x, int16_t y) {
  playTouchSound();

  int btnW = 62;
  int btnH = 52;
  int spacing = 10;
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
        webRadioVolume--;
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
      if (webRadioVolume < 21) {
        webRadioVolume++;
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
  int volBarW = 290;
  int volBarH = 36;
  if (y >= WR_VOLUME_Y && y <= WR_VOLUME_Y + volBarH) {
    if (x >= volBarX && x <= volBarX + volBarW) {
      // Calcola nuovo volume dalla posizione X
      int newVol = map(x - volBarX, 0, volBarW, 0, 21);
      newVol = constrain(newVol, 0, 21);
      if (newVol != webRadioVolume) {
        webRadioVolume = newVol;
        setWebRadioVolume(webRadioVolume);
        Serial.printf("[WEBRADIO-UI] Volume impostato a %d\n", webRadioVolume);
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
  int exitBtnW = 140;
  int exitBtnH = 46;
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
