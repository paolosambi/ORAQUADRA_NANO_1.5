// ================== VU METER DISPLAY ==================
#ifdef AUDIO

// Configurazione VU Meter
#define VU_CENTER_X     240       // Centro X del display (480/2)
#define VU_CENTER_Y     240       // Centro Y del display (480/2)
#define VU_WIDTH        300       // Larghezza totale VU meter
#define VU_HEIGHT       24        // Altezza barra VU meter
#define VU_BARS         15        // Numero di barre
#define VU_BAR_GAP      2         // Gap tra le barre
#define VU_DECAY        3         // Velocità decadimento (più alto = più veloce)
#define VU_ATTACK       8         // Velocità attacco (più alto = più veloce)

// Colori VU meter (verde -> giallo -> rosso)
#define VU_COLOR_LOW    0x07E0    // Verde
#define VU_COLOR_MID    0xFFE0    // Giallo
#define VU_COLOR_HIGH   0xF800    // Rosso
#define VU_COLOR_BG     0x2104    // Grigio scuro per barre spente
#define VU_COLOR_BORDER 0x4208    // Grigio per bordo

// Variabili VU meter
static uint8_t vuLevel = 0;           // Livello corrente (0-100)
static uint8_t vuPeak = 0;            // Livello picco
static unsigned long vuLastUpdate = 0;
static bool vuVisible = false;        // Flag se VU meter è visibile
static bool inSequence = false;       // Flag per sequenza (non nascondere VU tra file)
bool vuMeterEnabled = false;          // VU meter DISABILITATO di default (abilitato solo negli annunci)

// Funzione per ottenere colore barra in base alla posizione
uint16_t getVUBarColor(int barIndex, int totalBars) {
  float ratio = (float)barIndex / (float)totalBars;
  if (ratio < 0.6) return VU_COLOR_LOW;      // Prime 60% verde
  if (ratio < 0.85) return VU_COLOR_MID;     // 60-85% giallo
  return VU_COLOR_HIGH;                       // Ultime 15% rosso
}

// Disegna il VU meter
void drawVUMeter(uint8_t level) {
  #ifdef AUDIO
  extern Arduino_RGB_Display *gfx;
  if (!gfx) return;

  int startX = VU_CENTER_X - VU_WIDTH / 2;
  int startY = VU_CENTER_Y - VU_HEIGHT / 2;
  int barWidth = (VU_WIDTH - (VU_BARS - 1) * VU_BAR_GAP) / VU_BARS;

  // Disegna sfondo solo la prima volta
  if (!vuVisible) {
    // Sfondo semi-trasparente (rettangolo nero con bordo)
    gfx->fillRoundRect(startX - 8, startY - 8, VU_WIDTH + 16, VU_HEIGHT + 16, 6, 0x0000);
    gfx->drawRoundRect(startX - 8, startY - 8, VU_WIDTH + 16, VU_HEIGHT + 16, 6, VU_COLOR_BORDER);
    vuVisible = true;
  }

  // Calcola quante barre accendere
  int activeBars = (level * VU_BARS) / 100;

  // Disegna le barre
  for (int i = 0; i < VU_BARS; i++) {
    int barX = startX + i * (barWidth + VU_BAR_GAP);

    if (i < activeBars) {
      // Barra accesa con colore graduato
      uint16_t color = getVUBarColor(i, VU_BARS);
      gfx->fillRoundRect(barX, startY, barWidth, VU_HEIGHT, 2, color);
    } else {
      // Barra spenta
      gfx->fillRoundRect(barX, startY, barWidth, VU_HEIGHT, 2, VU_COLOR_BG);
    }
  }

  // Indicatore picco (linea bianca)
  if (vuPeak > 0) {
    int peakBar = (vuPeak * VU_BARS) / 100;
    if (peakBar >= VU_BARS) peakBar = VU_BARS - 1;
    int peakX = startX + peakBar * (barWidth + VU_BAR_GAP);
    gfx->fillRect(peakX, startY - 2, barWidth, 2, 0xFFFF);  // Linea bianca sopra
  }
  #endif
}

// Forza ridisegno completo dell'orologio
void forceClockRedraw() {
  extern DisplayMode currentMode;

  // Reset variabili flip clock per forzare ridisegno
  #ifdef EFFECT_FLIP_CLOCK
  extern uint8_t lastHourTens, lastHourOnes;
  extern uint8_t lastMinTens, lastMinOnes;
  extern uint8_t lastSecTens, lastSecOnes;
  lastHourTens = 99;
  lastHourOnes = 99;
  lastMinTens = 99;
  lastMinOnes = 99;
  lastSecTens = 99;
  lastSecOnes = 99;
  #endif

  // Puliamo l'area del VU meter
  extern Arduino_RGB_Display *gfx;
  if (gfx) {
    int startX = VU_CENTER_X - VU_WIDTH / 2;
    int startY = VU_CENTER_Y - VU_HEIGHT / 2;
    gfx->fillRect(startX - 10, startY - 12, VU_WIDTH + 20, VU_HEIGHT + 24, 0x0000);
  }

  // Forza ridisegno immediato in base alla modalità corrente
  #ifdef EFFECT_FLIP_CLOCK
  if (currentMode == MODE_FLIP_CLOCK) {
    extern void updateFlipClock();
    updateFlipClock();
    return;
  }
  #endif
  #ifdef EFFECT_ANALOG_CLOCK
  if (currentMode == MODE_ANALOG_CLOCK) {
    extern void updateAnalogClock();
    updateAnalogClock();
    return;
  }
  #endif
  #ifdef EFFECT_LED_RING
  if (currentMode == MODE_LED_RING) {
    extern void updateLedRingClock();
    updateLedRingClock();
    return;
  }
  #endif
  #ifdef EFFECT_BTTF
  if (currentMode == MODE_BTTF) {
    extern void updateBTTF();
    updateBTTF();
    return;
  }
  #endif
  #ifdef EFFECT_CLOCK
  if (currentMode == MODE_CLOCK) {
    extern void updateClockMode();
    updateClockMode();
    return;
  }
  #endif

  // Per tutti gli altri mode, forza un aggiornamento completo del display
  // Questo risolve il problema del VU meter che non ripristina lo sfondo
  forceDisplayUpdate();
}

// Nascondi VU meter e ripristina display
void hideVUMeter() {
  #ifdef AUDIO
  if (!vuVisible) return;

  // Forza ridisegno dell'orologio per coprire l'area del VU meter
  forceClockRedraw();

  vuVisible = false;
  vuLevel = 0;
  vuPeak = 0;
  #endif
}

// Aggiorna VU meter con simulazione livello audio
void updateVUMeter() {
  #ifdef AUDIO
  // Se VU meter disabilitato globalmente o temporaneamente, esci subito
  if (!setupOptions.vuMeterShowEnabled || !vuMeterEnabled) return;

  if (!isPlaying) {
    if (vuVisible) hideVUMeter();
    return;
  }

  unsigned long now = millis();
  if (now - vuLastUpdate < 30) return;  // Aggiorna ogni 30ms (~33fps)
  vuLastUpdate = now;

  // Simula livello audio con variazione pseudo-casuale realistica
  static uint8_t targetLevel = 0;
  static uint8_t smoothLevel = 0;

  // Genera nuovo target ogni tanto
  if (random(100) < 30) {
    targetLevel = random(40, 95);  // Livello tra 40% e 95%
  }

  // Smooth attack/decay
  if (smoothLevel < targetLevel) {
    int newLevel = smoothLevel + VU_ATTACK;
    smoothLevel = (newLevel > targetLevel) ? targetLevel : (uint8_t)newLevel;
  } else {
    int newLevel = smoothLevel - VU_DECAY;
    smoothLevel = (newLevel < targetLevel) ? targetLevel : (uint8_t)newLevel;
  }

  // Aggiungi variazione per effetto più realistico
  vuLevel = smoothLevel + random(-5, 6);
  if (vuLevel > 100) vuLevel = 100;
  if (vuLevel < 10) vuLevel = 10;

  // Aggiorna picco
  if (vuLevel > vuPeak) {
    vuPeak = vuLevel;
  } else if (vuPeak > 0) {
    vuPeak--;  // Decadimento lento del picco
  }

  drawVUMeter(vuLevel);
  #endif
}

#endif // AUDIO

// Controlla se è cambiata l'ora per annunciarla vocalmente
// Funziona sia con audio I2S (#ifdef AUDIO) che con audio WiFi esterno (ESP32C3)
/*void checkTimeAndAnnounce() {
  // Controlla se l'annuncio orario è abilitato
  if (!hourlyAnnounceEnabled) {
    return; // Annuncio orario disabilitato da impostazioni
  }

  // Se il radar è attivo e non c'è nessuno nella stanza, non annunciare (display spento = audio muto)
  if (radarConnectedOnce && !presenceDetected) {
    return; // Nessuna presenza - salta annuncio orario
  }

  // Verifica se l'ora corrente (ottenuta dal fuso orario) è un valore valido (non maggiore di 23).
  if (myTZ.hour() > 23) {
    Serial.println("Errore lettura ora"); // Stampa un messaggio di errore se l'ora non è valida.
    return;                             // Esce dalla funzione senza fare altro.
  }

  // Ottiene l'ora corrente formattata come stringa (ore:minuti:secondi) per scopi di debug.
  String timeString = myTZ.dateTime("H:i:s");

  // Previene controlli multipli nello stesso minuto. Se il minuto corrente è lo stesso dell'ultimo minuto controllato,
  if (myTZ.minute() == lastMinuteChecked) {
    return; // esce dalla funzione per evitare annunci ripetuti nello stesso minuto.
  }

  // Aggiorna la variabile statica lastMinuteChecked con il minuto corrente, segnando che è stato controllato.
  lastMinuteChecked = myTZ.minute();

  // Stampa l'ora attuale sulla seriale per debug.
  Serial.println("Ora attuale: " + timeString);

  // Verifica se l'ora è cambiata rispetto all'ultima ora annunciata E se i minuti sono esattamente a zero.
  if (myTZ.hour() != lastHour && myTZ.minute() == 0) {
    Serial.println("Cambio ora rilevato: " + String(myTZ.hour()) + ":00"); // Indica un cambio di ora.

    // Controlla se l'ora corrente è dentro la finestra di annunci automatici (usa orari configurabili)
    // Usa checkIsNightTime per consistenza con la logica giorno/notte
    uint8_t currentHourCheck = myTZ.hour();
    uint8_t currentMinCheck = myTZ.minute();
    if (!checkIsNightTime(currentHourCheck, currentMinCheck)) {
      // Ora all'interno della finestra permessa (giorno) - annuncia
      Serial.println("Ora dentro finestra annunci - annuncio orario");
      announceTimeFixed();
    } else {
      // Ora fuori dalla finestra permessa (notte) - salta annuncio
      Serial.printf("Ora fuori finestra annunci (notte dalle %02d:%02d) - annuncio saltato\n", nightStartHour, nightStartMinute);
    }

    // Aggiorna la variabile statica lastHour con l'ora corrente.
    lastHour = myTZ.hour();
  }
}*/

void checkTimeAndAnnounce() {
  // 1. Filtri di uscita immediata
  if (!hourlyAnnounceEnabled || isAnnouncing || isPlaying) return;

  // 2. Acquisizione ora e minuti
  int currentH = myTZ.hour();
  int currentM = myTZ.minute();

  // 3. Gestione Radar (Se non c'è presenza, non parlare)
  if (radarConnectedOnce && !presenceDetected) return; 

  // 4. Protezione contro errori NTP
  if (currentH > 23) return;

  // 5. PROTEZIONE ANTI-RIPETIZIONE (Tua variabile static)
  static uint8_t lastMinuteChecked = 255;
  if (currentM == lastMinuteChecked) return;

  // 6. IL TRIGGER: Minuto 00 e ora diversa dall'ultimo annuncio audio
  if (currentM == 0 && currentH != lastAudioAnnounceHour) {
    
    // Sincronizziamo variabili per la voce
    currentHour = currentH;
    currentMinute = currentM;

    // 7. Controllo fascia oraria Notte/Giorno
    if (!checkIsNightTime(currentH, currentM)) {
      Serial.printf("[AUDIO] Tentativo annuncio ora esatta: %02d:00\n", currentH);
      
      if (announceTimeFixed()) {
        // Successo: blocchiamo ora e minuto
        lastAudioAnnounceHour = currentH;
        lastMinuteChecked = currentM;
      } else {
        Serial.println(F("[AUDIO] Fallimento critico: file non trovato o DFPlayer occupato"));
      }
    } else {
      // È notte: segniamo come "fatto" per non riprovare ogni 10 secondi
      lastAudioAnnounceHour = currentH;
      lastMinuteChecked = currentM;
      Serial.println(F("[AUDIO] Fascia notturna attiva: annuncio soppresso."));
    }
  } else {
    // Se non siamo al minuto 00, aggiorniamo solo il minuto di controllo
    // per "armare" il sistema per lo scoccare della prossima ora
    lastMinuteChecked = currentM;
  }
}

// Funzione per annunciare l'ora corrente tramite sintesi vocale (TTS)
bool announceTime() {
  #ifdef AUDIO
  cleanupAudio();
  delay(10);

  // Costruisce il messaggio vocale dell'ora
  String timeMessage;

  if (currentHour == 0 || currentHour == 24) {
    timeMessage = "È mezzanotte";
  } else if (currentHour == 12) {
    timeMessage = "È mezzogiorno";
  } else if (currentHour == 1 || currentHour == 13) {
    timeMessage = "È l'una";
  } else {
    timeMessage = "Sono le " + String(currentHour > 12 ? currentHour - 12 : currentHour);
  }

  if (currentMinute > 0) {
    timeMessage += currentMinute == 1 ? " e un minuto" : " e " + String(currentMinute) + " minuti";
  }

  Serial.println("Annuncio ora: " + timeMessage);

  // Tenta di riprodurre il messaggio TTS (playTTS ora usa Audio.h internamente)
  bool result = false;
  for (int retry = 0; retry < 2; retry++) {
    result = playTTS(timeMessage, "it");
    if (result) {
      Serial.println("TTS completato con successo");
      return true;
    } else if (retry == 0) {
      Serial.println("Primo tentativo TTS fallito, riprovo...");
      cleanupAudio();
      delay(500);
    }
  }

  Serial.println("Errore avvio TTS dopo tentativi");
  return false;
  #endif
}

bool announceTimeFixed() {
  // --- GESTIONE ORIGINALE: Radar e Impostazioni ---
  if (!hourlyAnnounceEnabled) return false;
  if (radarConnectedOnce && !presenceDetected) {
    Serial.println("Nessuna presenza: annuncio saltato");
    return false;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastAnnounceTime < 10000) return false;
  lastAnnounceTime = currentTime;

  #ifdef AUDIO
  // ========== AUDIO I2S LOCALE ==========
  extern bool useTTSAnnounce;  // Variabile globale per scelta TTS/MP3

  Serial.printf("Annuncio ora (modalità: %s)\n", useTTSAnnounce ? "Google TTS" : "MP3 locali");

  isAnnouncing = true;
  announceStartTime = currentTime;

  // Abilita VU meter per l'annuncio orario
  vuMeterEnabled = true;

  // Scegli tra Google TTS e MP3 locali in base alla preferenza utente
  bool result = useTTSAnnounce ? announceTime() : announceTimeLocal();

  // Disabilita VU meter dopo l'annuncio
  vuMeterEnabled = false;

  if (result) {
    // Imposta timestamp fine annuncio per bloccare suoni flip
    extern unsigned long announceEndTime;
    announceEndTime = millis();

    // Forza ridisegno orologio
    forceClockRedraw();
    Serial.println("Ridisegno orologio forzato");

    isAnnouncing = false;
    return true;
  }

  // Fallback: nessun tono (la gestione web radio è in playLocalMP3)
  forceClockRedraw();
  isAnnouncing = false;
  return false;

  #else
  // ========== AUDIO WIFI (ESP32C3) ==========
  if (!audioSlaveConnected) return false;
  isAnnouncing = true;
  announceStartTime = currentTime;
  bool resI2C = announceTimeViaI2C(myTZ.hour(), myTZ.minute());
  if (!resI2C) isAnnouncing = false;
  return resI2C;
  #endif
}

// generateSineWave() rimossa - non piu' necessaria con Audio.h

// Riproduce un tono (usa beep.mp3 invece di generare onda sinusoidale)
void playTone(int frequency, int duration_ms) {
  #ifdef AUDIO
  playLocalMP3("beep.mp3");
  #endif
}

// Riproduce un testo tramite sintesi vocale (TTS) utilizzando il servizio di Google Translate.
bool playTTS(const String& text, const String& language) {
  #ifdef AUDIO
  extern bool ttsVoiceFemale;  // Variabile globale per scelta voce
  cleanupAudio();

  String tempFile = "/tts_temp.mp3";
  if (LittleFS.exists(tempFile)) {
    LittleFS.remove(tempFile);
  }

  // Costruisce URL per Google TTS
  // Nota: Google TTS gratuito non ha parametri ufficiali per il genere
  // Proviamo con client diversi: tw-ob (femminile) vs gtx (può variare)
  String url = "https://translate.google.com/translate_tts?ie=UTF-8";

  if (ttsVoiceFemale) {
    // Voce femminile - client standard
    url += "&client=tw-ob";
  } else {
    // Voce maschile - prova client alternativo
    url += "&client=gtx";
  }

  url += "&q=" + myUrlEncode(text);
  url += "&tl=" + language;
  url += "&textlen=" + String(text.length());

  // Aggiungi parametro velocità per voce più naturale
  url += "&ttsspeed=1";

  Serial.println("Download TTS: " + text);

  HTTPClient http;
  http.setTimeout(15000);
  http.setUserAgent("Mozilla/5.0");

  if (!http.begin(url)) {
    Serial.println("HTTP begin fallito");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  File fileOut = LittleFS.open(tempFile, FILE_WRITE);
  if (!fileOut) {
    Serial.println("Errore creazione file");
    http.end();
    return false;
  }

  uint32_t startTime = millis();
  WiFiClient* stream = http.getStreamPtr();
  uint8_t buffer[2048];
  size_t written = 0;

  while (http.connected()) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buffer, min((size_t)sizeof(buffer), size));
      fileOut.write(buffer, c);
      written += c;
    } else if (written > 0 && millis() - startTime > 1500) {
      break;
    }
    yield();
  }

  fileOut.close();
  http.end();

  Serial.printf("Download completato: %d bytes in %d ms\n", written, millis() - startTime);

  if (written < 1000) {
    Serial.println("File troppo piccolo");
    LittleFS.remove(tempFile);
    return false;
  }

  delay(100);

  // Riproduzione: usa playLocalMP3 (stessa libreria Audio.h della web radio)
  return playLocalMP3("tts_temp.mp3");
  #endif
}

// Genera uno sweep di frequenze audio, utile per scopi diagnostici.
void playFrequencySweep() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  const int startFreq = 100;  // Frequenza iniziale dello sweep (Hz).
  const int endFreq = 3000;   // Frequenza finale dello sweep (Hz).
  const int stepFreq = 100;   // Incremento di frequenza ad ogni passo (Hz).
  const int durationMs = 200; // Durata di ogni tono nello sweep (millisecondi).

  // Cicla attraverso le frequenze dallo start all'end con l'incremento specificato.
  for (int freq = startFreq; freq <= endFreq; freq += stepFreq) {
    playTone(freq, durationMs); // Riproduce un tono alla frequenza corrente per la durata specificata.
    delay(50);                   // Breve pausa tra un tono e l'altro.
  }
#endif // Chiusura del blocco #ifdef AUDIO
}

// Funzione per codificare una stringa in formato URL (usata per l'invio a servizi web come Google TTS).
String myUrlEncode(const String& msg) {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  const char *hex = "0123456789ABCDEF"; // Array di caratteri esadecimali.
  String encodedMsg = "";                // Stringa per contenere il messaggio codificato.

  // Itera attraverso ogni carattere del messaggio originale.
  for (int i = 0; i < msg.length(); i++) {
    char c = msg.charAt(i); // Ottiene il carattere corrente.
    if (c == ' ') {
      encodedMsg += '+'; // Gli spazi vengono sostituiti con il simbolo '+'.
    } else if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedMsg += c; // I caratteri alfanumerici e alcuni simboli non vengono codificati.
    } else {
      encodedMsg += '%';                      // Gli altri caratteri vengono codificati con il simbolo '%'.
      encodedMsg += hex[c >> 4];               // Aggiunge la parte alta del byte in esadecimale.
      encodedMsg += hex[c & 15];              // Aggiunge la parte bassa del byte in esadecimale.
    }
  }
  return encodedMsg; // Restituisce la stringa codificata.
#endif // Chiusura del blocco #ifdef AUDIO
}

void cleanupAudio() {
  #ifdef AUDIO
  extern Audio audio;
  audio.stopSong();
  isPlaying = false;
  #endif
}

// Imposta volume audio locale (0-100)
void setVolumeLocal(uint8_t volume) {
  #ifdef AUDIO
  extern Audio audio;
  if (volume > 100) volume = 100;
  // Audio.h usa range 0-21
  uint8_t vol21 = map(volume, 0, 100, 0, 21);
  audio.setVolume(vol21);
  Serial.printf("[AUDIO] Volume locale impostato: %d%% (vol21=%d)\n", volume, vol21);
  #endif
}

bool playMP3Sequence(const String files[], int count) {
  #ifdef AUDIO
  extern Audio audio;
  extern bool webRadioEnabled;
  extern String webRadioUrl;
  extern uint8_t webRadioVolume;
  extern uint8_t announceVolume;

  bool success = true;
  bool radioWasPlaying = webRadioEnabled;

  // Ferma web radio all'inizio della sequenza
  if (radioWasPlaying) {
    Serial.println("[AUDIO SEQ] Pausa web radio per sequenza...");
    audio.stopSong();
    delay(50);
  }

  // Imposta volume annunci (converte 0-100 a 0-21)
  audio.setVolume(map(announceVolume, 0, 100, 0, 21));
  Serial.printf("[AUDIO SEQ] Volume annunci: %d%%\n", announceVolume);

  inSequence = true;  // Inizia sequenza - non nascondere VU tra file

  for (int i = 0; i < count; i++) {
    if (!playLocalMP3(files[i].c_str())) {
      success = false;
    }
    yield();
  }

  inSequence = false;  // Fine sequenza
  hideVUMeter();       // Nascondi VU meter alla fine della sequenza

  // Riprendi web radio alla fine della sequenza
  if (radioWasPlaying) {
    Serial.println("[AUDIO SEQ] Riprendo web radio...");
    audio.setVolume(map(webRadioVolume, 0, 100, 0, 21));  // Ripristina volume web radio
    delay(100);
    audio.connecttohost(webRadioUrl.c_str());
  }

  return success;
  #endif
  return false;
}

bool concatenateMP3Files(const String files[], int count, const char* outputFile) {
  #ifdef AUDIO

  String outputPath = "/";
  outputPath += outputFile;
  if (LittleFS.exists(outputPath)) {
    LittleFS.remove(outputPath);
  }

  File outFile = LittleFS.open(outputPath, FILE_WRITE);
  if (!outFile) {
    Serial.println("Errore apertura file output: " + outputPath);
    return false;
  }

  // Ridotto a 512 per non saturare il bus SPI/Flash
  uint8_t buffer[512]; 
  size_t totalWritten = 0;

  for (int i = 0; i < count; i++) {
    String filePath = "/";
    filePath += files[i];

    if (!LittleFS.exists(filePath)) continue;

    File inFile = LittleFS.open(filePath, FILE_READ);
    if (!inFile) continue;

    while (inFile.available()) {
      size_t bytesRead = inFile.read(buffer, sizeof(buffer));
      outFile.write(buffer, bytesRead);
      totalWritten += bytesRead;

      // MODIFICA CRITICA: 
      // Cediamo il controllo ad OGNI pacchetto da 512 byte.
      // Questo permette al DMA del display di ricaricare i buffer.
      yield(); 
      
      // Se il glitch persiste ancora, aggiungi questo piccolissimo delay:
      // delay(1); 
    }

    inFile.close();
  }

  outFile.close();
  Serial.printf("File pronto: %d bytes\n", totalWritten);

  return totalWritten > 0;
  #endif
  return false;
}

bool announceTimeLocal() {
  #ifdef AUDIO
  String audioFiles[12];
  int fileCount = 0;

  if (currentHour == 0 || currentHour == 24) {
    audioFiles[fileCount++] = "mezzanotte.mp3";
  } else if (currentHour == 12) {
    audioFiles[fileCount++] = "mezzogiorno.mp3";
  } else {
    audioFiles[fileCount++] = "sonole.mp3";
    audioFiles[fileCount++] = String(currentHour) + ".mp3";
  }

  if (currentMinute > 0) {
    audioFiles[fileCount++] = "e.mp3";
    audioFiles[fileCount++] = (currentMinute == 12) ? "T-12.mp3" : String(currentMinute) + ".mp3";
    audioFiles[fileCount++] = "minuti.mp3";
  }

  return playMP3Sequence(audioFiles, fileCount);
  #endif
  return false;
}

// Annuncio boot locale (riprodotto direttamente dall'ESP32S3)
// Frase: "Ciao sono OraQuadra. Oggi è [giorno] [numero] [mese] [anno]. Sono le ore [ora] e [minuti] minuti."
bool announceBootLocal() {
  #ifdef AUDIO
  Serial.println("\n=== ANNUNCIO BOOT LOCALE ===");

  // Prima disegna lo sfondo dell'orologio così l'utente lo vede durante l'annuncio
  extern Arduino_RGB_Display *gfx;
  if (gfx) {
    gfx->fillScreen(0x0000);  // Sfondo nero
    // Disegna un messaggio di benvenuto al centro
    gfx->setTextColor(0xFFFF);  // Bianco
    gfx->setTextSize(2);
    gfx->setCursor(140, 200);
    gfx->print("OraQuadra");
    gfx->setCursor(160, 240);
    gfx->setTextSize(1);
    gfx->print("Avvio in corso...");
    Serial.println("[BOOT] Schermata di benvenuto visualizzata");
  }

  // Abilita VU meter per l'annuncio boot
  vuMeterEnabled = true;

  // Giorni della settimana (0=Domenica, 1=Lunedì, ..., 6=Sabato)
  const char* daysOfWeek[] = {
    "domenica.mp3", "lunedi.mp3", "martedi.mp3", "mercoledi.mp3",
    "giovedi.mp3", "venerdi.mp3", "sabato.mp3"
  };

  // Mesi (1=Gennaio, ..., 12=Dicembre)
  const char* months[] = {
    "", "gennaio.mp3", "febbraio.mp3", "marzo.mp3", "aprile.mp3",
    "maggio.mp3", "giugno.mp3", "luglio.mp3", "agosto.mp3",
    "settembre.mp3", "ottobre.mp3", "novembre.mp3", "dicembre.mp3"
  };

  // Ottieni data e ora correnti
  uint8_t dayOfWeekRaw = myTZ.weekday();  // 1=Dom, 2=Lun, ..., 7=Sab
  uint8_t dayOfWeek = (dayOfWeekRaw >= 1 && dayOfWeekRaw <= 7) ? dayOfWeekRaw - 1 : 0;
  uint8_t day = myTZ.day();
  uint8_t month = myTZ.month();
  uint16_t year = myTZ.year();
  uint8_t hour = myTZ.hour();
  uint8_t minute = myTZ.minute();

  Serial.printf("Data: %s %d %s %d, Ora: %02d:%02d\n",
                daysOfWeek[dayOfWeek], day, months[month], year, hour, minute);

  // Verifica validità valori
  if (year < 2024 || year > 2030 || month < 1 || month > 12 || day < 1 || day > 31) {
    Serial.println("Valori tempo non validi - skip annuncio boot");
    return false;
  }

  // Costruisci sequenza file audio
  String audioFiles[20];
  int fileCount = 0;

  // 1. Saluto: "Ciao sono OraQuadra"
  audioFiles[fileCount++] = "saluto.mp3";

  // 2. Giorno della settimana (es. "domenica")
  if (dayOfWeek < 7) {
    audioFiles[fileCount++] = daysOfWeek[dayOfWeek];
  }

  // 3. Numero del giorno (es. "20")
  if (day > 0 && day < 60) {
    audioFiles[fileCount++] = String(day) + ".mp3";
  }

  // 4. Mese (es. "ottobre")
  if (month >= 1 && month <= 12) {
    audioFiles[fileCount++] = months[month];
  }

  // 5. Anno (es. "H-2025.mp3")
  if (year >= 2024 && year <= 2030) {
    audioFiles[fileCount++] = "H-" + String(year) + ".mp3";
  }

  // 6. "Sono le"
  audioFiles[fileCount++] = "sonole.mp3";

  // 7. "ore"
  audioFiles[fileCount++] = "ore.mp3";

  // 8. Ora (es. "20")
  if (hour < 60) {
    audioFiles[fileCount++] = String(hour) + ".mp3";
  }

  // 9. "e X minuti"
  audioFiles[fileCount++] = "e.mp3";
  if (minute < 60) {
    audioFiles[fileCount++] = (minute == 12) ? "T-12.mp3" : String(minute) + ".mp3";
  }
  audioFiles[fileCount++] = "minuti.mp3";

  Serial.printf("Riproduzione sequenza di %d file\n", fileCount);

  bool result = playMP3Sequence(audioFiles, fileCount);

  // Disabilita VU meter dopo l'annuncio
  vuMeterEnabled = false;

  Serial.println("=== ANNUNCIO BOOT COMPLETATO ===\n");
  return result;
  #endif
  return false;
}

bool playLocalMP3(const char* filename) {
  #ifdef AUDIO
  // Usa la stessa libreria Audio della web radio (nessun conflitto I2S)
  extern Audio audio;
  extern bool webRadioEnabled;
  extern bool inSequence;
  extern String webRadioUrl;
  extern uint8_t webRadioVolume;
  extern uint8_t announceVolume;
  static bool radioWasPlayingForLocal = false;

  String filePath = "/" + String(filename);

  // Verifica che il file esista
  if (!LittleFS.exists(filePath)) {
    Serial.printf("[AUDIO] File non trovato: %s\n", filePath.c_str());
    return false;
  }

  // Ferma web radio se attiva - solo se non siamo già in sequenza
  if (!inSequence && webRadioEnabled) {
    Serial.println("[AUDIO] Pausa web radio per audio locale...");
    audio.stopSong();
    radioWasPlayingForLocal = true;
    delay(50);
  }

  // Se non siamo in sequenza, imposta volume annunci (la sequenza lo imposta all'inizio)
  if (!inSequence) {
    audio.setVolume(map(announceVolume, 0, 100, 0, 21));
  }

  Serial.printf("[AUDIO] Play: %s\n", filename);

  // Usa audio.connecttoFS per riprodurre da LittleFS
  if (!audio.connecttoFS(LittleFS, filePath.c_str())) {
    Serial.println("[AUDIO] Errore connessione file!");
    return false;
  }

  isPlaying = true;

  // Timeout di sicurezza (max 30 secondi per file)
  unsigned long playStart = millis();
  const unsigned long MAX_PLAY_TIME = 30000;

  // Attendi fine riproduzione (audio.loop() è chiamato da audioTask)
  // Variabili per lampeggio LED RGB a ritmo del parlato
  #ifdef EFFECT_LED_RGB
  static uint8_t bootLedTarget = 0;
  static uint8_t bootLedSmooth = 0;
  extern Adafruit_NeoPixel ledStrip;
  extern uint8_t ledRgbBrightness;
  extern void getLedColorForMode(DisplayMode mode, uint8_t &r, uint8_t &g, uint8_t &b);
  #endif

  while (audio.isRunning()) {
    if (millis() - playStart > MAX_PLAY_TIME) {
      Serial.println("[AUDIO] Timeout riproduzione!");
      audio.stopSong();
      break;
    }

    // Aggiorna VU meter durante riproduzione
    if (vuMeterEnabled) updateVUMeter();

    // Lampeggio LED RGB a ritmo del parlato (durante annunci)
    #ifdef EFFECT_LED_RGB
    if (vuMeterEnabled) {
      // Simula livello audio con attacco/decadimento
      if (random(100) < 35) {
        bootLedTarget = random(25, 100);
      }
      if (bootLedSmooth < bootLedTarget) {
        bootLedSmooth = min((int)bootLedSmooth + 12, (int)bootLedTarget);
      } else {
        bootLedSmooth = max((int)bootLedSmooth - 6, (int)bootLedTarget);
      }
      uint8_t level = constrain((int)bootLedSmooth + random(-5, 6), 5, 100);

      // Modula brightness LED: da 20% a 100% della brightness impostata
      uint8_t bri = ledRgbBrightness > 0 ? ledRgbBrightness : 80;
      uint8_t minBri = bri / 5;
      uint8_t pulseBri = minBri + (uint8_t)((uint16_t)(bri - minBri) * level / 100);
      ledStrip.setBrightness(pulseBri);

      // Colore tema per il modo corrente
      uint8_t lr, lg, lb;
      getLedColorForMode((DisplayMode)currentMode, lr, lg, lb);
      uint32_t color = ledStrip.Color(lr, lg, lb);
      for (int i = 0; i < 12; i++) {
        ledStrip.setPixelColor(i, color);
      }
      ledStrip.show();
    }
    #endif

    delay(10);  // Breve pausa per non saturare CPU
  }

  // Spegni LED dopo annuncio (il loop principale li gestirà)
  #ifdef EFFECT_LED_RGB
  if (vuMeterEnabled) {
    bootLedSmooth = 0;
    bootLedTarget = 0;
    ledStrip.clear();
    ledStrip.show();
  }
  #endif

  // Nascondi VU meter solo se NON siamo in una sequenza
  if (!inSequence && vuMeterEnabled) {
    hideVUMeter();
  }

  isPlaying = false;

  // Riprendi web radio se era attiva e non siamo in sequenza
  if (!inSequence && radioWasPlayingForLocal) {
    Serial.println("[AUDIO] Riprendo web radio...");
    audio.setVolume(map(webRadioVolume, 0, 100, 0, 21));  // Ripristina volume web radio
    delay(100);
    audio.connecttohost(webRadioUrl.c_str());
    radioWasPlayingForLocal = false;
  }

  return true;
  #else
  return false; // AUDIO non abilitato
  #endif
}