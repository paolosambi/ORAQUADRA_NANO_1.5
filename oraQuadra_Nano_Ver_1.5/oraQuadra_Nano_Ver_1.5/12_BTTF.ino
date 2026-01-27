//http://TUO IP:8080/settime?year=2025&month=1&day=1&hour=0&minute=0&second=0
//PER TESTARE CAPODANNO MA FUNZIONA UNA SOLA VOLTA PER RIPROVARLO RIAVVIA ESP E RISCRIVI LA RIGA SOPRA
#ifdef EFFECT_BTTF
// ================== MODALITÀ RITORNO AL FUTURO (BTTF) con VERO DOUBLE BUFFERING ==================

#include "font/DS_DIGI20pt7b.h"
#include "font/DS_DIGIB20pt7b.h"

// Forward declaration per stato pagina setup sveglie
extern bool bttfAlarmSetupActive;

// ================== VERO DOUBLE BUFFER ==================
#define BTTF_SCREEN_WIDTH  480
#define BTTF_SCREEN_HEIGHT 480

// Buffer principale dove disegnare tutto PRIMA di trasferirlo al display
uint16_t *frameBuffer = nullptr;

// OffscreenGFX ora è definito nel file principale .ino (condiviso tra tutti i moduli)
OffscreenGFX *offscreenGfx = nullptr;

// Date configurabili
BTTFDate destinationTime = {10, 26, 1985, 1, 20, "AM"};
BTTFDate lastDeparted = {11, 5, 1955, 6, 0, "AM"};

// ================== COORDINATE ==================
#define BTTF_PANEL1_BASE_Y  78
#define BTTF_PANEL2_BASE_Y  248
#define BTTF_PANEL3_BASE_Y  406

#define BTTF_PANEL1_MONTH_X   35
#define BTTF_PANEL1_DAY_X     133
#define BTTF_PANEL1_YEAR_X    200
#define BTTF_PANEL1_AMPM_X    303
#define BTTF_PANEL1_HOUR_X    335
#define BTTF_PANEL1_COLON_X   395
#define BTTF_PANEL1_MIN_X     420

#define BTTF_PANEL2_MONTH_X   35
#define BTTF_PANEL2_DAY_X     133
#define BTTF_PANEL2_YEAR_X    200
#define BTTF_PANEL2_AMPM_X    300
#define BTTF_PANEL2_HOUR_X    330
#define BTTF_PANEL2_COLON_X   390
#define BTTF_PANEL2_MIN_X     415

#define BTTF_PANEL3_MONTH_X   35
#define BTTF_PANEL3_DAY_X     133
#define BTTF_PANEL3_YEAR_X    200
#define BTTF_PANEL3_AMPM_X    300
#define BTTF_PANEL3_HOUR_X    330
#define BTTF_PANEL3_COLON_X   390
#define BTTF_PANEL3_MIN_X     415

#define BTTF_AM_Y_OFFSET  -20
#define BTTF_PM_Y_OFFSET  +11

#define BTTF_COLON_Y_OFFSET_TOP   -15
#define BTTF_COLON_Y_SPACING       13
#define BTTF_COLON_RADIUS           3

// ================== SISTEMA SVEGLIA ==================
#define BUZZER_PIN -1

bool alarmDestinationEnabled = false;
bool alarmLastDepartedEnabled = false;
bool alarmDestinationTriggered = false;
bool alarmLastDepartedTriggered = false;

// Ripetizione giornaliera sveglie (suona ogni giorno alla stessa ora)
bool alarmDestinationDailyRepeat = false;
bool alarmLastDepartedDailyRepeat = false;

volatile bool bttfNeedsRedraw = false;

// Colori
#define BTTF_BG_COLOR       0x5ACB
#define BTTF_TEXT_COLOR     0x07E0
#define BTTF_RED_COLOR      0xF800
#define BTTF_AMBER_COLOR    0xFC00

const char* monthNames[] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// Stato tracking
static uint8_t lastDisplayedHour = 255;
static uint8_t lastDisplayedMinute = 255;
static uint8_t lastDisplayedDay = 255;
static uint8_t lastDisplayedMonth = 255;
static uint16_t lastDisplayedYear = 0;

// Lampeggio
static unsigned long lastColonBlinkTime = 0;
static bool showColon = true;
static bool lastColonState = true;
#define BTTF_COLON_BLINK_INTERVAL 500

// Buffer sfondo
uint16_t *bttfBackgroundBuffer = nullptr;

// ================== FUNZIONI ==================

// Disegna pannello su GFX generico (può essere offscreen o display)
void drawTimePanelGeneric(Arduino_GFX *targetGfx, int16_t baseY, 
                         uint8_t month, uint8_t day, uint16_t year,
                         uint8_t hour, uint8_t minute, const char* ampm, uint16_t labelColor,
                         int16_t monthX, int16_t dayX, int16_t yearX,
                         int16_t ampmX, int16_t hourX, int16_t colonX, int16_t minX, 
                         bool showColon) {

  char monthStr[4], dayStr[3], yearStr[5], hourStr[3], minStr[3];
  strcpy(monthStr, monthNames[month - 1]);
  sprintf(dayStr, "%02d", day);
  sprintf(yearStr, "%04d", year);
  sprintf(hourStr, "%02d", hour);
  sprintf(minStr, "%02d", minute);

  targetGfx->setFont(&DS_DIGIB20pt7b);
  targetGfx->setTextColor(labelColor);
  
  targetGfx->setCursor(monthX, baseY);
  targetGfx->print(monthStr);
  
  targetGfx->setCursor(dayX, baseY);
  targetGfx->print(dayStr);
  
  targetGfx->setCursor(yearX, baseY);
  targetGfx->print(yearStr);

  // Spia AM/PM
  targetGfx->fillCircle(ampmX, baseY + BTTF_AM_Y_OFFSET, 6, 0x0000);
  targetGfx->fillCircle(ampmX, baseY + BTTF_PM_Y_OFFSET, 6, 0x0000);

  if (strcmp(ampm, "PM") == 0) {
    targetGfx->fillCircle(ampmX, baseY + BTTF_PM_Y_OFFSET, 5, labelColor);
  } else {
    targetGfx->fillCircle(ampmX, baseY + BTTF_AM_Y_OFFSET, 5, labelColor);
  }

  targetGfx->setCursor(hourX, baseY);
  targetGfx->print(hourStr);

  // Due punti
  if (showColon) {
    int16_t colonTopY = baseY + BTTF_COLON_Y_OFFSET_TOP;
    targetGfx->fillCircle(colonX, colonTopY, BTTF_COLON_RADIUS, labelColor);
    targetGfx->fillCircle(colonX, colonTopY + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, labelColor);
  }

  targetGfx->setCursor(minX, baseY);
  targetGfx->print(minStr);
}

// Callback JPEG che scrive nel buffer invece che sul display
int bttfJpegDrawCallback(JPEGDRAW *pDraw) {
  if (bttfBackgroundBuffer != nullptr) {
    for (int y = 0; y < pDraw->iHeight; y++) {
      for (int x = 0; x < pDraw->iWidth; x++) {
        int bufferIndex = (pDraw->y + y) * BTTF_SCREEN_WIDTH + (pDraw->x + x);
        if (bufferIndex < BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT) {
          bttfBackgroundBuffer[bufferIndex] = pDraw->pPixels[y * pDraw->iWidth + x];
        }
      }
    }
  }
  return 1;
}

bool loadBTTFBackgroundToBuffer() {
  String filepath = "/bttf.jpg";

  Serial.printf("[BTTF] Caricamento sfondo: '%s'\n", filepath.c_str());

  if (bttfBackgroundBuffer == nullptr) {
    bttfBackgroundBuffer = (uint16_t*)heap_caps_malloc(
      BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t), 
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    
    if (bttfBackgroundBuffer == nullptr) {
      bttfBackgroundBuffer = (uint16_t*)ps_malloc(BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t));
      if (bttfBackgroundBuffer == nullptr) {
        Serial.println("[BTTF] ERRORE: Memoria insufficiente!");
        return false;
      }
    }
  }

  if (!SD.exists(filepath.c_str())) {
    Serial.println("[BTTF] File non trovato, uso colore piatto");
    for (int i = 0; i < BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT; i++) {
      bttfBackgroundBuffer[i] = BTTF_BG_COLOR;
    }
    return false;
  }

  File jpegFile = SD.open(filepath.c_str(), FILE_READ);
  if (!jpegFile) return false;

  size_t fileSize = jpegFile.size();
  uint8_t *jpegBuffer = (uint8_t *)malloc(fileSize);
  if (!jpegBuffer) {
    jpegFile.close();
    return false;
  }

  jpegFile.read(jpegBuffer, fileSize);
  jpegFile.close();

  int result = jpeg.openRAM(jpegBuffer, fileSize, bttfJpegDrawCallback);
  if (result == 1) {
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    jpeg.decode(0, 0, 0);
    jpeg.close();
    Serial.println("[BTTF] Sfondo caricato!");
  }

  free(jpegBuffer);
  return (result == 1);
}

extern bool loadBTTFConfigFromSD();

void initBTTF() {
  if (bttfInitialized) return;

  Serial.println("=== INIT BTTF MODE - VERO DOUBLE BUFFERING ===");

  // Alloca frameBuffer per double buffering
  if (frameBuffer == nullptr) {
    frameBuffer = (uint16_t*)heap_caps_malloc(
      BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t),
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    
    if (frameBuffer == nullptr) {
      frameBuffer = (uint16_t*)ps_malloc(BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t));
      if (frameBuffer == nullptr) {
        Serial.println("[BTTF] ERRORE: Impossibile allocare frameBuffer!");
        return;
      }
    }
    Serial.println("[BTTF] FrameBuffer allocato");
  }

  // Crea GFX offscreen
  if (offscreenGfx == nullptr) {
    offscreenGfx = new OffscreenGFX(frameBuffer, BTTF_SCREEN_WIDTH, BTTF_SCREEN_HEIGHT);
    Serial.println("[BTTF] OffscreenGFX creato");
  }

  loadBTTFConfigFromSD();
  loadBTTFBackgroundToBuffer();

  lastDisplayedHour = 255;
  lastDisplayedMinute = 255;
  lastDisplayedDay = 255;
  lastDisplayedMonth = 255;
  lastDisplayedYear = 0;

  bttfInitialized = true;
}

void updateBTTF() {
  // Se pagina setup sveglie attiva, non aggiornare display BTTF
  if (bttfAlarmSetupActive) {
    return;
  }

  uint8_t currentHour = myTZ.hour();
  uint8_t currentMinute = myTZ.minute();
  uint8_t currentDay = myTZ.day();
  uint8_t currentMonth = myTZ.month();
  uint16_t currentYear = myTZ.year();

  // Lampeggio
  unsigned long currentTime = millis();
  if (currentTime - lastColonBlinkTime >= BTTF_COLON_BLINK_INTERVAL) {
    showColon = !showColon;
    lastColonBlinkTime = currentTime;
  }

  // Converti 12h
  const char* currentAMPM = "AM";
  uint8_t displayHour = currentHour;

  if (currentHour == 0) {
    displayHour = 12;
  } else if (currentHour == 12) {
    currentAMPM = "PM";
  } else if (currentHour > 12) {
    displayHour = currentHour - 12;
    currentAMPM = "PM";
  }

  if (!bttfInitialized) {
    initBTTF();
  }

  // Aggiornamento COMPLETO
  bool needsFullUpdate = (bttfNeedsRedraw ||
                          lastDisplayedHour != displayHour ||
                          lastDisplayedMinute != currentMinute ||
                          lastDisplayedDay != currentDay ||
                          lastDisplayedMonth != currentMonth ||
                          lastDisplayedYear != currentYear);

  // Aggiornamento PARZIALE (solo due punti)
  bool needsColonUpdate = (showColon != lastColonState);

  if (needsFullUpdate && offscreenGfx != nullptr && frameBuffer != nullptr) {
    Serial.println("[BTTF] *** RENDERING OFFSCREEN - INIZIO ***");

    // ===== FASE 1: DISEGNA TUTTO SUL FRAMEBUFFER (OFFSCREEN) =====
    
    // Copia sfondo nel frameBuffer
    if (bttfBackgroundBuffer != nullptr) {
      memcpy(frameBuffer, bttfBackgroundBuffer, BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT * sizeof(uint16_t));
    } else {
      for (int i = 0; i < BTTF_SCREEN_WIDTH * BTTF_SCREEN_HEIGHT; i++) {
        frameBuffer[i] = BTTF_BG_COLOR;
      }
    }

    // Disegna i 3 pannelli SULL'OFFSCREEN GFX
    drawTimePanelGeneric(offscreenGfx, BTTF_PANEL1_BASE_Y,
                        destinationTime.month, destinationTime.day, destinationTime.year,
                        destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                        BTTF_RED_COLOR,
                        BTTF_PANEL1_MONTH_X, BTTF_PANEL1_DAY_X, BTTF_PANEL1_YEAR_X,
                        BTTF_PANEL1_AMPM_X, BTTF_PANEL1_HOUR_X, BTTF_PANEL1_COLON_X,
                        BTTF_PANEL1_MIN_X, showColon);

    drawTimePanelGeneric(offscreenGfx, BTTF_PANEL2_BASE_Y,
                        currentMonth, currentDay, currentYear,
                        displayHour, currentMinute, currentAMPM,
                        BTTF_TEXT_COLOR,
                        BTTF_PANEL2_MONTH_X, BTTF_PANEL2_DAY_X, BTTF_PANEL2_YEAR_X,
                        BTTF_PANEL2_AMPM_X, BTTF_PANEL2_HOUR_X, BTTF_PANEL2_COLON_X,
                        BTTF_PANEL2_MIN_X, showColon);

    drawTimePanelGeneric(offscreenGfx, BTTF_PANEL3_BASE_Y,
                        lastDeparted.month, lastDeparted.day, lastDeparted.year,
                        lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                        BTTF_AMBER_COLOR,
                        BTTF_PANEL3_MONTH_X, BTTF_PANEL3_DAY_X, BTTF_PANEL3_YEAR_X,
                        BTTF_PANEL3_AMPM_X, BTTF_PANEL3_HOUR_X, BTTF_PANEL3_COLON_X,
                        BTTF_PANEL3_MIN_X, showColon);

    // Mostra indirizzo IP in basso a sinistra
    offscreenGfx->setFont((const GFXfont *)NULL);  // Font piccolo di default
    offscreenGfx->setTextColor(WHITE);
    offscreenGfx->setCursor(5, 470);
    String ipAddress = WiFi.localIP().toString() + ":8080/bttf";
    offscreenGfx->print(ipAddress);

    Serial.println("[BTTF] Rendering offscreen completato");

    // ===== FASE 2: TRASFERISCI TUTTO AL DISPLAY IN UN COLPO SOLO =====
    Serial.println("[BTTF] Trasferimento al display...");
    
    gfx->draw16bitRGBBitmap(0, 0, frameBuffer, BTTF_SCREEN_WIDTH, BTTF_SCREEN_HEIGHT);
    
    Serial.println("[BTTF] *** ZERO FLICKERING - FATTO! ***");

    bttfNeedsRedraw = false;
    lastDisplayedHour = displayHour;
    lastDisplayedMinute = currentMinute;
    lastDisplayedDay = currentDay;
    lastDisplayedMonth = currentMonth;
    lastDisplayedYear = currentYear;
    lastColonState = showColon;

  } else if (needsColonUpdate) {
    // Aggiorna solo i due punti (veloce)
    gfx->startWrite();
    
    if (showColon) {
      int16_t y1 = BTTF_PANEL1_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1, BTTF_COLON_RADIUS, BTTF_RED_COLOR);
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, BTTF_RED_COLOR);

      int16_t y2 = BTTF_PANEL2_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2, BTTF_COLON_RADIUS, BTTF_TEXT_COLOR);
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, BTTF_TEXT_COLOR);

      int16_t y3 = BTTF_PANEL3_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3, BTTF_COLON_RADIUS, BTTF_AMBER_COLOR);
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, BTTF_AMBER_COLOR);
    } else {
      int16_t y1 = BTTF_PANEL1_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1, BTTF_COLON_RADIUS, 0x0000);
      gfx->fillCircle(BTTF_PANEL1_COLON_X, y1 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, 0x0000);

      int16_t y2 = BTTF_PANEL2_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2, BTTF_COLON_RADIUS, 0x0000);
      gfx->fillCircle(BTTF_PANEL2_COLON_X, y2 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, 0x0000);

      int16_t y3 = BTTF_PANEL3_BASE_Y + BTTF_COLON_Y_OFFSET_TOP;
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3, BTTF_COLON_RADIUS, 0x0000);
      gfx->fillCircle(BTTF_PANEL3_COLON_X, y3 + BTTF_COLON_Y_SPACING, BTTF_COLON_RADIUS, 0x0000);
    }
    
    gfx->endWrite();
    lastColonState = showColon;
  }

  checkBTTFAlarms(currentHour, currentMinute, currentDay, currentMonth, currentYear);
}

void forceBTTFRedraw() {
  Serial.println("[BTTF] Ridisegno forzato");
  bttfNeedsRedraw = true;
}

// ================== ALLARMI ==================

void triggerBTTFAlarm(const char* alarmName) {
  Serial.printf("[BTTF ALARM] 🔔 %s\n", alarmName);

  // Attiva l'allarme con riproduzione continua di beep.mp3
  bttfAlarmRinging = true;
  bttfAlarmLastBeep = 0;  // Forza riproduzione immediata
  bttfAlarmStartTime = millis();  // Salva timestamp inizio per timeout

  Serial.println("[BTTF ALARM] Allarme attivato - tocca lo schermo o attendi 5 min per fermarlo");
}

void checkBTTFAlarms(uint8_t currentHour, uint8_t currentMinute, uint8_t currentDay,
                    uint8_t currentMonth, uint16_t currentYear) {

  // Helper: converte ora 12h + AM/PM in formato 24h
  auto convertTo24h = [](uint8_t hour12, const char* ampm) -> uint8_t {
    if (strcmp(ampm, "AM") == 0) {
      // AM: 12:XX AM = 00:XX, 1-11 AM = 1-11
      return (hour12 == 12) ? 0 : hour12;
    } else {
      // PM: 12:XX PM = 12:XX, 1-11 PM = 13-23
      return (hour12 == 12) ? 12 : (hour12 + 12);
    }
  };

  // ========== DESTINATION TIME ALARM ==========
  if (alarmDestinationEnabled && !alarmDestinationTriggered) {
    bool timeMatches = false;
    uint8_t destHour24 = convertTo24h(destinationTime.hour, destinationTime.ampm);

    if (alarmDestinationDailyRepeat) {
      // RIPETIZIONE GIORNALIERA: controlla solo ora e minuto
      timeMatches = (destHour24 == currentHour &&
                     destinationTime.minute == currentMinute);

      // DEBUG: stampa confronto
      static uint8_t lastDebugMinute1 = 255;
      if (currentMinute != lastDebugMinute1) {
        Serial.printf("[ALARM DEBUG] DEST: %d:%02d %s -> 24h=%d | Now: %d:%02d | Match=%d\n",
                     destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                     destHour24, currentHour, currentMinute, timeMatches);
        lastDebugMinute1 = currentMinute;
      }
    } else {
      // SVEGLIA SINGOLA: controlla data completa + ora + minuto
      timeMatches = (destinationTime.year == currentYear &&
                     destinationTime.month == currentMonth &&
                     destinationTime.day == currentDay &&
                     destHour24 == currentHour &&
                     destinationTime.minute == currentMinute);
    }

    if (timeMatches) {
      triggerBTTFAlarm("DESTINATION TIME");
      alarmDestinationTriggered = true;
    }
  }

  // ========== LAST TIME DEPARTED ALARM ==========
  if (alarmLastDepartedEnabled && !alarmLastDepartedTriggered) {
    bool timeMatches = false;
    uint8_t lastHour24 = convertTo24h(lastDeparted.hour, lastDeparted.ampm);

    if (alarmLastDepartedDailyRepeat) {
      // RIPETIZIONE GIORNALIERA: controlla solo ora e minuto
      timeMatches = (lastHour24 == currentHour &&
                     lastDeparted.minute == currentMinute);

      // DEBUG: stampa confronto
      static uint8_t lastDebugMinute2 = 255;
      if (currentMinute != lastDebugMinute2) {
        Serial.printf("[ALARM DEBUG] LAST: %d:%02d %s -> 24h=%d | Now: %d:%02d | Match=%d\n",
                     lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                     lastHour24, currentHour, currentMinute, timeMatches);
        lastDebugMinute2 = currentMinute;
      }
    } else {
      // SVEGLIA SINGOLA: controlla data completa + ora + minuto
      timeMatches = (lastDeparted.year == currentYear &&
                     lastDeparted.month == currentMonth &&
                     lastDeparted.day == currentDay &&
                     lastHour24 == currentHour &&
                     lastDeparted.minute == currentMinute);
    }

    if (timeMatches) {
      triggerBTTFAlarm("LAST TIME DEPARTED");
      alarmLastDepartedTriggered = true;
    }
  }

  static uint8_t lastCheckedMinute = 255;
  if (currentMinute != lastCheckedMinute) {
    lastCheckedMinute = currentMinute;
    if (currentMinute != destinationTime.minute) {
      alarmDestinationTriggered = false;
    }
    if (currentMinute != lastDeparted.minute) {
      alarmLastDepartedTriggered = false;
    }
  }
}

/**
 * Gestisce la riproduzione continua del beep dell'allarme BTTF
 * Da chiamare nel loop principale
 */
void updateBTTFAlarmSound() {
  if (!bttfAlarmRinging) {
    return;  // Allarme non attivo
  }

  // Se il radar è attivo e non c'è nessuno nella stanza, non riprodurre beep (display spento = audio muto)
  if (radarConnectedOnce && !presenceDetected) {
    return; // Nessuna presenza - salta beep allarme
  }

  unsigned long currentTime = millis();

  // ========== TIMEOUT AUTOMATICO 5 MINUTI ==========
  if (currentTime - bttfAlarmStartTime >= BTTF_ALARM_TIMEOUT) {
    Serial.println("[BTTF ALARM] ⏱️  Timeout 5 minuti - allarme spento automaticamente");
    stopBTTFAlarm();
    return;
  }

  // Riproduce beep.mp3 ogni BTTF_ALARM_BEEP_INTERVAL millisecondi
  if (currentTime - bttfAlarmLastBeep >= BTTF_ALARM_BEEP_INTERVAL) {
    // Riproduce beep.mp3 tramite ESP32C3 via I2C
    //playBeepViaI2C();
    playLocalMP3("beep.mp3");
    bttfAlarmLastBeep = currentTime;

    // Calcola tempo rimanente
    unsigned long elapsed = currentTime - bttfAlarmStartTime;
    unsigned long remaining = BTTF_ALARM_TIMEOUT - elapsed;
    int remainingSeconds = remaining / 1000;

    Serial.printf("[BTTF ALARM] Beep riprodotto - tocca per fermare (timeout: %d sec)\n", remainingSeconds);
  }
}

/**
 * Ferma l'allarme BTTF
 */
void stopBTTFAlarm() {
  if (bttfAlarmRinging) {
    bttfAlarmRinging = false;
    Serial.println("[BTTF ALARM] ⏹️  Allarme fermato");
  }
}

// ================== PAGINA SETUP SVEGLIE BTTF ==================

// Forward declaration
void closeBTTFAlarmSetup();

// Stato pagina setup sveglie
bool bttfAlarmSetupActive = false;
static uint8_t bttfAlarmSetupCursor = 0;  // 0-10 (11 opzioni totali)

// Copie temporanee per editing orari sveglie
static uint8_t editDestHour = 0;
static uint8_t editDestMinute = 0;
static bool editDestPM = false;
static uint8_t editLastHour = 0;
static uint8_t editLastMinute = 0;
static bool editLastPM = false;

/**
 * Mostra la pagina di setup sveglie BTTF
 */
void showBTTFAlarmSetup() {
  bttfAlarmSetupActive = true;
  bttfAlarmSetupCursor = 0;

  // Inizializza copie temporanee con valori attuali
  editDestHour = destinationTime.hour;
  editDestMinute = destinationTime.minute;
  editDestPM = (strcmp(destinationTime.ampm, "PM") == 0);

  editLastHour = lastDeparted.hour;
  editLastMinute = lastDeparted.minute;
  editLastPM = (strcmp(lastDeparted.ampm, "PM") == 0);

  drawBTTFAlarmSetupPage();
}

/**
 * Disegna la pagina di setup sveglie - CON MODIFICA ORARI
 * Cursore: 0=ORA_D, 1=MIN_D, 2=AMPM_D, 3=SVEGLIA_D, 4=RIPETI_D
 *          5=ORA_L, 6=MIN_L, 7=AMPM_L, 8=SVEGLIA_L, 9=RIPETI_L, 10=ESCI
 */
void drawBTTFAlarmSetupPage() {
  gfx->fillScreen(BLACK);
  gfx->flush();
  gfx->setFont((const GFXfont *)NULL);

  // ============ TITOLO ============
  gfx->fillRect(0, 0, 480, 35, 0x0320);
  gfx->setTextSize(2);
  gfx->setTextColor(BTTF_TEXT_COLOR);
  gfx->setCursor(150, 8);
  gfx->print("SETUP SVEGLIE");

  // ============ DESTINATION TIME ============
  gfx->setTextSize(2);
  gfx->setTextColor(BTTF_RED_COLOR);
  gfx->setCursor(15, 42);
  gfx->print("DESTINATION TIME");

  gfx->fillRoundRect(10, 60, 460, 80, 6, 0x1000);
  gfx->drawRoundRect(10, 60, 460, 80, 6, BTTF_RED_COLOR);

  // Riga 1: ORA - MINUTI - AM/PM
  int y1 = 68;
  // ORA (cursor 0)
  bool s0 = (bttfAlarmSetupCursor == 0);
  gfx->fillRoundRect(20, y1, 70, 30, 4, s0 ? BTTF_RED_COLOR : 0x2104);
  gfx->setTextColor(s0 ? BLACK : BTTF_RED_COLOR);
  gfx->setCursor(35, y1 + 7);
  char hh[4]; sprintf(hh, "%02d", editDestHour);
  gfx->print(hh);

  gfx->setTextColor(BTTF_RED_COLOR);
  gfx->setCursor(95, y1 + 7);
  gfx->print(":");

  // MINUTI (cursor 1)
  bool s1 = (bttfAlarmSetupCursor == 1);
  gfx->fillRoundRect(110, y1, 70, 30, 4, s1 ? BTTF_RED_COLOR : 0x2104);
  gfx->setTextColor(s1 ? BLACK : BTTF_RED_COLOR);
  gfx->setCursor(125, y1 + 7);
  char mm[4]; sprintf(mm, "%02d", editDestMinute);
  gfx->print(mm);

  // AM/PM (cursor 2)
  bool s2 = (bttfAlarmSetupCursor == 2);
  gfx->fillRoundRect(190, y1, 50, 30, 4, s2 ? BTTF_RED_COLOR : 0x2104);
  gfx->setTextColor(s2 ? BLACK : BTTF_RED_COLOR);
  gfx->setCursor(198, y1 + 7);
  gfx->print(editDestPM ? "PM" : "AM");

  // SVEGLIA (cursor 3)
  bool s3 = (bttfAlarmSetupCursor == 3);
  gfx->fillRoundRect(260, y1, 90, 30, 4, s3 ? 0x4208 : 0x2104);
  if (s3) gfx->drawRoundRect(260, y1, 90, 30, 4, YELLOW);
  gfx->setTextColor(s3 ? YELLOW : WHITE);
  gfx->setCursor(268, y1 + 7);
  gfx->print("SVE");
  gfx->fillRoundRect(310, y1 + 3, 35, 24, 3, alarmDestinationEnabled ? GREEN : 0x8000);
  gfx->setTextColor(alarmDestinationEnabled ? BLACK : RED);
  gfx->setCursor(315, y1 + 7);
  gfx->print(alarmDestinationEnabled ? "ON" : "OF");

  // RIPETI (cursor 4)
  bool s4 = (bttfAlarmSetupCursor == 4);
  gfx->fillRoundRect(365, y1, 95, 30, 4, s4 ? 0x4208 : 0x2104);
  if (s4) gfx->drawRoundRect(365, y1, 95, 30, 4, YELLOW);
  gfx->setTextColor(s4 ? YELLOW : WHITE);
  gfx->setCursor(373, y1 + 7);
  gfx->print("RIP");
  gfx->fillRoundRect(415, y1 + 3, 35, 24, 3, alarmDestinationDailyRepeat ? GREEN : 0x8000);
  gfx->setTextColor(alarmDestinationDailyRepeat ? BLACK : RED);
  gfx->setCursor(420, y1 + 7);
  gfx->print(alarmDestinationDailyRepeat ? "ON" : "OF");

  // Riga 2: info data (non editabile)
  gfx->setTextSize(1);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(20, 108);
  char destDate[30];
  sprintf(destDate, "Data: %s %02d %04d (da web)",
          monthNames[destinationTime.month - 1], destinationTime.day, destinationTime.year);
  gfx->print(destDate);

  // ============ LAST TIME DEPARTED ============
  gfx->setTextSize(2);
  gfx->setTextColor(BTTF_AMBER_COLOR);
  gfx->setCursor(15, 150);
  gfx->print("LAST TIME DEPARTED");

  gfx->fillRoundRect(10, 168, 460, 80, 6, 0x1000);
  gfx->drawRoundRect(10, 168, 460, 80, 6, BTTF_AMBER_COLOR);

  // Riga 1: ORA - MINUTI - AM/PM
  int y2 = 176;
  // ORA (cursor 5)
  bool s5 = (bttfAlarmSetupCursor == 5);
  gfx->fillRoundRect(20, y2, 70, 30, 4, s5 ? BTTF_AMBER_COLOR : 0x2104);
  gfx->setTextColor(s5 ? BLACK : BTTF_AMBER_COLOR);
  gfx->setCursor(35, y2 + 7);
  sprintf(hh, "%02d", editLastHour);
  gfx->print(hh);

  gfx->setTextColor(BTTF_AMBER_COLOR);
  gfx->setCursor(95, y2 + 7);
  gfx->print(":");

  // MINUTI (cursor 6)
  bool s6 = (bttfAlarmSetupCursor == 6);
  gfx->fillRoundRect(110, y2, 70, 30, 4, s6 ? BTTF_AMBER_COLOR : 0x2104);
  gfx->setTextColor(s6 ? BLACK : BTTF_AMBER_COLOR);
  gfx->setCursor(125, y2 + 7);
  sprintf(mm, "%02d", editLastMinute);
  gfx->print(mm);

  // AM/PM (cursor 7)
  bool s7 = (bttfAlarmSetupCursor == 7);
  gfx->fillRoundRect(190, y2, 50, 30, 4, s7 ? BTTF_AMBER_COLOR : 0x2104);
  gfx->setTextColor(s7 ? BLACK : BTTF_AMBER_COLOR);
  gfx->setCursor(198, y2 + 7);
  gfx->print(editLastPM ? "PM" : "AM");

  // SVEGLIA (cursor 8)
  bool s8 = (bttfAlarmSetupCursor == 8);
  gfx->fillRoundRect(260, y2, 90, 30, 4, s8 ? 0x4208 : 0x2104);
  if (s8) gfx->drawRoundRect(260, y2, 90, 30, 4, YELLOW);
  gfx->setTextColor(s8 ? YELLOW : WHITE);
  gfx->setCursor(268, y2 + 7);
  gfx->print("SVE");
  gfx->fillRoundRect(310, y2 + 3, 35, 24, 3, alarmLastDepartedEnabled ? GREEN : 0x8000);
  gfx->setTextColor(alarmLastDepartedEnabled ? BLACK : RED);
  gfx->setCursor(315, y2 + 7);
  gfx->print(alarmLastDepartedEnabled ? "ON" : "OF");

  // RIPETI (cursor 9)
  bool s9 = (bttfAlarmSetupCursor == 9);
  gfx->fillRoundRect(365, y2, 95, 30, 4, s9 ? 0x4208 : 0x2104);
  if (s9) gfx->drawRoundRect(365, y2, 95, 30, 4, YELLOW);
  gfx->setTextColor(s9 ? YELLOW : WHITE);
  gfx->setCursor(373, y2 + 7);
  gfx->print("RIP");
  gfx->fillRoundRect(415, y2 + 3, 35, 24, 3, alarmLastDepartedDailyRepeat ? GREEN : 0x8000);
  gfx->setTextColor(alarmLastDepartedDailyRepeat ? BLACK : RED);
  gfx->setCursor(420, y2 + 7);
  gfx->print(alarmLastDepartedDailyRepeat ? "ON" : "OF");

  // Riga 2: info data
  gfx->setTextSize(1);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(20, 216);
  char lastDate[30];
  sprintf(lastDate, "Data: %s %02d %04d (da web)",
          monthNames[lastDeparted.month - 1], lastDeparted.day, lastDeparted.year);
  gfx->print(lastDate);

  // ============ ISTRUZIONI NAVIGAZIONE ============
  gfx->drawFastHLine(10, 260, 460, 0x4208);
  gfx->setTextSize(2);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(50, 275);
  gfx->print("<< PREC");
  gfx->setCursor(195, 275);
  gfx->print("MODIFICA");
  gfx->setCursor(355, 275);
  gfx->print("SUCC >>");

  gfx->setTextSize(1);
  gfx->setTextColor(0x5AEB);
  gfx->setCursor(60, 300);
  gfx->print("ORA: +1h | MIN: +5min | AM/PM: toggle | SVE/RIP: on/off");

  // ============ PULSANTE SALVA ESCI - PIU IN BASSO E CENTRATO ============
  // Posizionato a y=340 e centrato esattamente (x=160-320 = zona touch centro)
  bool s10 = (bttfAlarmSetupCursor == 10);
  gfx->fillRoundRect(160, 340, 160, 50, 10, s10 ? GREEN : 0x0320);
  if (!s10) gfx->drawRoundRect(160, 340, 160, 50, 10, GREEN);
  gfx->setTextSize(2);
  gfx->setTextColor(s10 ? BLACK : GREEN);
  gfx->setCursor(185, 355);
  gfx->print("ESCI");

  // Nota sotto il pulsante
  gfx->setTextSize(1);
  gfx->setTextColor(s10 ? GREEN : 0x5AEB);
  gfx->setCursor(150, 400);
  gfx->print("Tocca ESCI per salvare e tornare");
}

/**
 * Gestisce il touch nella pagina setup sveglie BTTF
 * Cursore: 0=ORA_D, 1=MIN_D, 2=AMPM_D, 3=SVEGLIA_D, 4=RIPETI_D
 *          5=ORA_L, 6=MIN_L, 7=AMPM_L, 8=SVEGLIA_L, 9=RIPETI_L, 10=ESCI
 */
bool handleBTTFAlarmSetupTouch(int16_t x, int16_t y) {
  if (!bttfAlarmSetupActive) return false;

  // Se cursore su ESCI (10) E tocco nella parte bassa (y > 300), esci subito
  // Questo permette di toccare direttamente il pulsante ESCI
  if (bttfAlarmSetupCursor == 10 || y > 320) {
    if (bttfAlarmSetupCursor == 10) {
      // Cursore già su ESCI - qualsiasi tocco attiva uscita
      closeBTTFAlarmSetup();
      return true;
    } else if (y > 320 && x >= 160 && x <= 320) {
      // Tocco diretto sul pulsante ESCI (anche se cursore non è lì)
      closeBTTFAlarmSetup();
      return true;
    }
  }

  // Dividi lo schermo in 3 aree orizzontali
  if (x < 160) {
    // SINISTRA - Cursor precedente
    if (bttfAlarmSetupCursor > 0) {
      bttfAlarmSetupCursor--;
    } else {
      bttfAlarmSetupCursor = 10;  // Wrap around
    }
    playTouchSound();
    drawBTTFAlarmSetupPage();
    return true;
  }
  else if (x > 320) {
    // DESTRA - Cursor successivo
    if (bttfAlarmSetupCursor < 10) {
      bttfAlarmSetupCursor++;
    } else {
      bttfAlarmSetupCursor = 0;  // Wrap around
    }
    playTouchSound();
    drawBTTFAlarmSetupPage();
    return true;
  }
  else {
    // CENTRO - Modifica valore selezionato
    switch (bttfAlarmSetupCursor) {
      case 0:  // ORA DEST (+1, wrap 12->1)
        editDestHour++;
        if (editDestHour > 12) editDestHour = 1;
        break;

      case 1:  // MINUTI DEST (+5, wrap 55->0)
        editDestMinute += 5;
        if (editDestMinute >= 60) editDestMinute = 0;
        break;

      case 2:  // AM/PM DEST (toggle)
        editDestPM = !editDestPM;
        break;

      case 3:  // SVEGLIA DEST (toggle)
        alarmDestinationEnabled = !alarmDestinationEnabled;
        alarmDestinationTriggered = false;
        break;

      case 4:  // RIPETI DEST (toggle)
        alarmDestinationDailyRepeat = !alarmDestinationDailyRepeat;
        break;

      case 5:  // ORA LAST (+1, wrap 12->1)
        editLastHour++;
        if (editLastHour > 12) editLastHour = 1;
        break;

      case 6:  // MINUTI LAST (+5, wrap 55->0)
        editLastMinute += 5;
        if (editLastMinute >= 60) editLastMinute = 0;
        break;

      case 7:  // AM/PM LAST (toggle)
        editLastPM = !editLastPM;
        break;

      case 8:  // SVEGLIA LAST (toggle)
        alarmLastDepartedEnabled = !alarmLastDepartedEnabled;
        alarmLastDepartedTriggered = false;
        break;

      case 9:  // RIPETI LAST (toggle)
        alarmLastDepartedDailyRepeat = !alarmLastDepartedDailyRepeat;
        break;

      case 10: // SALVA ESCI
        closeBTTFAlarmSetup();
        return true;
    }

    playTouchSound();
    drawBTTFAlarmSetupPage();

    Serial.printf("[BTTF] Edit: DEST %02d:%02d %s | LAST %02d:%02d %s\n",
                  editDestHour, editDestMinute, editDestPM ? "PM" : "AM",
                  editLastHour, editLastMinute, editLastPM ? "PM" : "AM");
    return true;
  }

  return false;
}

/**
 * Chiude la pagina setup sveglie e salva i valori editati
 */
void closeBTTFAlarmSetup() {
  // Salva orari editati nelle strutture BTTF
  destinationTime.hour = editDestHour;
  destinationTime.minute = editDestMinute;
  destinationTime.ampm = editDestPM ? "PM" : "AM";

  lastDeparted.hour = editLastHour;
  lastDeparted.minute = editLastMinute;
  lastDeparted.ampm = editLastPM ? "PM" : "AM";

  // Reset flag triggered per permettere nuova attivazione
  alarmDestinationTriggered = false;
  alarmLastDepartedTriggered = false;

  Serial.println("\n=== SVEGLIE SALVATE ===");
  Serial.printf("DESTINATION: %02d:%02d %s - Sveglia=%s, Ripeti=%s\n",
                destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                alarmDestinationEnabled ? "ON" : "OFF",
                alarmDestinationDailyRepeat ? "ON" : "OFF");
  Serial.printf("LAST DEPARTED: %02d:%02d %s - Sveglia=%s, Ripeti=%s\n",
                lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                alarmLastDepartedEnabled ? "ON" : "OFF",
                alarmLastDepartedDailyRepeat ? "ON" : "OFF");
  Serial.println("========================\n");

  // Prima disattiva la pagina setup
  bttfAlarmSetupActive = false;

  // Feedback sonoro
  playTouchSound();

  // Pulisci schermo e forza ridisegno completo BTTF
  gfx->fillScreen(BLACK);
  bttfNeedsRedraw = true;

  // Reset tracking per forzare ridisegno completo
  lastDisplayedHour = 255;
  lastDisplayedMinute = 255;
  lastDisplayedDay = 255;
  lastDisplayedMonth = 255;
  lastDisplayedYear = 0;

  Serial.println("[BTTF] Ritorno a display BTTF");
}

#endif // EFFECT_BTTF
// ================== GUIDA PERSONALIZZAZIONE DISPLAY BTTF ==================
//
// FONT UTILIZZATI:
// - DS_DIGIB20pt7b - Font DS-Digital Bold 20pt (stile LED display a 7 segmenti)
//
// COORDINATE DA MODIFICARE PER ALLINEARE ALLA TUA SKIN:
//
// Linee 23-25: Baseline Y dei 3 pannelli
//  - BTTF_PANEL1_BASE_Y = 78   (DESTINATION TIME - pannello alto)
//  - BTTF_PANEL2_BASE_Y = 248  (PRESENT TIME - pannello centro)
//  - BTTF_PANEL3_BASE_Y = 406  (LAST TIME DEPARTED - pannello basso)
//
// Linee 32-57: Coordinate X COMPLETAMENTE SEPARATE per TUTTI i campi di ogni pannello
// Ogni pannello ha controllo completo e indipendente della posizione orizzontale di ogni elemento!
//
//  PANNELLO 1 (DESTINATION TIME - rosso):
//    - BTTF_PANEL1_MONTH_X = 35   (mese 3 lettere)
//    - BTTF_PANEL1_DAY_X = 133    (giorno 2 cifre)
//    - BTTF_PANEL1_YEAR_X = 200   (anno 4 cifre)
//    - BTTF_PANEL1_AMPM_X = 300   (spia LED AM/PM)
//    - BTTF_PANEL1_HOUR_X = 335   (ore 2 cifre)
//    - BTTF_PANEL1_COLON_X = 395  (due punti ":")
//    - BTTF_PANEL1_MIN_X = 420    (minuti 2 cifre)
//
//  PANNELLO 2 (PRESENT TIME - verde):
//    - BTTF_PANEL2_MONTH_X = 35   (mese 3 lettere)
//    - BTTF_PANEL2_DAY_X = 133    (giorno 2 cifre)
//    - BTTF_PANEL2_YEAR_X = 200   (anno 4 cifre)
//    - BTTF_PANEL2_AMPM_X = 300   (spia LED AM/PM)
//    - BTTF_PANEL2_HOUR_X = 330   (ore 2 cifre)
//    - BTTF_PANEL2_COLON_X = 390  (due punti ":")
//    - BTTF_PANEL2_MIN_X = 415    (minuti 2 cifre)
//
//  PANNELLO 3 (LAST TIME DEPARTED - ambra):
//    - BTTF_PANEL3_MONTH_X = 35   (mese 3 lettere)
//    - BTTF_PANEL3_DAY_X = 133    (giorno 2 cifre)
//    - BTTF_PANEL3_YEAR_X = 200   (anno 4 cifre)
//    - BTTF_PANEL3_AMPM_X = 300   (spia LED AM/PM)
//    - BTTF_PANEL3_HOUR_X = 330   (ore 2 cifre)
//    - BTTF_PANEL3_COLON_X = 390  (due punti ":")
//    - BTTF_PANEL3_MIN_X = 415    (minuti 2 cifre)
//
// Linee 60-61: Offset Y per spia AM/PM (separati per AM e PM)
//  - BTTF_AM_Y_OFFSET = -20 (offset AM rispetto al baseline Y)
//  - BTTF_PM_Y_OFFSET = +11 (offset PM rispetto al baseline Y)
//
// Linee 63-65: Parametri DUE PUNTI ":" (completamente personalizzabili)
//  - BTTF_COLON_Y_OFFSET_TOP = -15  (offset Y pallino superiore rispetto al baseline)
//  - BTTF_COLON_Y_SPACING = 13      (distanza verticale tra i due pallini)
//  - BTTF_COLON_RADIUS = 3          (raggio dei pallini)
//
// NOTA: I due punti ":" sono sempre visibili su tutti e 3 i pannelli.
// Il lampeggio è stato disabilitato per evitare flickering del display.
//
// ✅ PERSONALIZZAZIONE COMPLETA ✅
// Ogni pannello ha ora coordinate X completamente indipendenti per TUTTI i campi:
// MONTH, DAY, YEAR, AMPM, HOUR, COLON, MIN
// Puoi posizionare ogni elemento singolarmente in orizzontale per ogni pannello!
//
// Per modificare le date via web: http://<IP_ESP32>:8080/bttf
