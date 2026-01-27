// ================== GESTIONE TOUCH E PULSANTI ==================

// Dichiarazioni esterne per BTTF alarm setup
#ifdef EFFECT_BTTF
extern bool bttfAlarmSetupActive;
extern void showBTTFAlarmSetup();
extern void closeBTTFAlarmSetup();
extern bool handleBTTFAlarmSetupTouch(int16_t x, int16_t y);
#endif

// Dichiarazioni esterne per Weather Station e magnetometro
#ifdef EFFECT_WEATHER_STATION
extern bool magnetometerConnected;
extern void calibrateMagnetometerGuided();
extern void initWeatherStation();
#endif

void checkButtons() {
  // Variabili statiche per mantenere lo stato tra le diverse chiamate a questa funzione.
  static bool waitingForRelease = false; // Flag che indica se stiamo aspettando che il tocco venga rilasciato.
  static uint32_t touchStartTime = 0;    // Memorizza il timestamp dell'inizio di un tocco prolungato (es. per il reset WiFi).
  static int16_t scrollStartY = -1;     // Memorizza la coordinata Y iniziale di un potenziale scroll nella pagina di setup.
  static bool checkingSetupScroll = false; // Flag che indica se stiamo attualmente rilevando un gesto di scroll per aprire il setup.
  static uint32_t lastCornerCheck = 0;   // Memorizza il timestamp dell'ultimo controllo degli angoli per il reset WiFi.
  static uint8_t cornerTouchLostCounter = 0; // Contatore per tenere traccia di quanti controlli consecutivi degli angoli non hanno rilevato tutti e quattro.
  static uint8_t touchSampleCounter = 0;   // Contatore per gestire la frequenza di campionamento del touch durante l'antirimbalzo.

  // ======= VARIABILI PER LONG PRESS BTTF ALARM SETUP =======
  static bool bttfLongPressActive = false;      // True se stiamo monitorando un long press centrale in BTTF
  static uint32_t bttfCenterTouchStart = 0;     // Timestamp inizio tocco centrale
  static bool bttfLongPressTriggered = false;   // True se abbiamo già triggerato il long press

  uint32_t currentMillis = millis();      // Ottiene il tempo attuale in millisecondi.


  // ====================== GESTIONE RILASCIO TOUCH ======================

  // Reset long press BTTF se non toccato (con lettura fresh del touch)
  #ifdef EFFECT_BTTF
  if (bttfLongPressActive) {
    ts.read();  // Lettura fresh per rilevare rilascio
    if (!ts.isTouched) {
      // Rilascio prima di 2 secondi in modalità BTTF = tocco breve = annuncia ora
      if (!bttfLongPressTriggered && currentMode == MODE_BTTF && !bttfAlarmSetupActive) {
        Serial.println("[BTTF] Rilascio prima di 2 sec - annuncio ora");
        playTouchSound();
        // NON cancelliamo il display - forceDisplayUpdate() gestisce il ridisegno
        // Annuncia ora solo se annuncio orario abilitato
        if (hourlyAnnounceEnabled) {
          announceTimeFixed();
        }
        delay(1000);
        forceDisplayUpdate();
      }
      bttfLongPressActive = false;
      bttfLongPressTriggered = false;
    }
  }
  #endif

  if(waitingForRelease){
    ts.read(); // Legge lo stato attuale del touch screen.
    if (!ts.isTouched) { // Se il touch screen NON è più toccato (rilascio).
      waitingForRelease = false; // Resetta il flag di attesa del rilascio.

      if (colorCycle.isActive) {      // FINE CICLAGGIO COLORI???
        colorCycle.isActive = false;  // Disattiva il ciclo dei colori.
        userColor = currentColor;     // Memorizza l'ultimo colore visualizzato come colore scelto dall'utente.
        currentPreset = 13;           // Imposta preset "Come piace a me" per sincronizzazione web

        // Salva la modalità corrente, il preset (13 indica colore utente) e il colore RGB nella EEPROM.
        EEPROM.write(EEPROM_MODE_ADDR, currentMode);
        EEPROM.write(EEPROM_PRESET_ADDR, 13);
        EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
        EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
        EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);

        // Salva colore e preset per la modalità corrente (sincronizza con pagina web)
        saveModeColor((uint8_t)currentMode, currentColor.r, currentColor.g, currentColor.b);
        saveModePreset((uint8_t)currentMode, 13);  // 13 = Personalizzato

        EEPROM.commit(); // Scrive i dati dalla cache della EEPROM alla memoria fisica.


        // Emette un breve suono di conferma al rilascio del tocco.
        playTouchSound();
        // Forza un aggiornamento completo del display per mostrare il nuovo colore.
        forceDisplayUpdate();
      }
      return; // Esce dalla funzione dopo aver gestito il rilascio.
    }
  }


  // ====================== ANTIRIMBALZO GLOBALE ======================
  static uint32_t lastTouchCheckTime = 0; // Memorizza l'ultimo timestamp in cui è stato controllato il touch.
  touchSampleCounter++; // Incrementa il contatore di campionamento del touch.

  // Implementa un antirimbalzo riducendo la frequenza di polling del touch screen.
  if (currentMillis - lastTouchCheckTime < 50) {
    // Riduce la frequenza di polling ma non la salta completamente.
    if (touchSampleCounter % 3 != 0) {  // Esegue una lettura periodica (ogni 3 campioni) anche durante l'antirimbalzo.
      if (resetCountdownStarted && currentMillis - lastCornerCheck > 200) {
        lastCornerCheck = currentMillis;
        // Se è iniziato il countdown per il reset WiFi, continua a controllare gli angoli.
        ts.read();
      } else {
        return; // Se non è in corso il reset WiFi, esce dalla funzione per evitare letture troppo frequenti.
      }
    }
  }
  lastTouchCheckTime = currentMillis; // Aggiorna il timestamp dell'ultimo controllo del touch.

  // ====================== LETTURA TOUCH ======================
  uint8_t readAttempts = 0;   // Contatore per il numero di tentativi di lettura del touch.
  bool touchDetected = false; // Flag per indicare se un tocco è stato rilevato.

  // Tenta di leggere lo stato del touch screen fino a 3 volte se non viene rilevato un tocco.
  while (readAttempts < 3 && !touchDetected) {
    ts.read(); // Legge lo stato del touch screen.
    if (ts.isTouched) {
      touchDetected = true; // Imposta il flag se viene rilevato un tocco.
    } else {
      readAttempts++;     // Incrementa il contatore dei tentativi.
      delay(10);          // Piccolo ritardo tra un tentativo e l'altro.
    }
  }

  // ====================== FERMA ALLARME BTTF AL TOCCO ======================
  #ifdef EFFECT_BTTF
  if (touchDetected && bttfAlarmRinging) {
    stopBTTFAlarm();
    playTouchSound();  // Feedback sonoro di conferma
    return;  // Esce per evitare altre azioni durante lo spegnimento allarme
  }

  // ====================== GESTIONE PAGINA SETUP SVEGLIE BTTF ======================
  // Nella pagina setup: SX=precedente, CENTRO=modifica, DX=successivo
  // Per uscire: navigare fino a SALVA ESCI e toccare centro
  if (bttfAlarmSetupActive) {
    if (ts.isTouched && !waitingForRelease) {
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      // Tutti i tocchi nella pagina setup passano a handleBTTFAlarmSetupTouch
      // che gestisce: SX (<160) = prev, CENTRO (160-320) = modifica, DX (>320) = next
      handleBTTFAlarmSetupTouch(x, y);
      waitingForRelease = true;
    } else if (!ts.isTouched) {
      waitingForRelease = false;
    }
    return;  // Quando setup sveglie è attivo, ignora tutto il resto
  }
  #endif

  // ====================== GESTIONE RESET WIFI ======================
  if (resetCountdownStarted) { // Se è stata avviata la sequenza di reset del WiFi (pressione dei 4 angoli).
    // Calcola il tempo trascorso dall'inizio della pressione.
    uint32_t elapsedTime = currentMillis - touchStartTime;

    // Variabili per tenere traccia se ogni angolo è ancora premuto.
    bool tl = false, tr = false, bl = false, br = false;
    bool anyCornerDetected = false; // Flag per indicare se è stato rilevato il tocco in almeno un angolo.

    // Itera attraverso i punti di contatto rilevati.
    for (int i = 0; i < ts.touches && i < 10; i++) {
      // Mappa le coordinate del touch screen alle coordinate del display.
      int x = map(ts.points[i].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
      int y = map(ts.points[i].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      // Verifica se il punto di contatto rientra in una delle zone degli angoli (con una tolleranza).
      if (x < 100 && y < 100) {
        tl = true;
        anyCornerDetected = true;
      }
      else if (x > 380 && y < 100) {
        tr = true;
        anyCornerDetected = true;
      }
      else if (x < 100 && y > 380) {
        bl = true;
        anyCornerDetected = true;
      }
      else if (x > 380 && y > 380) {
        br = true;
        anyCornerDetected = true;
      }
    }

    bool allCornersPressed = (tl && tr && bl && br); // Vero se tutti e quattro gli angoli sono premuti.

    // Se tutti gli angoli sono ancora premuti, resetta il contatore di perdita del tocco.
    if (allCornersPressed) {
      cornerTouchLostCounter = 0;

      // Aggiorna la visualizzazione del countdown ogni secondo.
      static int lastSecondDisplayed = -1;
      int currentSecond = 5 - (elapsedTime / 1000); // Calcola i secondi rimanenti.

      if (currentSecond != lastSecondDisplayed) {
        gfx->fillScreen(YELLOW);
        gfx->setTextColor(BLACK);
        gfx->setCursor(80, 180);
        gfx->println(F("RESET WIFI TRA"));
        gfx->setCursor(210, 240);
        gfx->print(currentSecond);
        gfx->println(F(" SEC"));
        gfx->setCursor(40, 300);
        gfx->println(F("RILASCIA PER ANNULLARE"));

        lastSecondDisplayed = currentSecond; // Aggiorna l'ultimo secondo visualizzato.
      }
    }
    // Se non tutti gli angoli sono premuti, ma ne è rilevato almeno uno.
    else if (anyCornerDetected) {
      // Tolleranza: permette fino a 3 controlli consecutivi con angoli mancanti.
      if (cornerTouchLostCounter < 3) {
        cornerTouchLostCounter++;
      } else {
        // Troppi controlli con angoli mancanti, annulla la sequenza di reset.
        resetCountdownStarted = false;
        cornerTouchLostCounter = 0;

        gfx->fillScreen(BLACK);
        gfx->setTextColor(RED);
        gfx->setCursor(80, 240);
        gfx->println(F("RESET WIFI ANNULLATO"));
        delay(1000);

        gfx->fillScreen(BLACK);

      }
    }
    // Se non viene rilevato alcun tocco in nessun angolo.
    else {
      // Annulla immediatamente la sequenza di reset.
      resetCountdownStarted = false;
      cornerTouchLostCounter = 0;

      gfx->fillScreen(BLACK);
      gfx->setTextColor(RED);
      gfx->setCursor(80, 240);
      gfx->println(F("RESET WIFI ANNULLATO"));
      delay(1000);

      gfx->fillScreen(BLACK);

    }

    // Se sono trascorsi 5 secondi dall'inizio della pressione, esegui il reset indipendentemente dallo stato degli angoli.
    if (elapsedTime >= 5000) {
      resetCountdownStarted = false;
      cornerTouchLostCounter = 0;

      // Esegui la funzione per resettare le impostazioni WiFi.
      resetWiFiSettings();
    }

    return; // Esce dalla funzione per concentrarsi sulla gestione del reset.
  }

  // ====================== GESTIONE PAGINA SETUP ======================
  if (setupPageActive) { // Se la pagina di setup è attiva.
    if (ts.isTouched) { // Se lo schermo è toccato.
      // Calcola le coordinate del tocco.
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      // Se non stiamo già gestendo uno scroll e non stiamo aspettando il rilascio di un tocco precedente.
      if (!setupScrollActive && !waitingForRelease) {
        // Verifica se il tocco corrente è l'inizio di uno scroll nella pagina di setup.
        if (checkSetupScroll()) {
          setupScrollActive = true;       // Imposta il flag di scroll attivo.
          waitingForRelease = true;     // Imposta il flag per aspettare il rilascio dopo lo scroll.
        } else {
          // Se non è uno scroll, gestisci il tocco come una normale interazione con un elemento del menu.
          handleSetupTouch(x, y);
          waitingForRelease = true;     // Imposta il flag per aspettare il rilascio dopo l'interazione.
        }
      }
    } else {
      // Se il tocco è rilasciato.
      waitingForRelease = false;     // Resetta il flag di attesa del rilascio.
      setupScrollActive = false;     // Resetta il flag di scroll attivo.
    }

    // Aggiorna il timestamp dell'ultima attività nella pagina di setup.
    setupLastActivity = currentMillis;
    return;  // Esce dalla funzione per concentrarsi solo sulla gestione della pagina di setup.
  }

  // ====================== RILEVAMENTO RESET WIFI ======================
  /*
   * Rileva una pressione simultanea sui 4 angoli dello schermo per avviare la sequenza di reset del WiFi.
   */
  if (ts.touches >= 4 && !resetCountdownStarted) { // Se ci sono almeno 4 tocchi e la sequenza di reset non è già iniziata.
    bool tl = false, tr = false, bl = false, br = false; // Flag per ogni angolo.

    // Verifica se ci sono tocchi nelle aree dei quattro angoli.
    for (int i = 0; i < ts.touches && i < 10; i++) {
      int x = map(ts.points[i].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
      int y = map(ts.points[i].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      if (x < 100 && y < 100) tl = true;
      else if (x > 380 && y < 100) tr = true;
      else if (x < 100 && y > 380) bl = true;
      else if (x > 380 && y > 380) br = true;
    }

    // Se tutti e quattro gli angoli sono toccati contemporaneamente.
    if (tl && tr && bl && br) {
      resetCountdownStarted = true;   // Avvia il countdown per il reset.
      touchStartTime = currentMillis;  // Memorizza il tempo di inizio.
      cornerTouchLostCounter = 0;      // Resetta il contatore di perdita del tocco.

      // Fornisce un feedback visivo iniziale all'utente.
      gfx->fillScreen(YELLOW);
      gfx->setTextColor(BLACK);
      gfx->setCursor(80, 180);
      gfx->println(F("RESET WIFI TRA"));
      gfx->setCursor(210, 240);
      gfx->println(F("5 SEC"));
      gfx->setCursor(40, 300);
      gfx->println(F("RILASCIA PER ANNULLARE"));

      waitingForRelease = true; // Imposta il flag per aspettare il rilascio (anche se non necessario per l'avvio).
      return;                   // Esce dalla funzione per concentrarsi sulla gestione del reset.
    }
  }


#ifdef MENU_SCROLL
  // ====================== CONTROLLO SCROLL SETUP ======================
  if (ts.isTouched && !waitingForRelease && !checkingSetupScroll) {
    // ANTIRIMBALZO - Controlla se è passato abbastanza tempo dall'ultimo tocco.
    if (currentMillis - lastTouchTime < TOUCH_DEBOUNCE_MS) {
      return;
    }

    // Aggiorna il timestamp dell'ultimo tocco.
    lastTouchTime = currentMillis;

    // Ottieni la coordinata Y del primo punto di contatto.
    int16_t y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

    // Se il tocco è nella parte superiore dello schermo (area tipicamente non usata per interazioni dirette).
    if (y < 50) {
      scrollStartY = y;          // Memorizza la coordinata Y iniziale del potenziale scroll.
      checkingSetupScroll = true; // Imposta il flag per indicare che stiamo controllando uno scroll.
      return;
    }

    // Se il tocco è in una posizione normale, memorizza le coordinate per le interazioni standard (quadranti).
    touch_last_x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
    touch_last_y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
#else
  // Se MENU_SCROLL non è definito, comunque otteniamo le coordinate del tocco se presente.
  if (ts.isTouched && !waitingForRelease) {
    touch_last_x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
    touch_last_y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
  }
#endif

  // ====================== GESTIONE DEI QUATTRO QUADRANTI ======================
  if (ts.isTouched && !waitingForRelease) {

  // ====================== SKIP TOUCH SE IN MODALITA' MP3 PLAYER ======================
  // Il lettore MP3 gestisce i propri tocchi internamente
  #ifdef EFFECT_MP3_PLAYER
  if (currentMode == MODE_MP3_PLAYER) {
    return;  // Il touch viene gestito da updateMP3Player()
  }
  #endif

  // ====================== ZONA NASCOSTA: MOSTRA URL SETTINGS (angolo basso-destra) ======================
  // Piccola area 50x50 pixel nell'angolo in basso a destra per mostrare l'URL della pagina settings
  if (touch_last_x >= 430 && touch_last_x <= 480 &&
      touch_last_y >= 430 && touch_last_y <= 480) {
    playTouchSound();
    showSettingsURL();
    waitingForRelease = true;
    return;
  }

  // ====================== PULSANTE CALIBRAZIONE MAGNETOMETRO (WEATHER STATION) ======================
  #ifdef EFFECT_WEATHER_STATION
  // In Weather Station mode: controlla se tocco il pulsante CAL (10, 455, 40, 20)
  if (currentMode == MODE_WEATHER_STATION && magnetometerConnected) {
    if (touch_last_x >= 10 && touch_last_x <= 50 &&
        touch_last_y >= 455 && touch_last_y <= 475) {
      Serial.println("[TOUCH] Pulsante CAL toccato - avvio calibrazione magnetometro");
      playTouchSound();
      calibrateMagnetometerGuided();
      // Dopo calibrazione, ridisegna Weather Station
      initWeatherStation();
      waitingForRelease = true;
      return;
    }
  }
  #endif

  #ifdef EFFECT_BTTF
  // In BTTF: reset long press se tocco è FUORI dal centro (per permettere cambio mode/preset)
  if (currentMode == MODE_BTTF && bttfLongPressActive) {
    bool isInCenter = (touch_last_x >= 190 && touch_last_x <= 290 &&
                       touch_last_y >= 190 && touch_last_y <= 290);
    if (!isInCenter) {
      // Tocco fuori dal centro - reset e procedi con i quadranti
      bttfLongPressActive = false;
      bttfLongPressTriggered = false;
    }
  }
  #endif

// --------------------- TOCCO CENTRALE: Annuncia ora attuale --------------------------------//
// In modalità BTTF: long press (2 sec) apre pagina setup sveglie
// Funziona sia con audio I2S locale che con audio WiFi esterno (ESP32C3)
// NOTA: Area centrale RIDOTTA per BTTF (190-290) per non interferire con quadranti
#ifdef EFFECT_BTTF
bool isBTTFCenter = (currentMode == MODE_BTTF &&
                     touch_last_x >= 190 && touch_last_x <= 290 &&
                     touch_last_y >= 190 && touch_last_y <= 290);
#else
bool isBTTFCenter = false;
#endif
bool isNormalCenter = (touch_last_x >= 170 && touch_last_x <= 310 &&
                       touch_last_y >= 170 && touch_last_y <= 310);

if ((currentMode == MODE_BTTF && isBTTFCenter) || (currentMode != MODE_BTTF && isNormalCenter)) {

  #ifdef EFFECT_BTTF
  // In modalità BTTF: gestisci long press per setup sveglie
  if (currentMode == MODE_BTTF) {
    if (!bttfLongPressActive) {
      // Inizio monitoraggio long press
      bttfLongPressActive = true;
      bttfCenterTouchStart = currentMillis;
      bttfLongPressTriggered = false;
      Serial.println("[BTTF] Monitoraggio long press centro iniziato...");
    } else if (!bttfLongPressTriggered && (currentMillis - bttfCenterTouchStart >= 2000)) {
      // Long press completato! Apri pagina setup sveglie
      bttfLongPressTriggered = true;
      bttfLongPressActive = false;
      Serial.println("[BTTF] >>> LONG PRESS 2 SEC - APRO PAGINA SETUP SVEGLIE <<<");
      showBTTFAlarmSetup();
      waitingForRelease = true;
      return;
    }
    // Non eseguire azioni finché non rilasciamo o non scade il timer
    return;
  }
  #endif

  // Area centrale più ampia (140x140 pixel) per facilitare il tocco.
  playTouchSound();  // Emette un suono di feedback al tocco.

  // NON cancelliamo il display - rimane visibile durante l'annuncio audio

  Serial.println(">>> TOCCO CENTRALE - ANNUNCIO ORA MANUALE <<<");

  // Annuncia ora solo se annuncio orario abilitato
  if (hourlyAnnounceEnabled) {
    // Tenta di annunciare l'ora vocalmente utilizzando la funzione migliorata.
    bool result = announceTimeFixed();

    // Se l'annuncio vocale fallisce.
    if (!result) {
      Serial.println("ERRORE: Annuncio orario manuale fallito");
      gfx->setTextColor(RED);
      gfx->setCursor(100, 240);
      gfx->println(F(""));
    }
  } else {
    Serial.println("Annuncio orario disabilitato - nessun audio");
  }

  // Attende un breve periodo e forza un aggiornamento completo del display per tornare alla visualizzazione normale.
  delay(1000);
  forceDisplayUpdate();
  return; // Esce per evitare di processare altri tocchi.
}

    // --------------------- QUADRANTE SINISTRA --------------------------------//
    if (touch_last_x < 240) {

      // --------------------- QUADRANTE 1 - ALTO SINISTRA: Cambia modalità --------------------------------//
      if (touch_last_y < 240) {
        playTouchSound();  // Feedback sonoro
        handleModeChange(); // Chiama la funzione per cambiare la modalità di visualizzazione dell'orologio.
      }

      // --------------------- QUADRANTE 3 - BASSO SINISTRA: Avvia ciclaggio colori --------------------------------//
      else if (touch_last_y > 240) {
        // Quadrante 3 (in basso a sinistra): Gestione del ciclo dei colori.
        // SOLO per modalità che usano l'orologio a parole (WordClock) senza animazione propria
        bool isWordClockMode = (currentMode == MODE_FADE ||
                                currentMode == MODE_SLOW ||
                                currentMode == MODE_FAST ||
                                currentMode == MODE_MATRIX ||
                                currentMode == MODE_MATRIX2 ||
                                currentMode == MODE_SNAKE ||
                                currentMode == MODE_WATER ||
                                currentMode == MODE_MARIO ||
                                currentMode == MODE_TRON ||
                                currentMode == MODE_GALAGA ||
                                currentMode == MODE_GALAGA2);

        if (isWordClockMode && !colorCycle.isActive) { // Solo se è WordClock e il ciclo non è attivo
          waitingForRelease = true; // Imposta il flag per aspettare il rilascio del tocco.

          // Ottiene la tonalità, la saturazione e il valore (HSV) del colore corrente.
          uint16_t startHue;
          uint8_t startS, startV;
          rgbToHsv(currentColor.r, currentColor.g, currentColor.b, startHue, startS, startV);

          // Se il colore corrente è bianco (saturazione zero), riparte dal rosso.
          if (startS == 0) {
            startHue = 0;
            startS = 255;
          }

          // Attiva il ciclo dei colori e inizializza le sue proprietà.
          colorCycle.isActive = true;
          colorCycle.lastColorChange = millis();
          colorCycle.hue = (startHue * 360) / 255; // Riscala la tonalità da 0-255 a 0-360.
          colorCycle.saturation = startS;
          colorCycle.fadingToWhite = false;
          colorCycle.showingWhite = false;

          playTouchSound();  // Feedback sonoro
        }

      }

    }
    // --------------------- QUADRANTE DESTRA --------------------------------//
    else if (touch_last_x > 240) {

      // --------------------- QUADRANTE 2 - ALTO DESTRA: Cambia preset --------------------------------//
      if (touch_last_y < 240) {
        uint8_t oldPreset = currentPreset; // Memorizza il preset corrente.
        currentPreset = (currentPreset + 1) % 14; // Passa al preset successivo (cicla da 0 a 13).
        EEPROM.write(EEPROM_PRESET_ADDR, currentPreset); // Salva il nuovo preset nella EEPROM.

        // Salva il preset anche per la modalità corrente (sincronizza con pagina web)
        saveModePreset((uint8_t)currentMode, currentPreset);

        EEPROM.commit(); // Scrive i dati.
        playTouchSound();  // Emette un suono di feedback.

        // Applica il nuovo preset chiamando la funzione apposita.
        applyPreset(currentPreset);
      }


      // --------------------- QUADRANTE 4 - BASSO DESTRA: Cambia stato lettera E --------------------------------//
      else if (touch_last_y > 240) {
        playTouchSound();  // Feedback sonoro.
        gfx->fillScreen(BLACK); // Pulisce lo schermo.
        delay(10);
        // Avanza allo stato successivo della visualizzazione della lettera 'E' (cicla tra 0 e 1).
        word_E_state = (word_E_state + 1) % 2;
          // Scrive il nuovo stato nella EEPROM.
        EEPROM.write(EEPROM_WORD_E_STATE_ADDR, word_E_state);
        EEPROM.commit();

        const char* Msg;
        switch (word_E_state) {
          case 0:
            Msg = "BLINK OFF";
            gfx->setTextColor(RED);
            break;
          case 1:
            Msg = "BLINK ON";
            gfx->setTextColor(GREEN);
            break;

        }

        // Calcola la posizione orizzontale per centrare il testo.
        int textWidth = strlen(Msg) * 18;  // Stima approssimativa della larghezza del testo.
        int xPos = (480 - textWidth) / 2;
        if (xPos < 0) xPos = 0;
        gfx->setFont(u8g2_font_maniac_te); // Imposta un font.
        gfx->setCursor(xPos, 240);
        gfx->println(Msg); // Stampa il messaggio ON/OFF.
        gfx->setFont(u8g2_font_inb21_mr); // Ripristina il font principale.

        delay(1000);
        forceDisplayUpdate(); // Forza l'aggiornamento del display.

      }
    }



    // Segnala che stiamo aspettando il rilascio del tocco corrente.
    waitingForRelease = true;
  }

  // ====================== CONTROLLO SCROLL SETUP ======================
  #ifdef MENU_SCROLL
  if (checkingSetupScroll) {
    if (!ts.isTouched) {
      // Rilascio del tocco, annulla il controllo dello scroll.
      checkingSetupScroll = false;
      scrollStartY = -1;
    } else {
      // Verifica se il gesto di scroll verso il basso è sufficiente.
      int16_t y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);

      if (y - scrollStartY > 100) {  // Se lo spostamento verticale è di almeno 100 pixel verso il basso.
        checkingSetupScroll = false; // Annulla il controllo dello scroll.
        scrollStartY = -1;           // Resetta la posizione iniziale dello scroll.

        // Attiva la visualizzazione della pagina di setup.
        showSetupPage();
        playTouchSound();  // Emette un suono di feedback.
      }
    }
  }
  #endif
}

void playTouchSound() {
  // Controlla se i suoni touch sono abilitati
  if (!setupOptions.touchSoundsEnabled) {
    return; // Suoni touch disabilitati da impostazioni
  }

  // Blocca suoni per 2 secondi dopo la fine dell'annuncio orario
  extern unsigned long announceEndTime;
  if (millis() - announceEndTime < 2000) {
    return; // Cooldown post-annuncio
  }

  // Audio locale - riproduce beep.mp3
  playLocalMP3("beep.mp3");
}

// ====================== MOSTRA QR CODE PAGINA SETTINGS ======================
// Mostra QR code per aprire la pagina web delle impostazioni
// Richiamabile da zona touch nascosta (angolo basso-destra)
void showSettingsURL() {
  Serial.println("[TOUCH] Zona nascosta - Mostra QR Code Settings");

  // Genera URL settings
  String settingsUrl = "http://" + WiFi.localIP().toString() + ":8080/settings";

  // Crea QR code (versione 4 = 33x33 moduli, sufficiente per URL breve)
  QRCode qrcode;
  uint8_t *qrcodeData = (uint8_t *)malloc(qrcode_getBufferSize(4));
  if (qrcodeData == NULL) {
    Serial.println("Errore allocazione memoria QR code");
    return;
  }
  qrcode_initText(&qrcode, qrcodeData, 4, ECC_LOW, settingsUrl.c_str());

  gfx->fillScreen(BLACK);

  // Titolo
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(CYAN);
  gfx->setCursor(100, 40);
  gfx->println("IMPOSTAZIONI WEB");

  // Istruzioni
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WHITE);
  gfx->setCursor(70, 75);
  gfx->println("Scansiona con il telefono");

  // Calcola dimensioni QR code
  int scale = 8;  // Ogni modulo QR = 8x8 pixel
  int qrSize = qrcode.size * scale;
  int offsetX = (480 - qrSize) / 2;
  int offsetY = 100;

  // Box bianco per QR code
  gfx->fillRect(offsetX - 15, offsetY - 15, qrSize + 30, qrSize + 30, WHITE);

  // Disegna QR code
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        gfx->fillRect(
          offsetX + x * scale,
          offsetY + y * scale,
          scale,
          scale,
          BLACK
        );
      }
    }
  }

  // Libera memoria
  free(qrcodeData);

  // URL sotto il QR
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(GREEN);
  int urlY = offsetY + qrSize + 35;
  int urlWidth = settingsUrl.length() * 7;
  int urlX = (480 - urlWidth) / 2;
  if (urlX < 10) urlX = 10;
  gfx->setCursor(urlX, urlY);
  gfx->println(settingsUrl);

  // Istruzioni uscita
  gfx->setTextColor(DARKGREY);
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setCursor(100, 460);
  gfx->println("Tocca per tornare indietro");

  // Aspetta tocco per uscire
  delay(500); // Debounce

  // Aspetta rilascio
  while (ts.isTouched) {
    ts.read();
    delay(50);
  }

  // Aspetta nuovo tocco per chiudere
  bool exitWait = true;
  uint32_t showTime = millis();
  while (exitWait) {
    ts.read();
    if (ts.isTouched) {
      exitWait = false;
    }
    // Auto-chiudi dopo 30 secondi
    if (millis() - showTime > 30000) {
      exitWait = false;
    }
    delay(50);
  }

  // Torna al display normale
  gfx->fillScreen(BLACK);
  forceDisplayUpdate();
}
