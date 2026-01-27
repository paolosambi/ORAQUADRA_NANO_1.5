// ================== MJPEG STREAM MODE ==================
// Streaming video MJPEG via WiFi da server Python
// Supporta visualizzazione video streaming con audio opzionale via ESP32C3
//
// Basato su MJPEG_Stream_Client di Sambinello Paolo
// Integrato in OraQuadra Nano come modalita' display
//
// Dimensioni display (480x480)
#define MJPEG_DISPLAY_WIDTH  480
#define MJPEG_DISPLAY_HEIGHT 480
//
// UTILIZZO:
// 1. Avvia stream_server.py sul PC
// 2. Configura STREAM_URL nelle impostazioni web
// 3. Seleziona modalita' MJPEG Stream
//
// La modalita' si riconnette automaticamente se lo stream viene interrotto
// Tocca lo schermo per tornare alla modalita' precedente

#ifdef EFFECT_MJPEG_STREAM

// ================== CONFIGURAZIONE STREAMING ==================
#define MJPEG_JPEG_BUFFER_SIZE 120000   // Buffer per singolo frame JPEG (120KB)
#define MJPEG_RECONNECT_DELAY 1000      // Delay tra tentativi di riconnessione (ms)
#define MJPEG_FRAME_TIMEOUT 3000        // Timeout per frame singolo (ms)
#define MJPEG_CONNECTION_TIMEOUT 5000   // Timeout connessione stream (ms)
#define MJPEG_READ_BUFFER_SIZE 4096     // Buffer lettura TCP (4KB)

// DOUBLE BUFFERING - Display 480x480 RGB565
#define MJPEG_DISPLAY_SIZE 480
#define MJPEG_FRAME_BUFFER_SIZE (MJPEG_DISPLAY_SIZE * MJPEG_DISPLAY_SIZE)  // 230400 pixels = ~460KB

// URL dello stream - configurabile via web
String mjpegStreamUrl = "http://192.168.1.24:5000/stream";  // Default, modificabile via web192.168.1.
String mjpegAudioUrl = "http://192.168.1.24:5000/audio";    // URL audio stream separato

// Dichiarazioni esterne per audio I2C
extern bool audioSlaveConnected;
extern bool playStreamViaI2C(const char* url);
extern bool stopStreamViaI2C();
extern bool setVolumeViaI2C(uint8_t volume);

// ================== VARIABILI STATO MJPEG ==================
bool mjpegInitialized = false;          // Flag inizializzazione
bool mjpegStreaming = false;            // Flag streaming attivo
bool mjpegShowOverlay = false;          // Mostra overlay FPS/info (tocca in alto per attivare)

// Buffer MJPEG (allocati in PSRAM)
uint8_t *mjpegJpegBuffer = nullptr;     // Buffer dati JPEG compressi
uint8_t *mjpegReadBuffer = nullptr;     // Buffer lettura TCP veloce
uint16_t *mjpegFrameBuffer = nullptr;   // DOUBLE BUFFER: Frame buffer 480x480 RGB565 (~460KB)

// Statistiche
unsigned long mjpegFrameCount = 0;      // Contatore frame totali
unsigned long mjpegLastFpsUpdate = 0;   // Timestamp ultimo calcolo FPS
float mjpegCurrentFps = 0;              // FPS corrente
int mjpegFramesThisSecond = 0;          // Frame nel secondo corrente
unsigned long mjpegLastFrameTime = 0;   // Timestamp ultimo frame ricevuto

// Stato schermata per evitare ridisegno continuo
int mjpegScreenState = -1;              // -1=nessuno, 0=waiting, 1=error, 2=streaming

// Stato audio C3
bool mjpegAudioC3Active = false;        // Flag per tracciare se audio C3 e' attivo
unsigned long mjpegLastLoopCheck = 0;   // Timestamp ultimo controllo loop
uint8_t mjpegC3Volume = 80;             // Volume corrente C3 (0-100)
unsigned long mjpegLastVolumeCheck = 0; // Timestamp ultimo controllo volume

// Ritardo avvio audio per sincronizzazione con video
bool mjpegAudioC3Pending = false;       // Flag: audio C3 in attesa di avvio
unsigned long mjpegAudioStartTime = 0;  // Timestamp quando avviare l'audio
#define MJPEG_AUDIO_DELAY_MS 0          // Nessun ritardo - audio parte da 0 sul server

// Client HTTP per streaming
WiFiClient mjpegWifiClient;
HTTPClient mjpegHttp;

// Decoder JPEG
JPEGDEC mjpegDecoder;

// ================== CALLBACK JPEG ==================
// DOUBLE BUFFERING: Scrive i pixel nel frame buffer invece che direttamente sul display
int mjpegDrawCallbackBuffered(JPEGDRAW *pDraw) {
  // Copia i pixel decodificati nel frame buffer
  // Il buffer è organizzato come array lineare 480x480
  uint16_t *src = pDraw->pPixels;

  for (int y = 0; y < pDraw->iHeight; y++) {
    int destY = pDraw->y + y;
    if (destY >= 0 && destY < MJPEG_DISPLAY_SIZE) {
      int destX = pDraw->x;
      int width = pDraw->iWidth;

      // Clipping orizzontale
      if (destX < 0) {
        src += (-destX);
        width += destX;
        destX = 0;
      }
      if (destX + width > MJPEG_DISPLAY_SIZE) {
        width = MJPEG_DISPLAY_SIZE - destX;
      }

      if (width > 0) {
        // Copia riga nel frame buffer
        memcpy(&mjpegFrameBuffer[destY * MJPEG_DISPLAY_SIZE + destX],
               src, width * sizeof(uint16_t));
      }
    }
    src += pDraw->iWidth;
  }
  return 1;
}

// Callback legacy per disegno diretto (usato come fallback)
int mjpegDrawCallback(JPEGDRAW *pDraw) {
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

// ================== INIZIALIZZAZIONE ==================
bool initMjpegStream() {
  Serial.println("[MJPEG] Inizializzazione con DOUBLE BUFFERING...");

  // Alloca buffer JPEG in PSRAM
  if (mjpegJpegBuffer == nullptr) {
    mjpegJpegBuffer = (uint8_t *)heap_caps_malloc(MJPEG_JPEG_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (mjpegJpegBuffer == nullptr) {
      Serial.println("[MJPEG] ERRORE: Impossibile allocare JPEG buffer!");
      return false;
    }
    Serial.printf("[MJPEG] JPEG buffer allocato: %d KB\n", MJPEG_JPEG_BUFFER_SIZE / 1024);
  }

  // Alloca buffer lettura TCP in PSRAM
  if (mjpegReadBuffer == nullptr) {
    mjpegReadBuffer = (uint8_t *)heap_caps_malloc(MJPEG_READ_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (mjpegReadBuffer == nullptr) {
      Serial.println("[MJPEG] ERRORE: Impossibile allocare read buffer!");
      return false;
    }
    Serial.printf("[MJPEG] Read buffer allocato: %d KB\n", MJPEG_READ_BUFFER_SIZE / 1024);
  }

  // DOUBLE BUFFERING: Alloca frame buffer 480x480 in PSRAM (~460KB)
  if (mjpegFrameBuffer == nullptr) {
    mjpegFrameBuffer = (uint16_t *)heap_caps_malloc(MJPEG_FRAME_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (mjpegFrameBuffer == nullptr) {
      Serial.println("[MJPEG] ERRORE: Impossibile allocare frame buffer!");
      return false;
    }
    // Inizializza a nero
    memset(mjpegFrameBuffer, 0, MJPEG_FRAME_BUFFER_SIZE * sizeof(uint16_t));
    Serial.printf("[MJPEG] DOUBLE BUFFER allocato: %d KB (480x480 RGB565)\n",
                  (MJPEG_FRAME_BUFFER_SIZE * sizeof(uint16_t)) / 1024);
  }

  // Reset statistiche
  mjpegFrameCount = 0;
  mjpegCurrentFps = 0;
  mjpegFramesThisSecond = 0;
  mjpegLastFpsUpdate = millis();
  mjpegLastFrameTime = millis();
  mjpegScreenState = -1;  // Reset stato schermata per forzare ridisegno

  mjpegInitialized = true;
  Serial.println("[MJPEG] Inizializzazione completata con DOUBLE BUFFERING");
  return true;
}

// ================== CONNESSIONE STREAM ==================
bool connectMjpegStream() {
  Serial.printf("[MJPEG] Connessione a: %s\n", mjpegStreamUrl.c_str());

  // IMPORTANTE: Controlla audio_output PRIMA di connettersi allo stream video
  // per evitare di bloccare la connessione video con richieste HTTP
  bool audioOnC3 = false;
  if (audioSlaveConnected) {
    // Controlla impostazione audio output dal server
    int pathStart = mjpegStreamUrl.indexOf("/stream");
    if (pathStart < 0) pathStart = mjpegStreamUrl.lastIndexOf('/');
    String baseUrl = (pathStart > 0) ? mjpegStreamUrl.substring(0, pathStart) : mjpegStreamUrl;
    String audioOutputUrl = baseUrl + "/api/audio/output";

    HTTPClient httpAudio;
    WiFiClient clientAudio;
    httpAudio.begin(clientAudio, audioOutputUrl);
    httpAudio.setTimeout(2000);  // Timeout breve
    int audioHttpCode = httpAudio.GET();

    if (audioHttpCode == 200) {
      String response = httpAudio.getString();
      if (response.indexOf("esp32c3") >= 0) {
        audioOnC3 = true;
        Serial.println("[MJPEG] Audio output: ESP32-C3");
      } else {
        Serial.println("[MJPEG] Audio output: Browser");
      }
    } else {
      Serial.printf("[MJPEG] Audio output check fallito: %d\n", audioHttpCode);
    }
    httpAudio.end();
  }

  // Ora connetti allo stream video
  mjpegHttp.begin(mjpegWifiClient, mjpegStreamUrl);
  mjpegHttp.setTimeout(MJPEG_CONNECTION_TIMEOUT);

  // Aggiungi header per raccogliere Content-Type
  const char* headerKeys[] = {"Content-Type"};
  mjpegHttp.collectHeaders(headerKeys, 1);

  int httpCode = mjpegHttp.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[MJPEG] Errore HTTP: %d\n", httpCode);
    mjpegHttp.end();
    return false;
  }

  // Verifica content-type MJPEG (opzionale)
  String contentType = mjpegHttp.header("Content-Type");
  Serial.printf("[MJPEG] Content-Type: '%s'\n", contentType.c_str());

  if (contentType.length() > 0 && contentType.indexOf("multipart") < 0 && contentType.indexOf("image") < 0) {
    Serial.println("[MJPEG] ATTENZIONE: Content-Type inaspettato");
  }

  Serial.println("[MJPEG] Stream video connesso!");
  mjpegStreaming = true;
  mjpegLastFrameTime = millis();
  mjpegScreenState = 2;  // In streaming

  // Ora schedula l'audio su C3 se necessario (con ritardo per sync con video)
  if (audioSlaveConnected) {
    if (audioOnC3) {
      // NON avviare subito! Schedula avvio dopo ritardo per sync con video
      Serial.printf("[MJPEG] Audio C3 schedulato tra %d ms per sync video\n", MJPEG_AUDIO_DELAY_MS);
      Serial.printf("[MJPEG] Audio URL: %s\n", mjpegAudioUrl.c_str());
      mjpegAudioC3Pending = true;
      mjpegAudioStartTime = millis() + MJPEG_AUDIO_DELAY_MS;
      mjpegAudioC3Active = false;  // Non ancora attivo
    } else {
      stopStreamViaI2C();
      mjpegAudioC3Active = false;
      mjpegAudioC3Pending = false;
    }
  }

  return true;
}

// ================== DISCONNESSIONE STREAM ==================
void disconnectMjpegStream() {
  if (mjpegStreaming) {
    // Ferma audio streaming su ESP32C3
    if (audioSlaveConnected) {
      Serial.println("[MJPEG] Arresto audio stream su ESP32C3...");
      stopStreamViaI2C();
    }

    mjpegHttp.end();
    mjpegStreaming = false;
    mjpegAudioC3Active = false;
    mjpegAudioC3Pending = false;  // Cancella eventuale avvio pendente
    Serial.println("[MJPEG] Stream video e audio disconnesso");
  }
}

// ================== LETTURA FRAME MJPEG ==================
// Legge un singolo frame JPEG dallo stream MJPEG (ottimizzato per velocità)
int readMjpegFrame() {
  WiFiClient* stream = mjpegHttp.getStreamPtr();

  if (!stream || !stream->connected()) {
    return -1;  // Connessione persa
  }

  // Attendi dati disponibili con timeout minimo
  int waitCount = 0;
  while (!stream->available() && waitCount < 100) {
    delayMicroseconds(100);  // 100us invece di 1ms
    waitCount++;
  }

  if (!stream->available()) {
    return 0;  // Nessun dato
  }

  int jpegSize = 0;
  bool foundSOI = false;
  uint8_t prevByte = 0;
  unsigned long timeout = millis() + 1500;  // 1.5 secondi timeout

  while (millis() < timeout && stream->connected()) {
    int available = stream->available();
    if (available > 0) {
      // Leggi a blocchi grandi per massima velocità
      int toRead = min(available, MJPEG_READ_BUFFER_SIZE);
      int bytesRead = stream->readBytes(mjpegReadBuffer, toRead);

      for (int i = 0; i < bytesRead; i++) {
        uint8_t b = mjpegReadBuffer[i];

        if (!foundSOI) {
          // Cerca marker SOI (0xFFD8)
          if (prevByte == 0xFF && b == 0xD8) {
            mjpegJpegBuffer[0] = 0xFF;
            mjpegJpegBuffer[1] = 0xD8;
            jpegSize = 2;
            foundSOI = true;
          }
          prevByte = b;
        } else {
          // Copia dati JPEG
          if (jpegSize < MJPEG_JPEG_BUFFER_SIZE - 1) {
            mjpegJpegBuffer[jpegSize++] = b;

            // Cerca marker EOI (End Of Image): 0xFFD9
            if (mjpegJpegBuffer[jpegSize-2] == 0xFF && mjpegJpegBuffer[jpegSize-1] == 0xD9) {
              return jpegSize;  // Frame completo!
            }
          } else {
            return 0;  // Buffer overflow
          }
        }
      }
    }
    // Nessun delay nel loop - massima velocità
  }

  return 0;  // Timeout o frame incompleto
}

// ================== DECODIFICA E VISUALIZZA FRAME ==================
// DOUBLE BUFFERING: Decodifica nel buffer, poi trasferisci tutto in un colpo solo
bool decodeAndDisplayMjpegFrame(uint8_t* jpegData, int jpegSize) {
  if (mjpegFrameBuffer == nullptr) {
    // Fallback: disegno diretto se buffer non disponibile
    if (mjpegDecoder.openRAM(jpegData, jpegSize, mjpegDrawCallback)) {
      mjpegDecoder.setPixelType(RGB565_LITTLE_ENDIAN);
      mjpegDecoder.decode(0, 0, 0);
      mjpegDecoder.close();
      return true;
    }
    return false;
  }

  // DOUBLE BUFFERING: Decodifica nel frame buffer
  if (mjpegDecoder.openRAM(jpegData, jpegSize, mjpegDrawCallbackBuffered)) {
    mjpegDecoder.setPixelType(RGB565_LITTLE_ENDIAN);
    // Decodifica nel frame buffer (callback scrive nel buffer)
    mjpegDecoder.decode(0, 0, 0);
    mjpegDecoder.close();

    // Trasferisci l'intero frame buffer al display in un'unica operazione
    // Questo elimina il flickering perché il display viene aggiornato tutto insieme
    gfx->draw16bitRGBBitmap(0, 0, mjpegFrameBuffer, MJPEG_DISPLAY_SIZE, MJPEG_DISPLAY_SIZE);

    return true;
  }
  return false;
}

// ================== OVERLAY INFORMAZIONI ==================
void drawMjpegOverlay() {
  if (!mjpegShowOverlay) return;

  // Barra superiore semi-trasparente (sfondo nero sottile)
  gfx->fillRect(0, 0, 480, 18, BLACK);

  // Usa font U8G2 piccolo
  gfx->setFont(u8g2_font_helvR08_tr);

  // FPS
  gfx->setTextColor(GREEN);
  gfx->setCursor(5, 13);
  gfx->printf("%.1f FPS", mjpegCurrentFps);

  // Frame count
  gfx->setTextColor(WHITE);
  gfx->setCursor(80, 13);
  gfx->printf("F:%lu", mjpegFrameCount);

  // WiFi signal
  int rssi = WiFi.RSSI();
  gfx->setCursor(160, 13);
  if (rssi > -50) {
    gfx->setTextColor(GREEN);
    gfx->print("WiFi:+++");
  } else if (rssi > -70) {
    gfx->setTextColor(YELLOW);
    gfx->print("WiFi:++");
  } else {
    gfx->setTextColor(RED);
    gfx->print("WiFi:+");
  }

  // Heap libero
  gfx->setTextColor(CYAN);
  gfx->setCursor(250, 13);
  gfx->printf("Heap:%dK", ESP.getFreeHeap() / 1024);

  // Indicatore Double Buffer
  if (mjpegFrameBuffer != nullptr) {
    gfx->setTextColor(MAGENTA);
    gfx->setCursor(340, 13);
    gfx->print("DB");
  }

  // PSRAM libera
  gfx->setTextColor(YELLOW);
  gfx->setCursor(370, 13);
  gfx->printf("PS:%dK", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);

  // Ripristina font default
  gfx->setFont(u8g2_font_inb21_mr);
}

// ================== MOSTRA STATO ATTESA ==================
void showMjpegWaitingScreen() {
  // Evita ridisegno se già in questo stato
  if (mjpegScreenState == 0) return;
  mjpegScreenState = 0;

  gfx->fillScreen(BLACK);

  // Usa font U8G2 per aspetto più pulito
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(CYAN);
  gfx->setCursor(150, 160);
  gfx->println("MJPEG STREAM");

  gfx->setFont(u8g2_font_helvR12_tr);
  gfx->setTextColor(WHITE);
  gfx->setCursor(180, 200);
  gfx->println("In attesa...");

  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(LIGHTGREY);
  gfx->setCursor(100, 260);
  gfx->println("Avvia lo streaming dal server:");

  gfx->setTextColor(GREEN);
  gfx->setCursor(80, 290);
  gfx->println(mjpegStreamUrl.c_str());

  // Ripristina font default
  gfx->setFont(u8g2_font_inb21_mr);
}

// ================== MOSTRA ERRORE CONNESSIONE ==================
void showMjpegConnectionError() {
  // Evita ridisegno se già in questo stato
  if (mjpegScreenState == 1) return;
  mjpegScreenState = 1;

  gfx->fillScreen(BLACK);

  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(RED);
  gfx->setCursor(140, 200);
  gfx->println("Stream disconnesso");

  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(170, 250);
  gfx->println("Riconnessione...");

  gfx->setFont(u8g2_font_helvR08_tr);
  gfx->setTextColor(LIGHTGREY);
  gfx->setCursor(150, 320);
  gfx->println("Tocca per cambiare modo");

  // Ripristina font default
  gfx->setFont(u8g2_font_inb21_mr);
}

// ================== UPDATE PRINCIPALE MJPEG ==================
void updateMjpegStream() {
  static unsigned long lastOverlayUpdate = 0;

  // Inizializza se necessario
  if (!mjpegInitialized) {
    if (!initMjpegStream()) {
      Serial.println("[MJPEG] Inizializzazione fallita!");
      gfx->fillScreen(BLACK);
      gfx->setFont(u8g2_font_helvB12_tr);
      gfx->setTextColor(RED);
      gfx->setCursor(120, 200);
      gfx->println("ERRORE MEMORIA!");
      gfx->setFont(u8g2_font_helvR10_tr);
      gfx->setCursor(100, 250);
      gfx->println("PSRAM non disponibile");
      gfx->setFont(u8g2_font_inb21_mr);
      delay(3000);
      handleModeChange();
      return;
    }
  }

  // Se non connesso allo stream, prova a connettersi
  if (!mjpegStreaming) {
    // Mostra schermata di attesa prima di tentare connessione
    showMjpegWaitingScreen();
    Serial.println("[MJPEG] Tentativo connessione stream...");
    if (connectMjpegStream()) {
      Serial.println("[MJPEG] Stream avviato con successo!");
    } else {
      Serial.println("[MJPEG] Connessione fallita, riprovo...");
      delay(MJPEG_RECONNECT_DELAY);
      return;
    }
  }

  // Ricevi e visualizza frame
  int jpegSize = readMjpegFrame();

  if (jpegSize > 0) {
    if (decodeAndDisplayMjpegFrame(mjpegJpegBuffer, jpegSize)) {
      mjpegFrameCount++;
      mjpegFramesThisSecond++;
      mjpegLastFrameTime = millis();

      // Disegna overlay solo ogni 500ms per ridurre overhead
      if (millis() - lastOverlayUpdate > 500) {
        drawMjpegOverlay();
        lastOverlayUpdate = millis();
      }
    }
  } else if (jpegSize < 0) {
    // Connessione persa - mostra attesa invece di errore
    Serial.println("[MJPEG] Connessione persa!");
    mjpegStreaming = false;
    mjpegScreenState = -1;  // Reset per permettere ridisegno
    mjpegHttp.end();
    showMjpegWaitingScreen();
    delay(500);
    return;
  }

  // Timeout: nessun frame ricevuto per troppo tempo
  if (millis() - mjpegLastFrameTime > MJPEG_FRAME_TIMEOUT) {
    Serial.println("[MJPEG] Timeout frame!");
    mjpegStreaming = false;
    mjpegScreenState = -1;  // Reset per permettere ridisegno
    mjpegHttp.end();
    showMjpegWaitingScreen();
    return;
  }

  // Calcola FPS ogni secondo
  unsigned long now = millis();
  if (now - mjpegLastFpsUpdate >= 1000) {
    mjpegCurrentFps = mjpegFramesThisSecond;
    mjpegFramesThisSecond = 0;
    mjpegLastFpsUpdate = now;
  }

  // Avvia audio C3 dopo ritardo (per sync con video)
  if (mjpegAudioC3Pending && !mjpegAudioC3Active && now >= mjpegAudioStartTime) {
    Serial.println("[MJPEG] Avvio audio C3 (dopo ritardo sync)...");
    if (playStreamViaI2C(mjpegAudioUrl.c_str())) {
      Serial.println("[MJPEG] Audio avviato!");
      mjpegAudioC3Active = true;
      mjpegAudioC3Pending = false;
      // Imposta volume iniziale
      setVolumeViaI2C(mjpegC3Volume);
      Serial.printf("[MJPEG] Volume C3 iniziale: %d%%\n", mjpegC3Volume);
    } else {
      Serial.println("[MJPEG] Audio fallito");
      mjpegAudioC3Pending = false;
    }
  }

  // Controlla se il video e' andato in loop (ogni 2 secondi) per riavviare audio C3
  if (mjpegAudioC3Active && audioSlaveConnected && (now - mjpegLastLoopCheck >= 2000)) {
    mjpegLastLoopCheck = now;

    // Chiedi al server se il video e' ricominciato
    int pathStart = mjpegStreamUrl.indexOf("/stream");
    if (pathStart < 0) pathStart = mjpegStreamUrl.lastIndexOf('/');
    String baseUrl = (pathStart > 0) ? mjpegStreamUrl.substring(0, pathStart) : mjpegStreamUrl;
    String loopedUrl = baseUrl + "/api/video/looped";

    HTTPClient httpLoop;
    WiFiClient clientLoop;
    httpLoop.begin(clientLoop, loopedUrl);
    httpLoop.setTimeout(1000);  // Timeout breve
    int httpCode = httpLoop.GET();

    if (httpCode == 200) {
      String response = httpLoop.getString();
      if (response.indexOf("true") >= 0) {
        // Video ricominciato - riavvia audio su C3
        Serial.println("[MJPEG] Video loop rilevato - riavvio audio C3");
        playStreamViaI2C(mjpegAudioUrl.c_str());
      }
    }
    httpLoop.end();
  }

  // Controlla se il volume C3 e' cambiato (ogni 1 secondo)
  if (mjpegAudioC3Active && audioSlaveConnected && (now - mjpegLastVolumeCheck >= 1000)) {
    mjpegLastVolumeCheck = now;

    // Chiedi al server il volume corrente
    int pathStart = mjpegStreamUrl.indexOf("/stream");
    if (pathStart < 0) pathStart = mjpegStreamUrl.lastIndexOf('/');
    String baseUrl = (pathStart > 0) ? mjpegStreamUrl.substring(0, pathStart) : mjpegStreamUrl;
    String volumeUrl = baseUrl + "/api/volume/c3";

    HTTPClient httpVol;
    WiFiClient clientVol;
    httpVol.begin(clientVol, volumeUrl);
    httpVol.setTimeout(1000);
    int httpCode = httpVol.GET();

    if (httpCode == 200) {
      String response = httpVol.getString();
      // Cerca "changed": true
      if (response.indexOf("\"changed\": true") >= 0 || response.indexOf("\"changed\":true") >= 0) {
        // Estrai volume dal JSON: "volume": XX
        int volIdx = response.indexOf("\"volume\":");
        if (volIdx >= 0) {
          volIdx += 9;  // Salta "volume":
          // Salta spazi
          while (volIdx < (int)response.length() && response[volIdx] == ' ') volIdx++;
          // Leggi numero
          String volStr = "";
          while (volIdx < (int)response.length() && response[volIdx] >= '0' && response[volIdx] <= '9') {
            volStr += response[volIdx];
            volIdx++;
          }
          if (volStr.length() > 0) {
            uint8_t newVolume = volStr.toInt();
            if (newVolume != mjpegC3Volume) {
              mjpegC3Volume = newVolume;
              Serial.printf("[MJPEG] Volume C3 cambiato: %d%%\n", mjpegC3Volume);
              setVolumeViaI2C(mjpegC3Volume);
            }
          }
        }
      }
    }
    httpVol.end();
  }
}

// ================== CLEANUP MJPEG ==================
void cleanupMjpegStream() {
  Serial.println("[MJPEG] Cleanup...");

  // Disconnetti stream
  disconnectMjpegStream();

  // Libera buffer (opzionale - si possono mantenere per riuso)
  // Se si vuole liberare memoria quando si esce dalla modalita':
  /*
  if (mjpegFrameBuffer != nullptr) {
    heap_caps_free(mjpegFrameBuffer);
    mjpegFrameBuffer = nullptr;
  }
  if (mjpegJpegBuffer != nullptr) {
    heap_caps_free(mjpegJpegBuffer);
    mjpegJpegBuffer = nullptr;
  }
  */

  mjpegInitialized = false;
  mjpegStreaming = false;
  mjpegScreenState = -1;  // Reset stato schermata

  Serial.println("[MJPEG] Cleanup completato");
}

// ================== GESTIONE TOUCH MJPEG ==================
// Chiamata dal touch handler quando in modalita' MJPEG
void handleMjpegTouch(int x, int y) {
  // Tocco in alto: toggle overlay
  if (y < 50) {
    mjpegShowOverlay = !mjpegShowOverlay;
    Serial.printf("[MJPEG] Overlay: %s\n", mjpegShowOverlay ? "ON" : "OFF");
  }
  // Tocco centrale: info debug
  else if (y > 200 && y < 300) {
    Serial.printf("[MJPEG] Info: FPS=%.1f, Frames=%lu, RSSI=%d, Heap=%d\n",
                  mjpegCurrentFps, mjpegFrameCount, WiFi.RSSI(), ESP.getFreeHeap());
  }
}

// ================== SET STREAM URL ==================
void setMjpegStreamUrl(const String& url) {
  mjpegStreamUrl = url;
  Serial.printf("[MJPEG] URL impostato: %s\n", mjpegStreamUrl.c_str());

  // Se in streaming, disconnetti per usare nuovo URL
  if (mjpegStreaming) {
    disconnectMjpegStream();
  }
}

// ================== GET STREAM URL ==================
String getMjpegStreamUrl() {
  return mjpegStreamUrl;
}

// ================== CONTROLLO AUDIO MUTE ==================
/**
 * Invia comando mute/unmute al server Python per controllare l'audio del browser
 * @param muted true = muta audio/video, false = riattiva audio/video
 */
void setMjpegAudioMute(bool muted) {
  // Estrai base URL da mjpegStreamUrl (es. http://192.168.1.24:5000)
  int pathStart = mjpegStreamUrl.indexOf("/stream");
  if (pathStart < 0) {
    pathStart = mjpegStreamUrl.lastIndexOf('/');
  }
  String baseUrl = (pathStart > 0) ? mjpegStreamUrl.substring(0, pathStart) : mjpegStreamUrl;
  String muteUrl = baseUrl + "/api/audio/mute";

  Serial.printf("[MJPEG] %s streaming: %s\n", muted ? "Pausing" : "Resuming", muteUrl.c_str());

  HTTPClient http;
  WiFiClient client;
  http.begin(client, muteUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = muted ? "{\"muted\":true}" : "{\"muted\":false}";
  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    Serial.printf("[MJPEG] Streaming %s OK\n", muted ? "in pausa" : "ripreso");
  } else {
    Serial.printf("[MJPEG] Errore controllo streaming: %d\n", httpCode);
  }

  http.end();
}

// ================== CONTROLLO START/STOP STREAMING ==================
/**
 * Avvia lo streaming sul server Python
 */
void startMjpegServerStream() {
  int pathStart = mjpegStreamUrl.indexOf("/stream");
  if (pathStart < 0) {
    pathStart = mjpegStreamUrl.lastIndexOf('/');
  }
  String baseUrl = (pathStart > 0) ? mjpegStreamUrl.substring(0, pathStart) : mjpegStreamUrl;
  String startUrl = baseUrl + "/api/start";

  Serial.printf("[MJPEG] Avvio streaming server: %s\n", startUrl.c_str());

  HTTPClient http;
  WiFiClient client;
  http.begin(client, startUrl);
  http.addHeader("Content-Type", "application/json");

  // Avvia con loop e audio abilitati
  String payload = "{\"loop\":true,\"audio\":true}";
  int httpCode = http.POST(payload);

  if (httpCode == 200) {
    Serial.println("[MJPEG] Streaming server avviato");
    // Assicurati che non sia in pausa
    setMjpegAudioMute(false);
  } else {
    Serial.printf("[MJPEG] Errore avvio streaming: %d\n", httpCode);
  }

  http.end();
}

/**
 * Ferma lo streaming sul server Python
 */
void stopMjpegServerStream() {
  int pathStart = mjpegStreamUrl.indexOf("/stream");
  if (pathStart < 0) {
    pathStart = mjpegStreamUrl.lastIndexOf('/');
  }
  String baseUrl = (pathStart > 0) ? mjpegStreamUrl.substring(0, pathStart) : mjpegStreamUrl;
  String stopUrl = baseUrl + "/api/stop";

  Serial.printf("[MJPEG] Stop streaming server: %s\n", stopUrl.c_str());

  HTTPClient http;
  WiFiClient client;
  http.begin(client, stopUrl);
  int httpCode = http.POST("");

  if (httpCode == 200) {
    Serial.println("[MJPEG] Streaming server fermato");
  } else {
    Serial.printf("[MJPEG] Errore stop streaming: %d\n", httpCode);
  }

  http.end();
}

#endif // EFFECT_MJPEG_STREAM
