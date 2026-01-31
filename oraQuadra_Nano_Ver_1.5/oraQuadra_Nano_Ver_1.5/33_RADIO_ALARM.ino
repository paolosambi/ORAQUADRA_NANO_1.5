// ================== RADIO ALARM - RADIOSVEGLIA ==================
// Sveglia con selezione stazione WebRadio
// Tema moderno coerente con WebRadio e MP3 Player

#ifndef EFFECT_RADIO_ALARM
#define EFFECT_RADIO_ALARM
#endif

#ifdef EFFECT_RADIO_ALARM

// ================== EEPROM ADDRESSES ==================
#define EEPROM_ALARM_ENABLED_ADDR    570  // Sveglia attiva (1 byte)
#define EEPROM_ALARM_HOUR_ADDR       571  // Ora sveglia (1 byte)
#define EEPROM_ALARM_MINUTE_ADDR     572  // Minuti sveglia (1 byte)
#define EEPROM_ALARM_STATION_ADDR    573  // Indice stazione radio (1 byte)
#define EEPROM_ALARM_DAYS_ADDR       574  // Giorni attivi bitmask (1 byte)
#define EEPROM_ALARM_VOLUME_ADDR     575  // Volume sveglia (1 byte, 0-21)
#define EEPROM_ALARM_SNOOZE_ADDR     576  // Minuti snooze (1 byte)
#define EEPROM_ALARM_VALID_ADDR      577  // Marker validita' (0xAA)

// ================== EXTERN VARIABILI WEB RADIO ==================
#ifdef EFFECT_WEB_RADIO
extern WebRadioStation webRadioStations[];
extern int webRadioStationCount;
extern bool webRadioEnabled;
extern void startWebRadio();
extern void stopWebRadio();
extern void setWebRadioVolume(uint8_t vol);
extern void selectWebRadioStation(int index);
#else
// Se WEB_RADIO non è abilitato, definisci variabili dummy
int webRadioStationCount = 0;
#endif

// ================== TEMA MODERNO - PALETTE COLORI ==================
// Sfondo e base
#define RA_BG_COLOR       0x0841  // Blu molto scuro
#define RA_BG_DARK        0x0000  // Nero puro
#define RA_BG_CARD        0x1082  // Grigio-blu scuro per cards

// Testo
#define RA_TEXT_COLOR     0xFFFF  // Bianco
#define RA_TEXT_DIM       0xB5B6  // Grigio chiaro
#define RA_TEXT_MUTED     0x7BCF  // Grigio medio

// Accenti - Ciano/Turchese moderno
#define RA_ACCENT_COLOR   0x07FF  // Ciano brillante
#define RA_ACCENT_DARK    0x0575  // Ciano scuro
#define RA_ACCENT_GLOW    0x5FFF  // Ciano chiaro (glow)

// Bottoni
#define RA_BUTTON_COLOR   0x2124  // Grigio scuro
#define RA_BUTTON_HOVER   0x3186  // Grigio medio
#define RA_BUTTON_ACTIVE  0x0575  // Ciano scuro (attivo)
#define RA_BUTTON_BORDER  0x4A69  // Grigio bordo
#define RA_BUTTON_SHADOW  0x1082  // Ombra bottone

// Colori speciali
#define RA_ALARM_ON       0x07E0  // Verde (sveglia attiva)
#define RA_ALARM_OFF      0xF800  // Rosso (sveglia spenta)
#define RA_SNOOZE_COLOR   0xFD20  // Arancione (snooze)

// ================== LAYOUT 480x480 ==================
#define RA_HEADER_Y       5
#define RA_TOGGLE_Y       55      // Grande bottone ON/OFF
#define RA_TIME_Y         125     // Orario
#define RA_DAYS_Y         200     // Giorni
#define RA_STATION_Y      260     // Stazione
#define RA_VOLUME_Y       325     // Volume
#define RA_CONTROLS_Y     375     // TEST e SNOOZE
#define RA_EXIT_Y         435

#define RA_CENTER_X       30
#define RA_CENTER_W       420

// ================== VARIABILI GLOBALI ==================
// (Struttura RadioAlarmSettings definita nel file principale)
RadioAlarmSettings radioAlarm;
bool radioAlarmInitialized = false;
bool radioAlarmNeedsRedraw = true;
bool radioAlarmRinging = false;       // Sveglia sta suonando
bool radioAlarmSnoozed = false;       // In modalita' snooze
uint32_t radioAlarmSnoozeUntil = 0;   // Timestamp fine snooze
uint8_t radioAlarmEditField = 0;      // Campo in modifica (0=ora, 1=min, 2=stazione, 3=volume)

// ================== PROTOTIPI FUNZIONI ==================
void initRadioAlarm();
void updateRadioAlarm();
void drawRadioAlarmUI();
void drawRAHeader();
void drawRAToggleButton();
void drawRATimeDisplay();
void drawRADaysSelector();
void drawRAStationSelector();
void drawRAVolumeBar();
void drawRAControls();
void drawRAExitButton();
void drawRAModernButton(int x, int y, int w, int h, bool active, uint16_t activeColor);
bool handleRadioAlarmTouch(int16_t x, int16_t y);
void saveRadioAlarmSettings();
void loadRadioAlarmSettings();
void checkRadioAlarmTrigger();
void triggerRadioAlarm();
void stopRadioAlarm();
void snoozeRadioAlarm();
String getRAStationName(int index);

// ================== INIZIALIZZAZIONE ==================
void initRadioAlarm() {
  if (radioAlarmInitialized) return;

  Serial.println("[RADIO-ALARM] Inizializzazione...");

  // Carica impostazioni
  loadRadioAlarmSettings();

  radioAlarmRinging = false;
  radioAlarmSnoozed = false;
  radioAlarmEditField = 0;
  radioAlarmNeedsRedraw = true;
  radioAlarmInitialized = true;

  Serial.printf("[RADIO-ALARM] Sveglia: %02d:%02d, Stazione: %d, Attiva: %s\n",
                radioAlarm.hour, radioAlarm.minute, radioAlarm.stationIndex,
                radioAlarm.enabled ? "SI" : "NO");
}

// ================== UPDATE LOOP ==================
void updateRadioAlarm() {
  static int lastActiveMode = -1;

  if (lastActiveMode != MODE_RADIO_ALARM) {
    radioAlarmNeedsRedraw = true;
    radioAlarmInitialized = false;
  }
  lastActiveMode = currentMode;

  if (!radioAlarmInitialized) {
    initRadioAlarm();
    return;
  }

  // Gestione touch
  ts.read();
  if (ts.isTouched && ts.touches > 0) {
    static uint32_t lastTouch = 0;
    uint32_t now = millis();
    if (now - lastTouch > 400) {  // Aumentato da 300 a 400ms
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 479);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 479);

      // Valida coordinate
      x = constrain(x, 0, 479);
      y = constrain(y, 0, 479);

      if (handleRadioAlarmTouch(x, y)) {
        radioAlarmInitialized = false;
        handleModeChange();
        return;
      }
      lastTouch = now;
    }
  }

  // Aggiorna periodicamente se in snooze (per mostrare countdown)
  if (radioAlarmSnoozed) {
    static uint32_t lastSnoozeUpdate = 0;
    if (millis() - lastSnoozeUpdate > 1000) {
      lastSnoozeUpdate = millis();
      radioAlarmNeedsRedraw = true;
    }
  }

  // Ridisegna se necessario
  if (radioAlarmNeedsRedraw) {
    drawRadioAlarmUI();
    radioAlarmNeedsRedraw = false;
  }
}

// ================== DISEGNO UI COMPLETA ==================
void drawRadioAlarmUI() {
  // Sfondo con gradiente
  for (int y = 0; y < 480; y += 4) {
    uint16_t shade = (y < 240) ? RA_BG_COLOR : RA_BG_DARK;
    gfx->fillRect(0, y, 480, 4, shade);
  }

  // Linee decorative
  gfx->drawFastHLine(0, 2, 480, RA_ACCENT_DARK);
  gfx->drawFastHLine(0, 477, 480, RA_ACCENT_DARK);

  drawRAHeader();
  yield();
  drawRAToggleButton();  // Grande bottone ON/OFF
  drawRATimeDisplay();
  yield();
  drawRADaysSelector();
  drawRAStationSelector();
  yield();
  drawRAVolumeBar();
  drawRAControls();
  drawRAExitButton();
  yield();
}

// ================== GRANDE BOTTONE TOGGLE ON/OFF ==================
void drawRAToggleButton() {
  int y = RA_TOGGLE_Y;
  int centerX = 240;
  int btnW = 360;
  int btnH = 55;
  int btnX = centerX - btnW / 2;

  // Colori in base allo stato
  uint16_t bgColor, borderColor, textColor;
  if (radioAlarmRinging) {
    // Sta suonando - arancione
    bgColor = 0x7A00;      // Arancione scuro
    borderColor = RA_SNOOZE_COLOR;
    textColor = RA_TEXT_COLOR;
  } else if (radioAlarmSnoozed) {
    // In snooze - blu/viola
    bgColor = 0x2010;      // Viola scuro
    borderColor = 0x781F;  // Viola
    textColor = 0xF81F;    // Magenta
  } else if (radioAlarm.enabled) {
    bgColor = 0x0400;      // Verde scuro
    borderColor = RA_ALARM_ON;
    textColor = RA_ALARM_ON;
  } else {
    bgColor = 0x4000;      // Rosso scuro
    borderColor = RA_ALARM_OFF;
    textColor = RA_ALARM_OFF;
  }

  // Ombra
  gfx->fillRoundRect(btnX + 3, y + 3, btnW, btnH, 12, RA_BUTTON_SHADOW);

  // Sfondo bottone
  gfx->fillRoundRect(btnX, y, btnW, btnH, 12, bgColor);

  // Bordo
  gfx->drawRoundRect(btnX, y, btnW, btnH, 12, borderColor);
  gfx->drawRoundRect(btnX + 1, y + 1, btnW - 2, btnH - 2, 11, borderColor);

  // Icona campana semplice
  int iconX = btnX + 40;
  int iconY = y + btnH / 2;

  if (radioAlarmRinging) {
    // Campana che suona con onde
    gfx->fillCircle(iconX, iconY - 3, 10, RA_TEXT_COLOR);
    gfx->fillRect(iconX - 10, iconY + 3, 20, 5, RA_TEXT_COLOR);
    gfx->fillCircle(iconX, iconY + 11, 3, RA_TEXT_COLOR);
    // Onde
    gfx->drawCircle(iconX, iconY, 16, RA_TEXT_COLOR);
    gfx->drawCircle(iconX, iconY, 20, RA_TEXT_COLOR);
  } else if (radioAlarmSnoozed) {
    // Campana con Zzz (snooze)
    gfx->fillCircle(iconX, iconY - 3, 10, 0x781F);
    gfx->fillRect(iconX - 10, iconY + 3, 20, 5, 0x781F);
    gfx->fillCircle(iconX, iconY + 11, 3, 0x781F);
    // Zzz
    gfx->setFont(u8g2_font_helvB10_tr);
    gfx->setTextColor(0xF81F);
    gfx->setCursor(iconX + 10, iconY - 8);
    gfx->print("z");
    gfx->setCursor(iconX + 16, iconY - 2);
    gfx->print("Z");
  } else if (radioAlarm.enabled) {
    // Campana attiva
    gfx->fillCircle(iconX, iconY - 3, 10, RA_ALARM_ON);
    gfx->fillRect(iconX - 10, iconY + 3, 20, 5, RA_ALARM_ON);
    gfx->fillCircle(iconX, iconY + 11, 3, RA_ALARM_ON);
  } else {
    // Campana spenta con X
    gfx->drawCircle(iconX, iconY - 3, 10, RA_ALARM_OFF);
    gfx->drawRect(iconX - 10, iconY + 3, 20, 5, RA_ALARM_OFF);
    gfx->drawCircle(iconX, iconY + 11, 3, RA_ALARM_OFF);
    gfx->drawLine(iconX - 12, iconY - 12, iconX + 12, iconY + 12, RA_ALARM_OFF);
    gfx->drawLine(iconX - 11, iconY - 12, iconX + 13, iconY + 12, RA_ALARM_OFF);
  }

  // Testo stato
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(textColor);

  if (radioAlarmRinging) {
    gfx->setCursor(btnX + 70, y + 38);
    gfx->print("SVEGLIA! TOCCA PER FERMARE");
  } else if (radioAlarmSnoozed) {
    gfx->setCursor(btnX + 70, y + 38);
    gfx->print("POSTICIPATA...");
    // Mostra tempo rimanente
    uint32_t remaining = (radioAlarmSnoozeUntil > millis()) ? (radioAlarmSnoozeUntil - millis()) / 1000 : 0;
    int mins = remaining / 60;
    int secs = remaining % 60;
    gfx->setFont(u8g2_font_helvR12_tr);
    gfx->setTextColor(RA_TEXT_MUTED);
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "tra %d:%02d", mins, secs);
    gfx->setCursor(btnX + 200, y + 38);
    gfx->print(timeStr);
  } else if (radioAlarm.enabled) {
    gfx->setCursor(btnX + 70, y + 38);
    gfx->print("SVEGLIA ON");
    gfx->setFont(u8g2_font_helvR12_tr);
    gfx->setTextColor(RA_TEXT_MUTED);
    gfx->setCursor(btnX + 230, y + 38);
    gfx->print("tocca per spegnere");
  } else {
    gfx->setCursor(btnX + 70, y + 38);
    gfx->print("SVEGLIA OFF");
    gfx->setFont(u8g2_font_helvR12_tr);
    gfx->setTextColor(RA_TEXT_MUTED);
    gfx->setCursor(btnX + 235, y + 38);
    gfx->print("tocca per attivare");
  }
}

// ================== HEADER ==================
void drawRAHeader() {
  int centerX = 240;

  // Titolo con icona
  gfx->setFont(u8g2_font_helvB24_tr);

  // Ombra
  gfx->setTextColor(RA_ACCENT_DARK);
  gfx->setCursor(centerX - 115 + 1, RA_HEADER_Y + 32);
  gfx->print("RADIOSVEGLIA");

  // Testo principale
  gfx->setTextColor(RA_ACCENT_COLOR);
  gfx->setCursor(centerX - 115, RA_HEADER_Y + 31);
  gfx->print("RADIOSVEGLIA");

  // Icona campana a destra del titolo (spostata 3cm a destra)
  int iconX = centerX + 165;
  int iconY = RA_HEADER_Y + 15;
  gfx->fillCircle(iconX, iconY, 10, RA_ACCENT_COLOR);
  gfx->fillRect(iconX - 10, iconY + 5, 20, 6, RA_ACCENT_COLOR);
  gfx->fillCircle(iconX, iconY + 15, 4, RA_ACCENT_COLOR);

  // Linea separatrice sottile
  gfx->drawFastHLine(50, RA_HEADER_Y + 42, 380, RA_ACCENT_DARK);
  gfx->drawFastHLine(50, RA_HEADER_Y + 43, 380, RA_ACCENT_COLOR);
}

// ================== DISPLAY ORARIO ==================
void drawRATimeDisplay() {
  int y = RA_TIME_Y;
  int centerX = 240;

  // Label
//  gfx->setFont(u8g2_font_helvB12_tr);
//  gfx->setTextColor(RA_ACCENT_COLOR);
//  gfx->setCursor(RA_CENTER_X, y + 17);
//  gfx->print("ORARIO:");

  // Card orario
  int cardW = 220;
  int cardH = 60;
  int cardX = centerX - cardW / 2;

  gfx->fillRoundRect(cardX + 2, y + 17, cardW, cardH, 12, RA_BUTTON_SHADOW);
  gfx->fillRoundRect(cardX, y + 15, cardW, cardH, 12, RA_BG_CARD);
  gfx->drawRoundRect(cardX, y + 15, cardW, cardH, 12, RA_ACCENT_DARK);

  // Orario grande centrato
  gfx->setFont(u8g2_font_logisoso42_tn);

  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", radioAlarm.hour, radioAlarm.minute);

  // Calcola posizione centrata
  int textW = 130;  // Larghezza approssimativa
  int textX = cardX + (cardW - textW) / 2;

  gfx->setTextColor(RA_TEXT_COLOR);
  gfx->setCursor(textX, y + 62);
  gfx->print(timeStr);

  // Frecce +/- per ora (all'esterno sinistra)
  int hourArrowX = cardX + 20;
  gfx->fillTriangle(hourArrowX, y + 25, hourArrowX - 12, y + 40, hourArrowX + 12, y + 40, RA_ACCENT_COLOR);  // Su
  gfx->fillTriangle(hourArrowX, y + 70, hourArrowX - 12, y + 55, hourArrowX + 12, y + 55, RA_ACCENT_COLOR);  // Giu

  // Frecce +/- per minuti (all'esterno destra)
  int minArrowX = cardX + cardW - 20;
  gfx->fillTriangle(minArrowX, y + 25, minArrowX - 12, y + 40, minArrowX + 12, y + 40, RA_ACCENT_COLOR);  // Su
  gfx->fillTriangle(minArrowX, y + 70, minArrowX - 12, y + 55, minArrowX + 12, y + 55, RA_ACCENT_COLOR);  // Giu
}

// ================== SELETTORE GIORNI ==================
void drawRADaysSelector() {
  int y = RA_DAYS_Y;
  int centerX = 240;

//  gfx->setFont(u8g2_font_helvB12_tr);
//  gfx->setTextColor(RA_ACCENT_COLOR);
//  gfx->setCursor(RA_CENTER_X, y);
//  gfx->print("GIORNI:");

  const char* days[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};
  int btnW = 52;
  int btnH = 32;
  int spacing = 6;
  int totalW = 7 * btnW + 6 * spacing;
  int startX = centerX - totalW / 2;

  for (int i = 0; i < 7; i++) {
    int bx = startX + i * (btnW + spacing);
    bool active = (radioAlarm.daysMask & (1 << i)) != 0;

    // Bottone giorno
    if (active) {
      gfx->fillRoundRect(bx, y + 18, btnW, btnH, 8, RA_ACCENT_DARK);
      gfx->drawRoundRect(bx, y + 18, btnW, btnH, 8, RA_ACCENT_COLOR);
    } else {
      gfx->fillRoundRect(bx, y + 18, btnW, btnH, 8, RA_BUTTON_COLOR);
      gfx->drawRoundRect(bx, y + 18, btnW, btnH, 8, RA_BUTTON_BORDER);
    }

    gfx->setFont(u8g2_font_helvB10_tr);
    gfx->setTextColor(active ? RA_TEXT_COLOR : RA_TEXT_MUTED);
    gfx->setCursor(bx + 8, y + 40);
    gfx->print(days[i]);
  }
}

// ================== SELETTORE STAZIONE ==================
void drawRAStationSelector() {
  int y = RA_STATION_Y;
  int centerX = 240;

//  gfx->setFont(u8g2_font_helvB12_tr);
//  gfx->setTextColor(RA_ACCENT_COLOR);
//  gfx->setCursor(RA_CENTER_X, y);
//  gfx->print("STAZIONE:");

  // Card stazione
  int cardW = 340;
  int cardH = 48;
  int cardX = centerX - cardW / 2;

  gfx->fillRoundRect(cardX + 2, y + 16, cardW, cardH, 10, RA_BUTTON_SHADOW);
  gfx->fillRoundRect(cardX, y + 14, cardW, cardH, 10, RA_BG_CARD);

  bool highlight = (radioAlarmEditField == 2);
  gfx->drawRoundRect(cardX, y + 14, cardW, cardH, 10, highlight ? RA_ACCENT_COLOR : RA_BUTTON_BORDER);

  // Freccia sinistra
  int arrowX = cardX + 20;
  int arrowY = y + 14 + cardH / 2;
  gfx->fillTriangle(arrowX, arrowY, arrowX + 12, arrowY - 10, arrowX + 12, arrowY + 10, RA_ACCENT_COLOR);

  // Nome stazione
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(highlight ? RA_ACCENT_GLOW : RA_TEXT_COLOR);

  String stationName = getRAStationName(radioAlarm.stationIndex);
  if (stationName.length() > 26) {
    stationName = stationName.substring(0, 23) + "...";
  }
  int textW = stationName.length() * 9;
  gfx->setCursor(cardX + (cardW - textW) / 2, y + 45);
  gfx->print(stationName);

  // Freccia destra
  arrowX = cardX + cardW - 20;
  gfx->fillTriangle(arrowX, arrowY, arrowX - 12, arrowY - 10, arrowX - 12, arrowY + 10, RA_ACCENT_COLOR);

  // Numero stazione
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(RA_TEXT_MUTED);
  char numStr[16];
  snprintf(numStr, sizeof(numStr), "%d/%d", radioAlarm.stationIndex + 1, webRadioStationCount);
  gfx->setCursor(cardX + cardW - 45, y + 10);
  gfx->print(numStr);
}

// ================== BARRA VOLUME ==================
void drawRAVolumeBar() {
  int y = RA_VOLUME_Y;
  int barX = 90;
  int barW = 300;
  int barH = 32;

//  gfx->setFont(u8g2_font_helvB12_tr);
//  gfx->setTextColor(RA_ACCENT_COLOR);
//  gfx->setCursor(RA_CENTER_X, y + 12);
//  gfx->print("VOLUME:");

  // Icona speaker
  int spkX = barX - 20;
  int spkY = y + 28;
  gfx->fillRect(spkX - 4, spkY - 3, 6, 6, RA_ACCENT_COLOR);
  gfx->fillTriangle(spkX + 2, spkY - 6, spkX + 2, spkY + 6, spkX + 10, spkY, RA_ACCENT_COLOR);

  // Barra volume
  gfx->fillRoundRect(barX, y + 18, barW, barH, 8, RA_BG_DARK);
  gfx->drawRoundRect(barX, y + 18, barW, barH, 8, RA_BUTTON_BORDER);

  // Riempimento gradiente
  int fillW = map(radioAlarm.volume, 0, 100, 0, barW - 6);
  if (fillW > 4) {
    for (int i = 0; i < fillW; i++) {
      // Gradiente da ciano scuro a ciano chiaro
      uint16_t col = (i < fillW / 3) ? RA_ACCENT_DARK : ((i < fillW * 2 / 3) ? RA_ACCENT_COLOR : RA_ACCENT_GLOW);
      gfx->drawFastVLine(barX + 3 + i, y + 22, barH - 8, col);
    }
  }

  // Knob
  int knobX = barX + 3 + fillW;
  if (knobX < barX + 8) knobX = barX + 8;
  if (knobX > barX + barW - 8) knobX = barX + barW - 8;
  gfx->fillCircle(knobX, y + 18 + barH / 2, 10, RA_ACCENT_COLOR);
  gfx->fillCircle(knobX, y + 18 + barH / 2, 5, RA_TEXT_COLOR);

  // Valore numerico
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  char volStr[4];
  snprintf(volStr, sizeof(volStr), "%d", radioAlarm.volume);
  gfx->setCursor(barX + barW + 15, y + 40);
  gfx->print(volStr);
}

// ================== CONTROLLI ==================
void drawRAControls() {
  int y = RA_CONTROLS_Y;
  int centerX = 240;

  // 3 bottoni: TEST | ON/OFF | SNOOZE
  int btnW = 110;
  int btnH = 55;
  int spacing = 20;
  int totalW = 3 * btnW + 2 * spacing;
  int startX = centerX - totalW / 2;

  // Bottone TEST / STOP (rosso quando suona)
  if (radioAlarmRinging) {
    // Mostra STOP in rosso
    drawRAModernButton(startX, y, btnW, btnH, true, RA_ALARM_OFF);
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(RA_TEXT_COLOR);
    gfx->setCursor(startX + 30, y + 35);
    gfx->print("STOP");
  } else {
    // Mostra TEST normale
    drawRAModernButton(startX, y, btnW, btnH, false, 0);
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(RA_ACCENT_COLOR);
    gfx->setCursor(startX + 30, y + 35);
    gfx->print("TEST");
  }

  // Bottone ON/OFF (centrale, piu' grande)
  int onoffX = startX + btnW + spacing;
  bool isOn = radioAlarm.enabled;
  uint16_t onoffColor = radioAlarmRinging ? RA_ALARM_OFF : (isOn ? RA_ALARM_ON : RA_ALARM_OFF);
  drawRAModernButton(onoffX - 5, y - 5, btnW + 10, btnH + 10, true, onoffColor);

  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  if (radioAlarmRinging) {
    gfx->setCursor(onoffX + 25, y + 38);
    gfx->print("OFF");
  } else {
    gfx->setCursor(onoffX + (isOn ? 35 : 30), y + 38);
    gfx->print(isOn ? "ON" : "OFF");
  }

  // Icona power
  int pwrX = onoffX + 15;
  int pwrY = y + 30;
  gfx->drawCircle(pwrX, pwrY, 8, RA_TEXT_COLOR);
  gfx->fillRect(pwrX - 2, pwrY - 12, 4, 10, RA_TEXT_COLOR);

  // Bottone SNOOZE (evidenziato quando suona)
  int snoozeX = onoffX + btnW + spacing + 10;
  if (radioAlarmRinging) {
    drawRAModernButton(snoozeX, y, btnW, btnH, true, RA_SNOOZE_COLOR);
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(RA_TEXT_COLOR);
  } else {
    drawRAModernButton(snoozeX, y, btnW, btnH, false, 0);
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(RA_SNOOZE_COLOR);
  }
  gfx->setCursor(snoozeX + 10, y + 30);
  gfx->print("RINVIA");
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(radioAlarmRinging ? RA_TEXT_COLOR : RA_TEXT_DIM);
  char snzStr[8];
  snprintf(snzStr, sizeof(snzStr), "%d min", radioAlarm.snoozeMinutes);
  gfx->setCursor(snoozeX + 28, y + 48);
  gfx->print(snzStr);
}

// ================== HELPER BOTTONE MODERNO ==================
void drawRAModernButton(int bx, int by, int bw, int bh, bool active, uint16_t activeColor) {
  gfx->fillRoundRect(bx + 2, by + 2, bw, bh, 12, RA_BUTTON_SHADOW);
  uint16_t bgColor = active ? activeColor : RA_BUTTON_COLOR;
  gfx->fillRoundRect(bx, by, bw, bh, 12, bgColor);
  uint16_t borderColor = active ? activeColor : RA_BUTTON_BORDER;
  gfx->drawRoundRect(bx, by, bw, bh, 12, borderColor);
  if (!active) {
    gfx->drawFastHLine(bx + 8, by + 4, bw - 16, RA_BUTTON_HOVER);
  }
}

// ================== EXIT BUTTON ==================
void drawRAExitButton() {
  int btnW = 120;
  int btnX = 240 - btnW / 2;
  int y = RA_EXIT_Y;
  int btnH = 40;

  // Ombra
  gfx->fillRoundRect(btnX + 2, y + 2, btnW, btnH, 10, 0x4000);

  // Sfondo rosso
  gfx->fillRoundRect(btnX, y, btnW, btnH, 10, RA_ALARM_OFF);
  gfx->drawRoundRect(btnX, y, btnW, btnH, 10, 0x7800);

  // Highlight
  gfx->drawFastHLine(btnX + 10, y + 3, btnW - 20, 0xFC00);

  // Icona X
  int iconX = btnX + 22;
  int iconY = y + btnH / 2;
  gfx->drawLine(iconX - 5, iconY - 5, iconX + 5, iconY + 5, RA_TEXT_COLOR);
  gfx->drawLine(iconX - 5, iconY + 5, iconX + 5, iconY - 5, RA_TEXT_COLOR);
  gfx->drawLine(iconX - 4, iconY - 5, iconX + 6, iconY + 5, RA_TEXT_COLOR);
  gfx->drawLine(iconX - 4, iconY + 5, iconX + 6, iconY - 5, RA_TEXT_COLOR);

  // Testo
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  gfx->setCursor(btnX + 42, y + 27);
  gfx->print("ESCI");
}

// ================== GESTIONE TOUCH ==================
bool handleRadioAlarmTouch(int16_t x, int16_t y) {
  playTouchSound();

  int centerX = 240;

  // ===== GRANDE BOTTONE TOGGLE ON/OFF - debounce lungo =====
  int toggleBtnW = 360;
  int toggleBtnX = centerX - toggleBtnW / 2;  // = 60
  if (y >= RA_TOGGLE_Y && y <= RA_TOGGLE_Y + 55) {
    if (x >= toggleBtnX && x <= toggleBtnX + toggleBtnW) {
      static uint32_t lastToggleTouch = 0;
      uint32_t now = millis();
      if (now - lastToggleTouch < 700) {
        return false;  // Ignora tocco troppo ravvicinato
      }
      lastToggleTouch = now;

      if (radioAlarmRinging) {
        // Se sta suonando, ferma tutto
        Serial.println("[RADIO-ALARM] Toggle: STOP sveglia");
        stopRadioAlarm();
      } else {
        // Altrimenti toggle ON/OFF
        radioAlarm.enabled = !radioAlarm.enabled;
        saveRadioAlarmSettings();
        Serial.printf("[RADIO-ALARM] Toggle: Sveglia %s\n", radioAlarm.enabled ? "ATTIVATA" : "DISATTIVATA");
      }
      radioAlarmNeedsRedraw = true;
      return false;
    }
  }

  // ===== ORARIO - Card =====
  int cardW = 220;
  int cardX = centerX - cardW / 2;
  int cardY = RA_TIME_Y + 15;
  if (y >= cardY && y <= cardY + 60) {
    // Frecce ora all'esterno sinistra
    int hourArrowX = cardX + 20;
    if (x >= hourArrowX - 20 && x <= hourArrowX + 20) {
      if (y < cardY + 35) {
        // Ora +
        radioAlarm.hour = (radioAlarm.hour + 1) % 24;
      } else {
        // Ora -
        radioAlarm.hour = (radioAlarm.hour == 0) ? 23 : radioAlarm.hour - 1;
      }
      saveRadioAlarmSettings();
      radioAlarmNeedsRedraw = true;
      return false;
    }

    // Frecce minuti all'esterno destra
    int minArrowX = cardX + cardW - 20;
    if (x >= minArrowX - 20 && x <= minArrowX + 20) {
      if (y < cardY + 35) {
        // Minuti +
        radioAlarm.minute = (radioAlarm.minute + 1) % 60;
      } else {
        // Minuti -
        radioAlarm.minute = (radioAlarm.minute == 0) ? 59 : radioAlarm.minute - 1;
      }
      saveRadioAlarmSettings();
      radioAlarmNeedsRedraw = true;
      return false;
    }
  }

  // ===== GIORNI =====
  if (y >= RA_DAYS_Y + 18 && y <= RA_DAYS_Y + 50) {
    int btnW = 52;
    int spacing = 6;
    int totalW = 7 * btnW + 6 * spacing;
    int startX = centerX - totalW / 2;

    for (int i = 0; i < 7; i++) {
      int bx = startX + i * (btnW + spacing);
      if (x >= bx && x <= bx + btnW) {
        radioAlarm.daysMask ^= (1 << i);  // Toggle giorno
        saveRadioAlarmSettings();
        radioAlarmNeedsRedraw = true;
        return false;
      }
    }
  }

  // ===== STAZIONE =====
  int stCardW = 340;
  int stCardX = centerX - stCardW / 2;
  if (y >= RA_STATION_Y + 14 && y <= RA_STATION_Y + 62 && webRadioStationCount > 0) {
    radioAlarmEditField = 2;

    // Freccia sinistra - stazione precedente
    if (x >= stCardX && x <= stCardX + 50) {
      if (radioAlarm.stationIndex > 0) {
        radioAlarm.stationIndex--;
      } else {
        radioAlarm.stationIndex = webRadioStationCount - 1;
      }
      saveRadioAlarmSettings();
      radioAlarmNeedsRedraw = true;
      return false;
    }

    // Freccia destra - stazione successiva
    if (x >= stCardX + stCardW - 50 && x <= stCardX + stCardW) {
      radioAlarm.stationIndex = (radioAlarm.stationIndex + 1) % webRadioStationCount;
      saveRadioAlarmSettings();
      radioAlarmNeedsRedraw = true;
      return false;
    }
  }

  // ===== VOLUME =====
  int volBarX = 90;
  int volBarW = 300;
  if (y >= RA_VOLUME_Y + 15 && y <= RA_VOLUME_Y + 55) {
    if (x >= volBarX && x <= volBarX + volBarW) {
      int newVol = map(x - volBarX, 0, volBarW, 0, 100);
      newVol = constrain(newVol, 0, 100);
      if (newVol != radioAlarm.volume) {
        radioAlarm.volume = newVol;
        saveRadioAlarmSettings();
        radioAlarmNeedsRedraw = true;
      }
      return false;
    }
  }

  // ===== CONTROLLI =====
  int ctrlY = RA_CONTROLS_Y;
  int btnW = 110;
  int btnH = 55;
  int spacing = 20;
  int totalW = 3 * btnW + 2 * spacing;
  int startX = centerX - totalW / 2;

  if (y >= ctrlY - 5 && y <= ctrlY + btnH + 10) {
    // TEST / STOP (diventa STOP quando suona) - debounce lungo
    if (x >= startX && x <= startX + btnW) {
      static uint32_t lastTestStopTouch = 0;
      uint32_t now = millis();
      if (now - lastTestStopTouch < 700) {
        return false;  // Ignora tocco troppo ravvicinato
      }
      lastTestStopTouch = now;

      if (radioAlarmRinging) {
        // Se sta suonando, ferma la sveglia (stopRadioAlarm ferma anche la radio)
        Serial.println("[RADIO-ALARM] STOP - Fermo sveglia");
        stopRadioAlarm();
      } else {
        // Altrimenti, test sveglia
        Serial.println("[RADIO-ALARM] Test sveglia");
        triggerRadioAlarm();
      }
      radioAlarmNeedsRedraw = true;
      return false;
    }

    // ON/OFF (ferma anche la sveglia se sta suonando) - debounce lungo
    int onoffX = startX + btnW + spacing;
    if (x >= onoffX - 5 && x <= onoffX + btnW + 15) {
      static uint32_t lastOnOffTouch = 0;
      uint32_t now = millis();
      if (now - lastOnOffTouch < 700) {
        return false;  // Ignora tocco troppo ravvicinato
      }
      lastOnOffTouch = now;

      if (radioAlarmRinging) {
        // Se sta suonando, ferma tutto (stopRadioAlarm ferma anche la radio)
        Serial.println("[RADIO-ALARM] OFF - Fermo sveglia");
        stopRadioAlarm();
        radioAlarm.enabled = false;
      } else {
        radioAlarm.enabled = !radioAlarm.enabled;
      }
      saveRadioAlarmSettings();
      Serial.printf("[RADIO-ALARM] Sveglia %s\n", radioAlarm.enabled ? "ATTIVATA" : "DISATTIVATA");
      radioAlarmNeedsRedraw = true;
      return false;
    }

    // SNOOZE (attiva snooze se sta suonando, altrimenti cambia durata)
    int snoozeX = onoffX + btnW + spacing + 10;
    if (x >= snoozeX && x <= snoozeX + btnW) {
      if (radioAlarmRinging) {
        // Se sta suonando, attiva snooze
        Serial.println("[RADIO-ALARM] SNOOZE attivato");
        snoozeRadioAlarm();
      } else {
        // Altrimenti, cambia durata snooze (cicla 5, 10, 15, 20, 30)
        if (radioAlarm.snoozeMinutes < 10) radioAlarm.snoozeMinutes = 10;
        else if (radioAlarm.snoozeMinutes < 15) radioAlarm.snoozeMinutes = 15;
        else if (radioAlarm.snoozeMinutes < 20) radioAlarm.snoozeMinutes = 20;
        else if (radioAlarm.snoozeMinutes < 30) radioAlarm.snoozeMinutes = 30;
        else radioAlarm.snoozeMinutes = 5;
        saveRadioAlarmSettings();
      }
      radioAlarmNeedsRedraw = true;
      return false;
    }
  }

  // ===== EXIT =====
  int exitBtnW = 120;
  int exitX = centerX - exitBtnW / 2;
  if (y >= RA_EXIT_Y && y <= RA_EXIT_Y + 40) {
    if (x >= exitX && x <= exitX + exitBtnW) {
      Serial.println("[RADIO-ALARM] EXIT");
      return true;
    }
  }

  return false;
}

// ================== SALVATAGGIO/CARICAMENTO ==================
void saveRadioAlarmSettings() {
  EEPROM.write(EEPROM_ALARM_ENABLED_ADDR, radioAlarm.enabled ? 1 : 0);
  EEPROM.write(EEPROM_ALARM_HOUR_ADDR, radioAlarm.hour);
  EEPROM.write(EEPROM_ALARM_MINUTE_ADDR, radioAlarm.minute);
  EEPROM.write(EEPROM_ALARM_STATION_ADDR, radioAlarm.stationIndex);
  EEPROM.write(EEPROM_ALARM_DAYS_ADDR, radioAlarm.daysMask);
  EEPROM.write(EEPROM_ALARM_VOLUME_ADDR, radioAlarm.volume);
  EEPROM.write(EEPROM_ALARM_SNOOZE_ADDR, radioAlarm.snoozeMinutes);
  EEPROM.write(EEPROM_ALARM_VALID_ADDR, 0xAA);
  EEPROM.commit();
  yield();  // Previene watchdog timeout

  Serial.printf("[RADIO-ALARM] Impostazioni salvate: %02d:%02d, Stazione %d, Volume %d\n",
                radioAlarm.hour, radioAlarm.minute, radioAlarm.stationIndex, radioAlarm.volume);
}

void loadRadioAlarmSettings() {
  if (EEPROM.read(EEPROM_ALARM_VALID_ADDR) == 0xAA) {
    radioAlarm.enabled = EEPROM.read(EEPROM_ALARM_ENABLED_ADDR) == 1;
    radioAlarm.hour = EEPROM.read(EEPROM_ALARM_HOUR_ADDR);
    radioAlarm.minute = EEPROM.read(EEPROM_ALARM_MINUTE_ADDR);
    radioAlarm.stationIndex = EEPROM.read(EEPROM_ALARM_STATION_ADDR);
    radioAlarm.daysMask = EEPROM.read(EEPROM_ALARM_DAYS_ADDR);
    radioAlarm.volume = EEPROM.read(EEPROM_ALARM_VOLUME_ADDR);
    radioAlarm.snoozeMinutes = EEPROM.read(EEPROM_ALARM_SNOOZE_ADDR);

    // Validazione
    if (radioAlarm.hour > 23) radioAlarm.hour = 7;
    if (radioAlarm.minute > 59) radioAlarm.minute = 0;
    if (radioAlarm.stationIndex >= webRadioStationCount) radioAlarm.stationIndex = 0;
    if (radioAlarm.volume > 100) radioAlarm.volume = 70;
    if (radioAlarm.snoozeMinutes < 1 || radioAlarm.snoozeMinutes > 60) radioAlarm.snoozeMinutes = 10;
    if (radioAlarm.daysMask == 0) radioAlarm.daysMask = 0x3E; // Lun-Ven default

    Serial.println("[RADIO-ALARM] Impostazioni caricate da EEPROM");
  } else {
    // Default
    radioAlarm.enabled = false;
    radioAlarm.hour = 7;
    radioAlarm.minute = 0;
    radioAlarm.stationIndex = 0;
    radioAlarm.daysMask = 0x3E;  // Lun-Ven (bit 1-5)
    radioAlarm.volume = 70;
    radioAlarm.snoozeMinutes = 10;

    Serial.println("[RADIO-ALARM] Impostazioni default");
  }
}

// ================== CONTROLLO TRIGGER SVEGLIA ==================
void checkRadioAlarmTrigger() {
  if (!radioAlarm.enabled) return;
  if (radioAlarmRinging) return;

  // Controlla snooze - se e' finito, ri-attiva la sveglia
  if (radioAlarmSnoozed) {
    if (millis() < radioAlarmSnoozeUntil) return;
    // Snooze finito, ri-attiva la sveglia!
    Serial.println("[RADIO-ALARM] Snooze terminato, ri-attivo sveglia!");
    radioAlarmSnoozed = false;
    triggerRadioAlarm();
    return;
  }

  // Ottieni ora corrente (usa ora locale, non UTC!)
  int currentHour = myTZ.hour();
  int currentMinute = myTZ.minute();
  int currentSecond = myTZ.second();
  int dayOfWeek = myTZ.weekday() - 1; // ezTime: 1=Dom -> 0=Dom

  // Controlla se e' il giorno giusto
  if (!(radioAlarm.daysMask & (1 << dayOfWeek))) return;

  // Controlla ora (attiva solo nei primi 5 secondi del minuto)
  if (currentHour == radioAlarm.hour && currentMinute == radioAlarm.minute && currentSecond < 5) {
    triggerRadioAlarm();
  }
}

// ================== ATTIVA SVEGLIA ==================
void triggerRadioAlarm() {
  Serial.println("[RADIO-ALARM] SVEGLIA!");
  radioAlarmRinging = true;

  #ifdef EFFECT_WEB_RADIO
  // Verifica che ci siano stazioni disponibili
  if (webRadioStationCount > 0) {
    // Valida indice stazione
    if (radioAlarm.stationIndex >= webRadioStationCount) {
      radioAlarm.stationIndex = 0;
    }

    // Seleziona stazione
    selectWebRadioStation(radioAlarm.stationIndex);

    // Avvia radio SE non attiva
    if (!webRadioEnabled) {
      startWebRadio();
    }

    // Imposta volume sveglia (dopo avvio, NON modifica webRadioVolume)
    extern Audio audio;
    audio.setVolume(map(radioAlarm.volume, 0, 100, 0, 21));  // Converte 0-100 a 0-21
    Serial.printf("[RADIO-ALARM] Volume sveglia: %d%%\n", radioAlarm.volume);
  } else {
    Serial.println("[RADIO-ALARM] Nessuna stazione radio disponibile!");
  }
  #endif

  // Mostra interfaccia sveglia
  currentMode = MODE_RADIO_ALARM;
  radioAlarmNeedsRedraw = true;
}

// ================== FERMA SVEGLIA ==================
void stopRadioAlarm() {
  Serial.println("[RADIO-ALARM] Sveglia fermata");
  radioAlarmRinging = false;
  radioAlarmSnoozed = false;

  // Ferma anche la radio
  #ifdef EFFECT_WEB_RADIO
  if (webRadioEnabled) {
    stopWebRadio();
    Serial.println("[RADIO-ALARM] Radio fermata");
  }
  #endif

  radioAlarmNeedsRedraw = true;
}

// ================== SNOOZE ==================
void snoozeRadioAlarm() {
  Serial.printf("[RADIO-ALARM] Snooze %d minuti\n", radioAlarm.snoozeMinutes);
  radioAlarmRinging = false;
  radioAlarmSnoozed = true;
  radioAlarmSnoozeUntil = millis() + ((uint32_t)radioAlarm.snoozeMinutes * 60UL * 1000UL);

  // Ferma la radio durante lo snooze
  #ifdef EFFECT_WEB_RADIO
  if (webRadioEnabled) {
    stopWebRadio();
    Serial.println("[RADIO-ALARM] Radio fermata per snooze");
  }
  #endif

  radioAlarmNeedsRedraw = true;
}

// ================== HELPER ==================
String getRAStationName(int index) {
  #ifdef EFFECT_WEB_RADIO
  if (index >= 0 && index < webRadioStationCount) {
    return String(webRadioStations[index].name);
  }
  #endif
  return "Nessuna stazione";
}

// ================== WEBSERVER ==================
const char RADIO_ALARM_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Radio Alarm</title><style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}
.c{max-width:500px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}
.h{background:linear-gradient(135deg,#00BCD4,#0097A7);padding:25px;text-align:center}
.h h1{font-size:1.5em;margin-bottom:5px}
.ct{padding:25px}
.s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:15px}
.s h3{margin-bottom:15px;color:#00BCD4}
.time-set{display:flex;justify-content:center;align-items:center;gap:10px;margin:15px 0}
.time-btn{width:50px;height:50px;border-radius:10px;border:none;background:#2a2a4a;color:#fff;font-size:1.5em;cursor:pointer}
.time-btn:hover{background:#3a3a5a}
.time-display{font-size:3em;font-weight:bold;color:#00BCD4;padding:0 15px}
.days{display:flex;justify-content:center;gap:8px;flex-wrap:wrap}
.day-btn{width:45px;height:40px;border-radius:8px;border:2px solid #444;background:#2a2a4a;color:#888;font-size:0.9em;cursor:pointer}
.day-btn.active{background:#0097A7;border-color:#00BCD4;color:#fff}
.station-select{width:100%;padding:12px;border-radius:10px;border:1px solid #444;background:#2a2a4a;color:#fff;font-size:1em}
.volume-row{display:flex;align-items:center;gap:15px}
.volume-slider{flex:1;height:8px;-webkit-appearance:none;background:#2a2a4a;border-radius:4px}
.volume-slider::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;background:#00BCD4;border-radius:50%;cursor:pointer}
.vol-val{min-width:30px;text-align:center;color:#00BCD4;font-weight:bold}
.btn{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px}
.btn-primary{background:linear-gradient(135deg,#00BCD4,#0097A7);color:#fff}
.btn-test{background:#FF9800;color:#fff}
.btn-stop{background:#f44336;color:#fff}
.hm{display:block;text-align:center;color:#94a3b8;padding:10px;text-decoration:none;font-size:.9em}.hm:hover{color:#fff}
.status{text-align:center;padding:15px;background:rgba(0,0,0,.2);border-radius:10px;margin-bottom:15px}
.status.on{color:#4CAF50;font-size:1.2em}.status.off{color:#f44336;font-size:1.2em}
.power-btn{width:100%;padding:20px;border:none;border-radius:15px;font-weight:bold;cursor:pointer;font-size:1.3em;margin-bottom:20px;display:flex;align-items:center;justify-content:center;gap:15px;transition:all 0.3s}
.power-btn.off{background:linear-gradient(135deg,#2a2a4a,#1a1a3a);color:#888;border:3px solid #444}
.power-btn.on{background:linear-gradient(135deg,#4CAF50,#388E3C);color:#fff;border:3px solid #4CAF50;box-shadow:0 0 20px rgba(76,175,80,0.4)}
.power-btn:hover{transform:scale(1.02)}
.power-icon{font-size:1.8em}
.power-text{font-size:1.1em}
</style></head><body><div class="c"><a href="/" class="hm">&larr; Home</a><div class="h">
<h1>⏰ Radio Alarm</h1><p>Wake up with your favorite station</p></div><div class="ct">
<div class="status" id="status">Loading...</div>
<button class="power-btn off" id="powerBtn" onclick="toggleAlarm()">
<span class="power-icon">⏰</span>
<span class="power-text" id="powerText">ALARM OFF</span>
</button>
<div class="s"><h3>Wake Time</h3>
<div class="time-set">
<button class="time-btn" onclick="adj('hour',-1)">-</button>
<span class="time-display" id="timeDisplay">07:00</span>
<button class="time-btn" onclick="adj('hour',1)">+</button>
</div>
<div class="time-set">
<button class="time-btn" onclick="adj('min',-1)">-</button>
<span style="color:#888">Minutes</span>
<button class="time-btn" onclick="adj('min',1)">+</button>
</div>
</div>
<div class="s"><h3>Days</h3>
<div class="days" id="days"></div>
</div>
<div class="s"><h3>Station</h3>
<select class="station-select" id="station" onchange="save()"></select>
</div>
<div class="s"><h3>Volume</h3>
<div class="volume-row">
<input type="range" class="volume-slider" id="volume" min="0" max="21" onchange="save()">
<span class="vol-val" id="volVal">15</span>
</div>
</div>
<button class="btn btn-test" onclick="test()">Test Alarm</button>
<button class="btn btn-stop" onclick="stop()">Stop Alarm</button>
<button class="btn btn-primary" onclick="activate()">Show on Display</button>
</div></div><script>
var cfg={hour:7,minute:0,enabled:false,station:0,days:0x3E,volume:15};
var dayNames=['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
function render(){
  document.getElementById('timeDisplay').textContent=String(cfg.hour).padStart(2,'0')+':'+String(cfg.minute).padStart(2,'0');
  document.getElementById('volume').value=cfg.volume;
  document.getElementById('volVal').textContent=cfg.volume;
  document.getElementById('status').textContent=cfg.enabled?'🔔 Alarm is ACTIVE':'🔕 Alarm is OFF';
  document.getElementById('status').className='status '+(cfg.enabled?'on':'off');
  var pwrBtn=document.getElementById('powerBtn');
  var pwrTxt=document.getElementById('powerText');
  if(cfg.enabled){
    pwrBtn.className='power-btn on';
    pwrTxt.textContent='ALARM ON - TAP TO DISABLE';
  }else{
    pwrBtn.className='power-btn off';
    pwrTxt.textContent='ALARM OFF - TAP TO ENABLE';
  }
  var daysHtml='';
  for(var i=0;i<7;i++){
    var active=(cfg.days&(1<<i))?'active':'';
    daysHtml+='<button class="day-btn '+active+'" onclick="toggleDay('+i+')">'+dayNames[i]+'</button>';
  }
  document.getElementById('days').innerHTML=daysHtml;
}
function toggleAlarm(){
  cfg.enabled=!cfg.enabled;
  render();
  save();
}
function adj(f,d){
  if(f=='hour'){cfg.hour=(cfg.hour+d+24)%24;}
  else{cfg.minute=(cfg.minute+d+60)%60;}
  render();save();
}
function toggleDay(d){cfg.days^=(1<<d);render();save();}
function save(){
  cfg.station=parseInt(document.getElementById('station').value);
  cfg.volume=parseInt(document.getElementById('volume').value);
  document.getElementById('volVal').textContent=cfg.volume;
  fetch('/radioalarm/save?enabled='+(cfg.enabled?1:0)+'&hour='+cfg.hour+'&minute='+cfg.minute+'&station='+cfg.station+'&days='+cfg.days+'&volume='+cfg.volume);
}
function test(){fetch('/radioalarm/test');}
function stop(){fetch('/radioalarm/stop');}
function activate(){fetch('/radioalarm/activate').then(()=>{alert('Radio Alarm mode activated on display!');});}
function load(){
  fetch('/radioalarm/status').then(r=>r.json()).then(d=>{
    cfg=d;render();
    var sel=document.getElementById('station');
    sel.innerHTML='';
    if(d.stations){
      for(var i=0;i<d.stations.length;i++){
        var opt=document.createElement('option');
        opt.value=i;opt.textContent=(i+1)+'. '+d.stations[i];
        if(i==cfg.station)opt.selected=true;
        sel.appendChild(opt);
      }
    }
  });
}
load();
</script></body></html>
)rawliteral";

void setup_radioalarm_webserver(AsyncWebServer* server) {
  Serial.println("[RADIO-ALARM-WEB] Configurazione endpoints...");

  // Pagina HTML
  server->on("/radioalarm", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", RADIO_ALARM_HTML);
  });

  // Status JSON
  server->on("/radioalarm/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"enabled\":" + String(radioAlarm.enabled ? "true" : "false") + ",";
    json += "\"hour\":" + String(radioAlarm.hour) + ",";
    json += "\"minute\":" + String(radioAlarm.minute) + ",";
    json += "\"station\":" + String(radioAlarm.stationIndex) + ",";
    json += "\"days\":" + String(radioAlarm.daysMask) + ",";
    json += "\"volume\":" + String(radioAlarm.volume) + ",";
    json += "\"snooze\":" + String(radioAlarm.snoozeMinutes) + ",";
    json += "\"ringing\":" + String(radioAlarmRinging ? "true" : "false") + ",";
    json += "\"stations\":[";

    #ifdef EFFECT_WEB_RADIO
    for (int i = 0; i < webRadioStationCount && i < 50; i++) {
      if (i > 0) json += ",";
      json += "\"" + String(webRadioStations[i].name) + "\"";
    }
    #endif

    json += "]}";
    request->send(200, "application/json", json);
  });

  // Save settings
  server->on("/radioalarm/save", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("enabled")) {
      radioAlarm.enabled = request->getParam("enabled")->value().toInt() == 1;
    }
    if (request->hasParam("hour")) {
      radioAlarm.hour = request->getParam("hour")->value().toInt();
    }
    if (request->hasParam("minute")) {
      radioAlarm.minute = request->getParam("minute")->value().toInt();
    }
    if (request->hasParam("station")) {
      radioAlarm.stationIndex = request->getParam("station")->value().toInt();
    }
    if (request->hasParam("days")) {
      radioAlarm.daysMask = request->getParam("days")->value().toInt();
    }
    if (request->hasParam("volume")) {
      radioAlarm.volume = request->getParam("volume")->value().toInt();
    }
    saveRadioAlarmSettings();
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Test alarm
  server->on("/radioalarm/test", HTTP_GET, [](AsyncWebServerRequest *request){
    triggerRadioAlarm();
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Stop alarm
  server->on("/radioalarm/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    stopRadioAlarm();
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Activate display mode
  server->on("/radioalarm/activate", HTTP_GET, [](AsyncWebServerRequest *request){
    currentMode = MODE_RADIO_ALARM;
    radioAlarmNeedsRedraw = true;
    forceDisplayUpdate();
    request->send(200, "application/json", "{\"success\":true}");
  });

  Serial.println("[RADIO-ALARM-WEB] Endpoints configurati su /radioalarm");
}

#endif // EFFECT_RADIO_ALARM
