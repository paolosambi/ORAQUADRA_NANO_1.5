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
#define WR_BG_COLOR 0x0841  // Blu molto scuro (8,16,8)
#define WR_BG_DARK 0x0000   // Nero puro per contrasto
#define WR_BG_CARD 0x1082   // Grigio-blu scuro per cards

// Testo
#define WR_TEXT_COLOR 0xFFFF  // Bianco
#define WR_TEXT_DIM 0xB5B6    // Grigio chiaro
#define WR_TEXT_MUTED 0x7BCF  // Grigio medio

// Accenti - Ciano/Turchese moderno
#define WR_ACCENT_COLOR 0x07FF  // Ciano brillante
#define WR_ACCENT_DARK 0x0575   // Ciano scuro
#define WR_ACCENT_GLOW 0x5FFF   // Ciano chiaro (glow)

// Bottoni
#define WR_BUTTON_COLOR 0x2124   // Grigio scuro
#define WR_BUTTON_HOVER 0x3186   // Grigio medio
#define WR_BUTTON_ACTIVE 0x0575  // Ciano scuro (attivo)
#define WR_BUTTON_BORDER 0x4A69  // Grigio bordo
#define WR_BUTTON_SHADOW 0x1082  // Ombra bottone

// VU meter - Gradiente moderno
#define WR_VU_LOW 0x07E0   // Verde brillante
#define WR_VU_LOW2 0x2FE0  // Verde-ciano
#define WR_VU_MID 0xFFE0   // Giallo
#define WR_VU_MID2 0xFD20  // Arancione
#define WR_VU_HIGH 0xF800  // Rosso
#define WR_VU_PEAK 0xF81F  // Magenta (peak indicator)
#define WR_VU_BG 0x18C3    // Grigio-blu VU background

// Status colors
#define WR_STATUS_ON 0x07E0    // Verde streaming
#define WR_STATUS_OFF 0xF800   // Rosso stopped
#define WR_STATUS_GLOW 0x2FE4  // Glow verde

// Layout FULL SCREEN 480x480 - TEMA MODERNO
// VU meters ai lati: 0-50 sinistra, 430-480 destra
// Area centrale: 55-425 (370px)
#define WR_HEADER_Y 8
#define WR_STATION_Y 60
#define WR_STATUS_Y 120
#define WR_VOLUME_Y 165
#define WR_CONTROLS_Y 215
#define WR_LIST_Y 285
#define WR_LIST_HEIGHT 125
// VU meters - ai lati con design sottile
#define WR_VU_LEFT_X 0
#define WR_VU_RIGHT_X 432
#define WR_VU_WIDTH 48
#define WR_VU_HEIGHT 400
#define WR_VU_Y_START 55
#define WR_VU_SEGMENTS 24

// Area centrale
#define WR_CENTER_X 55
#define WR_CENTER_W 370

// ================== VARIABILI GLOBALI ==================
bool webRadioInitialized = false;
bool webRadioNeedsRedraw = true;
int webRadioScrollOffset = 0;
int webRadioSelectedStation = 0;
bool webRadioVolumeNeedsRedraw = false;

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

  // 1. GESTIONE TOUCH
  ts.read();
  if (ts.isTouched) {
    static uint32_t lastTouch = 0;
    uint32_t now = millis();
    if (now - lastTouch > 400) {  
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 479);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 479);

      if (handleWebRadioTouch(x, y)) {
        // Exit richiesto
        webRadioInitialized = false;
        saveWebRadioSettings();
        handleModeChange();
        return;
      }
      lastTouch = now;
    }
  }

  // 2. RIDISEGNO TOTALE (Solo se necessario, es. cambio stazione o avvio)
  if (webRadioNeedsRedraw) {
    drawWebRadioUI();
    webRadioNeedsRedraw = false;
    webRadioVolumeNeedsRedraw = false; // Se ridisegno tutto, la barra è già inclusa
  } 
  // 3. RIDISEGNO SELETTIVO VOLUME (Evita il glitch dello sfondo)
  else if (webRadioVolumeNeedsRedraw) {
    drawWebRadioVolumeBar();
    webRadioVolumeNeedsRedraw = false;
  }

  // 4. AGGIORNAMENTO ANIMAZIONI STATUS (Streaming / Stopped pulse)
  // Eseguito ogni 50ms per garantire fluidità (20 fps) senza bloccare il sistema
  static uint32_t lastAnimUpdate = 0;
  if (millis() - lastAnimUpdate > 50) {
    drawWebRadioStatus();
    lastAnimUpdate = millis();
  }

  // 5. AGGIORNA VU METERS
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
  int centerX = 240;                       // Centro esatto dello schermo (480 / 2)
  int textWidth = 174;                     // Larghezza della scritta calcolata sulle tue proporzioni (2,6 cm)
  int startX = centerX - (textWidth / 2);  // Punto di inizio per la centratura (153)

  // Icona radio stilizzata (onde a sinistra)
  for (int i = 0; i < 3; i++) {
    int r = 12 + i * 8;
    gfx->drawCircle(85, WR_HEADER_Y + 25, r, WR_ACCENT_DARK);
    for (int a = -45; a <= 45; a += 5) {
      float rad = a * 3.14159 / 180.0;
      int px = 85 + cos(rad) * r;
      int py = WR_HEADER_Y + 25 + sin(rad) * r;
      gfx->drawPixel(px, py, WR_ACCENT_COLOR);
    }
  }

  // Impostazione Font
  gfx->setFont(u8g2_font_helvB24_tr);

  // Effetto Glow (ombra/bagliore) centrato
  gfx->setTextColor(WR_ACCENT_DARK);
  gfx->setCursor(startX + 1, WR_HEADER_Y + 36);
  gfx->print("WEB RADIO");

  // Testo principale centrato
  gfx->setTextColor(WR_ACCENT_COLOR);
  gfx->setCursor(startX, WR_HEADER_Y + 35);
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

  // Linea separatrice sotto l'intestazione
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

  // Ombra e Card principale
  gfx->fillRoundRect(boxX + 3, WR_STATION_Y + 3, boxW, boxH, 12, WR_BUTTON_SHADOW);
  gfx->fillRoundRect(boxX, WR_STATION_Y, boxW, boxH, 12, WR_BG_CARD);

  // Bordo con effetto glow
  gfx->drawRoundRect(boxX, WR_STATION_Y, boxW, boxH, 12, WR_ACCENT_DARK);
  gfx->drawRoundRect(boxX + 1, WR_STATION_Y + 1, boxW - 2, boxH - 2, 11, WR_ACCENT_COLOR);

  // --- MODIFICA 1: Icona nota musicale abbassata di 4 pixel ---
  int iconX = boxX + 15;
  int iconY = WR_STATION_Y + 22;  // Era + 18, ora + 22 per abbassarla
  gfx->fillCircle(iconX, iconY + 10, 6, WR_ACCENT_COLOR);
  gfx->fillCircle(iconX + 12, iconY + 6, 6, WR_ACCENT_COLOR);
  gfx->fillRect(iconX + 4, iconY - 8, 3, 18, WR_ACCENT_COLOR);
  gfx->fillRect(iconX + 16, iconY - 12, 3, 18, WR_ACCENT_COLOR);
  gfx->fillRect(iconX + 4, iconY - 10, 15, 3, WR_ACCENT_COLOR);

  // --- MODIFICA 2: Centratura dinamica del testo ---
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WR_TEXT_COLOR);

  String stationName = webRadioName;
  if (stationName.length() > 24) {
    stationName = stationName.substring(0, 21) + "...";
  }

  // Calcolo larghezza testo (circa 9 pixel a carattere per questo font)
  int16_t tw = stationName.length() * 9;

  // Centratura perfetta nel boxW (480 - eventuali margini laterali)
  int textX = boxX + (boxW - tw) / 2;

  gfx->setCursor(textX, WR_STATION_Y + 34);
  gfx->print(stationName);
}

// ================== STATO - DESIGN MODERNO ==================
void drawWebRadioStatus() {
  int centerX = 240;
  int y = WR_STATUS_Y;
  uint32_t now = millis();
  float phase = (float)(now % 3000) / 3000.0; // Ciclo di 3 secondi (0.0 a 1.0)

  if (webRadioEnabled) {
    // 1. INDICATORE LED (Sinistra)
    gfx->fillCircle(centerX - 125, y + 18, 8, WR_STATUS_ON);
    gfx->drawCircle(centerX - 125, y + 18, 11, WR_STATUS_GLOW);

    // 2. TESTO STREAMING
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setTextColor(WR_STATUS_ON);
    gfx->setCursor(centerX - 85, y + 26);
    gfx->print("STREAMING");

    // 3. BARRE AUDIO ANIMATE (Destra) - ALZATE e ANIMATE
    // Puliamo l'area delle barre per l'animazione
    gfx->fillRect(centerX + 105, y, 40, 30, WR_BG_COLOR); 
    
    for (int i = 0; i < 3; i++) {
      // Calcolo altezza dinamica (sinusoide sfasata per ogni barra)
      float barPhase = phase * 2.0 * PI + (i * 1.5);
      int barH = 6 + (int)(12 * (0.5 + 0.5 * sin(barPhase)));
      
      int barX = centerX + 110 + (i * 10);
      // Y portata a y + 14 per allinearla alla parte alta/media del testo
      gfx->fillRoundRect(barX, y + 14 - barH / 2, 5, barH, 2, WR_STATUS_ON);
    }
  } else {
    // --- SINISTRA: LED ROSSO ---
    gfx->fillCircle(centerX - 100, y + 18, 8, WR_STATUS_OFF);

    // --- CENTRO: SCRITTA STOPPED ---
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setTextColor(WR_STATUS_OFF);
    gfx->setCursor(centerX - 65, y + 26);
    gfx->print("STOPPED");

    // --- DESTRA: ICONA QUADRATO CON FADE ---
    // Calcoliamo il colore sfumato (da Rosso a Blu scuro)
    float pulse = 0.5 + 0.5 * sin(phase * 2.0 * PI);
    
    // Estrazione componenti colore per il blend (RGB565)
    uint16_t r1 = (WR_STATUS_OFF >> 11) & 0x1F;
    uint16_t g1 = (WR_STATUS_OFF >> 5) & 0x3F;
    uint16_t b1 = WR_STATUS_OFF & 0x1F;
    uint16_t r2 = (WR_BG_COLOR >> 11) & 0x1F;
    uint16_t g2 = (WR_BG_COLOR >> 5) & 0x3F;
    uint16_t b2 = WR_BG_COLOR & 0x1F;
    
    uint16_t r = r2 + (r1 - r2) * pulse;
    uint16_t g = g2 + (g1 - g2) * pulse;
    uint16_t b = b2 + (b1 - b2) * pulse;
    uint16_t animatedRed = (r << 11) | (g << 5) | b;

    gfx->fillRoundRect(centerX + 85, y + 10, 16, 16, 2, animatedRed);
  }
}
// ================== BARRA VOLUME - DESIGN MODERNO ==================
void drawWebRadioVolumeBar() {
  int barX = 95;
  int barW = 290;
  int barH = 30;            // Altezza impostata a 30 pixel (numero tondo)
  int y = WR_VOLUME_Y - 7;  // Mantiene l'innalzamento di 7px
  int numSteps = 21;

  // --- NUOVA ICONA SPEAKER (Simbolo stilizzato) ---
  int spkX = barX - 35;  // Leggermente più a sinistra per far spazio al disegno
  int spkY = y + (barH / 2);

  gfx->setTextColor(WR_ACCENT_COLOR);
  // Disegno corpo speaker
  gfx->fillRect(spkX, spkY - 4, 5, 8, WR_ACCENT_COLOR);                                         // Base quadrata
  gfx->fillTriangle(spkX + 5, spkY - 8, spkX + 5, spkY + 8, spkX + 12, spkY, WR_ACCENT_COLOR);  // Cono

  // Onde sonore (due piccoli archi a destra del cono)
  gfx->drawFastVLine(spkX + 15, spkY - 3, 6, WR_ACCENT_COLOR);
  gfx->drawFastVLine(spkX + 18, spkY - 6, 12, WR_ACCENT_COLOR);

  // --- DISEGNO TACCHETTE ---
  int activeSteps = map(webRadioVolume, 0, 100, 0, numSteps);
  int spacing = barW / numSteps;

  for (int i = 1; i <= numSteps; i++) {
    int tickH = map(i, 1, numSteps, 4, barH);
    int tickX = barX + (i - 1) * spacing;
    int tickY = y + barH - tickH;

    uint16_t color = (i <= activeSteps) ? WR_ACCENT_COLOR : WR_BG_CARD;
    gfx->fillRect(tickX, tickY, 3, tickH, color);
  }

  // --- VALORE NUMERICO AVVICINATO ---
  int valX = barX + barW + 5;
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);

  gfx->fillRect(valX, y - 5, 40, barH + 10, WR_BG_COLOR);

  String volStr = String(webRadioVolume);
  int textOffset = (webRadioVolume < 10) ? 8 : 0;

  gfx->setCursor(valX + textOffset, y + barH - 2);
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
      color = WR_VU_LOW;  // Verde (0-40%)
      dimColor = 0x0320;
    } else if (ratio < 0.55) {
      color = WR_VU_LOW2;  // Verde-ciano (40-55%)
      dimColor = 0x0220;
    } else if (ratio < 0.70) {
      color = WR_VU_MID;  // Giallo (55-70%)
      dimColor = 0x4200;
    } else if (ratio < 0.85) {
      color = WR_VU_MID2;  // Arancione (70-85%)
      dimColor = 0x4100;
    } else {
      color = WR_VU_HIGH;  // Rosso (85-100%)
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

  // Aggiorna VU ogni 30ms [cite: 546]
  if (now - wrLastVuUpdate < 30) return;
  wrLastVuUpdate = now;

  // CONTROLLO DI SICUREZZA: Spegnimento immediato
  if (!webRadioEnabled) {
    wrVuLeft = 0;
    wrVuRight = 0;
    wrVuPeakLeft = 0;
    wrVuPeakRight = 0;
    return;  // Interrompe l'esecuzione per evitare calcoli fantasma
  }

  // --- Logica quando la radio è accesa --- [cite: 547]
  static uint8_t targetLeft = 0;
  static uint8_t targetRight = 0;

  if (random(100) < 30) {
    targetLeft = random(40, 95);
    targetRight = random(40, 95);
  }

  // Smooth attack/decay [cite: 549]
  if (wrVuLeft < targetLeft) {
    int newVal = (int)wrVuLeft + 8;
    wrVuLeft = (newVal > targetLeft) ? targetLeft : (uint8_t)newVal;
  } else {
    int newVal = (int)wrVuLeft - 3;
    wrVuLeft = (newVal < targetLeft) ? targetLeft : (uint8_t)newVal;
  }

  if (wrVuRight < targetRight) {
    int newVal = (int)wrVuRight + 8;
    wrVuRight = (newVal > targetRight) ? targetRight : (uint8_t)newVal;
  } else {
    int newVal = (int)wrVuRight - 3;
    wrVuRight = (newVal < targetRight) ? targetRight : (uint8_t)newVal;
  }

  // Aggiorna picchi [cite: 552]
  if (wrVuLeft > wrVuPeakLeft) wrVuPeakLeft = wrVuLeft;
  else if (wrVuPeakLeft > 0) wrVuPeakLeft--;

  if (wrVuRight > wrVuPeakRight) wrVuPeakRight = wrVuRight;
  else if (wrVuPeakRight > 0) wrVuPeakRight--;
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
  int y = WR_CONTROLS_Y - 8;  // Alzato tutto di 8 pixel

  // Impostiamo il font per i tasti volume (lo stesso di station)
  gfx->setFont(u8g2_font_helvB14_tr);

  // --- Pulsante PREV (<<) ---
  drawWRModernButton(startX, y, btnW, btnH, false, false);
  int cx = startX + btnW / 2;
  int cy = y + btnH / 2;
  gfx->fillTriangle(cx - 5, cy, cx + 5, cy - 10, cx + 5, cy + 10, WR_ACCENT_COLOR);
  gfx->fillTriangle(cx - 15, cy, cx - 5, cy - 10, cx - 5, cy + 10, WR_ACCENT_COLOR);

  // --- Pulsante VOL- (Scritta VOL -) ---
  int x1 = startX + btnW + spacing;
  drawWRModernButton(x1, y, btnW, btnH, false, false);
  gfx->setTextColor(WR_ACCENT_COLOR);
  // Centratura manuale del testo nel bottone (VOL - è largo circa 45px)
  gfx->setCursor(x1 + 6, y + 33);
  gfx->print("VOL -");

  // --- Pulsante PLAY/STOP (Centrale) ---
  int x2 = x1 + btnW + spacing;
  int playBtnW = btnW + 4;
  // Anche questo viene alzato di 8px (y-3-8 = y-11 nel draw)
  drawWRModernButton(x2 - 2, y - 3, playBtnW, btnH + 6, webRadioEnabled, true);
  cx = x2 + btnW / 2;
  cy = y + btnH / 2;
  if (webRadioEnabled) {
    gfx->fillRoundRect(cx - 10, cy - 10, 20, 20, 3, WR_TEXT_COLOR);
  } else {
    gfx->fillTriangle(cx - 8, cy - 12, cx - 8, cy + 12, cx + 12, cy, WR_TEXT_COLOR);
  }

  // --- Pulsante VOL+ (Scritta VOL +) ---
  int x3 = x2 + btnW + spacing;
  drawWRModernButton(x3, y, btnW, btnH, false, false);
  gfx->setTextColor(WR_ACCENT_COLOR);
  // Centratura manuale del testo nel bottone
  gfx->setCursor(x3 + 5, y + 33);
  gfx->print("VOL +");

  // --- Pulsante NEXT (>>) ---
  int x4 = x3 + btnW + spacing;
  drawWRModernButton(x4, y, btnW, btnH, false, false);
  cx = x4 + btnW / 2;
  cy = y + btnH / 2;
  gfx->fillTriangle(cx + 5, cy, cx - 5, cy - 10, cx - 5, cy + 10, WR_ACCENT_COLOR);
  gfx->fillTriangle(cx + 15, cy, cx + 5, cy - 10, cx + 5, cy + 10, WR_ACCENT_COLOR);
}

// ================== LISTA STAZIONI - DESIGN MODERNO ==================
void drawWebRadioStationList() {
  int listX = WR_CENTER_X;
  int listW = WR_CENTER_W;
  // Aumentiamo l'altezza di 2 pixel rispetto alla costante originale
  int listH = WR_LIST_HEIGHT + 2;

  // Icona antenna
  gfx->fillRect(listX + 5, WR_LIST_Y - 18, 2, 12, WR_ACCENT_COLOR);
  gfx->fillCircle(listX + 6, WR_LIST_Y - 20, 3, WR_ACCENT_COLOR);
  gfx->drawLine(listX + 6, WR_LIST_Y - 15, listX + 2, WR_LIST_Y - 8, WR_ACCENT_DARK);
  gfx->drawLine(listX + 6, WR_LIST_Y - 15, listX + 10, WR_LIST_Y - 8, WR_ACCENT_DARK);

  
  // --- 1. HEADER E CONTATORE (Invariati) ---
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);
  gfx->setCursor(listX + 18, WR_LIST_Y - 6);
  gfx->print("STATIONS");

  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(WR_TEXT_DIM);
  String countStr = "(" + String(webRadioStationCount) + ")";
  gfx->setCursor(listX + 115, WR_LIST_Y - 6);
  gfx->print(countStr);

  // --- 2. DISEGNO CARD CONTENITORE ---
  gfx->fillRoundRect(listX, WR_LIST_Y, listW, listH, 10, WR_BG_CARD);
  gfx->drawRoundRect(listX, WR_LIST_Y, listW, listH, 10, WR_BUTTON_BORDER);

  // --- 3. LISTA STAZIONI ---
  int itemH = 25;
  int maxVisible = listH / itemH;

  for (int i = 0; i < maxVisible && (webRadioScrollOffset + i) < webRadioStationCount; i++) {
    int idx = webRadioScrollOffset + i;
    // Offset Y: partiamo da +3 per distanziarci dal bordo superiore
    int itemY = WR_LIST_Y + 3 + (i * itemH);

    if (idx == webRadioCurrentIndex) {
      // MODIFICA: Larghezza ridotta (listW - 35) per non coprire le freccette a destra
      // Altezza ridotta (itemH - 4) per non sbordare sotto
      gfx->fillRoundRect(listX + 4, itemY, listW - 35, itemH - 4, 6, WR_ACCENT_DARK);
      gfx->drawRoundRect(listX + 4, itemY, listW - 35, itemH - 4, 6, WR_ACCENT_COLOR);

      int triX = listX + 10;
      int triY = itemY + (itemH / 2) - 2;
      gfx->fillTriangle(triX, triY - 5, triX, triY + 5, triX + 8, triY, WR_ACCENT_GLOW);
    }

    // Testo stazione
    gfx->setFont(u8g2_font_helvR10_tr);
    uint16_t numColor = (idx == webRadioCurrentIndex) ? WR_ACCENT_GLOW : WR_TEXT_MUTED;
    gfx->setTextColor(numColor);
    int numX = (idx == webRadioCurrentIndex) ? listX + 22 : listX + 10;
    gfx->setCursor(numX, itemY + 15);
    gfx->print(String(idx + 1) + ".");

    gfx->setFont(u8g2_font_helvR12_tr);
    uint16_t nameColor = (idx == webRadioCurrentIndex) ? WR_TEXT_COLOR : WR_TEXT_DIM;
    gfx->setTextColor(nameColor);
    gfx->setCursor(numX + 28, itemY + 16);

    String stName = webRadioStations[idx].name;
    if (stName.length() > 25) stName = stName.substring(0, 22) + "...";
    gfx->print(stName);
  }

  // --- 4. ICONE DI SCORRIMENTO (Sempre visibili a destra) ---
  int arrowX = listX + listW - 18;

  // Freccia SU
  if (webRadioScrollOffset > 0) {
    gfx->fillTriangle(arrowX, WR_LIST_Y + 8, arrowX - 6, WR_LIST_Y + 18, arrowX + 6, WR_LIST_Y + 18, WR_ACCENT_COLOR);
  } else {
    gfx->drawTriangle(arrowX, WR_LIST_Y + 8, arrowX - 6, WR_LIST_Y + 18, arrowX + 6, WR_LIST_Y + 18, WR_BG_CARD);  // Nascosta
  }

  // Freccia GIU
  if (webRadioScrollOffset + maxVisible < webRadioStationCount) {
    gfx->fillTriangle(arrowX, WR_LIST_Y + listH - 8, arrowX - 6, WR_LIST_Y + listH - 18, arrowX + 6, WR_LIST_Y + listH - 18, WR_ACCENT_COLOR);
  }
}
// ================== PULSANTE MODE >> (basso centro) ==================
void drawWebRadioExitButton() {
  gfx->setFont(u8g2_font_helvR08_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);
  gfx->setCursor(210, 479);
  gfx->print("MODE >>");
}

// ================== GESTIONE TOUCH ==================
bool handleWebRadioTouch(int16_t x, int16_t y) {
  playTouchSound();

  int btnW = 62;
  int btnH = 52;
  int spacing = 10;
  int totalW = 5 * btnW + 4 * spacing;
  int startX = (480 - totalW) / 2;

  // --- AGGIORNAMENTO 1: Coordinate controlli alzate di 8px ---
  int ctrlY = WR_CONTROLS_Y - 8;

  // ===== CONTROLLI (PREV, VOL-, PLAY, VOL+, NEXT) =====
  if (y >= ctrlY && y <= ctrlY + btnH) {
    // PREV
    if (x >= startX && x <= startX + btnW) {
      if (webRadioCurrentIndex > 0) {
        selectWebRadioStation(webRadioCurrentIndex - 1);
      } else {
        selectWebRadioStation(webRadioStationCount - 1);
      }
      // AUTO-SCROLL LOGIC
      int maxVisible = WR_LIST_HEIGHT / 25;
      if (webRadioCurrentIndex < webRadioScrollOffset) webRadioScrollOffset = webRadioCurrentIndex;
      else if (webRadioCurrentIndex >= webRadioScrollOffset + maxVisible) webRadioScrollOffset = webRadioCurrentIndex - maxVisible + 1;

      webRadioNeedsRedraw = true;
      return false;
    }

    // VOL-
    int x1 = startX + btnW + spacing;
    if (x >= x1 && x <= x1 + btnW) {
      if (webRadioVolume >= 5) webRadioVolume -= 5;
      else webRadioVolume = 0;
      setWebRadioVolume(webRadioVolume);
      webRadioVolumeNeedsRedraw = true;
      //webRadioNeedsRedraw = true;
      //drawWebRadioVolumeBar();
      return false;
    }

    // PLAY/STOP
    int x2 = x1 + btnW + spacing;
    if (x >= x2 && x <= x2 + btnW) {
      static uint32_t lastPlayStopTouch = 0;
      if (millis() - lastPlayStopTouch > 700) {
        lastPlayStopTouch = millis();
        if (webRadioEnabled) {
          stopWebRadio();  // Questa funzione DEVE contenere webRadioEnabled = false; [cite: 564]
        } else {
          startWebRadio();  // Questa funzione DEVE contenere webRadioEnabled = true;
        }
        webRadioNeedsRedraw = true;
      }
      return false;
    }

    // VOL+
    int x3 = x2 + btnW + spacing;
    if (x >= x3 && x <= x3 + btnW) {
      webRadioVolume += 5;
      if (webRadioVolume > 100) webRadioVolume = 100;
      setWebRadioVolume(webRadioVolume);
      webRadioVolumeNeedsRedraw = true;
      //webRadioNeedsRedraw = true;
      //drawWebRadioVolumeBar();
      return false;
    }

    // NEXT
    int x4 = x3 + btnW + spacing;
    if (x >= x4 && x <= x4 + btnW) {
      if (webRadioCurrentIndex < webRadioStationCount - 1) {
        selectWebRadioStation(webRadioCurrentIndex + 1);
      } else {
        selectWebRadioStation(0);
      }
      // AUTO-SCROLL LOGIC
      int maxVisible = WR_LIST_HEIGHT / 25;
      if (webRadioCurrentIndex < webRadioScrollOffset) webRadioScrollOffset = webRadioCurrentIndex;
      else if (webRadioCurrentIndex >= webRadioScrollOffset + maxVisible) webRadioScrollOffset = webRadioCurrentIndex - maxVisible + 1;

      webRadioNeedsRedraw = true;
      return false;
    }
  }

  // --- AGGIORNAMENTO 2: BARRA VOLUME (Alzata 7px, H 30px) ---
  int volBarX = 95;
  int volBarW = 290;
  int volBarH = 30;
  int volY = WR_VOLUME_Y - 7;
  if (y >= volY && y <= volY + volBarH) {
    if (x >= volBarX && x <= volBarX + volBarW) {
      int newVol = map(x - volBarX, 0, volBarW, 0, 100);
      webRadioVolume = constrain(newVol, 0, 100);
      setWebRadioVolume(webRadioVolume);
      webRadioVolumeNeedsRedraw = true;
      //webRadioNeedsRedraw = true;
      //drawWebRadioVolumeBar();
      return false;
    }
  }

  // ===== LISTA STAZIONI (Touch su card) =====
  if (y >= WR_LIST_Y && y <= WR_LIST_Y + WR_LIST_HEIGHT) {
    int itemH = 25;
    int clickedItem = (y - WR_LIST_Y) / itemH;
    int stationIdx = webRadioScrollOffset + clickedItem;

    if (stationIdx < webRadioStationCount) {
      selectWebRadioStation(stationIdx);
      webRadioNeedsRedraw = true;
    }
    return false;
  }

  // ===== MODE >> (basso centro y>455, x 180-310) =====
  if (y > 455 && x >= 180 && x <= 310) return true;

  // ===== SCROLL MANUALE (Aree sopra/sotto la lista) =====
  if (y >= WR_LIST_Y - 30 && y < WR_LIST_Y && webRadioScrollOffset > 0) {
    webRadioScrollOffset--;
    webRadioNeedsRedraw = true;
    return false;
  }

  int maxVisible = WR_LIST_HEIGHT / 25;
  if (y > WR_LIST_Y + WR_LIST_HEIGHT && y <= WR_LIST_Y + WR_LIST_HEIGHT + 35) {
    if (webRadioScrollOffset + maxVisible < webRadioStationCount) {
      webRadioScrollOffset++;
      webRadioNeedsRedraw = true;
    }
    return false;
  }

  return false;
}

// ================== HTML PAGINA WEB RADIO ==================
const char WEB_RADIO_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Web Radio</title><style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}
.c{max-width:600px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}
.h{background:linear-gradient(135deg,#00d4ff,#0099cc);padding:25px;text-align:center}
.h h1{font-size:1.5em;margin-bottom:5px}.h p{opacity:.8;font-size:.9em}
.ct{padding:25px}
.s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:20px}
.s h3{margin-bottom:15px;color:#00d4ff}
.nav{display:flex;justify-content:space-between;padding:10px 15px}
.nav a{color:#94a3b8;text-decoration:none;font-size:.9em;padding:5px 10px}
.nav a:hover{color:#fff}
.status{text-align:center;padding:20px;background:rgba(0,0,0,.2);border-radius:15px;margin-bottom:20px}
.status.on{border:2px solid #4CAF50;background:rgba(76,175,80,.15)}
.status.off{border:2px solid #666;background:rgba(0,0,0,.3)}
.now-playing{font-size:1.3em;font-weight:bold;margin-bottom:8px;color:#00d4ff}
.station-url{font-size:.8em;opacity:.5;word-break:break-all}
.power-btn{width:100%;padding:20px;border:none;border-radius:15px;font-weight:bold;cursor:pointer;font-size:1.2em;margin-bottom:20px;display:flex;align-items:center;justify-content:center;gap:15px;transition:all .3s}
.power-btn.off{background:linear-gradient(135deg,#2a2a4a,#1a1a3a);color:#888;border:3px solid #444}
.power-btn.on{background:linear-gradient(135deg,#4CAF50,#388E3C);color:#fff;border:3px solid #4CAF50;box-shadow:0 0 25px rgba(76,175,80,0.4)}
.power-btn:hover{transform:scale(1.02)}
.power-icon{font-size:1.8em}
.station-select{width:100%;padding:14px;border-radius:10px;border:2px solid #555;background:#2a2a4a;color:#fff;font-size:1em;margin-bottom:15px}
.station-select:focus{outline:none;border-color:#00d4ff}
.volume-row{display:flex;align-items:center;gap:15px}
.volume-slider{flex:1;height:10px;-webkit-appearance:none;background:#2a2a4a;border-radius:5px;border:1px solid #555}
.volume-slider::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;background:#00d4ff;border-radius:50%;cursor:pointer;box-shadow:0 0 10px rgba(0,212,255,.5)}
.vol-val{min-width:50px;text-align:center;color:#00d4ff;font-weight:bold;font-size:1.2em}
.btn{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px;transition:all .2s}
.btn:hover{transform:scale(1.02)}
.btn-display{background:linear-gradient(135deg,#9c27b0,#7b1fa2);color:#fff}
.vu{display:flex;justify-content:center;gap:4px;height:50px;align-items:flex-end;margin:20px 0}
.vu-bar{width:10px;background:linear-gradient(to top,#00d4ff,#00ff88,#ff9800);border-radius:3px;transition:height 0.1s}
.add-section{margin-top:15px;padding-top:15px;border-top:1px solid #444}
.add-row{display:flex;gap:10px;margin-bottom:10px}
.add-input{flex:1;padding:10px;border-radius:8px;border:1px solid #555;background:#2a2a4a;color:#fff}
.add-input:focus{outline:none;border-color:#00d4ff}
.btn-add{padding:10px 20px;background:#4CAF50;color:#fff;border:none;border-radius:8px;cursor:pointer}
.btn-remove{padding:10px 20px;background:#f44336;color:#fff;border:none;border-radius:8px;cursor:pointer}
</style></head><body><div class="c">
<div class="nav"><a href="/">&larr; Home</a><a href="/settings">Settings &rarr;</a></div>
<div class="h"><h1>📻 WEB RADIO</h1><p>Ascolta le tue stazioni preferite</p></div><div class="ct">
<div class="status off" id="statusBox">
<div class="now-playing" id="nowPlaying">Radio spenta</div>
<div class="station-url" id="stationUrl"></div>
</div>
<button class="power-btn off" id="powerBtn" onclick="toggleRadio()">
<span class="power-icon">📻</span>
<span id="powerText">ACCENDI RADIO</span>
</button>
<div class="vu" id="vuMeter"></div>
<div class="s"><h3>Stazione</h3>
<select class="station-select" id="stationSelect" onchange="selectStation()"></select>
</div>
<div class="s"><h3>Volume</h3>
<div class="volume-row">
<span style="font-size:1.3em">🔊</span>
<input type="range" class="volume-slider" id="volume" min="0" max="100" onchange="setVolume()">
<span class="vol-val" id="volVal">100</span>
</div>
</div>
<div class="s"><h3>Gestisci Stazioni</h3>
<div class="add-row">
<input type="text" class="add-input" id="newName" placeholder="Nome stazione">
</div>
<div class="add-row">
<input type="text" class="add-input" id="newUrl" placeholder="URL stream (http://...)">
<button class="btn-add" onclick="addStation()">+</button>
</div>
<div class="add-row" style="margin-top:10px">
<button class="btn-remove" onclick="removeStation()">Rimuovi stazione selezionata</button>
</div>
</div>
<button class="btn btn-display" onclick="activate()">Mostra su Display</button>
</div></div><script>
var cfg={enabled:false,station:0,volume:100,name:'',url:'',stations:[]};
function render(){
  var st=document.getElementById('statusBox');
  var pwrBtn=document.getElementById('powerBtn');
  var pwrTxt=document.getElementById('powerText');
  if(cfg.enabled){
    st.className='status on';
    document.getElementById('nowPlaying').textContent=cfg.name||'In riproduzione...';
    document.getElementById('stationUrl').textContent=cfg.url||'';
    pwrBtn.className='power-btn on';
    pwrTxt.textContent='RADIO ACCESA - tocca per spegnere';
  }else{
    st.className='status off';
    document.getElementById('nowPlaying').textContent='Radio spenta';
    document.getElementById('stationUrl').textContent='';
    pwrBtn.className='power-btn off';
    pwrTxt.textContent='ACCENDI RADIO';
  }
  document.getElementById('volume').value=cfg.volume;
  document.getElementById('volVal').textContent=cfg.volume+'%';
  var sel=document.getElementById('stationSelect');
  sel.innerHTML='';
  for(var i=0;i<cfg.stations.length;i++){
    var opt=document.createElement('option');
    opt.value=i;opt.textContent=(i+1)+'. '+cfg.stations[i].name;
    if(i==cfg.station)opt.selected=true;
    sel.appendChild(opt);
  }
}
function toggleRadio(){
  fetch('/webradio/cmd?action='+(cfg.enabled?'stop':'play')).then(r=>r.json()).then(d=>{
    cfg.enabled=d.enabled;cfg.station=d.station;cfg.volume=d.volume;
    cfg.name=d.name;cfg.url=d.url;cfg.stations=d.stations||[];
    render();
  });
}
function selectStation(){
  var idx=document.getElementById('stationSelect').value;
  fetch('/webradio/cmd?action=select&station='+idx).then(r=>r.json()).then(d=>{
    cfg.enabled=d.enabled;cfg.station=d.station;cfg.name=d.name;cfg.url=d.url;
    render();
  });
}
function setVolume(){
  var vol=document.getElementById('volume').value;
  document.getElementById('volVal').textContent=vol+'%';
  fetch('/webradio/cmd?action=volume&value='+vol);
  cfg.volume=vol;
}
function addStation(){
  var name=document.getElementById('newName').value.trim();
  var url=document.getElementById('newUrl').value.trim();
  if(!name||!url){alert('Inserisci nome e URL');return;}
  if(!url.startsWith('http')){alert('URL deve iniziare con http');return;}
  fetch('/webradio/cmd?action=add&name='+encodeURIComponent(name)+'&url='+encodeURIComponent(url))
    .then(r=>r.json()).then(d=>{
      cfg.stations=d.stations||[];render();
      document.getElementById('newName').value='';
      document.getElementById('newUrl').value='';
    });
}
function removeStation(){
  var idx=document.getElementById('stationSelect').value;
  if(cfg.stations.length<=1){alert('Devi avere almeno una stazione');return;}
  if(!confirm('Rimuovere questa stazione?'))return;
  fetch('/webradio/cmd?action=remove&station='+idx).then(r=>r.json()).then(d=>{
    cfg.stations=d.stations||[];cfg.station=d.station;render();
  });
}
function activate(){fetch('/webradio/activate');}
function updateVU(l,r){
  var vu=document.getElementById('vuMeter');
  var html='';
  for(var i=0;i<8;i++){html+='<div class="vu-bar" style="height:'+Math.max(5,l>i*12?Math.min(50,l/2):5)+'px"></div>';}
  for(var i=7;i>=0;i--){html+='<div class="vu-bar" style="height:'+Math.max(5,r>i*12?Math.min(50,r/2):5)+'px"></div>';}
  vu.innerHTML=html;
}
function load(){
  fetch('/webradio/status').then(r=>r.json()).then(d=>{
    cfg.enabled=d.enabled;cfg.station=d.station;cfg.volume=d.volume;
    cfg.name=d.name;cfg.url=d.url;cfg.stations=d.stations||[];
    render();
    updateVU(d.vuL||0,d.vuR||0);
  }).catch(()=>{});
}
load();setInterval(load,2000);
</script></body></html>
)rawliteral";

// ================== WEBSERVER ENDPOINTS ==================
void setup_webradio_webserver(AsyncWebServer* server) {
  Serial.println("[WEBRADIO-WEB] Configurazione endpoints web...");

  // IMPORTANTE: Endpoint specifici PRIMA di quello generico!

  // Status JSON
  server->on("/webradio/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    char jsonBuf[2048];
    int pos = 0;

    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos,
      "{\"enabled\":%s,\"station\":%d,\"volume\":%d,\"name\":\"%s\",\"url\":\"%s\",\"vuL\":%d,\"vuR\":%d,\"stations\":[",
      webRadioEnabled ? "true" : "false",
      webRadioCurrentIndex,
      webRadioVolume,
      webRadioName.c_str(),
      webRadioUrl.c_str(),
      wrVuLeft, wrVuRight
    );

    for (int i = 0; i < webRadioStationCount && i < 30; i++) {
      if (i > 0) pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, ",");
      String safeName = webRadioStations[i].name;
      safeName.replace("\"", "'");
      String safeUrl = webRadioStations[i].url;
      safeUrl.replace("\"", "'");
      pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos,
        "{\"name\":\"%s\",\"url\":\"%s\"}", safeName.c_str(), safeUrl.c_str());
      if (pos >= sizeof(jsonBuf) - 200) break;
    }

    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "]}");
    request->send(200, "application/json", jsonBuf);
  });

  // Comandi
  server->on("/webradio/cmd", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("action")) {
      String action = request->getParam("action")->value();

      if (action == "play") {
        if (!webRadioEnabled) {
          startWebRadio();
        }
        if (currentMode == MODE_WEB_RADIO) webRadioNeedsRedraw = true;
      }
      else if (action == "stop") {
        if (webRadioEnabled) {
          stopWebRadio();
        }
        if (currentMode == MODE_WEB_RADIO) webRadioNeedsRedraw = true;
      }
      else if (action == "select" && request->hasParam("station")) {
        int idx = request->getParam("station")->value().toInt();
        if (idx >= 0 && idx < webRadioStationCount) {
          selectWebRadioStation(idx);
          saveWebRadioSettings();
          if (currentMode == MODE_WEB_RADIO) webRadioNeedsRedraw = true;
        }
      }
      else if (action == "volume" && request->hasParam("value")) {
        int vol = request->getParam("value")->value().toInt();
        vol = constrain(vol, 0, 100);
        webRadioVolume = vol;
        setWebRadioVolume(vol);
        saveWebRadioSettings();
        if (currentMode == MODE_WEB_RADIO) webRadioNeedsRedraw = true;
      }
      else if (action == "add" && request->hasParam("name") && request->hasParam("url")) {
        String name = request->getParam("name")->value();
        String url = request->getParam("url")->value();
        if (addWebRadioStation(name, url)) {
          saveWebRadioStationsToSD();
          Serial.printf("[WEBRADIO-WEB] Stazione aggiunta: %s\n", name.c_str());
        }
      }
      else if (action == "remove" && request->hasParam("station")) {
        int idx = request->getParam("station")->value().toInt();
        if (removeWebRadioStation(idx)) {
          saveWebRadioStationsToSD();
          Serial.printf("[WEBRADIO-WEB] Stazione rimossa: %d\n", idx);
        }
      }
    }

    // Rispondi con stato aggiornato
    char jsonBuf[2048];
    int pos = 0;
    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos,
      "{\"enabled\":%s,\"station\":%d,\"volume\":%d,\"name\":\"%s\",\"url\":\"%s\",\"stations\":[",
      webRadioEnabled ? "true" : "false",
      webRadioCurrentIndex,
      webRadioVolume,
      webRadioName.c_str(),
      webRadioUrl.c_str()
    );
    for (int i = 0; i < webRadioStationCount && i < 30; i++) {
      if (i > 0) pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, ",");
      String safeName = webRadioStations[i].name;
      safeName.replace("\"", "'");
      pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "{\"name\":\"%s\"}", safeName.c_str());
      if (pos >= sizeof(jsonBuf) - 100) break;
    }
    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "]}");
    request->send(200, "application/json", jsonBuf);
  });

  // Attiva modalità Web Radio sul display
  server->on("/webradio/activate", HTTP_GET, [](AsyncWebServerRequest* request) {
    currentMode = MODE_WEB_RADIO;
    webRadioNeedsRedraw = true;
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Pagina HTML - DEVE essere registrata DOPO gli endpoint specifici!
  server->on("/webradio", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", WEB_RADIO_HTML);
  });

  Serial.println("[WEBRADIO-WEB] Endpoints configurati su /webradio");
}

#endif  // EFFECT_WEB_RADIO
