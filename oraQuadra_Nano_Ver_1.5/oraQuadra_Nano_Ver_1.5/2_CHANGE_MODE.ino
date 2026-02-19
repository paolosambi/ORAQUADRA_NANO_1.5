//===================================================================//
//           CLEANUP RISORSE MODALITA' PRECEDENTE                     //
//===================================================================//
// Chiamata PRIMA di cambiare pagina per liberare memoria e risorse
void cleanupPreviousMode(DisplayMode previousMode) {
  Serial.printf("[CLEANUP] Pulizia risorse modalità %d...\n", previousMode);

  // Accesso all'oggetto audio globale
  #ifdef AUDIO
  extern Audio audio;
  #endif

  //-------------------------------------------------------------------
  // 1. STOP AUDIO E CLEANUP COMPLETO (MP3 Player, Web Radio, Radio Alarm)
  //-------------------------------------------------------------------
#ifdef EFFECT_MP3_PLAYER
  if (previousMode == MODE_MP3_PLAYER) {
    #ifdef AUDIO
    // Ferma l'audio - stopSong() interrompe qualsiasi riproduzione
    audio.stopSong();
    Serial.println("[CLEANUP] MP3 Player: audio.stopSong() chiamato");
    #endif
    // NON chiamare cleanupMP3Player() - libererebbe memoria causando crash
    // Il flag verrà resettato così alla prossima apertura si reinizializza
    mp3PlayerInitialized = false;
    Serial.println("[CLEANUP] MP3 Player: audio fermato");
  }
#endif

#ifdef EFFECT_WEB_RADIO
  if (previousMode == MODE_WEB_RADIO) {
    #ifdef AUDIO
    // Ferma streaming audio direttamente
    audio.stopSong();
    Serial.println("[CLEANUP] Web Radio: audio.stopSong() chiamato");
    #endif
    webRadioEnabled = false;
    webRadioInitialized = false;
    webRadioNeedsRedraw = true;
    Serial.println("[CLEANUP] Web Radio: streaming fermato e UI reset");
  }
#endif

#ifdef EFFECT_RADIO_ALARM
  if (previousMode == MODE_RADIO_ALARM) {
    #ifdef AUDIO
    // Ferma audio direttamente
    audio.stopSong();
    Serial.println("[CLEANUP] Radio Alarm: audio.stopSong() chiamato");
    #endif
    webRadioEnabled = false;
    radioAlarmInitialized = false;
    radioAlarmNeedsRedraw = true;
    Serial.println("[CLEANUP] Radio Alarm: audio fermato e UI reset");
  }
#endif

  //-------------------------------------------------------------------
  // 2. CHIUDI STREAMING (MJPEG, ESP32-CAM)
  // NOTA: NON liberare buffer PSRAM - verranno riutilizzati
  //-------------------------------------------------------------------
#ifdef EFFECT_MJPEG_STREAM
  if (previousMode == MODE_MJPEG_STREAM) {
    mjpegDecoder.close();
    mjpegInitialized = false;
    Serial.println("[CLEANUP] MJPEG Stream: decoder chiuso");
  }
#endif

#ifdef EFFECT_ESP32CAM
  if (previousMode == MODE_ESP32CAM) {
    esp32camDecoder.close();
    // NON liberare buffer - verranno riutilizzati alla prossima apertura
    esp32camInitialized = false;
    Serial.println("[CLEANUP] ESP32-CAM: decoder chiuso");
  }
#endif

  //-------------------------------------------------------------------
  // 3. RESET FLAG MODALITA' GRAFICHE (NON liberare buffer PSRAM)
  // I buffer rimangono allocati per riutilizzo - evita crash e riallocazioni
  //-------------------------------------------------------------------
#ifdef EFFECT_BTTF
  if (previousMode == MODE_BTTF) {
    bttfInitialized = false;
    Serial.println("[CLEANUP] BTTF: reset flag");
  }
#endif

#ifdef EFFECT_FLIP_CLOCK
  if (previousMode == MODE_FLIP_CLOCK) {
    flipClockInitialized = false;
    Serial.println("[CLEANUP] FlipClock: reset flag");
  }
#endif

#ifdef EFFECT_LED_RING
  if (previousMode == MODE_LED_RING) {
    ledRingInitialized = false;
    Serial.println("[CLEANUP] LED Ring: reset flag");
  }
#endif

#ifdef EFFECT_CLOCK
  if (previousMode == MODE_CLOCK) {
    clockInitNeeded = true;
    Serial.println("[CLEANUP] Clock: reset flag");
  }
#endif

#ifdef EFFECT_FLUX_CAPACITOR
  if (previousMode == MODE_FLUX_CAPACITOR) {
    fluxCapacitorInitialized = false;
    Serial.println("[CLEANUP] Flux Capacitor: reset flag");
  }
#endif

  //-------------------------------------------------------------------
  // 4. RESET BUFFER STATICI E VARIABILI MODALITA'
  //-------------------------------------------------------------------

  // Matrix
  if (previousMode == MODE_MATRIX || previousMode == MODE_MATRIX2) {
    matrixInitialized = false;
    // Reset drops array
    for (int i = 0; i < NUM_DROPS; i++) {
      drops[i].active = false;
      drops[i].y = 0;
    }
    Serial.println("[CLEANUP] Matrix: drops resettati");
  }

  // Snake
  if (previousMode == MODE_SNAKE) {
    snakeInitNeeded = true;
    // Reset snake trail colors
    memset(snakeTrailColors, 0, sizeof(snakeTrailColors));
    Serial.println("[CLEANUP] Snake: trail colors resettati");
  }

  // Water Drop
  if (previousMode == MODE_WATER) {
    waterDropInitNeeded = true;
    Serial.println("[CLEANUP] Water Drop: reset");
  }

  // Mario
  if (previousMode == MODE_MARIO) {
    marioInitNeeded = true;
    Serial.println("[CLEANUP] Mario: reset");
  }

  // Tron
  if (previousMode == MODE_TRON) {
    tronInitialized = false;
    Serial.println("[CLEANUP] Tron: reset");
  }

#ifdef EFFECT_GALAGA
  if (previousMode == MODE_GALAGA) {
    galagaInitNeeded = true;
    Serial.println("[CLEANUP] Galaga: reset");
  }
#endif

#ifdef EFFECT_GALAGA2
  if (previousMode == MODE_GALAGA2) {
    galaga2InitNeeded = true;
    Serial.println("[CLEANUP] Galaga2: reset");
  }
#endif

#ifdef EFFECT_ANALOG_CLOCK
  if (previousMode == MODE_ANALOG_CLOCK) {
    analogClockInitNeeded = true;
    Serial.println("[CLEANUP] Analog Clock: reset");
  }
#endif

#ifdef EFFECT_WEATHER_STATION
  if (previousMode == MODE_WEATHER_STATION) {
    weatherStationInitialized = false;
    Serial.println("[CLEANUP] Weather Station: reset");
  }
#endif

#ifdef EFFECT_CALENDAR
  if (previousMode == MODE_CALENDAR) {
    calendarStationInitialized = false;
    Serial.println("[CLEANUP] Calendar: reset");
  }
#endif

#ifdef EFFECT_YOUTUBE
  if (previousMode == MODE_YOUTUBE) {
    youtubeInitialized = false;
    Serial.println("[CLEANUP] YouTube: reset");
  }
#endif

#ifdef EFFECT_NEWS
  if (previousMode == MODE_NEWS) {
    newsInitialized = false;
    Serial.println("[CLEANUP] News: reset");
  }
#endif

#ifdef EFFECT_PONG
  if (previousMode == MODE_PONG) {
    pongInitialized = false;
    Serial.println("[CLEANUP] Pong: reset");
  }
#endif

#ifdef EFFECT_SCROLLTEXT
  if (previousMode == MODE_SCROLLTEXT) {
    scrollTextInitialized = false;
    cleanupScrollText();  // Libera canvas double buffer (~460KB PSRAM)
    Serial.println("[CLEANUP] ScrollText: reset + canvas free");
  }
#endif

#ifdef EFFECT_GEMINI_AI
  if (previousMode == MODE_GEMINI_AI) {
    geminiInitialized = false;
    geminiNeedsRedraw = true;
    Serial.println("[CLEANUP] Gemini AI: reset");
  }
#endif

#ifdef EFFECT_CHRISTMAS
  if (previousMode == MODE_CHRISTMAS) {
    christmasInitialized = false;
    Serial.println("[CLEANUP] Christmas: reset");
  }
#endif

#ifdef EFFECT_FIRE
  if (previousMode == MODE_FIRE) {
    fireInitialized = false;
    Serial.println("[CLEANUP] Fire: reset");
  }
#endif

#ifdef EFFECT_FIRE_TEXT
  if (previousMode == MODE_FIRE_TEXT) {
    fireTextInitialized = false;
    Serial.println("[CLEANUP] Fire Text: reset");
  }
#endif

  // Fade/Slow modes
  if (previousMode == MODE_FADE) {
    fadeInitialized = false;
    fadePhase = FADE_SONO_LE;
    fadeStep = 0;
    Serial.println("[CLEANUP] Fade: reset");
  }

  if (previousMode == MODE_SLOW) {
    slowInitialized = false;
    Serial.println("[CLEANUP] Slow: reset");
  }

  //-------------------------------------------------------------------
  // 5. RESET BUFFER DISPLAY COMUNI
  //-------------------------------------------------------------------
  memset(displayBuffer, 0, sizeof(displayBuffer));
  memset(activePixels, 0, sizeof(activePixels));
  memset(targetPixels, 0, sizeof(targetPixels));
  memset(targetPixels_1, 0, sizeof(targetPixels_1));
  memset(targetPixels_2, 0, sizeof(targetPixels_2));
  memset(pixelChanged, 0, sizeof(pixelChanged));

  //-------------------------------------------------------------------
  // 6. FORZA GARBAGE COLLECTION (se disponibile)
  //-------------------------------------------------------------------
  // Stampa memoria libera per debug
  Serial.printf("[CLEANUP] Heap libero: %d bytes, PSRAM libera: %d bytes\n",
                ESP.getFreeHeap(), ESP.getFreePsram());

  Serial.println("[CLEANUP] Pulizia completata.");
}

// Funzione per forzare un aggiornamento completo del display
void forceDisplayUpdate() {
  // Se allarme calendario attivo, ridisegna la schermata allarme e NON il modo corrente
  #ifdef EFFECT_CALENDAR
  extern bool calendarAlarmActive;
  if (calendarAlarmActive) {
    extern void showCalendarAlarmScreen();
    showCalendarAlarmScreen();
    return;
  }
  #endif

  // Cleanup ScrollText double buffer se stiamo uscendo da MODE_SCROLLTEXT
  // (necessario per cambio modo via web che non passa per cleanupPreviousMode)
  #ifdef EFFECT_SCROLLTEXT
  extern uint16_t *scrollFrameBuf;
  extern OffscreenGFX *scrollOffscreen;
  extern bool scrollTextInitialized;
  if (currentMode != MODE_SCROLLTEXT && scrollFrameBuf) {
    scrollTextInitialized = false;
    cleanupScrollText();  // Libera ~460KB PSRAM + reset gfx
    Serial.println("[FORCE_UPDATE] ScrollText buffer liberato");
  }
  #endif

  // Reset stato gfx a default (previene corruzione font da modalità speciali)
  gfx->setFont(u8g2_font_inb21_mr);
  gfx->setTextSize(1);
  gfx->setTextWrap(true);

  // Pulizia completa dello schermo impostando tutti i pixel a nero.
  gfx->fillScreen(BLACK);

  // Reset completo degli array di stato utilizzati per la gestione dei pixel.
  memset(activePixels, 0, sizeof(activePixels));   // Imposta tutti i pixel attivi a false (spenti).
  memset(targetPixels, 0, sizeof(targetPixels));   // Imposta tutti i pixel target a false (nessun cambiamento in corso).
  memset(pixelChanged, 1, sizeof(pixelChanged));   // Imposta tutti i pixel come "cambiati" per forzare un aggiornamento.

  // Aggiornamento specifico in base alla modalità di visualizzazione corrente.
  switch (currentMode) {
    case MODE_MATRIX:
      matrixInitialized = false; // Forza la reinizializzazione della modalità matrix.
      updateMatrix();            // Chiama la funzione per aggiornare la modalità matrix.
      break;
    case MODE_MATRIX2:
      matrixInitialized = false; // Forza la reinizializzazione della modalità matrix continua.
      updateMatrix2();           // Chiama la funzione per aggiornare la modalità matrix continua.
      break;
    case MODE_SNAKE:
      snakeInitNeeded = true;    // Forza la reinizializzazione della modalità snake.
      updateSnake();             // Chiama la funzione per aggiornare la modalità snake.
      break;
    case MODE_WATER:
      waterDropInitNeeded = true; // Forza la reinizializzazione della modalità goccia d'acqua.
      updateWaterDrop();          // Chiama la funzione per aggiornare la modalità goccia d'acqua.
      break;
    case MODE_MARIO:
      marioInitNeeded = true;     // Forza la reinizializzazione della modalità Mario Bros.
      updateMarioMode();          // Chiama la funzione per aggiornare la modalità Mario Bros.
      break;
    case MODE_TRON:
      tronInitialized = false;    // Forza la reinizializzazione della modalità Tron.
      updateTron();               // Chiama la funzione per aggiornare la modalità Tron.
      break;
#ifdef EFFECT_GALAGA
    case MODE_GALAGA:
      galagaInitNeeded = true;    // Forza la reinizializzazione della modalità Galaga.
      updateGalagaMode();         // Chiama la funzione per aggiornare la modalità Galaga.
      break;
#endif
#ifdef EFFECT_GALAGA2
    case MODE_GALAGA2:
      galaga2InitNeeded = true;   // Forza la reinizializzazione della modalità Galaga 2.
      updateGalagaMode2();        // Chiama la funzione per aggiornare la modalità Galaga 2.
      break;
#endif
#ifdef EFFECT_ANALOG_CLOCK
    case MODE_ANALOG_CLOCK:
      analogClockInitNeeded = true; // Forza la reinizializzazione dell'orologio analogico.
      updateAnalogClock();        // Chiama la funzione per aggiornare l'orologio analogico.
      break;
#endif
#ifdef EFFECT_FLIP_CLOCK
    case MODE_FLIP_CLOCK:
      flipClockInitialized = false; // Forza la reinizializzazione del flip clock.
      updateFlipClock();          // Chiama la funzione per aggiornare il flip clock.
      break;
#endif
#ifdef EFFECT_BTTF
    case MODE_BTTF:
      bttfInitialized = false;    // Forza la reinizializzazione della modalità BTTF.
      updateBTTF();               // Chiama la funzione per aggiornare la modalità BTTF.
      break;
#endif
#ifdef EFFECT_LED_RING
    case MODE_LED_RING:
      ledRingInitialized = false; // Forza la reinizializzazione della modalità LED Ring.
      updateLedRingClock();       // Chiama la funzione per aggiornare la modalità LED Ring.
      break;
#endif
#ifdef EFFECT_GEMINI_AI
    case MODE_GEMINI_AI:
      geminiInitialized = false;  // Forza la reinizializzazione della modalità Gemini AI.
      geminiNeedsRedraw = true;   // Forza il ridisegno completo dello schermo.
      displayGeminiMode();        // Chiama la funzione per aggiornare la modalità Gemini AI.
      break;
#endif
#ifdef EFFECT_WEATHER_STATION
    case MODE_WEATHER_STATION:
      weatherStationInitialized = false;  // Forza la reinizializzazione della stazione meteo.
      initWeatherStation();               // Chiama la funzione per inizializzare la stazione meteo.
      break;
#endif
#ifdef EFFECT_CALENDAR
    case MODE_CALENDAR:
      calendarStationInitialized = false;  // Forza la reinizializzazione di calendar.
      initCalendarStation();               // Chiama la funzione per inizializzare il calendario.
      break;
#endif

#ifdef EFFECT_YOUTUBE
    case MODE_YOUTUBE:
      youtubeInitialized = false;          // Forza la reinizializzazione di YouTube Stats.
      initYoutubeStation();                // Chiama la funzione per inizializzare YouTube Stats.
      break;
#endif

#ifdef EFFECT_NEWS
    case MODE_NEWS:
      newsInitialized = false;             // Forza la reinizializzazione di News Feed.
      initNewsStation();                   // Chiama la funzione per inizializzare News Feed.
      break;
#endif

#ifdef EFFECT_CLOCK
    case MODE_CLOCK:
      clockInitNeeded = true;             // Forza la reinizializzazione dell'orologio con skin.
      updateClockMode();                  // Chiama la funzione per aggiornare l'orologio.
      break;
#endif
#ifdef EFFECT_MJPEG_STREAM
    case MODE_MJPEG_STREAM:
      mjpegInitialized = false;           // Forza la reinizializzazione dello streaming MJPEG.
      updateMjpegStream();                // Chiama la funzione per aggiornare lo streaming.
      break;
#endif
#ifdef EFFECT_ESP32CAM
    case MODE_ESP32CAM:
      esp32camInitialized = false;        // Forza la reinizializzazione dello streaming ESP32-CAM.
      updateEsp32camStream();             // Chiama la funzione per aggiornare lo streaming.
      break;
#endif
#ifdef EFFECT_FLUX_CAPACITOR
    case MODE_FLUX_CAPACITOR:
      fluxCapacitorInitialized = false;   // Forza la reinizializzazione del Flux Capacitor.
      updateFluxCapacitor();              // Chiama la funzione per aggiornare il Flux Capacitor.
      break;
#endif
#ifdef EFFECT_CHRISTMAS
    case MODE_CHRISTMAS:
      christmasInitialized = false;       // Forza la reinizializzazione del tema natalizio.
      initChristmas();                    // Chiama la funzione per inizializzare il tema natalizio.
      break;
#endif
#ifdef EFFECT_FIRE
    case MODE_FIRE:
      fireInitialized = false;            // Forza la reinizializzazione dell'effetto fuoco.
      initFire();                         // Chiama la funzione per inizializzare il fuoco.
      break;
#endif
#ifdef EFFECT_FIRE_TEXT
    case MODE_FIRE_TEXT:
      fireTextInitialized = false;        // Forza la reinizializzazione delle lettere fiammeggianti.
      initFireText();                     // Chiama la funzione per inizializzare le lettere fiammeggianti.
      break;
#endif
#ifdef EFFECT_MP3_PLAYER
    case MODE_MP3_PLAYER:
      mp3PlayerInitialized = false;       // Forza la reinizializzazione del lettore MP3.
      initMP3Player();                    // Chiama la funzione per inizializzare il lettore MP3.
      break;
#endif
#ifdef EFFECT_WEB_RADIO
    case MODE_WEB_RADIO:
      webRadioInitialized = false;        // Forza la reinizializzazione della Web Radio.
      webRadioNeedsRedraw = true;         // Forza il ridisegno.
      initWebRadioUI();                   // Chiama la funzione per inizializzare la Web Radio.
      break;
#endif
#ifdef EFFECT_RADIO_ALARM
    case MODE_RADIO_ALARM:
      radioAlarmInitialized = false;      // Forza la reinizializzazione della Radio Alarm.
      radioAlarmNeedsRedraw = true;       // Forza il ridisegno.
      initRadioAlarm();                   // Chiama la funzione per inizializzare la Radio Alarm.
      break;
#endif
    case MODE_FADE:
      fadeInitialized = false;   // Forza la reinizializzazione della modalità dissolvenza.
      updateFadeMode();          // Chiama la funzione per aggiornare la modalità dissolvenza.
      break;
    case MODE_SLOW:
      slowInitialized = false;   // Forza la reinizializzazione della modalità lenta.
      updateSlowMode();          // Chiama la funzione per aggiornare la modalità lenta.
      break;
    case MODE_FAST:
      updateFastMode();          // Chiama la funzione per aggiornare la modalità veloce.
      break;
#ifdef EFFECT_PONG
    case MODE_PONG:
      pongInitialized = false;
      initPong();
      break;
#endif
#ifdef EFFECT_SCROLLTEXT
    case MODE_SCROLLTEXT:
      scrollTextInitialized = false;
      initScrollText();
      break;
#endif
  }
}


void applyPreset(uint8_t preset) {
  // *** CLEANUP DELLA MODALITA' PRECEDENTE ***
  cleanupPreviousMode(currentMode);

  const char* presetName = "";
  const char* title = "PRESET:";
  uint16_t presetColor = WHITE;

  switch (preset) {
    case 0:  // Normale - Solo orario con colori casuali
      presetName = "RANDOM";
      presetColor = WHITE;
      currentMode = (DisplayMode)random(NUM_MODES);           // modo random
      currentColor = Color(random(256), random(256), random(256));  // colore random
      break;
    case 1:  // Veloce con colore acqua
      presetName = "VELOCE ACQUA";
      presetColor = CYAN;
      currentMode = MODE_FAST;
      currentColor = Color(0, 255, 255);
      break;
    case 2: // Lento con colore viola
      presetName = "LENTO VIOLA";
      presetColor = PURPLE;
      currentMode = MODE_SLOW;
      currentColor = Color(255, 0, 255);
      break;
    case 3: // Lento con arancione
      presetName = "LENTO ARANCIONE";
      presetColor = ORANGE;
      currentMode = MODE_SLOW;
      currentColor = Color(255, 165, 0);
      break;
    case 4: // Fade con rosa
      presetName = "FADE ROSSO";
      presetColor = RED;
      currentMode = MODE_FADE;
      currentColor = Color(255, 0, 0);
      break;
    case 5: // Fade con verde
      presetName = "FADE VERDE";
      presetColor = GREEN;
      currentMode = MODE_FADE;
      currentColor = Color(0, 255, 0);
      break;
    case 6: // Fade con colore blu
      presetName = "FADE BLU";
      presetColor = BLUE;
      currentMode = MODE_FADE;
      currentColor = Color(0, 0, 255);
      break;
    case 7: // Matrix con parole gialle
      presetName = "MATRIX GIALLO";
      presetColor = YELLOW;
      currentMode = MODE_MATRIX;
      currentColor = Color(255, 255, 0);
      break;
    case 8: // Matrix con parole ciano
      presetName = "MATRIX CIANO";
      presetColor = CYAN;
      currentMode = MODE_MATRIX;
      currentColor = Color(0, 255, 255);
      break;
    case 9: // Matrix2 con parole verdi
      presetName = "MATRIX CONTINUO VERDE";
      presetColor = GREEN;
      currentMode = MODE_MATRIX2;
      currentColor = Color(0, 255, 0);
      break;
    case 10: // Matrix2 con parole bianche
      presetName = "MATRIX CONTINUO BIANCO";
      presetColor = WHITE;
      currentMode = MODE_MATRIX2;
      currentColor = Color(255, 255, 255);
      break;
    case 11:  // Effetto serpente
      presetName = "SNAKE";
      presetColor = YELLOW;
      currentMode = MODE_SNAKE;
      currentColor = Color(255, 255, 0);
      break;
    case 12:  // Effetto goccia d'acqua
      presetName = "GOCCIA ACQUA";
      presetColor = CYAN;
      currentMode = MODE_WATER;
      currentColor = Color(0, 150, 255);
      break;
    case 13:  // Custom effect
      presetName = "COME PIACE A ME!";
      presetColor = RED;
      currentMode = userMode;
      currentColor = userColor;
      break;
    default:
      presetName = "PRESET DEFAULT";
      presetColor = WHITE;
      currentMode = MODE_FAST;
      currentColor = Color(255, 255, 255);
      break;
  }

  // Mostra il nome del preset
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(presetColor);

  int textWidth;
  int xPos;

  // Calcola la larghezza approssimativa del testo per centrarlo
  textWidth = strlen(title) * 18;
  xPos = (gfx->width() - textWidth) / 2;
  if (xPos < 0) xPos = 0;
  gfx->setCursor(xPos, 210);
  gfx->println(title);

  // Calcola la larghezza approssimativa del testo per centrarlo
  textWidth = strlen(presetName) * 18;
  xPos = (gfx->width() - textWidth) / 2;
  if (xPos < 0) xPos = 0;
  gfx->setCursor(xPos, 240);
  gfx->println(presetName);

  // Pausa per mostrare il nome
  delay(1000);

  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_inb21_mr);
  delay(100);

  EEPROM.write(EEPROM_PRESET_ADDR, preset);

  // Salva il colore corrente in EEPROM
  EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
  EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
  EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);

  // Salva la modalità corrente in EEPROM
  EEPROM.write(EEPROM_MODE_ADDR, currentMode);

  // Sincronizza preset per la modalità corrente (per pagina web)
  saveModePreset((uint8_t)currentMode, preset);

  // NON salvare il colore personalizzato quando si applica un preset predefinito
  // Il colore personalizzato deve cambiare SOLO da color picker o color cycle display
  // Eccezione: preset 13 (personalizzato) usa il colore già salvato

  // Aggiorna anche userColor per coerenza
  userColor = currentColor;

  // Commit dei dati in EEPROM
  EEPROM.commit();

  // Forza ridisegno immediato
  forceDisplayUpdate();
}



// Funzione helper per verificare se un modo è valido
bool isValidMode(DisplayMode mode) {
  switch (mode) {
    case MODE_FADE:
    case MODE_SLOW:
    case MODE_FAST:
    case MODE_MATRIX:
    case MODE_MATRIX2:
    case MODE_SNAKE:
    case MODE_WATER:
    case MODE_MARIO:
    case MODE_TRON:
#ifdef EFFECT_GALAGA
    case MODE_GALAGA:
#endif
#ifdef EFFECT_GALAGA2
    case MODE_GALAGA2:
#endif
#ifdef EFFECT_ANALOG_CLOCK
    case MODE_ANALOG_CLOCK:
#endif
#ifdef EFFECT_FLIP_CLOCK
    case MODE_FLIP_CLOCK:
#endif
#ifdef EFFECT_BTTF
    case MODE_BTTF:
#endif
#ifdef EFFECT_LED_RING
    case MODE_LED_RING:
#endif
#ifdef EFFECT_WEATHER_STATION
    case MODE_WEATHER_STATION:
#endif
#ifdef EFFECT_CALENDAR
    case MODE_CALENDAR:
#endif
#ifdef EFFECT_YOUTUBE
    case MODE_YOUTUBE:
#endif
#ifdef EFFECT_NEWS
    case MODE_NEWS:
#endif
#ifdef EFFECT_CLOCK
    case MODE_CLOCK:
#endif
#ifdef EFFECT_GEMINI_AI
    case MODE_GEMINI_AI:
#endif
#ifdef EFFECT_MJPEG_STREAM
    case MODE_MJPEG_STREAM:
#endif
#ifdef EFFECT_ESP32CAM
    case MODE_ESP32CAM:
#endif
#ifdef EFFECT_FLUX_CAPACITOR
    case MODE_FLUX_CAPACITOR:
#endif
#ifdef EFFECT_CHRISTMAS
    case MODE_CHRISTMAS:
#endif
#ifdef EFFECT_FIRE
    case MODE_FIRE:
#endif
#ifdef EFFECT_FIRE_TEXT
    case MODE_FIRE_TEXT:
#endif
#ifdef EFFECT_MP3_PLAYER
    case MODE_MP3_PLAYER:
#endif
#ifdef EFFECT_WEB_RADIO
    case MODE_WEB_RADIO:
#endif
#ifdef EFFECT_RADIO_ALARM
    case MODE_RADIO_ALARM:
#endif
#ifdef EFFECT_PONG
    case MODE_PONG:
#endif
#ifdef EFFECT_SCROLLTEXT
    case MODE_SCROLLTEXT:
#endif
      return true;
    default:
      return false;
  }
}

void handleModeChange() {
  // ========== VERIFICA PROTEZIONE DISTRIBUITA ==========
  if (!distributedCheck2()) {
    protectionFailCount++;
    if (protectionFailCount >= 5) protectionFailed(0xD2);
  }

  // Salva il mode precedente per cleanup
  DisplayMode previousMode = currentMode;

  // *** CLEANUP DELLA MODALITA' PRECEDENTE ***
  cleanupPreviousMode(previousMode);

  // Passa alla modalità di visualizzazione successiva nel ciclo.
  // Usa getNextEnabledMode che considera sia i modi validi (#ifdef) che quelli abilitati dall'utente
  int attempts = 0;
  DisplayMode startMode = currentMode;
  do {
    currentMode = (DisplayMode)((currentMode + 1) % NUM_MODES);
    attempts++;
    // Controlla che sia valido (compilato con #ifdef) E abilitato dall'utente
  } while ((!isValidMode(currentMode) || !isModeEnabled((uint8_t)currentMode)) && attempts < NUM_MODES);

  // Se nessuna modalità valida trovata, torna a quella di partenza
  if (attempts >= NUM_MODES) {
    currentMode = startMode;
  }

#ifdef EFFECT_CLOCK
  // Skip MODE_CLOCK se BME280 o OpenWeatherMap API non sono disponibili
  // Questo mode richiede entrambi i sensori per funzionare correttamente
  if (currentMode == MODE_CLOCK && !canUseEffectClock()) {
    Serial.println("MODE_CLOCK skippato: BME280 o OpenWeatherMap API non disponibili");
    do {
      currentMode = (DisplayMode)((currentMode + 1) % NUM_MODES);
    } while (!isValidMode(currentMode) || !isModeEnabled((uint8_t)currentMode));
  }
#endif

  userMode = currentMode; // Aggiorna anche la modalità utente.

  // Carica il preset e colore salvati per la nuova modalità (sincronizzato con web)
  uint8_t modePreset = loadModePreset((uint8_t)currentMode);

  // Se c'è un preset salvato per questa modalità, usa quello
  if (modePreset != 255) {
    currentPreset = modePreset;
  } else {
    currentPreset = 13;  // Personalizzato
  }

  // Gestione rainbow mode: attiva/disattiva in base al preset per-modo
  extern bool rainbowModeEnabled;
  if (modePreset == 100) {  // PRESET_RAINBOW
    rainbowModeEnabled = true;
    EEPROM.write(704, 1);  // EEPROM_RAINBOW_MODE_ADDR
  } else {
    rainbowModeEnabled = false;
    EEPROM.write(704, 0);
  }

  // Applica il colore corretto per il preset (o colore per-modo se personalizzato)
  uint8_t modeR, modeG, modeB;
  getColorForPreset(currentPreset, (uint8_t)currentMode, modeR, modeG, modeB);
  currentColor = Color(modeR, modeG, modeB);
  userColor = currentColor;

  // Salva la modalità corrente e colore/preset nella EEPROM
  EEPROM.write(EEPROM_MODE_ADDR, currentMode);
  EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);
  EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
  EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
  EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);

  EEPROM.commit();

#ifdef EFFECT_MJPEG_STREAM
  // Gestione audio streaming: muta quando esci da MJPEG, riattiva quando entri
  if (previousMode == MODE_MJPEG_STREAM && currentMode != MODE_MJPEG_STREAM) {
    // Uscita da MJPEG: muta audio del browser
    setMjpegAudioMute(true);
  } else if (previousMode != MODE_MJPEG_STREAM && currentMode == MODE_MJPEG_STREAM) {
    // Entrata in MJPEG: riattiva audio del browser
    setMjpegAudioMute(false);
  }
#endif

  // Visualizza il nome della modalità corrente sul display.
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);

  // Determina il nome e il colore del testo in base alla modalità corrente.
  const char* title = "MODE:";
  const char* modeName = "";
  uint16_t modeColor = WHITE;

  switch (currentMode) {
    case MODE_FADE:
      modeName = "MODO FADE";
      modeColor = BLUE;
      break;
    case MODE_SLOW:
      modeName = "MODO LENTO";
      modeColor = PURPLE;
      break;
    case MODE_FAST:
      modeName = "MODO VELOCE";
      modeColor = CYAN;
      break;
    case MODE_MATRIX:
      modeName = "MODO MATRIX";
      modeColor = GREEN;
      break;
    case MODE_MATRIX2:
      modeName = "MODO MATRIX 2";
      modeColor = BLUE;
      break;
    case MODE_SNAKE:
      modeName = "MODO SNAKE";
      modeColor = YELLOW;
      break;
    case MODE_WATER:
      modeName = "MODO GOCCIA";
      modeColor = CYAN;
      break;
    case MODE_MARIO:
      modeName = "MODO MARIO";
      modeColor = RED;
      break;
    case MODE_TRON:
      modeName = "MODO TRON";
      modeColor = CYAN;
      break;
#ifdef EFFECT_GALAGA
    case MODE_GALAGA:
      modeName = "MODO GALAGA";
      modeColor = CYAN;  // Ciano brillante
      break;
#endif
#ifdef EFFECT_GALAGA2
    case MODE_GALAGA2:
      modeName = "MODO GALAGA 2";
      modeColor = CYAN;  // Ciano brillante
      break;
#endif
#ifdef EFFECT_ANALOG_CLOCK
    case MODE_ANALOG_CLOCK:
      modeName = "OROLOGIO ANALOGICO";
      modeColor = WHITE;
      break;
#endif
#ifdef EFFECT_FLIP_CLOCK
    case MODE_FLIP_CLOCK:
      modeName = "OROLOGIO A PALETTE";
      modeColor = CYAN;
      break;
#endif
#ifdef EFFECT_BTTF
    case MODE_BTTF:
      modeName = "RITORNO AL FUTURO";
      modeColor = GREEN;
      break;
#endif
#ifdef EFFECT_LED_RING
    case MODE_LED_RING:
      modeName = "OROLOGIO LED RING";
      modeColor = CYAN;
      break;
#endif
#ifdef EFFECT_WEATHER_STATION
    case MODE_WEATHER_STATION:
      modeName = "STAZIONE METEO";
      modeColor = CYAN;
      break;
#endif
#ifdef EFFECT_CALENDAR
    case MODE_CALENDAR:
      modeName = "CALENDARIO";
      modeColor = CYAN;
      break;
#endif
#ifdef EFFECT_YOUTUBE
    case MODE_YOUTUBE:
      modeName = "YOUTUBE";
      modeColor = RED;
      break;
#endif
#ifdef EFFECT_NEWS
    case MODE_NEWS:
      modeName = "NEWS";
      modeColor = CYAN;
      break;
#endif
#ifdef EFFECT_CLOCK
    case MODE_CLOCK:
      modeName = "OROLOGIO SKIN";
      modeColor = YELLOW;  // Giallo oro per effetto lusso
      break;
#endif
#ifdef EFFECT_GEMINI_AI
    case MODE_GEMINI_AI:
      modeName = "GEMINI AI";
      modeColor = BLUE;  // Blu come colore dell'AI
      break;
#endif
#ifdef EFFECT_MJPEG_STREAM
    case MODE_MJPEG_STREAM:
      modeName = "MJPEG STREAM";
      modeColor = MAGENTA;  // Magenta per streaming video
      break;
#endif
#ifdef EFFECT_ESP32CAM
    case MODE_ESP32CAM:
      modeName = "ESP32-CAM";
      modeColor = CYAN;  // Ciano per camera streaming
      break;
#endif
#ifdef EFFECT_FLUX_CAPACITOR
    case MODE_FLUX_CAPACITOR:
      modeName = "FLUX CAPACITOR";
      modeColor = YELLOW;  // Giallo come le luci del flusso canalizzatore
      break;
#endif
#ifdef EFFECT_CHRISTMAS
    case MODE_CHRISTMAS:
      modeName = "BUON NATALE!";
      modeColor = RED;  // Rosso natalizio
      break;
#endif
#ifdef EFFECT_FIRE
    case MODE_FIRE:
      modeName = "FUOCO CAMINO";
      modeColor = ORANGE;  // Arancione fuoco
      break;
#endif
#ifdef EFFECT_FIRE_TEXT
    case MODE_FIRE_TEXT:
      modeName = "LETTERE DI FUOCO";
      modeColor = ORANGE;  // Arancione fuoco
      break;
#endif
#ifdef EFFECT_MP3_PLAYER
    case MODE_MP3_PLAYER:
      modeName = "LETTORE MP3";
      modeColor = CYAN;  // Ciano per il player
      break;
#endif
#ifdef EFFECT_WEB_RADIO
    case MODE_WEB_RADIO:
      modeName = "WEB RADIO";
      modeColor = GREEN;  // Verde per la radio
      break;
#endif
#ifdef EFFECT_RADIO_ALARM
    case MODE_RADIO_ALARM:
      modeName = "RADIO ALARM";
      modeColor = CYAN;  // Ciano per la radiosveglia
      break;
#endif
#ifdef EFFECT_PONG
    case MODE_PONG:
      modeName = "PONG";
      modeColor = WHITE;
      break;
#endif
#ifdef EFFECT_SCROLLTEXT
    case MODE_SCROLLTEXT:
      modeName = "SCROLL TEXT";
      modeColor = GREEN;
      break;
#endif
    default:
      modeName = "MODO SCONOSCIUTO";
      modeColor = WHITE;
      break;
  }

  // Mostra il nome della modalità sul display.
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(modeColor);

  int textWidth;
  int xPos;

  // Calcola la larghezza approssimativa del testo del titolo per centrarlo.
  textWidth = strlen(title) * 18;  // Stima 18 pixel per carattere.
  xPos = (gfx->width() - textWidth) / 2;
  if (xPos < 0) xPos = 0;  // Evita posizioni negative.
  // Mostra il titolo.
  gfx->setCursor(xPos, 210);
  gfx->println(title);

  // Calcola la larghezza approssimativa del testo del nome della modalità per centrarlo.
  textWidth = strlen(modeName) * 18;  // Stima 18 pixel per carattere.
  xPos = (gfx->width() - textWidth) / 2;
  if (xPos < 0) xPos = 0;  // Evita posizioni negative.
  // Mostra il nome della modalità.
  gfx->setCursor(xPos, 240);
  gfx->println(modeName);


  // Attende un secondo per mostrare il nome della modalità all'utente.
  delay(1000);

  // Esegue un reset completo del display quando si cambia modalità.
  gfx->setFont(u8g2_font_inb21_mr);
  gfx->fillScreen(BLACK);

  // Forza un aggiornamento immediato del display per avviare la nuova modalità.
  forceDisplayUpdate();

  // Dual display: invia sync immediato al cambio modo (non aspettare il prossimo ciclo 33ms)
#ifdef EFFECT_DUAL_DISPLAY
  extern bool dualDisplayEnabled;
  extern bool dualDisplayInitialized;
  extern uint8_t panelRole;
  if (dualDisplayEnabled && dualDisplayInitialized && panelRole == 1) {
    extern void sendSyncPacket();
    sendSyncPacket();
  }
#endif

  // Notifica ESP32C3 se entrati/usciti da MODE_GEMINI_AI (già chiamato in forceDisplayUpdate)
  // Nota: forceDisplayUpdate() chiama già notifyGeminiModeChange(), non serve duplicare
}