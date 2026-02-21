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

// ================== TEMA MODERNO - IDENTICO A WEBRADIO ==================
// Sfondo e base
#define RA_BG_COLOR       0x0841  // Blu molto scuro (come WebRadio)
#define RA_BG_DARK        0x0000  // Nero puro per contrasto
#define RA_BG_CARD        0x1082  // Grigio-blu scuro per cards

// Testo
#define RA_TEXT_COLOR     0xFFFF  // Bianco
#define RA_TEXT_DIM       0xB5B6  // Grigio chiaro
#define RA_TEXT_MUTED     0x7BCF  // Grigio medio

// Accenti - Ciano/Turchese moderno (identico a WebRadio)
#define RA_ACCENT_COLOR   0x07FF  // Ciano brillante
#define RA_ACCENT_DARK    0x0575  // Ciano scuro
#define RA_ACCENT_GLOW    0x5FFF  // Ciano chiaro (glow)

// Bottoni (coerente con WebRadio)
#define RA_BUTTON_COLOR   0x2124  // Grigio scuro
#define RA_BUTTON_HOVER   0x3186  // Grigio medio
#define RA_BUTTON_ACTIVE  0x0575  // Ciano scuro (attivo)
#define RA_BUTTON_BORDER  0x4A69  // Grigio bordo
#define RA_BUTTON_SHADOW  0x1082  // Ombra bottone

// Colori speciali
#define RA_ALARM_ON       0x07E0  // Verde (sveglia attiva)
#define RA_ALARM_OFF      0xF800  // Rosso (sveglia spenta)
#define RA_SNOOZE_COLOR   0xFD20  // Arancione (snooze)

// ================== LAYOUT 480x480 - SPAZIATURA CORRETTA ==================
#define RA_HEADER_Y       5       // Header con titolo
#define RA_TOGGLE_Y       55      // Bottone ON/OFF (H=48)
#define RA_TIME_Y         115     // Orario sveglia (H=65)
#define RA_DAYS_Y         190     // Giorni settimana (H=45)
#define RA_STATION_Y      245     // Stazione radio (H=50)
#define RA_VOLUME_Y       305     // Volume (H=45)
#define RA_CONTROLS_Y     360     // TEST/SNOOZE (H=50)
#define RA_CENTER_X       20      // Margine laterale
#define RA_CENTER_W       440     // Larghezza area centrale

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

  // Touch gestito interamente da checkButtons() in 1_TOUCH.ino
  // (evita doppio ts.read() che causa perdita touch events)

  // Aggiorna periodicamente se in snooze (per mostrare countdown)
  if (radioAlarmSnoozed) {
    static uint32_t lastSnoozeUpdate = 0;
    if (millis() - lastSnoozeUpdate > 1000) {
      lastSnoozeUpdate = millis();
      //radioAlarmNeedsRedraw = true;
      drawRAToggleButton();

    }
  }

  // Aggiorna continuamente se la sveglia sta suonando (effetto lampeggiante)
  if (radioAlarmRinging) {
    static uint32_t lastRingingUpdate = 0;
    if (millis() - lastRingingUpdate > 100) {  // 10 fps per animazione fluida
      lastRingingUpdate = millis();
      radioAlarmNeedsRedraw = true;
    }
  }

  // Ridisegna se necessario
  if (radioAlarmNeedsRedraw) {
    drawRadioAlarmUI();
    radioAlarmNeedsRedraw = false;
  }
}

// ================== SCHERMATA ALLARME ATTIVO (FULLSCREEN) ==================
void drawAlarmRingingScreen() {
  // Sfondo rosso/arancione lampeggiante
  static bool flashState = false;
  static uint32_t lastFlash = 0;

  if (millis() - lastFlash > 500) {
    flashState = !flashState;
    lastFlash = millis();
  }

  uint16_t bgColor = flashState ? 0x8000 : 0x4000;  // Rosso scuro alternato
  gfx->fillScreen(bgColor);

  // Bordo arancione brillante
  for (int i = 0; i < 5; i++) {
    gfx->drawRect(i, i, 480 - i*2, 480 - i*2, RA_SNOOZE_COLOR);
  }

  // 1.  CAMPANA E  ONDE 
  int centerX = 240;
  int centerY = 130; 

  gfx->fillCircle(centerX, centerY - 20, 50, RA_TEXT_COLOR);
  gfx->fillRect(centerX - 50, centerY + 25, 100, 20, RA_TEXT_COLOR);
  gfx->fillCircle(centerX, centerY + 55, 15, RA_TEXT_COLOR);

  if (flashState) {
    gfx->drawCircle(centerX, centerY, 70, RA_SNOOZE_COLOR);
    gfx->drawCircle(centerX, centerY, 85, RA_SNOOZE_COLOR);
    gfx->drawCircle(centerX, centerY, 100, RA_SNOOZE_COLOR);
  }

  // 2. SCRITTA SVEGLIA! 
  gfx->setFont(u8g2_font_helvB24_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  gfx->setCursor(167, 260); 
  gfx->print("SVEGLIA!");

  // 3. ORARIO 
  gfx->setFont(u8g2_font_logisoso42_tn);
  gfx->setTextColor(RA_SNOOZE_COLOR);
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", myTZ.hour(), myTZ.minute());
  gfx->setCursor(180, 325); 
  gfx->print(timeStr);

  // 4. TASTO SNOOZE 
  int snzW = 200;
  int snzH = 55;
  int snzX = 240 - snzW / 2;
  int snzY = 345; // Era 355, alzato a 345

  gfx->fillRoundRect(snzX, snzY, snzW, snzH, 12, RA_SNOOZE_COLOR);
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(0x0000); 
  gfx->setCursor(snzX + 53, snzY + 36);
  gfx->print("SNOOZE");

  // 5. ISTRUZIONI 
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  gfx->setCursor(37, 430);
  gfx->print("TOCCA OVUNQUE PER FERMARE");
    
  // Nome stazione - RESTA DOVE ERA (465)
  gfx->setFont(u8g2_font_helvR12_tr);
  gfx->setTextColor(RA_TEXT_DIM);
  String station = getRAStationName(radioAlarm.stationIndex);
  // ... (logica substring)
  gfx->setCursor(240 - (station.length() * 7)/2, 465);
  gfx->print(station);

}

// ================== DISEGNO UI COMPLETA ==================
void drawRadioAlarmUI() {
  // Se la sveglia sta suonando, mostra schermata fullscreen dedicata
  if (radioAlarmRinging) {
    drawAlarmRingingScreen();
    return;
  }

  // Sfondo con gradiente simulato (identico a WebRadio)
  for (int y = 0; y < 480; y += 4) {
    uint16_t shade = (y < 240) ? RA_BG_COLOR : RA_BG_DARK;
    gfx->fillRect(0, y, 480, 4, shade);
  }

  // Linee decorative sottili in alto e in basso
  gfx->drawFastHLine(0, 2, 480, RA_ACCENT_DARK);
  gfx->drawFastHLine(0, 477, 480, RA_ACCENT_DARK);

  drawRAHeader();
  yield();
  drawRAToggleButton();
  yield();
  drawRATimeDisplay();
  yield();
  drawRADaysSelector();
  yield();
  drawRAStationSelector();
  yield();
  drawRAVolumeBar();
  yield();
  drawRAControls();
  drawRAExitButton();
  yield();
}


// ================== TOGGLE ON/OFF - CARD STILE WEBRADIO ==================
void drawRAToggleButton() {
  int y = RA_TOGGLE_Y;
  int btnX = RA_CENTER_X;
  int btnW = RA_CENTER_W;
  int btnH = 48;

  // Colori in base allo stato
  uint16_t bgColor, borderColor, textColor, iconColor;
  if (radioAlarmSnoozed) {
    bgColor = 0x2010;
    borderColor = 0x781F;
    textColor = 0xF81F;
    iconColor = 0x781F;
  } else if (radioAlarm.enabled) {
    bgColor = 0x0320;      // Verde molto scuro
    borderColor = RA_ALARM_ON;
    textColor = RA_ALARM_ON;
    iconColor = RA_ALARM_ON;
  } else {
    bgColor = 0x3000;      // Rosso molto scuro
    borderColor = RA_ALARM_OFF;
    textColor = RA_ALARM_OFF;
    iconColor = RA_ALARM_OFF;
  }

  // Ombra e Card (stile WebRadio)
  gfx->fillRoundRect(btnX + 3, y + 3, btnW, btnH, 10, RA_BUTTON_SHADOW);
  gfx->fillRoundRect(btnX, y, btnW, btnH, 10, bgColor);

  // Bordo con glow
  gfx->drawRoundRect(btnX, y, btnW, btnH, 10, borderColor);
  gfx->drawRoundRect(btnX + 1, y + 1, btnW - 2, btnH - 2, 9, borderColor);

  // Icona campana
  int iconX = btnX + 35;
  int iconY = y + btnH / 2;
  gfx->fillCircle(iconX, iconY - 4, 8, iconColor);
  gfx->fillRect(iconX - 8, iconY + 2, 16, 4, iconColor);
  gfx->fillCircle(iconX, iconY + 9, 3, iconColor);

  if (!radioAlarm.enabled && !radioAlarmSnoozed) {
    // X sulla campana se spenta
    gfx->drawLine(iconX - 10, iconY - 10, iconX + 10, iconY + 10, RA_ALARM_OFF);
    gfx->drawLine(iconX - 9, iconY - 10, iconX + 11, iconY + 10, RA_ALARM_OFF);
  }

  // Testo stato
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(textColor);

  if (radioAlarmSnoozed) {
    gfx->setCursor(btnX + 60, y + 32);
    gfx->print("POSTICIPATA");
    uint32_t remaining = (radioAlarmSnoozeUntil > millis()) ? (radioAlarmSnoozeUntil - millis()) / 1000 : 0;
    gfx->setFont(u8g2_font_helvR12_tr);
    gfx->setTextColor(RA_TEXT_DIM);
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "risveglio tra %d:%02d", (int)(remaining/60), (int)(remaining%60));
    gfx->setCursor(btnX + 200, y + 32);
    gfx->print(timeStr);
  } else if (radioAlarm.enabled) {
    gfx->setCursor(btnX + 60, y + 32);
    gfx->print("SVEGLIA ATTIVA");
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(RA_TEXT_MUTED);
    gfx->setCursor(btnX + 250, y + 32);
    gfx->print("tocca per disattivare");
  } else {
    gfx->setCursor(btnX + 60, y + 32);
    gfx->print("SVEGLIA SPENTA");
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(RA_TEXT_MUTED);
    gfx->setCursor(btnX + 250, y + 32);
    gfx->print("tocca per attivare");
  }
}

// ================== HEADER - STILE WEBRADIO ==================
void drawRAHeader() {
  int centerX = 240;

  // Icona campana stilizzata (sinistra)
  int iconY = RA_HEADER_Y + 22;
  gfx->fillCircle(70, iconY, 12, RA_ACCENT_COLOR);
  gfx->fillRect(58, iconY + 8, 24, 6, RA_ACCENT_COLOR);
  gfx->fillCircle(70, iconY + 18, 4, RA_ACCENT_COLOR);

  // Effetto Glow (ombra)
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(RA_ACCENT_DARK);
  gfx->setCursor(146, RA_HEADER_Y + 32);
  gfx->print("RADIOSVEGLIA");

  // Testo principale
  gfx->setTextColor(RA_ACCENT_COLOR);
  gfx->setCursor(145, RA_HEADER_Y + 31);
  gfx->print("RADIOSVEGLIA");

  // Icona campana stilizzata (destra)
  gfx->fillCircle(410, iconY, 12, RA_ACCENT_COLOR);
  gfx->fillRect(398, iconY + 8, 24, 6, RA_ACCENT_COLOR);
  gfx->fillCircle(410, iconY + 18, 4, RA_ACCENT_COLOR);

  // Linea separatrice tripla (come WebRadio)
  for (int i = 0; i < 3; i++) {
    uint16_t lineColor = (i == 1) ? RA_ACCENT_COLOR : RA_ACCENT_DARK;
    gfx->drawFastHLine(RA_CENTER_X, RA_HEADER_Y + 44 + i, RA_CENTER_W, lineColor);
  }
}

// ================== DISPLAY ORARIO - CARD STILE WEBRADIO ==================
void drawRATimeDisplay() {
  int y = RA_TIME_Y;
  int centerX = 240;

  // Card orario centrata
  int cardW = 260;
  int cardH = 65;
  int cardX = centerX - cardW / 2;

  // Ombra e Card
  gfx->fillRoundRect(cardX + 3, y + 3, cardW, cardH, 12, RA_BUTTON_SHADOW);
  gfx->fillRoundRect(cardX, y, cardW, cardH, 12, RA_BG_CARD);

  // Bordo con glow
  gfx->drawRoundRect(cardX, y, cardW, cardH, 12, RA_ACCENT_DARK);
  gfx->drawRoundRect(cardX + 1, y + 1, cardW - 2, cardH - 2, 11, RA_ACCENT_COLOR);

  // Frecce ora (sinistra del card)
  int hourArrowX = cardX - 25;
  gfx->fillTriangle(hourArrowX, y + 15, hourArrowX - 15, y + 30, hourArrowX + 15, y + 30, RA_ACCENT_COLOR);
  gfx->fillTriangle(hourArrowX, y + 50, hourArrowX - 15, y + 35, hourArrowX + 15, y + 35, RA_ACCENT_COLOR);
  gfx->setFont(u8g2_font_helvR08_tr);
  gfx->setTextColor(RA_TEXT_MUTED);
  gfx->setCursor(hourArrowX - 10, y + 68);
  gfx->print("ORA");
 
  // Orario grande centrato
  gfx->setFont(u8g2_font_logisoso42_tn);
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", radioAlarm.hour, radioAlarm.minute);
  
  // PULIZIA AREA: Disegna un rettangolo pieno per coprire l'orario precedente
  // Usiamo il colore RA_BG_CARD per non far vedere il "buco"
  gfx->fillRect(cardX + 5, y + 5, cardW - 10, cardH - 10, RA_BG_CARD);
  
  gfx->setTextColor(RA_TEXT_COLOR);
  gfx->setCursor(cardX + 70, y + 52); // X centrata 
  gfx->print(timeStr);

  // Frecce minuti (destra del card)
  int minArrowX = cardX + cardW + 25;
  gfx->fillTriangle(minArrowX, y + 15, minArrowX - 15, y + 30, minArrowX + 15, y + 30, RA_ACCENT_COLOR);
  gfx->fillTriangle(minArrowX, y + 50, minArrowX - 15, y + 35, minArrowX + 15, y + 35, RA_ACCENT_COLOR);
  gfx->setFont(u8g2_font_helvR08_tr);
  gfx->setTextColor(RA_TEXT_MUTED);
  gfx->setCursor(minArrowX - 10, y + 68);
  gfx->print("MIN");
}

// ================== SELETTORE GIORNI - STILE WEBRADIO ==================
void drawRADaysSelector() {
  int y = RA_DAYS_Y;
  int centerX = 240;

  // PULIZIA AREA GIORNI: cancella la riga dei giorni prima di ridisegnarli
  // Usiamo un rettangolo nero o del colore del gradiente di quella zona (RA_BG_DARK)
  gfx->fillRect(0, y, 480, 45, RA_BG_DARK); 

  const char* daysFull[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};
  int btnW = 54;
  int btnH = 36;
  int spacing = 8;
  int totalW = 7 * btnW + 6 * spacing;
  int startX = centerX - totalW / 2;

  for (int i = 0; i < 7; i++) {
    int bx = startX + i * (btnW + spacing);
    bool active = (radioAlarm.daysMask & (1 << i)) != 0;

    // Ombra
    if (active) {
      gfx->fillRoundRect(bx + 2, y + 7, btnW, btnH, 6, RA_BUTTON_SHADOW);
    }

    // Bottone
    uint16_t bgCol = active ? RA_ACCENT_DARK : RA_BUTTON_COLOR;
    gfx->fillRoundRect(bx, y + 5, btnW, btnH, 6, bgCol);

    // Bordo
    uint16_t borderCol = active ? RA_ACCENT_COLOR : RA_BUTTON_BORDER;
    gfx->drawRoundRect(bx, y + 5, btnW, btnH, 6, borderCol);

    // Testo giorno
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(active ? RA_TEXT_COLOR : RA_TEXT_MUTED);
    gfx->setCursor(bx + 10, y + 30);
    gfx->print(daysFull[i]);
  }
}

// ================== SELETTORE STAZIONE - STILE WEBRADIO ==================
void drawRAStationSelector() {
  int y = RA_STATION_Y;
  int cardX = RA_CENTER_X;
  int cardW = RA_CENTER_W;
  int cardH = 50;

  // Ombra e Card
  gfx->fillRoundRect(cardX + 3, y + 3, cardW, cardH, 10, RA_BUTTON_SHADOW);
  gfx->fillRoundRect(cardX, y, cardW, cardH, 10, RA_BG_CARD);

  // Bordo con glow
  gfx->drawRoundRect(cardX, y, cardW, cardH, 10, RA_ACCENT_DARK);
  gfx->drawRoundRect(cardX + 1, y + 1, cardW - 2, cardH - 2, 9, RA_ACCENT_COLOR);

  // Icona nota musicale (sinistra)
  int iconX = cardX + 25;
  int iconY = y + cardH / 2;
  gfx->fillCircle(iconX, iconY + 5, 5, RA_ACCENT_COLOR);
  gfx->fillCircle(iconX + 10, iconY + 2, 5, RA_ACCENT_COLOR);
  gfx->fillRect(iconX + 3, iconY - 10, 2, 15, RA_ACCENT_COLOR);
  gfx->fillRect(iconX + 13, iconY - 13, 2, 15, RA_ACCENT_COLOR);
  gfx->fillRect(iconX + 3, iconY - 12, 12, 2, RA_ACCENT_COLOR);

  // Freccia sinistra
  int arrowLX = cardX + 55;
  int arrowY = y + cardH / 2;
  gfx->fillTriangle(arrowLX, arrowY, arrowLX + 12, arrowY - 8, arrowLX + 12, arrowY + 8, RA_ACCENT_COLOR);

  // --- AGGIUNTA PULIZIA NOME STAZIONE ---
  gfx->fillRect(cardX + 70, y + 5, cardW - 140, cardH - 10, RA_BG_CARD);

  // Nome stazione centrato
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  String stationName = getRAStationName(radioAlarm.stationIndex);
  if (stationName.length() > 20) {
    stationName = stationName.substring(0, 17) + "...";
  }
  int textW = stationName.length() * 8;
  gfx->setCursor(cardX + (cardW - textW) / 2, y + 33);
  gfx->print(stationName);

  // Freccia destra
  int arrowRX = cardX + cardW - 55;
  gfx->fillTriangle(arrowRX, arrowY, arrowRX - 12, arrowY - 8, arrowRX - 12, arrowY + 8, RA_ACCENT_COLOR);

  // --- AGGIUNTA PULIZIA NUMERO STAZIONE ---
  gfx->fillRect(cardX + cardW - 45, y + 20, 40, 20, RA_BG_CARD);

  // Numero stazione (a fianco freccia destra)
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(RA_TEXT_MUTED);
  char numStr[16];
  snprintf(numStr, sizeof(numStr), "%d/%d", radioAlarm.stationIndex + 1, webRadioStationCount);
  gfx->setCursor(cardX + cardW - 40, y + 33);
  gfx->print(numStr);
}

// ================== BARRA VOLUME - STILE WEBRADIO ==================
void drawRAVolumeBar() {
  int y = RA_VOLUME_Y;
  int barX = RA_CENTER_X + 50;
  int barW = RA_CENTER_W - 100;
  int barH = 28;

  // Icona speaker (sinistra)
  int spkX = RA_CENTER_X + 20;
  int spkY = y + 14;
  gfx->fillRect(spkX, spkY, 6, 8, RA_ACCENT_COLOR);
  gfx->fillTriangle(spkX + 6, spkY - 4, spkX + 6, spkY + 12, spkX + 14, spkY + 4, RA_ACCENT_COLOR);
  // Onde sonore
  gfx->drawArc(spkX + 16, spkY + 4, 6, 6, 300, 60, RA_ACCENT_COLOR);
  gfx->drawArc(spkX + 16, spkY + 4, 10, 10, 300, 60, RA_ACCENT_DARK);

  // --- AGGIUNTA PULIZIA BARRA E NUMERO ---
  gfx->fillRect(barX, y + 3, barW + 60, barH, RA_BG_DARK); // RA_BG_DARK o il colore dello sfondo generale

  // Barra volume con ombra
  gfx->fillRoundRect(barX + 2, y + 5, barW, barH, 8, RA_BUTTON_SHADOW);
  gfx->fillRoundRect(barX, y + 3, barW, barH, 8, RA_BG_CARD);
  gfx->drawRoundRect(barX, y + 3, barW, barH, 8, RA_ACCENT_DARK);

  // Riempimento gradiente
  int fillW = map(radioAlarm.volume, 0, 100, 0, barW - 8);
  if (fillW > 4) {
    for (int i = 0; i < fillW; i++) {
      uint16_t col = (i < fillW / 3) ? RA_ACCENT_DARK : ((i < fillW * 2 / 3) ? RA_ACCENT_COLOR : RA_ACCENT_GLOW);
      gfx->drawFastVLine(barX + 4 + i, y + 7, barH - 8, col);
    }
  }

  // Knob
  int knobX = barX + 4 + fillW;
  if (knobX < barX + 10) knobX = barX + 10;
  if (knobX > barX + barW - 10) knobX = barX + barW - 10;
  gfx->fillCircle(knobX, y + 3 + barH / 2, 12, RA_ACCENT_COLOR);
  gfx->fillCircle(knobX, y + 3 + barH / 2, 6, RA_TEXT_COLOR);

  // Valore numerico (destra)
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  char volStr[5];
  snprintf(volStr, sizeof(volStr), "%d%%", radioAlarm.volume);
  gfx->setCursor(barX + barW + 15, y + 23);
  gfx->print(volStr);
}

// ================== CONTROLLI - STILE WEBRADIO ==================
void drawRAControls() {
  int y = RA_CONTROLS_Y;
  int centerX = 240;

  // 3 bottoni: TEST | ON/OFF | SNOOZE
  int btnW = 130;
  int btnH = 50;
  int spacing = 15;
  int totalW = 3 * btnW + 2 * spacing;
  int startX = centerX - totalW / 2;

  // ===== Bottone TEST =====
  drawRAModernButton(startX, y, btnW, btnH, false, 0);
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(RA_ACCENT_COLOR);
  gfx->setCursor(startX + 40, y + 33);
  gfx->print("TEST");

  // ===== Bottone STOP (centrale) =====
  int stopX = startX + btnW + spacing;
  drawRAModernButton(stopX, y, btnW, btnH, true, RA_ALARM_OFF);
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(RA_TEXT_COLOR);
  gfx->setCursor(stopX + 40, y + 33);
  gfx->print("STOP");

  // ===== Bottone SNOOZE =====
  int snoozeX = stopX + btnW + spacing;
  drawRAModernButton(snoozeX, y, btnW, btnH, false, 0);
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(RA_SNOOZE_COLOR);
  gfx->setCursor(snoozeX + 30, y + 28);
  gfx->print("SNOOZE");
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(RA_TEXT_MUTED);
  char snzStr[8];
  snprintf(snzStr, sizeof(snzStr), "%d min", radioAlarm.snoozeMinutes);
  gfx->setCursor(snoozeX + 42, y + 44);
  gfx->print(snzStr);
}

// ================== HELPER BOTTONE MODERNO - STILE WEBRADIO ==================
void drawRAModernButton(int bx, int by, int bw, int bh, bool active, uint16_t activeColor) {
  // Ombra
  gfx->fillRoundRect(bx + 2, by + 2, bw, bh, 8, RA_BUTTON_SHADOW);

  // Sfondo
  uint16_t bgColor = active ? activeColor : RA_BUTTON_COLOR;
  gfx->fillRoundRect(bx, by, bw, bh, 8, bgColor);

  // Bordo con glow
  uint16_t borderColor = active ? activeColor : RA_BUTTON_BORDER;
  gfx->drawRoundRect(bx, by, bw, bh, 8, borderColor);
  if (active) {
    gfx->drawRoundRect(bx + 1, by + 1, bw - 2, bh - 2, 7, activeColor);
  }
}

// ================== PULSANTE MODE >> (basso centro) ==================
void drawRAExitButton() {
  gfx->setFont(u8g2_font_helvR08_tr);
  gfx->setTextColor(RA_ACCENT_COLOR);
  gfx->setCursor(210, 479);
  gfx->print("MODE >>");
}

// ================== GESTIONE TOUCH ==================
bool handleRadioAlarmTouch(int16_t x, int16_t y) {
  playTouchSound();

  int centerX = 240;

// 1. GESTIONE TOCCO DURANTE ALLARME (Schermata Rossa)
  if (radioAlarmRinging) {
    // Coordinate del tasto SNOOZE (snzX, snzY, snzW, snzH)
    int snzW = 200;
    int snzH = 55;
    int snzX = 240 - snzW / 2;
    int snzY = 345; 

    // Se il tocco cade dentro il tasto SNOOZE
    if (x >= snzX && x <= snzX + snzW && y >= snzY && y <= snzY + snzH) {
      Serial.println("[RADIO-ALARM] SNOOZE ATTIVATO");
      snoozeRadioAlarm();
    } 
    // Se tocca qualsiasi altra parte dello schermo (STOP)
    else {
      Serial.println("[RADIO-ALARM] STOP DEFINITIVO");
      stopRadioAlarm();
      
    }
    
    radioAlarmNeedsRedraw = true;
    return false;
  }

  // ===== TOGGLE ON/OFF =====
  if (y >= RA_TOGGLE_Y && y <= RA_TOGGLE_Y + 48) {
    if (x >= RA_CENTER_X && x <= RA_CENTER_X + RA_CENTER_W) {
      static uint32_t lastToggleTouch = 0;
      uint32_t now = millis();
      if (now - lastToggleTouch < 700) return false;
      lastToggleTouch = now;

      radioAlarm.enabled = !radioAlarm.enabled;
      saveRadioAlarmSettings();
      Serial.printf("[RADIO-ALARM] Toggle: Sveglia %s\n", radioAlarm.enabled ? "ATTIVATA" : "DISATTIVATA");
      //radioAlarmNeedsRedraw = true;
      drawRAToggleButton();
      return false;
    }
  }

  // ===== ORARIO - Frecce esterne =====
  int timeCardW = 260;
  int timeCardX = centerX - timeCardW / 2;
  if (y >= RA_TIME_Y && y <= RA_TIME_Y + 75) {
    // Frecce ora (sinistra del card)
    int hourArrowX = timeCardX - 25;
    if (x >= hourArrowX - 25 && x <= hourArrowX + 25) {
      if (y < RA_TIME_Y + 35) {
        radioAlarm.hour = (radioAlarm.hour + 1) % 24;
      } else {
        radioAlarm.hour = (radioAlarm.hour == 0) ? 23 : radioAlarm.hour - 1;
      }
      radioAlarm.enabled = false;
      //saveRadioAlarmSettings();
      //radioAlarmNeedsRedraw = true;
      drawRATimeDisplay(); // Ridisegna SUBITO solo la card dell'orario
      drawRAToggleButton(); // Opzionale: aggiorna il tasto in rosso se vuoi vederlo subito
      return false;
    }

    // Frecce minuti (destra del card)
    int minArrowX = timeCardX + timeCardW + 25;
    if (x >= minArrowX - 25 && x <= minArrowX + 25) {
      if (y < RA_TIME_Y + 35) {
        radioAlarm.minute = (radioAlarm.minute + 1) % 60;
      } else {
        radioAlarm.minute = (radioAlarm.minute == 0) ? 59 : radioAlarm.minute - 1;
      }
      radioAlarm.enabled = false;
      //saveRadioAlarmSettings();
      //radioAlarmNeedsRedraw = true;
      drawRATimeDisplay(); // Ridisegna SUBITO solo la card dell'orario
      drawRAToggleButton(); // Opzionale: aggiorna il tasto in rosso se vuoi vederlo subito
      return false;
    }
  }

  // ===== GIORNI =====
  if (y >= RA_DAYS_Y && y <= RA_DAYS_Y + 45) {
    int btnW = 54;
    int spacing = 8;
    int totalW = 7 * btnW + 6 * spacing;
    int startX = centerX - totalW / 2;

    for (int i = 0; i < 7; i++) {
      int bx = startX + i * (btnW + spacing);
      if (x >= bx && x <= bx + btnW) {
        radioAlarm.daysMask ^= (1 << i);
        radioAlarm.enabled = false;
        //saveRadioAlarmSettings();
        //radioAlarmNeedsRedraw = true;
        drawRADaysSelector();   // Ridisegna solo i bottoni dei giorni
        drawRAToggleButton();   // Ridisegna solo il tastone in alto (che ora è rosso)
        return false;
      }
    }
  }

  // ===== STAZIONE =====
  if (y >= RA_STATION_Y && y <= RA_STATION_Y + 50 && webRadioStationCount > 0) {
    // Freccia sinistra
    if (x >= RA_CENTER_X && x <= RA_CENTER_X + 70) {
      if (radioAlarm.stationIndex > 0) {
        radioAlarm.stationIndex--;
      } else {
        radioAlarm.stationIndex = webRadioStationCount - 1;
      }
      //saveRadioAlarmSettings();
      radioAlarm.enabled = false;
      //radioAlarmNeedsRedraw = true;
      drawRAStationSelector(); // Aggiorna solo questa card
      drawRAToggleButton();    // Aggiorna il tasto in alto in ROSSO
      return false;
    }

    // Freccia destra
    if (x >= RA_CENTER_X + RA_CENTER_W - 70 && x <= RA_CENTER_X + RA_CENTER_W) {
      radioAlarm.stationIndex = (radioAlarm.stationIndex + 1) % webRadioStationCount;
      //saveRadioAlarmSettings();
      //radioAlarmNeedsRedraw = true;
      drawRAStationSelector(); // Aggiorna solo questa card
      drawRAToggleButton();    // Aggiorna il tasto in alto in ROSSO
      return false;
    }
  }

  // ===== VOLUME =====
  int volBarX = RA_CENTER_X + 50;
  int volBarW = RA_CENTER_W - 100;
  if (y >= RA_VOLUME_Y && y <= RA_VOLUME_Y + 35) {
    if (x >= volBarX && x <= volBarX + volBarW) {
      int newVol = map(x - volBarX, 0, volBarW, 0, 100);
      newVol = constrain(newVol, 0, 100);
      if (newVol != radioAlarm.volume) {
        radioAlarm.volume = newVol;
        radioAlarm.enabled = false;
        //saveRadioAlarmSettings();
        //radioAlarmNeedsRedraw = true;
        drawRAVolumeBar();     // Aggiorna solo la barra
        drawRAToggleButton();  // Aggiorna il tasto in alto in ROSSO
      }
      return false;
    }
  }

  // ===== CONTROLLI =====
  int ctrlY = RA_CONTROLS_Y;
  int btnW = 130;
  int btnH = 50;
  int spacing = 15;
  int totalW = 3 * btnW + 2 * spacing;
  int startX = centerX - totalW / 2;

  if (y >= ctrlY && y <= ctrlY + btnH) {
    // TEST
    if (x >= startX && x <= startX + btnW) {
      Serial.println("[RADIO-ALARM] Test sveglia");
      triggerRadioAlarm();
      radioAlarmNeedsRedraw = true;
      return false;
    }

    // STOP
    int stopX = startX + btnW + spacing;
    if (x >= stopX && x <= stopX + btnW) {
      Serial.println("[RADIO-ALARM] STOP sveglia");
      stopRadioAlarm();
      radioAlarmNeedsRedraw = true;
      return false;
    }

    // SNOOZE
    int snoozeX = stopX + btnW + spacing;
    if (x >= snoozeX && x <= snoozeX + btnW) {
      // Cambia durata snooze (cicla 5, 10, 15, 20, 30)
      if (radioAlarm.snoozeMinutes < 10) radioAlarm.snoozeMinutes = 10;
      else if (radioAlarm.snoozeMinutes < 15) radioAlarm.snoozeMinutes = 15;
      else if (radioAlarm.snoozeMinutes < 20) radioAlarm.snoozeMinutes = 20;
      else if (radioAlarm.snoozeMinutes < 30) radioAlarm.snoozeMinutes = 30;
      else radioAlarm.snoozeMinutes = 5;
      radioAlarm.enabled = false;
      //saveRadioAlarmSettings();
      radioAlarmNeedsRedraw = true;
      return false;
    }
  }

  // ===== MODE >> (basso centro y>455, x 180-310) =====
  if (y > 455 && x >= 180 && x <= 310) {
    Serial.println("[RADIO-ALARM] MODE >>");
    saveRadioAlarmSettings();
    return true;
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
  //extern void cleanupPreviousMode(DisplayMode);
  //cleanupPreviousMode(currentMode);
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
.time-set{display:flex;justify-content:center;align-items:center;gap:10px;margin:15px 0}
.time-btn{width:55px;height:55px;border-radius:10px;border:2px solid #00d4ff;background:#2a2a4a;color:#00d4ff;font-size:1.8em;cursor:pointer;transition:all .2s}
.time-btn:hover{background:#0099cc;color:#fff}
.time-display{font-size:3.5em;font-weight:bold;color:#00d4ff;padding:0 20px;text-shadow:0 0 20px rgba(0,212,255,.3)}
.days{display:flex;justify-content:center;gap:8px;flex-wrap:wrap}
.day-btn{width:50px;height:45px;border-radius:8px;border:2px solid #555;background:#2a2a4a;color:#888;font-size:0.9em;cursor:pointer;transition:all .2s}
.day-btn.active{background:rgba(0,212,255,.2);border-color:#00d4ff;color:#00d4ff}
.day-btn:hover{border-color:#00d4ff}
.station-select{width:100%;padding:14px;border-radius:10px;border:2px solid #555;background:#2a2a4a;color:#fff;font-size:1em}
.station-select:focus{outline:none;border-color:#00d4ff}
.volume-row{display:flex;align-items:center;gap:15px}
.volume-slider{flex:1;height:10px;-webkit-appearance:none;background:#2a2a4a;border-radius:5px;border:1px solid #555}
.volume-slider::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;background:#00d4ff;border-radius:50%;cursor:pointer;box-shadow:0 0 10px rgba(0,212,255,.5)}
.vol-val{min-width:40px;text-align:center;color:#00d4ff;font-weight:bold;font-size:1.2em}
.btn{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px;transition:all .2s}
.btn:hover{transform:scale(1.02)}
.btn-primary{background:linear-gradient(135deg,#00d4ff,#0099cc);color:#fff}
.btn-test{background:linear-gradient(135deg,#ff9800,#f57c00);color:#fff}
.btn-stop{background:linear-gradient(135deg,#f44336,#d32f2f);color:#fff}
.btn-display{background:linear-gradient(135deg,#9c27b0,#7b1fa2);color:#fff}
.status{text-align:center;padding:15px;background:rgba(0,0,0,.2);border-radius:10px;margin-bottom:15px;font-size:1.1em}
.status.on{color:#4CAF50;border:1px solid #4CAF50;background:rgba(76,175,80,.1)}
.status.off{color:#f44336;border:1px solid #f44336;background:rgba(244,67,54,.1)}
.status.ring{color:#ff9800;border:2px solid #ff9800;background:rgba(255,152,0,.2);animation:pulse 1s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
.power-btn{width:100%;padding:22px;border:none;border-radius:15px;font-weight:bold;cursor:pointer;font-size:1.3em;margin-bottom:20px;display:flex;align-items:center;justify-content:center;gap:15px;transition:all .3s}
.power-btn.off{background:linear-gradient(135deg,#2a2a4a,#1a1a3a);color:#888;border:3px solid #444}
.power-btn.on{background:linear-gradient(135deg,#4CAF50,#388E3C);color:#fff;border:3px solid #4CAF50;box-shadow:0 0 25px rgba(76,175,80,0.4)}
.power-btn:hover{transform:scale(1.02)}
.power-icon{font-size:2em}
.snooze-row{display:flex;align-items:center;gap:10px;margin-top:15px}
.snooze-label{color:#888;font-size:.9em}
.snooze-btns{display:flex;gap:5px}
.snooze-btn{padding:8px 15px;border-radius:8px;border:1px solid #555;background:#2a2a4a;color:#888;cursor:pointer;font-size:.9em}
.snooze-btn.active{background:rgba(0,212,255,.2);border-color:#00d4ff;color:#00d4ff}
</style></head><body><div class="c">
<div class="nav"><a href="/">&larr; Home</a><a href="/settings">Settings &rarr;</a></div>
<div class="h"><h1>)) RADIOSVEGLIA ((</h1><p>Svegliati con la tua stazione preferita</p></div><div class="ct">
<div class="status" id="status">Caricamento...</div>
<button class="power-btn off" id="powerBtn" onclick="toggleAlarm()">
<span class="power-icon">&#x23F0;</span>
<span id="powerText">SVEGLIA OFF</span>
</button>
<div class="s"><h3>Orario Sveglia</h3>
<div class="time-set">
<button class="time-btn" onclick="adj('hour',-1)">-</button>
<span class="time-display" id="timeDisplay">07:00</span>
<button class="time-btn" onclick="adj('hour',1)">+</button>
</div>
<div class="time-set">
<button class="time-btn" onclick="adj('min',-1)">-</button>
<span style="color:#888;font-size:1.1em">Minuti</span>
<button class="time-btn" onclick="adj('min',1)">+</button>
</div>
</div>
<div class="s"><h3>Giorni Attivi</h3>
<div class="days" id="days"></div>
</div>
<div class="s"><h3>Stazione Radio</h3>
<select class="station-select" id="station" onchange="save()"></select>
<div id="noStations" style="display:none;text-align:center;padding:20px;background:rgba(255,152,0,.15);border-radius:10px;border:1px solid #ff9800;">
<p style="font-size:1.1em;margin-bottom:10px;">&#x26A0; Nessuna stazione radio disponibile</p>
<p style="color:#aaa;font-size:.9em;margin-bottom:15px;">Le stazioni WebRadio devono essere aggiunte dalla pagina Settings nella sezione "WebRadio Stations"</p>
<a href="/settings" style="display:inline-block;padding:12px 25px;background:linear-gradient(135deg,#00d4ff,#0099cc);color:#fff;text-decoration:none;border-radius:8px;font-weight:bold;">Vai a Settings</a>
</div>
</div>
<div class="s"><h3>Volume Sveglia</h3>
<div class="volume-row">
<span style="font-size:1.3em">&#x1F508;</span>
<input type="range" class="volume-slider" id="volume" min="0" max="100" onchange="save()">
<span class="vol-val" id="volVal">70</span>
</div>
<div class="snooze-row">
<span class="snooze-label">Snooze:</span>
<div class="snooze-btns" id="snooze"></div>
</div>
</div>
<button class="btn btn-test" onclick="test()">Test Sveglia</button>
<button class="btn btn-stop" onclick="stop()">Ferma Sveglia</button>
<button class="btn btn-snooze" id="snoozeBtn" onclick="doSnooze()" style="display:none;background:linear-gradient(135deg,#ff9800,#f57c00);color:#fff;">Snooze</button>
<button class="btn btn-display" onclick="activate()">Mostra su Display</button>
</div></div><script>
var cfg={hour:7,minute:0,enabled:false,station:0,days:0x3E,volume:70,snooze:10,ringing:false,snoozed:false,snoozeRemaining:0};
var dayNames=['DOM','LUN','MAR','MER','GIO','VEN','SAB'];
var snoozeOpts=[5,10,15,20,30];
function render(){
  document.getElementById('timeDisplay').textContent=String(cfg.hour).padStart(2,'0')+':'+String(cfg.minute).padStart(2,'0');
  document.getElementById('volume').value=cfg.volume;
  document.getElementById('volVal').textContent=cfg.volume;
  var st=document.getElementById('status');
  var snzBtn=document.getElementById('snoozeBtn');
  if(cfg.ringing){
    st.textContent='SVEGLIA IN CORSO!';st.className='status ring';
    snzBtn.style.display='block';
  }else if(cfg.snoozed){
    var m=Math.floor(cfg.snoozeRemaining/60);var s=cfg.snoozeRemaining%60;
    st.textContent='POSTICIPATA - risveglio tra '+m+':'+String(s).padStart(2,'0');
    st.className='status ring';st.style.color='#ff9800';st.style.borderColor='#ff9800';
    snzBtn.style.display='none';
  }else if(cfg.enabled){
    st.textContent='Sveglia ATTIVA';st.className='status on';
    snzBtn.style.display='none';
  }else{
    st.textContent='Sveglia DISATTIVATA';st.className='status off';
    snzBtn.style.display='none';
  }
  var pwrBtn=document.getElementById('powerBtn');
  var pwrTxt=document.getElementById('powerText');
  if(cfg.snoozed){
    pwrBtn.className='power-btn on';pwrBtn.style.background='linear-gradient(135deg,#ff9800,#f57c00)';
    pwrBtn.style.borderColor='#ff9800';pwrTxt.textContent='POSTICIPATA - tocca per annullare';
  }else if(cfg.enabled){
    pwrBtn.className='power-btn on';pwrBtn.style.background='';pwrBtn.style.borderColor='';
    pwrTxt.textContent='SVEGLIA ON - tocca per disattivare';
  }else{
    pwrBtn.className='power-btn off';pwrBtn.style.background='';pwrBtn.style.borderColor='';
    pwrTxt.textContent='SVEGLIA OFF - tocca per attivare';
  }
  var daysHtml='';
  for(var i=0;i<7;i++){
    var active=(cfg.days&(1<<i))?'active':'';
    daysHtml+='<button class="day-btn '+active+'" onclick="toggleDay('+i+')">'+dayNames[i]+'</button>';
  }
  document.getElementById('days').innerHTML=daysHtml;
  var snzHtml='';
  for(var i=0;i<snoozeOpts.length;i++){
    var active=cfg.snooze==snoozeOpts[i]?'active':'';
    snzHtml+='<button class="snooze-btn '+active+'" onclick="setSnooze('+snoozeOpts[i]+')">'+snoozeOpts[i]+' min</button>';
  }
  document.getElementById('snooze').innerHTML=snzHtml;
}
function toggleAlarm(){
  if(cfg.snoozed){fetch('/radioalarm/stop').then(()=>{setTimeout(load,300);});return;}
  cfg.enabled=!cfg.enabled;render();save();
}
function adj(f,d){
  if(f=='hour'){cfg.hour=(cfg.hour+d+24)%24;}
  else{cfg.minute=(cfg.minute+d+60)%60;}
  render();save();
}
function toggleDay(d){cfg.days^=(1<<d);render();save();}
function setSnooze(m){cfg.snooze=m;render();save();}
function save(){
  var selEl=document.getElementById('station');
  if(selEl&&selEl.value)cfg.station=parseInt(selEl.value);
  cfg.volume=parseInt(document.getElementById('volume').value);
  document.getElementById('volVal').textContent=cfg.volume;
  fetch('/radioalarm/save?enabled='+(cfg.enabled?1:0)+'&hour='+cfg.hour+'&minute='+cfg.minute+'&station='+cfg.station+'&days='+cfg.days+'&volume='+cfg.volume+'&snooze='+cfg.snooze);
}
function test(){fetch('/radioalarm/test').then(()=>{setTimeout(load,500);});}
function stop(){fetch('/radioalarm/stop').then(()=>{setTimeout(load,500);});}
function doSnooze(){fetch('/radioalarm/snooze').then(()=>{setTimeout(load,500);});}
function activate(){fetch('/radioalarm/activate');}
function load(){
  fetch('/radioalarm/status').then(r=>r.json()).then(d=>{
    cfg.hour=d.hour;cfg.minute=d.minute;cfg.enabled=d.enabled;cfg.station=d.station;
    cfg.days=d.days;cfg.volume=d.volume;cfg.snooze=d.snooze||10;cfg.ringing=d.ringing;
    cfg.snoozed=d.snoozed||false;cfg.snoozeRemaining=d.snoozeRemaining||0;
    render();
    var sel=document.getElementById('station');
    var noSt=document.getElementById('noStations');
    sel.innerHTML='';
    if(d.stations && d.stations.length>0){
      sel.style.display='block';
      noSt.style.display='none';
      for(var i=0;i<d.stations.length;i++){
        var opt=document.createElement('option');
        opt.value=i;opt.textContent=(i+1)+'. '+d.stations[i];
        if(i==cfg.station)opt.selected=true;
        sel.appendChild(opt);
      }
    }else{
      sel.style.display='none';
      noSt.style.display='block';
    }
  });
}
load();setInterval(load,2000);
</script></body></html>
)rawliteral";

void setup_radioalarm_webserver(AsyncWebServer* server) {
  Serial.println("[RADIO-ALARM-WEB] Configurazione endpoints...");

  // IMPORTANTE: Registrare endpoint specifici PRIMA di quello generico!

  // Status JSON
  server->on("/radioalarm/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Calcola tempo rimanente snooze
    uint32_t snoozeRemaining = 0;
    if (radioAlarmSnoozed && radioAlarmSnoozeUntil > millis()) {
      snoozeRemaining = (radioAlarmSnoozeUntil - millis()) / 1000;
    }

    // Costruisci JSON con buffer
    char jsonBuf[2048];
    int pos = 0;

    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos,
      "{\"enabled\":%s,\"hour\":%d,\"minute\":%d,\"station\":%d,\"days\":%d,\"volume\":%d,\"snooze\":%d,\"ringing\":%s,\"snoozed\":%s,\"snoozeRemaining\":%lu,\"stations\":[",
      radioAlarm.enabled ? "true" : "false",
      radioAlarm.hour,
      radioAlarm.minute,
      radioAlarm.stationIndex,
      radioAlarm.daysMask,
      radioAlarm.volume,
      radioAlarm.snoozeMinutes,
      radioAlarmRinging ? "true" : "false",
      radioAlarmSnoozed ? "true" : "false",
      snoozeRemaining
    );

    #ifdef EFFECT_WEB_RADIO
    for (int i = 0; i < webRadioStationCount && i < 20; i++) {
      if (i > 0) pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, ",");
      // Escape eventuali caratteri problematici nel nome
      String safeName = webRadioStations[i].name;
      safeName.replace("\"", "'");
      pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "\"%s\"", safeName.c_str());
      if (pos >= sizeof(jsonBuf) - 100) break; // Evita overflow
    }
    #endif

    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "]}");

    request->send(200, "application/json", jsonBuf);
  });

  // Save settings (sincronizzato con display)
  server->on("/radioalarm/save", HTTP_GET, [](AsyncWebServerRequest *request){
    bool changed = false;

    if (request->hasParam("enabled")) {
      bool newEnabled = request->getParam("enabled")->value().toInt() == 1;
      if (radioAlarm.enabled != newEnabled) { radioAlarm.enabled = newEnabled; changed = true; }
    }
    if (request->hasParam("hour")) {
      uint8_t newHour = request->getParam("hour")->value().toInt();
      if (radioAlarm.hour != newHour) { radioAlarm.hour = newHour; changed = true; }
    }
    if (request->hasParam("minute")) {
      uint8_t newMinute = request->getParam("minute")->value().toInt();
      if (radioAlarm.minute != newMinute) { radioAlarm.minute = newMinute; changed = true; }
    }
    if (request->hasParam("station")) {
      uint8_t newStation = request->getParam("station")->value().toInt();
      if (radioAlarm.stationIndex != newStation) { radioAlarm.stationIndex = newStation; changed = true; }
    }
    if (request->hasParam("days")) {
      uint8_t newDays = request->getParam("days")->value().toInt();
      if (radioAlarm.daysMask != newDays) { radioAlarm.daysMask = newDays; changed = true; }
    }
    if (request->hasParam("volume")) {
      uint8_t newVolume = request->getParam("volume")->value().toInt();
      if (radioAlarm.volume != newVolume) { radioAlarm.volume = newVolume; changed = true; }
    }
    if (request->hasParam("snooze")) {
      uint8_t newSnooze = request->getParam("snooze")->value().toInt();
      if (radioAlarm.snoozeMinutes != newSnooze) { radioAlarm.snoozeMinutes = newSnooze; changed = true; }
    }

    if (changed) {
      saveRadioAlarmSettings();
      // Forza ridisegno display se siamo in modalità Radio Alarm
      if (currentMode == MODE_RADIO_ALARM) {
        radioAlarmNeedsRedraw = true;
      }
      Serial.println("[RADIO-ALARM-WEB] Impostazioni aggiornate da web");
    }

    request->send(200, "application/json", "{\"success\":true}");
  });

  // Test alarm
  server->on("/radioalarm/test", HTTP_GET, [](AsyncWebServerRequest *request){
    triggerRadioAlarm();
    Serial.println("[RADIO-ALARM-WEB] Test sveglia da web");
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Stop alarm
  server->on("/radioalarm/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    stopRadioAlarm();
    // Forza ridisegno display
    if (currentMode == MODE_RADIO_ALARM) {
      radioAlarmNeedsRedraw = true;
    }
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Snooze alarm
  server->on("/radioalarm/snooze", HTTP_GET, [](AsyncWebServerRequest *request){
    if (radioAlarmRinging) {
      snoozeRadioAlarm();
      Serial.println("[RADIO-ALARM-WEB] Snooze attivato da web");
    }
    // Forza ridisegno display
    if (currentMode == MODE_RADIO_ALARM) {
      radioAlarmNeedsRedraw = true;
    }
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Activate display mode
  server->on("/radioalarm/activate", HTTP_GET, [](AsyncWebServerRequest *request){
    currentMode = MODE_RADIO_ALARM;
    radioAlarmNeedsRedraw = true;
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Pagina HTML - DEVE essere registrata DOPO gli endpoint specifici!
  server->on("/radioalarm", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", RADIO_ALARM_HTML);
  });

  Serial.println("[RADIO-ALARM-WEB] Endpoints configurati su /radioalarm");
}

#endif // EFFECT_RADIO_ALARM
