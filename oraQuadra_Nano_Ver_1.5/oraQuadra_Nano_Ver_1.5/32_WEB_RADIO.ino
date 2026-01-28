// ================== WEB RADIO DISPLAY INTERFACE ==================
// Interfaccia grafica a display per controllare la Web Radio
// Simile a MP3 Player ma per streaming radio

// Define locale per garantire compilazione (il main viene compilato dopo)
#ifndef EFFECT_WEB_RADIO
#define EFFECT_WEB_RADIO
#endif

#ifdef EFFECT_WEB_RADIO

// ================== COSTANTI UI ==================
#define WR_BG_COLOR       0x0000  // Nero
#define WR_TEXT_COLOR     0xFFFF  // Bianco
#define WR_ACCENT_COLOR   0x07E0  // Verde
#define WR_BUTTON_COLOR   0x4208  // Grigio scuro
#define WR_BUTTON_ACTIVE  0x07E0  // Verde (attivo)
#define WR_BUTTON_BORDER  0x6B6D  // Grigio chiaro

// Colori VU meter
#define WR_VU_LOW         0x07E0  // Verde
#define WR_VU_MID         0xFFE0  // Giallo
#define WR_VU_HIGH        0xF800  // Rosso
#define WR_VU_BG          0x2104  // Grigio scuro

// Layout FULL SCREEN 480x480
// VU meters ai lati: 0-55 sinistra, 425-480 destra
// Area centrale: 60-420 (360px)
#define WR_HEADER_Y       5
#define WR_STATION_Y      55
#define WR_STATUS_Y       115
#define WR_VOLUME_Y       160
#define WR_CONTROLS_Y     210
#define WR_LIST_Y         280
#define WR_LIST_HEIGHT    130
#define WR_EXIT_Y         420

// VU meters - ai lati (tutto lo schermo in altezza)
#define WR_VU_LEFT_X      0
#define WR_VU_RIGHT_X     425
#define WR_VU_WIDTH       55
#define WR_VU_HEIGHT      420
#define WR_VU_Y_START     50
#define WR_VU_SEGMENTS    20

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
    if (now - lastTouch > 300) {
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
  gfx->fillScreen(WR_BG_COLOR);
  drawWebRadioVUBackground();
  drawWebRadioHeader();
  drawWebRadioStation();
  drawWebRadioStatus();
  drawWebRadioVolumeBar();
  drawWebRadioControls();
  drawWebRadioStationList();
  drawWebRadioExitButton();
}

// ================== HEADER ==================
void drawWebRadioHeader() {
  gfx->setFont(u8g2_font_helvB24_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);

  const char* title = "WEB RADIO";
  int16_t tw = strlen(title) * 15;
  gfx->setCursor((480 - tw) / 2, WR_HEADER_Y + 35);
  gfx->print(title);

  // Linea separatrice (area centrale tra i VU)
  gfx->drawFastHLine(60, WR_HEADER_Y + 45, 360, WR_BUTTON_BORDER);
}

// ================== NOME STAZIONE ==================
void drawWebRadioStation() {
  // Box stazione (area centrale tra VU: x=60, width=360)
  gfx->fillRoundRect(60, WR_STATION_Y, 360, 50, 10, WR_BUTTON_COLOR);
  gfx->drawRoundRect(60, WR_STATION_Y, 360, 50, 10, WR_BUTTON_BORDER);

  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WR_TEXT_COLOR);

  String stationName = webRadioName;
  if (stationName.length() > 26) {
    stationName = stationName.substring(0, 23) + "...";
  }

  int16_t tw = stationName.length() * 9;
  gfx->setCursor((480 - tw) / 2, WR_STATION_Y + 34);
  gfx->print(stationName);
}

// ================== STATO ==================
void drawWebRadioStatus() {
  gfx->setFont(u8g2_font_helvB18_tr);

  // Stato ON/OFF
  String status;
  uint16_t statusColor;
  if (webRadioEnabled) {
    status = "STREAMING";
    statusColor = WR_ACCENT_COLOR;
  } else {
    status = "STOPPED";
    statusColor = 0xF800;  // Rosso
  }

  gfx->setTextColor(statusColor);
  int16_t tw = status.length() * 12;
  gfx->setCursor((480 - tw) / 2, WR_STATUS_Y + 28);
  gfx->print(status);
}

// ================== BARRA VOLUME ==================
void drawWebRadioVolumeBar() {
  int barX = 100;
  int barW = 280;
  int barH = 30;
  int y = WR_VOLUME_Y;

  // Etichetta VOL
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WR_TEXT_COLOR);
  gfx->setCursor(60, y + 22);
  gfx->print("VOL");

  // Sfondo barra
  gfx->fillRoundRect(barX, y, barW, barH, 8, WR_BUTTON_COLOR);
  gfx->drawRoundRect(barX, y, barW, barH, 8, WR_BUTTON_BORDER);

  // Barra di riempimento (webRadioVolume va da 0 a 21)
  int fillW = map(webRadioVolume, 0, 21, 0, barW - 8);
  if (fillW > 0) {
    gfx->fillRoundRect(barX + 4, y + 4, fillW, barH - 8, 5, WR_ACCENT_COLOR);
  }

  // Valore numerico
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WR_TEXT_COLOR);
  String volStr = String(webRadioVolume);
  gfx->setCursor(barX + barW + 10, y + 22);
  gfx->print(volStr);
}

// ================== VU METER BACKGROUND ==================
void drawWebRadioVUBackground() {
  // VU sinistro
  gfx->fillRoundRect(WR_VU_LEFT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT, 5, WR_VU_BG);
  gfx->drawRoundRect(WR_VU_LEFT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT, 5, WR_ACCENT_COLOR);

  // VU destro
  gfx->fillRoundRect(WR_VU_RIGHT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT, 5, WR_VU_BG);
  gfx->drawRoundRect(WR_VU_RIGHT_X, WR_VU_Y_START, WR_VU_WIDTH, WR_VU_HEIGHT, 5, WR_ACCENT_COLOR);

  // Reset stato precedente
  wrPrevActiveLeft = -1;
  wrPrevActiveRight = -1;
  wrPrevPeakLeft = -1;
  wrPrevPeakRight = -1;
  wrVuBackgroundDrawn = true;
}

// ================== VU METER DRAWING ==================
void drawWebRadioVUMeter(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide) {
  // Calcola dimensioni segmenti
  int segmentH = (h - 20) / WR_VU_SEGMENTS;
  int segmentW = w - 10;
  int segmentX = x + 5;
  int startY = y + h - 10;  // Parte dal basso

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

  for (int i = minSeg; i <= maxSeg && i < WR_VU_SEGMENTS; i++) {
    int segY = startY - (i + 1) * segmentH;
    uint16_t color;

    // Determina colore in base alla posizione
    if (i < WR_VU_SEGMENTS * 0.6) {
      color = WR_VU_LOW;    // Verde (60% inferiore)
    } else if (i < WR_VU_SEGMENTS * 0.85) {
      color = WR_VU_MID;    // Giallo (25% centrale)
    } else {
      color = WR_VU_HIGH;   // Rosso (15% superiore)
    }

    if (i < activeSegments) {
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 2, 2, color);
    } else {
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 2, 2, WR_VU_BG);
    }
  }

  // Gestione picco
  if (peakSegment != prevPeak && peakSegment > 0 && peakSegment < WR_VU_SEGMENTS) {
    if (prevPeak > 0 && prevPeak < WR_VU_SEGMENTS) {
      int oldPeakY = startY - (prevPeak + 1) * segmentH;
      uint16_t oldColor = (prevPeak < activeSegments) ?
        ((prevPeak < WR_VU_SEGMENTS * 0.6) ? WR_VU_LOW :
         (prevPeak < WR_VU_SEGMENTS * 0.85) ? WR_VU_MID : WR_VU_HIGH) : WR_VU_BG;
      gfx->fillRoundRect(segmentX, oldPeakY, segmentW, segmentH - 2, 2, oldColor);
    }
    int newPeakY = startY - (peakSegment + 1) * segmentH;
    gfx->drawRect(segmentX, newPeakY, segmentW, segmentH - 2, 0xFFFF);
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

// ================== CONTROLLI ==================
void drawWebRadioControls() {
  int btnW = 65;
  int btnH = 55;
  int spacing = 8;
  int totalW = 5 * btnW + 4 * spacing;  // 357
  int startX = (480 - totalW) / 2;      // 61
  int y = WR_CONTROLS_Y;

  // Pulsante PREV
  gfx->fillRoundRect(startX, y, btnW, btnH, 10, WR_BUTTON_COLOR);
  gfx->drawRoundRect(startX, y, btnW, btnH, 10, WR_BUTTON_BORDER);
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WR_TEXT_COLOR);
  gfx->setCursor(startX + 18, y + 36);
  gfx->print("<<");

  // Pulsante VOL-
  int x1 = startX + btnW + spacing;
  gfx->fillRoundRect(x1, y, btnW, btnH, 10, WR_BUTTON_COLOR);
  gfx->drawRoundRect(x1, y, btnW, btnH, 10, WR_BUTTON_BORDER);
  gfx->setCursor(x1 + 18, y + 36);
  gfx->print("V-");

  // Pulsante PLAY/STOP (centrale)
  int x2 = x1 + btnW + spacing;
  uint16_t playColor = webRadioEnabled ? WR_BUTTON_ACTIVE : WR_BUTTON_COLOR;
  gfx->fillRoundRect(x2, y, btnW, btnH, 10, playColor);
  gfx->drawRoundRect(x2, y, btnW, btnH, 10, WR_BUTTON_BORDER);
  gfx->setTextColor(webRadioEnabled ? WR_BG_COLOR : WR_TEXT_COLOR);
  gfx->setCursor(x2 + (webRadioEnabled ? 8 : 10), y + 36);
  gfx->print(webRadioEnabled ? "STOP" : "PLAY");

  // Pulsante VOL+
  int x3 = x2 + btnW + spacing;
  gfx->fillRoundRect(x3, y, btnW, btnH, 10, WR_BUTTON_COLOR);
  gfx->drawRoundRect(x3, y, btnW, btnH, 10, WR_BUTTON_BORDER);
  gfx->setTextColor(WR_TEXT_COLOR);
  gfx->setCursor(x3 + 18, y + 36);
  gfx->print("V+");

  // Pulsante NEXT
  int x4 = x3 + btnW + spacing;
  gfx->fillRoundRect(x4, y, btnW, btnH, 10, WR_BUTTON_COLOR);
  gfx->drawRoundRect(x4, y, btnW, btnH, 10, WR_BUTTON_BORDER);
  gfx->setCursor(x4 + 18, y + 36);
  gfx->print(">>");
}

// ================== LISTA STAZIONI ==================
void drawWebRadioStationList() {
  // Titolo lista
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WR_ACCENT_COLOR);
  gfx->setCursor(60, WR_LIST_Y - 8);
  gfx->print("Stazioni:");

  // Area lista (tra i VU meters: x=60, width=360)
  gfx->fillRoundRect(60, WR_LIST_Y, 360, WR_LIST_HEIGHT, 8, 0x1082);  // Grigio scuro sfondo
  gfx->drawRoundRect(60, WR_LIST_Y, 360, WR_LIST_HEIGHT, 8, WR_BUTTON_BORDER);

  // Mostra stazioni visibili
  int itemH = 26;
  int maxVisible = WR_LIST_HEIGHT / itemH;

  gfx->setFont(u8g2_font_helvR12_tr);

  for (int i = 0; i < maxVisible && (webRadioScrollOffset + i) < webRadioStationCount; i++) {
    int idx = webRadioScrollOffset + i;
    int itemY = WR_LIST_Y + 5 + i * itemH;

    // Evidenzia stazione corrente (area centrale)
    if (idx == webRadioCurrentIndex) {
      gfx->fillRoundRect(64, itemY, 352, itemH - 2, 5, 0x0320);  // Verde scuro
    }

    // Numero e nome
    gfx->setTextColor(idx == webRadioCurrentIndex ? WR_ACCENT_COLOR : WR_TEXT_COLOR);
    gfx->setCursor(70, itemY + 18);

    String entry = String(idx + 1) + ". " + webRadioStations[idx].name;
    if (entry.length() > 32) {
      entry = entry.substring(0, 29) + "...";
    }
    gfx->print(entry);
  }

  // Indicatori scroll (dentro area centrale)
  if (webRadioScrollOffset > 0) {
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(WR_ACCENT_COLOR);
    gfx->setCursor(400, WR_LIST_Y + 18);
    gfx->print("^");
  }
  if (webRadioScrollOffset + maxVisible < webRadioStationCount) {
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(WR_ACCENT_COLOR);
    gfx->setCursor(400, WR_LIST_Y + WR_LIST_HEIGHT - 10);
    gfx->print("v");
  }
}

// ================== PULSANTE EXIT ==================
void drawWebRadioExitButton() {
  int btnW = 160;
  int btnH = 50;
  int x = (480 - btnW) / 2;

  gfx->fillRoundRect(x, WR_EXIT_Y, btnW, btnH, 10, 0xF800);  // Rosso
  gfx->drawRoundRect(x, WR_EXIT_Y, btnW, btnH, 10, WR_BUTTON_BORDER);

  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(WR_TEXT_COLOR);
  gfx->setCursor(x + 50, WR_EXIT_Y + 34);
  gfx->print("EXIT");
}

// ================== GESTIONE TOUCH ==================
bool handleWebRadioTouch(int16_t x, int16_t y) {
  playTouchSound();

  int btnW = 65;
  int btnH = 55;
  int spacing = 8;
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

    // PLAY/STOP
    int x2 = x1 + btnW + spacing;
    if (x >= x2 && x <= x2 + btnW) {
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
  int volBarX = 100;
  int volBarW = 280;
  int volBarH = 30;
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
    int itemH = 26;
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
  int itemH = 26;
  int maxVisible = WR_LIST_HEIGHT / itemH;
  if (y > WR_LIST_Y + WR_LIST_HEIGHT && y <= WR_LIST_Y + WR_LIST_HEIGHT + 35) {
    if (webRadioScrollOffset + maxVisible < webRadioStationCount) {
      webRadioScrollOffset++;
      webRadioNeedsRedraw = true;
    }
    return false;
  }

  // ===== EXIT =====
  int exitBtnW = 160;
  int exitBtnH = 50;
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
