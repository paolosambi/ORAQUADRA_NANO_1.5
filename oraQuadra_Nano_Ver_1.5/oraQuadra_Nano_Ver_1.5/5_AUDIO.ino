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
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita

  // Pulisce le risorse audio utilizzate in precedenza per eventuali riproduzioni in corso.
  cleanupAudio();

  delay(10);                          // Breve ritardo.

  // Costruisce il messaggio vocale dell'ora in base all'ora e ai minuti correnti.
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

  // Stampa il messaggio che verrà annunciato sulla seriale per debug.
  Serial.println("Annuncio ora: " + timeMessage);

  // Verifica se l'oggetto per l'output audio I2S è stato inizializzato correttamente.
  if (output == nullptr) {
    Serial.println("Output audio non inizializzato, reinizializzo...");
    output = new AudioOutputI2S();                 // Crea una nuova istanza dell'output audio I2S.
    output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // Imposta i pin per la comunicazione I2S.
    output->SetGain(VOLUME_LEVEL);                 // Imposta il livello del volume.
    output->SetChannels(1);                      // Imposta il numero di canali audio (mono).
    delay(100);                                   // Attende un breve periodo per l'inizializzazione.
  }

  // Tenta di riprodurre il messaggio TTS con una gestione dei tentativi in caso di fallimento iniziale.
  bool result = false;

  for (int retry = 0; retry < 2; retry++) {
    result = playTTS(timeMessage, "it"); // Chiama la funzione per riprodurre il testo tramite TTS in italiano.
    if (result) {
      Serial.println("TTS avviato con successo, attendo completamento...");
      delay(300); // Attende un breve periodo per assicurarsi che la riproduzione sia iniziata.

      // Attende che la riproduzione finisca, con un timeout massimo di 10 secondi per evitare blocchi.
      unsigned long startTime = millis();
      while (mp3 && mp3->isRunning() && millis() - startTime < 10000) {
        if (!mp3->loop()) break; // Interrompe il loop se la riproduzione è finita o in errore.
        delay(10);
      }

      Serial.println("Riproduzione terminata");
      cleanupAudio(); // Pulisce le risorse audio dopo la riproduzione.
      return true;   // Indica che l'annuncio è avvenuto con successo.
    } else if (retry == 0) {
      Serial.println("Primo tentativo TTS fallito, riprovo...");
      cleanupAudio(); // Pulisce le risorse audio prima di riprovare.
      delay(500);    // Breve ritardo prima del secondo tentativo.
    }
  }

  Serial.println("Errore avvio TTS dopo tentativi"); // Se entrambi i tentativi falliscono.
  return false;                                   // Indica che l'annuncio non è avvenuto.
   #endif // Chiusura del blocco #ifdef AUDIO
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
  Serial.println("Annuncio ora migliorato (Sequenza I2S)");

  isAnnouncing = true;
  announceStartTime = currentTime;

  // Abilita VU meter per l'annuncio orario
  vuMeterEnabled = true;

  // Pulizia profonda una volta sola prima di iniziare
  cleanupAudio();
  delay(50);

  bool result = announceTimeLocal();

  // Disabilita VU meter dopo l'annuncio
  vuMeterEnabled = false;

  if (result) {
    // Aspettiamo che l'ultima parola esca completamente dalle casse
    delay(500);
    cleanupAudio();

    // Imposta timestamp fine annuncio per bloccare suoni flip
    extern unsigned long announceEndTime;
    announceEndTime = millis();

    // Forza ridisegno orologio (il loop principale aggiornerà il display)
    forceClockRedraw();
    Serial.println("Ridisegno orologio forzato");

    isAnnouncing = false;
    return true;
  }

  // Fallback toni
  int hourTones = (currentHour == 0 || currentHour == 12) ? 12 : currentHour % 12;
  for (int i = 0; i < hourTones; i++) {
    playTone(880, 150);
    delay(150);
    yield();
  }
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

// Genera un'onda sinusoidale a una frequenza specifica per il buffer audio.
void generateSineWave() {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  const float frequency = 440.0;  // Frequenza del tono (La4 a 440 Hz).
  const float amplitude = 10000.0; // Ampiezza dell'onda (valore massimo: +/- 32767 per int16_t).

  // Popola il buffer sineBuffer con i campioni dell'onda sinusoidale.
  for (int i = 0; i < bufferLen; i++) {
    // Calcola l'angolo per ogni campione basato sull'indice, la frequenza e la frequenza di campionamento.
    float angle = i * 2.0 * PI * frequency / sampleRate;
    // Calcola il valore del campione usando la funzione seno e l'ampiezza, convertendolo a un intero a 16 bit.
    sineBuffer[i] = (int16_t)(sin(angle) * amplitude);
  }
  #endif // Chiusura del blocco #ifdef AUDIO
}

// Riproduce un tono a una data frequenza per una data durata.
void playTone(int frequency, int duration_ms) {
  #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  // Calcola il numero totale di campioni necessari per la durata specificata (stereo).
  const int samples = sampleRate * duration_ms / 1000;

  // Alloca memoria per il buffer audio (stereo) nella PSRAM invece che in SRAM.
  int16_t *buffer = (int16_t*)heap_caps_malloc(samples * sizeof(int16_t) * 2, MALLOC_CAP_SPIRAM);

  // Verifica se l'allocazione di memoria ha avuto successo.
  if (!buffer) {
    Serial.println("Errore allocazione buffer in PSRAM - provo in SRAM");
    // Fallback: prova ad allocare in SRAM interna se PSRAM non disponibile
    buffer = (int16_t*)malloc(samples * sizeof(int16_t) * 2);
    if (!buffer) {
      Serial.println("Errore allocazione buffer anche in SRAM");
      return; // Esce dalla funzione.
    }
  }

  // Genera l'onda sinusoidale per la frequenza specificata e la popola nel buffer (stereo).
  for (int i = 0; i < samples; i++) {
    float angle = i * 2.0 * PI * frequency / sampleRate;
    int16_t sample = (int16_t)(sin(angle) * 10000); // Calcola il campione.
    buffer[i * 2] = sample;     // Scrive il campione per il canale sinistro.
    buffer[i * 2 + 1] = sample; // Scrive lo stesso campione per il canale destro (tono mono riprodotto su entrambi i canali).
  }

  // Riproduce il buffer audio tramite l'output I2S.
  int16_t samplePair[2]; // Buffer temporaneo per contenere una coppia di campioni (sinistro e destro).

  for (int i = 0; i < samples; i++) {
    // Copia una coppia di campioni dal buffer principale al buffer temporaneo.
    samplePair[0] = buffer[i * 2];     // Canale sinistro.
    samplePair[1] = buffer[i * 2 + 1]; // Canale destro.

    // Invia la coppia di campioni al DAC I2S tramite l'oggetto output.
    if (output) {
      output->ConsumeSample(samplePair); // Invia un frame stereo (due campioni).
    }

    // Permette al watchdog timer di resettarsi periodicamente durante la riproduzione lunga.
    if (i % 1000 == 0) {
      yield();
    }
  }

  // Libera la memoria allocata per il buffer audio (PSRAM o SRAM).
  heap_caps_free(buffer);

  // Attende un breve periodo per assicurarsi che l'audio abbia terminato di essere riprodotto.
  delay(100);
  #endif // Chiusura del blocco #ifdef AUDIO
}

// Riproduce un testo tramite sintesi vocale (TTS) utilizzando il servizio di Google Translate.
bool playTTS(const String& text, const String& language) {
   #ifdef AUDIO // Blocco di codice compilato solo se la macro AUDIO è definita
  // Pulisce le risorse audio utilizzate in precedenza.
  cleanupAudio();

  // Definisce il nome del file temporaneo dove verrà salvato l'audio MP3.
  String tempFile = "/tts_temp.mp3";
  // Se il file temporaneo esiste già, lo elimina.
  if (LittleFS.exists(tempFile)) {
    LittleFS.remove(tempFile);
  }

  // Costruisce l'URL per richiedere il TTS da Google Translate.
  String url = "https://translate.google.com/translate_tts?ie=UTF-8&client=tw-ob&q=";
  // CORREZIONE: Utilizza la funzione myUrlEncode per codificare correttamente il testo nell'URL.
  url += myUrlEncode(text);
  url += "&tl=" + language;                     // Specifica la lingua del testo.
  url += "&textlen=" + String(text.length()); // Indica la lunghezza del testo.

  // Stampa l'URL di download sulla seriale per debug.
  Serial.println("Download TTS: " + text);

  // Inizializza un oggetto HTTPClient per effettuare la richiesta GET.
  HTTPClient http;
  http.setTimeout(15000);           // Imposta un timeout per la connessione HTTP.
  http.setUserAgent("Mozilla/5.0"); // Imposta l'user agent per simulare un browser.

  // Inizia la connessione HTTP con l'URL specificato.
  if (!http.begin(url)) {
    Serial.println("HTTP begin fallito"); // Stampa un errore se la connessione non può essere stabilita.
    return false;                         // Indica un fallimento.
  }

  // Effettua la richiesta GET e ottiene il codice di risposta HTTP.
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", httpCode); // Stampa l'errore HTTP.
    http.end();                                  // Termina la connessione HTTP.
    return false;                                // Indica un fallimento.
  }

  // Apre il file temporaneo in modalità scrittura per salvare l'audio scaricato.
  File fileOut = LittleFS.open(tempFile, FILE_WRITE);
  if (!fileOut) {
    Serial.println("Errore creazione file"); // Stampa un errore se il file non può essere creato.
    http.end();                              // Termina la connessione HTTP.
    return false;                            // Indica un fallimento.
  }

  // Inizia il download del flusso audio dal server HTTP e lo scrive nel file.
  uint32_t startTime = millis();
  WiFiClient* stream = http.getStreamPtr(); // Ottiene un puntatore al flusso di dati.
  uint8_t buffer[2048];                     // Buffer per leggere i dati dal flusso.
  size_t written = 0;

  // Continua a leggere dal flusso finché la connessione HTTP è attiva.
  while (http.connected()) {
    size_t size = stream->available(); // Ottiene il numero di byte disponibili nel flusso.
    if (size) {
      // Legge i byte disponibili nel buffer.
      int c = stream->readBytes(buffer, min((size_t)sizeof(buffer), size));
      fileOut.write(buffer, c); // Scrive i byte letti nel file.
      written += c;             // Aggiorna il numero di byte scritti.
    } else if (written > 0 && millis() - startTime > 1500) {
      break; // Interrompe se non ci sono dati e il download iniziale è avvenuto.
    }
    yield(); // Permette al watchdog di resettarsi.
  }

  fileOut.close(); // Chiude il file dopo il download.
  http.end();      // Termina la connessione HTTP.

  // Stampa informazioni sul download (dimensione e tempo).
  Serial.printf("Download completato: %d bytes in %d ms\n", written, millis() - startTime);

  // Verifica se il file scaricato è troppo piccolo (potrebbe indicare un errore).
  if (written < 1000) {
    Serial.println("File troppo piccolo");
    LittleFS.remove(tempFile); // Elimina il file incompleto.
    return false;            // Indica un fallimento.
  }

  // Attende un breve periodo dopo il download.
  delay(100);

  // Prepara gli oggetti per la riproduzione dell'audio MP3 dal file scaricato.
  file = new AudioFileSourceLittleFS(tempFile.c_str()); // Sorgente audio dal file LittleFS.
  buff = new AudioFileSourceBuffer(file, 256);       // Buffer per la sorgente audio.
  mp3 = new AudioGeneratorMP3();                    // Decodificatore MP3.

  // Inizia la riproduzione dell'audio MP3.
  if (mp3->begin(buff, output)) {
    isPlaying = true;                      // Imposta il flag di riproduzione a true.
    Serial.println("Riproduzione avviata con successo"); // Indica che la riproduzione è iniziata.
    return true;                           // Indica successo.
  } else {
    Serial.println("Errore avvio riproduzione"); // Indica un errore nell'avvio della riproduzione.
    cleanupAudio();                          // Pulisce le risorse audio.
    return false;                            // Indica fallimento.
  }
 #endif // Chiusura del blocco #ifdef AUDIO
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
  if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
  if (buff) { delete buff; buff = nullptr; }
  if (file) { delete file; file = nullptr; }
  // NON eliminiamo l'output qui per evitare conflitti di registro
  isPlaying = false;
  #endif
}

// Imposta volume audio locale (0-100)
void setVolumeLocal(uint8_t volume) {
  #ifdef AUDIO
  if (volume > 100) volume = 100;
  float gain = (float)volume / 100.0;
  if (output) {
    output->SetGain(gain);
  }
  Serial.printf("[AUDIO] Volume locale impostato: %d%% (gain=%.2f)\n", volume, gain);
  #endif
}

bool playMP3Sequence(const String files[], int count) {
  #ifdef AUDIO
  bool success = true;
  inSequence = true;  // Inizia sequenza - non nascondere VU tra file

  for (int i = 0; i < count; i++) {
    if (!playLocalMP3(files[i].c_str())) {
      success = false;
    }
    yield();
  }

  inSequence = false;  // Fine sequenza
  hideVUMeter();       // Nascondi VU meter alla fine della sequenza

  return success;
  #endif
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
  String filePath = "/" + String(filename);
  if (!LittleFS.exists(filePath)) return false;

  // 1. Pulizia chirurgica: non resettiamo l'output I2S tra le parole
  if (mp3) { if (mp3->isRunning()) mp3->stop(); delete mp3; mp3 = nullptr; }
  if (buff) { delete buff; buff = nullptr; }
  if (file) { delete file; file = nullptr; }

  // 2. Assicuriamoci che l'output sia pronto
  if (output == nullptr) {
    output = new AudioOutputI2S();
    output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    output->SetGain(VOLUME_LEVEL);
    output->SetChannels(1);
    delay(100); 
  }

  file = new AudioFileSourceLittleFS(filePath.c_str());
  // Alziamo il buffer a 4096 per dare stabilità al display RGB
  buff = new AudioFileSourceBuffer(file, 4096); 
  mp3 = new AudioGeneratorMP3();

  if (mp3->begin(buff, output)) {
    isPlaying = true;

    // Piccolo ritardo iniziale per evitare il "clic" o il taglio della prima sillaba
    delay(30);

    // Timeout di sicurezza per evitare loop infiniti (max 30 secondi per file)
    unsigned long playStart = millis();
    const unsigned long MAX_PLAY_TIME = 30000;

    while (mp3->isRunning()) {
      // Timeout di sicurezza
      if (millis() - playStart > MAX_PLAY_TIME) {
        Serial.println("[AUDIO] Timeout riproduzione!");
        mp3->stop();
        break;
      }

      if (!mp3->loop()) {
        // Il file è finito nel decoder, ma NON fermiamo subito.
        // Aspettiamo che l'I2S svuoti l'audio fisico (quello che senti)
        unsigned long drainStart = millis();
        while (millis() - drainStart < 100) {
          if (vuMeterEnabled) updateVUMeter();
          yield();
        }
        mp3->stop();
        break;  // Esci dal loop dopo stop
      }

      // Aggiorna VU meter durante riproduzione (solo se abilitato)
      if (vuMeterEnabled) updateVUMeter();
      yield();
    }

    // Nascondi VU meter solo se NON siamo in una sequenza e se era abilitato
    if (!inSequence && vuMeterEnabled) {
      hideVUMeter();
    }

    isPlaying = false;
    return true;
  }
  return false;
  #endif
}